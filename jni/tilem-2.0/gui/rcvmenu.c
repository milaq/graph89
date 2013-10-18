/*
 * TilEm II
 *
 * Copyright (c) 2010-2011 Thibault Duponchelle
 * Copyright (c) 2012 Benjamin Moody
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <ticalcs.h>
#include <ticonv.h>
#include <tilem.h>
#include <tilemdb.h>
#include <scancodes.h>

#include "gui.h"
#include "disasmview.h"
#include "memmodel.h"
#include "files.h"
#include "filedlg.h"
#include "msgbox.h"
#include "fixedtreeview.h"

static GtkTreeModel* fill_varlist(TilemReceiveDialog *rcvdialog);
TilemReceiveDialog* create_receive_menu(TilemCalcEmulator *emu);

/* Columns */
enum
{
	COL_ENTRY = 0,
	COL_SLOT_STR,
	COL_NAME_STR,
	COL_TYPE_STR,
	COL_SIZE_STR,
	COL_SIZE,
  	NUM_COLS
};

#define RESPONSE_REFRESH 1

/* Prompt to overwrite a list of files. */
static gboolean prompt_overwrite(GtkWindow *win, const char *dirname,
                                 char **filenames)
{
	int i;
	char *dname;
	GString *conflicts = NULL;
	int nconflicts = 0;
	GtkWidget *dlg, *btn;
	int response;

	for (i = 0; filenames[i]; i++) {
		if (g_file_test(filenames[i], G_FILE_TEST_EXISTS)) {
			if (conflicts)
				g_string_append_c(conflicts, '\n');
			else
				conflicts = g_string_new(NULL);
			dname = g_filename_display_basename(filenames[i]);
			g_string_append(conflicts, dname);
			g_free(dname);
			nconflicts++;
		}
	}

	if (!conflicts)
		return TRUE;

	dname = g_filename_display_basename(dirname);

	dlg = gtk_message_dialog_new
		(win, GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
		 _n("Replace existing file?",
		    "Replace existing files?",
		    nconflicts));

	gtk_message_dialog_format_secondary_text
		(GTK_MESSAGE_DIALOG(dlg),
		 _n("The file \"%2$s\" already exists in \"%1$s\"."
		    "  Replacing it will overwrite its contents.",
		    "The following files already exist in \"%s\"."
		    "  Replacing them will overwrite their contents:\n%s",
		    nconflicts),
		 dname, conflicts->str);

	g_free(dname);
	g_string_free(conflicts, TRUE);

	gtk_dialog_add_button(GTK_DIALOG(dlg),
	                      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);

btn = gtk_button_new_with_mnemonic(_("_Replace"));
	gtk_button_set_image(GTK_BUTTON(btn),
	                     gtk_image_new_from_stock(GTK_STOCK_SAVE,
	                                              GTK_ICON_SIZE_BUTTON));
	gtk_widget_show(btn);
	gtk_dialog_add_action_widget(GTK_DIALOG(dlg), btn,
	                             GTK_RESPONSE_ACCEPT);

	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dlg),
	                                        GTK_RESPONSE_ACCEPT,
	                                        GTK_RESPONSE_CANCEL,
	                                        -1);

	response = gtk_dialog_run(GTK_DIALOG(dlg));
	gtk_widget_destroy(dlg);
	return (response == GTK_RESPONSE_ACCEPT);
}

/* #### SIGNALS CALLBACK #### */

/* Prompt to save a single file. */
static gboolean prompt_save_single(TilemReceiveDialog *rcvdialog, TilemVarEntry *tve)
{
	char *dir, *default_filename, *default_filename_r, *filename, *pattern;

	default_filename = get_default_filename(tve);
	default_filename_r = utf8_to_restricted_utf8(default_filename);
	g_free(default_filename);

	tilem_config_get("download", "receivefile_recentdir/f", &dir, NULL);	
	if (!dir) dir = g_get_current_dir();

	pattern = g_strconcat("*.", tve->file_ext, NULL);

	filename = prompt_save_file(_("Save File"), GTK_WINDOW(rcvdialog->window),
	                            default_filename_r, dir,
	                            tve->filetype_desc, pattern,
	                            _("All files"), "*",
	                            NULL);
	g_free(default_filename_r);
	g_free(pattern);
	g_free(dir);

	if (!filename)
		return FALSE;

	dir = g_path_get_dirname(filename);
	tilem_config_set("download", "receivefile_recentdir/f", dir, NULL);
	g_free(dir);

	tilem_link_receive_file(rcvdialog->emu, tve, filename);
	g_free(filename);
	return TRUE;
}

/* Prompt to save a list of variables as a group file. */
static gboolean prompt_save_group(TilemReceiveDialog *rcvdialog, GList *rows)
{
	char *dir, *default_filename, *pattern_desc, *pattern, *filename, *fext;
	int tfmodel;
	gboolean can_group = TRUE;
	const char *model_str;
	GList *l;
	GtkTreePath *path;
	GtkTreeIter iter;
	TilemVarEntry *tve;
	GSList *velist = NULL;

	tilem_config_get("download", "receivefile_recentdir/f", &dir, NULL);	
	if (!dir) dir = g_get_current_dir();

	for (l = rows; l; l = l->next) {
		path = (GtkTreePath*) l->data;
		gtk_tree_model_get_iter(rcvdialog->model, &iter, path);
		gtk_tree_model_get(rcvdialog->model, &iter, COL_ENTRY, &tve, -1);
		velist = g_slist_prepend(velist, tve);
		if (!tve->can_group)
			can_group = FALSE;
	}

	velist = g_slist_reverse(velist);

	tfmodel = get_calc_model(rcvdialog->emu->calc);

	fext = g_ascii_strdown(tifiles_fext_of_group(tfmodel), -1);
	pattern = g_strconcat("*.", fext, NULL);
	default_filename = g_strdup_printf(_("untitled.%s"),
	                                   (can_group ? fext : "tig"));
	g_free(fext);

	model_str = tifiles_model_to_string(tfmodel);
	pattern_desc = g_strdup_printf(_("%s group files"), model_str);

	filename = prompt_save_file(_("Save File"), GTK_WINDOW(rcvdialog->window),
	                            default_filename, dir,
	                            pattern_desc, (can_group ? pattern : ""),
	                            _("TIGroup files"), "*.tig",
	                            _("All files"), "*",
	                            NULL);

	g_free(default_filename);
	g_free(dir);
	g_free(pattern_desc);
	g_free(pattern);

	if (!filename) {
		g_slist_free(velist);
		return FALSE;
	}

	dir = g_path_get_dirname(filename);
	tilem_config_set("download",
	                 "receivefile_recentdir/f", dir,
	                 "save_as_group/b", TRUE,
	                 NULL);
	g_free(dir);

	tilem_link_receive_group(rcvdialog->emu, velist, filename);

	g_free(filename);
	g_slist_free(velist);
	return TRUE;
}

/* Prompt to save a list of files.  Input is a list of GtkTreePaths */
static gboolean prompt_save_multiple(TilemReceiveDialog *rcvdialog, GList *rows)
{
	char *dir, *dir_selected, *default_filename, *default_filename_f;
	GList *l;
	GtkTreePath *path;
	GtkTreeIter iter;
	TilemVarEntry *tve, **vars;
	char **names;
	gboolean is_81, use_group;
	int i;

	is_81 = (rcvdialog->emu->calc->hw.model_id == TILEM_CALC_TI81);
	use_group = gtk_toggle_button_get_active
		(GTK_TOGGLE_BUTTON(rcvdialog->group_rb));

	if (use_group && !is_81)
		return prompt_save_group(rcvdialog, rows);

	tilem_config_get("download", "receivefile_recentdir/f", &dir, NULL);	
	if (!dir) dir = g_get_current_dir();

	dir_selected = prompt_select_dir(_("Save Files to Directory"),
	                                 GTK_WINDOW(rcvdialog->window),
	                                 dir);
	g_free(dir);

	if (!dir_selected)
		return FALSE;

	tilem_config_set("download",
	                 "receivefile_recentdir/f", dir_selected,
	                 "save_as_group/b", use_group,
	                 NULL);

	vars = g_new(TilemVarEntry *, g_list_length(rows) + 1);
	names = g_new(char *, g_list_length(rows) + 1);

	for (l = rows, i = 0; l; l = l->next, i++) {
		path = (GtkTreePath*) l->data;
		gtk_tree_model_get_iter(rcvdialog->model, &iter, path);
		gtk_tree_model_get(rcvdialog->model, &iter, COL_ENTRY, &tve, -1);

		vars[i] = tve;

		default_filename = get_default_filename(tve);
		default_filename_f = utf8_to_filename(default_filename);
		names[i] = g_build_filename(dir_selected,
		                            default_filename_f, NULL);
		g_free(default_filename);
		g_free(default_filename_f);
	}

	vars[i] = NULL;
	names[i] = NULL;

	if (!prompt_overwrite(GTK_WINDOW(rcvdialog->window),
	                      dir_selected, names)) {
		g_free(vars);
		g_strfreev(names);
		g_free(dir_selected);
		return FALSE;
	}

	for (i = 0; vars[i]; i++)
		tilem_link_receive_file(rcvdialog->emu, vars[i], names[i]);

	g_free(vars);
	g_strfreev(names);
	g_free(dir_selected);
	return TRUE;
}

/* Event called on Send button click. Get the selected var/app and save it. */
static gboolean prompt_save(TilemReceiveDialog *rcvdialog)
{
	TilemVarEntry *tve;		/* Variable entry */
	GtkTreeSelection* selection = NULL; /* GtkTreeSelection */
	GtkTreeModel *model;
	GtkTreeIter iter;
	GList *rows, *l;
	GtkTreePath *path;
	gboolean status;

	/* Get the selected entry */
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(rcvdialog->treeview));
	rows = gtk_tree_selection_get_selected_rows(selection, &model);

	if (!rows)
		return FALSE;

	if (!rows->next) {
		path = (GtkTreePath*) rows->data;
		gtk_tree_model_get_iter(model, &iter, path);
		gtk_tree_model_get(model, &iter, COL_ENTRY, &tve, -1);
		status = prompt_save_single(rcvdialog, tve);
	}
	else {
		status = prompt_save_multiple(rcvdialog, rows);
	}

	for (l = rows; l; l = l->next)
		gtk_tree_path_free(l->data);
	g_list_free(rows);
	return status;
}

/* Dialog response button clicked */
static void dialog_response(GtkDialog *dlg, gint response, gpointer data)
{
	TilemReceiveDialog* rcvdialog = (TilemReceiveDialog*) data;

	switch (response) {
	case RESPONSE_REFRESH:
		if (!rcvdialog->refresh_pending) {
			rcvdialog->refresh_pending = TRUE;
			tilem_link_get_dirlist(rcvdialog->emu);
		}
		break;

	case GTK_RESPONSE_ACCEPT:
		if (!prompt_save(rcvdialog))
			break;
	default:
		gtk_widget_hide(GTK_WIDGET(dlg));
	}
}

/* Selection changed */
static void selection_changed(GtkTreeSelection *sel, gpointer data)
{
	TilemReceiveDialog* rcvdialog = data;
	int n = gtk_tree_selection_count_selected_rows(sel);

	gtk_dialog_set_response_sensitive(GTK_DIALOG(rcvdialog->window),
	                                  GTK_RESPONSE_ACCEPT, (n > 0));
	gtk_widget_set_sensitive(rcvdialog->mode_box, (n > 1));
}

/* Row activated in tree view */
static void row_activated(G_GNUC_UNUSED GtkTreeView *treeview,
                          G_GNUC_UNUSED GtkTreePath *path,
                          G_GNUC_UNUSED GtkTreeViewColumn *col,
                          gpointer data)
{
	TilemReceiveDialog* rcvdialog = (TilemReceiveDialog*) data;
	gtk_dialog_response(GTK_DIALOG(rcvdialog->window), GTK_RESPONSE_ACCEPT);
}

/* #### WIDGET CREATION #### */

/* Create a new scrolled window with sensible default settings. */
static GtkWidget *new_scrolled_window(GtkWidget *contents)
{
        GtkWidget *sw; 
        sw = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                       GTK_POLICY_NEVER,
                                       GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
                                            GTK_SHADOW_IN);
        gtk_container_add(GTK_CONTAINER(sw), contents);
        return sw;
}

/* Create the (empty) GtkTreeView to show the vars list */
static GtkWidget *create_varlist(TilemReceiveDialog *rcvdialog)
{
	GtkCellRenderer   *renderer;
	GtkWidget         *treeview;
	GtkTreeSelection  *sel;
	GtkTreeViewColumn *c1, *c2, *c3, *c4;
	gboolean           is_81;

	g_return_val_if_fail(rcvdialog->emu != NULL, NULL);
	g_return_val_if_fail(rcvdialog->emu->calc != NULL, NULL);

	is_81 = (rcvdialog->emu->calc->hw.model_id == TILEM_CALC_TI81);

	/* Create the stack list tree view and set title invisible */
	treeview = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), TRUE);
	gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(treeview), TRUE);
	gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(treeview), TRUE);
	
	/* Allow multiple selection */
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
	gtk_tree_selection_set_mode(sel, GTK_SELECTION_MULTIPLE);

	/* Create the columns */
	renderer = gtk_cell_renderer_text_new();

	if (is_81) {
		c1 = gtk_tree_view_column_new_with_attributes
			(_("Slot"), renderer, "text", COL_SLOT_STR, NULL);

		gtk_tree_view_column_set_sizing(c1, GTK_TREE_VIEW_COLUMN_FIXED);
		gtk_tree_view_column_set_sort_column_id(c1, COL_SLOT_STR);
		gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), c1);
	}

	c2 = gtk_tree_view_column_new_with_attributes
		(_("Name"), renderer, "text", COL_NAME_STR, NULL);

	gtk_tree_view_column_set_sizing(c2, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_sort_column_id(c2, COL_NAME_STR);
	gtk_tree_view_column_set_expand(c2, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), c2);

	if (!is_81) {
		c3 = gtk_tree_view_column_new_with_attributes
			(_("Type"), renderer, "text", COL_TYPE_STR, NULL);
		
		gtk_tree_view_column_set_sizing(c3, GTK_TREE_VIEW_COLUMN_FIXED);
		gtk_tree_view_column_set_sort_column_id(c3, COL_TYPE_STR);
		gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), c3);
	}

	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "xalign", 1.0, NULL);
	c4 = gtk_tree_view_column_new_with_attributes
		(_("Size"), renderer, "text", COL_SIZE_STR, NULL);

	gtk_tree_view_column_set_sizing(c4, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_sort_column_id(c4, COL_SIZE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), c4);

	g_signal_connect(sel, "changed",
	                 G_CALLBACK(selection_changed), rcvdialog);

	g_signal_connect(treeview, "row-activated",
	                 G_CALLBACK(row_activated), rcvdialog);

	return treeview;
}

/* Fill the list of vars. In fact, add all vars from list to a GtkListStore */
static GtkTreeModel* fill_varlist(TilemReceiveDialog *rcvdialog)
{
	GSList *l;
	TilemVarEntry *tve;
	GtkListStore *store;
	GtkTreeIter iter;
	char *size_str;

	store = gtk_list_store_new(6,
	                           G_TYPE_POINTER,
	                           G_TYPE_STRING,
	                           G_TYPE_STRING,
	                           G_TYPE_STRING,
	                           G_TYPE_STRING,
	                           G_TYPE_INT);

	for (l = rcvdialog->vars; l; l = l->next) {
		tve = l->data;
		gtk_list_store_append(store, &iter);
#ifdef G_OS_WIN32
		size_str = g_strdup_printf(_("%d"), tve->size);
#else
		size_str = g_strdup_printf(_("%'d"), tve->size);
#endif
		gtk_list_store_set(store, &iter,
		                   COL_ENTRY, tve,
		                   COL_SLOT_STR, tve->slot_str,
		                   COL_NAME_STR, tve->name_str,
		                   COL_TYPE_STR, tve->type_str,
		                   COL_SIZE_STR, size_str,
		                   COL_SIZE, tve->size,
		                   -1);
		g_free(size_str);
	}

	return GTK_TREE_MODEL(store);
}

/* Create a new menu for receiving vars. */
/* Previous allocated and filled varlist is needed */
TilemReceiveDialog* tilem_receive_dialog_new(TilemCalcEmulator *emu)
{
	TilemReceiveDialog* rcvdialog = g_slice_new0(TilemReceiveDialog);
	GtkWidget *scroll, *btn, *vbox, *lbl, *rb, *vbox2;
	int defheight = 300;
	gboolean is_81;
	gboolean use_group;

	g_return_val_if_fail(emu != NULL, NULL);
	g_return_val_if_fail(emu->ewin != NULL, NULL);
	g_return_val_if_fail(emu->calc != NULL, NULL);

	rcvdialog->emu = emu;
	emu->rcvdlg = rcvdialog;

	is_81 = (emu->calc->hw.model_id == TILEM_CALC_TI81);

	rcvdialog->window = gtk_dialog_new();
	gtk_window_set_transient_for(GTK_WINDOW(rcvdialog->window),
	                             GTK_WINDOW(emu->ewin->window));

	gtk_window_set_title(GTK_WINDOW(rcvdialog->window), _("Receive File"));

	btn = gtk_dialog_add_button(GTK_DIALOG(rcvdialog->window),
	                            GTK_STOCK_REFRESH, RESPONSE_REFRESH);

	if (is_81)
		gtk_widget_hide(btn);

	gtk_dialog_add_button(GTK_DIALOG(rcvdialog->window),
	                      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button(GTK_DIALOG(rcvdialog->window),
	                      GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT);

	gtk_dialog_set_default_response(GTK_DIALOG(rcvdialog->window),
	                                GTK_RESPONSE_ACCEPT);
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(rcvdialog->window),
	                                        RESPONSE_REFRESH,
	                                        GTK_RESPONSE_ACCEPT,
	                                        GTK_RESPONSE_CANCEL,
	                                        -1);

	/* Set the size of the dialog */
	gtk_window_set_default_size(GTK_WINDOW(rcvdialog->window), -1, defheight);
	
	/* Create and fill tree view */
	rcvdialog->treeview = create_varlist(rcvdialog);

	/* Allow scrolling the list because we can't know how many vars the calc contains */
	scroll = new_scrolled_window(rcvdialog->treeview);

	vbox = gtk_vbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
	gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(scroll), TRUE, TRUE, 0);

	rcvdialog->mode_box = gtk_hbox_new(FALSE, 6);
	lbl = gtk_label_new(_("Save as:"));
	gtk_box_pack_start(GTK_BOX(rcvdialog->mode_box), lbl, FALSE, FALSE, 0);

	rb = gtk_radio_button_new_with_mnemonic(NULL, _("S_eparate files"));
	gtk_box_pack_start(GTK_BOX(rcvdialog->mode_box), rb, FALSE, FALSE, 0);
	rcvdialog->multiple_rb = rb;

	rb = gtk_radio_button_new_with_mnemonic_from_widget
		(GTK_RADIO_BUTTON(rb), _("_Group file"));
	gtk_box_pack_start(GTK_BOX(rcvdialog->mode_box), rb, FALSE, FALSE, 0);
	rcvdialog->group_rb = rb;

	tilem_config_get("download", "save_as_group/b=1", &use_group, NULL);
	if (use_group)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb), TRUE);

	if (is_81)
		gtk_widget_set_no_show_all(rcvdialog->mode_box, TRUE);

	gtk_box_pack_start(GTK_BOX(vbox), rcvdialog->mode_box, FALSE, FALSE, 0);
	vbox2 = gtk_dialog_get_content_area(GTK_DIALOG(rcvdialog->window));
	gtk_box_pack_start(GTK_BOX(vbox2), vbox, TRUE, TRUE, 0);

	/* Signals callback */
	g_signal_connect(rcvdialog->window, "response",
	                 G_CALLBACK(dialog_response), rcvdialog);
	g_signal_connect(rcvdialog->window, "delete-event",
	                 G_CALLBACK(gtk_widget_hide_on_delete), NULL);
	
	gtk_widget_show_all(vbox);

	return rcvdialog;
}

/* Destroy a TilemReceiveDialog */
void tilem_receive_dialog_free(TilemReceiveDialog *rcvdialog)
{
	GSList *l;

	g_return_if_fail(rcvdialog != NULL);

	gtk_widget_destroy(rcvdialog->window);

	for (l = rcvdialog->vars; l; l = l->next)
		tilem_var_entry_free(l->data);
	g_slist_free(rcvdialog->vars);

	g_slice_free(TilemReceiveDialog, rcvdialog);
}

void tilem_receive_dialog_update(TilemReceiveDialog *rcvdialog, GSList *varlist)
{
	GSList *l;

	g_return_if_fail(rcvdialog != NULL);

	rcvdialog->refresh_pending = FALSE;

	for (l = rcvdialog->vars; l; l = l->next)
		tilem_var_entry_free(l->data);
	g_slist_free(rcvdialog->vars);

	rcvdialog->vars = varlist;
	rcvdialog->model = fill_varlist(rcvdialog);
	gtk_tree_view_set_model(GTK_TREE_VIEW(rcvdialog->treeview), rcvdialog->model);

	fixed_tree_view_init(rcvdialog->treeview, 0,
	                     COL_SLOT_STR, "PrgmM ",
	                     COL_NAME_STR, "MMMMMMMMM ",
	                     COL_TYPE_STR, "MMMMMM ",
	                     COL_SIZE_STR, "00,000,000",
	                     -1);

	gtk_widget_grab_focus(rcvdialog->treeview);
	gtk_window_present(GTK_WINDOW(rcvdialog->window));
}

/* Popup the receive window */
/* This is the entry point */
void popup_receive_menu(TilemEmulatorWindow *ewin)
{
	g_return_if_fail(ewin != NULL);
	g_return_if_fail(ewin->emu != NULL);
	g_return_if_fail(ewin->emu->calc != NULL);

	if (ewin->emu->rcvdlg && ewin->emu->rcvdlg->refresh_pending)
		return;

	/* TI-81 takes no time to refresh, so do it automatically */
	if (!ewin->emu->rcvdlg
	    || ewin->emu->calc->hw.model_id == TILEM_CALC_TI81) {
		tilem_link_get_dirlist(ewin->emu);
	}
	else {
		gtk_widget_grab_focus(ewin->emu->rcvdlg->treeview);
		gtk_window_present(GTK_WINDOW(ewin->emu->rcvdlg->window));
	}
}
