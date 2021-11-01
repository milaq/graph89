/*
 * TilEm II
 *
 * Copyright (c) 2010-2011 Thibault Duponchelle
 * Copyright (c) 2010-2012 Benjamin Moody
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
#include <gtk/gtk.h>
#include <ticalcs.h>
#include <tilem.h>

#include "gui.h"
#include "emucore.h"
#include "filedlg.h"
#include "ti81prg.h"


/* Send a series of files */
static void send_files(TilemCalcEmulator *emu, char **filenames, int *slots)
{
	int i;

	for (i = 0; filenames[i]; i++) {
		tilem_link_send_file(emu, filenames[i],
		                     slots ? slots[i] : -1,
		                     (i == 0),
		                     (filenames[i + 1] == NULL));

		/* FIXME: macros should record slot numbers */
		if (emu->isMacroRecording)
			tilem_macro_add_action(emu->macro, 1, filenames[i]);
	}
}

static int string_to_slot(const char *str)
{
	if (!g_ascii_strncasecmp(str, "prgm", 4))
		str += 4;
	else if (!g_ascii_strncasecmp(str, "ti81_", 5))
		str += 5;
	else
		return TI81_SLOT_AUTO;

	if (g_ascii_isdigit(str[0]) && !g_ascii_isalnum(str[1]))
		return TI81_SLOT_0 + str[0] - '0';
	else if (g_ascii_isalpha(str[0]) && !g_ascii_isalnum(str[1]))
		return TI81_SLOT_A + g_ascii_toupper(str[0]) - 'A';
	else if (str[0] == '@'
	         || !g_ascii_strncasecmp(str, "theta", 5)
	         || !strncmp(str, "\316\270", 2)
	         || !strncmp(str, "\316\230", 2))
		return TI81_SLOT_THETA;
	else
		return TI81_SLOT_AUTO;
}

/* Guess program slot for a filename */
static int guess_slot(const char *filename)
{
	char *base;
	int slot;
	base = g_filename_display_basename(filename);
	slot = string_to_slot(base);
	g_free(base);
	return slot;
}

static int display_index_to_slot(int i)
{
	if (i < 9)
		return i + 1;
	else if (i == 9)
		return 0;
	else
		return i;
}

struct slotdialog {
	int nfiles;
	char **filenames;
	int *slots;
	TI81ProgInfo info[TI81_SLOT_MAX + 1];
	GtkTreeModel *prgm_model;
	GtkTreeModel *slot_model;
};

static void slot_edited(G_GNUC_UNUSED GtkCellRendererText *cell,
                        gchar *pathstr, gchar *text, gpointer data)
{
	struct slotdialog *slotdlg = data;
	GtkTreeIter iter;
	int n, slot;
	char *end;

	slot = string_to_slot(text);
	if (slot < 0)
		return;

	n = strtol(pathstr, &end, 10);
	gtk_tree_model_iter_nth_child(slotdlg->prgm_model, &iter, NULL, n);
	gtk_list_store_set(GTK_LIST_STORE(slotdlg->prgm_model),
	                   &iter, 1, text, -1);

	slotdlg->slots[n] = slot;
}

/* Prompt user to assign program slots to filenames */
static void prompt_program_slots(TilemCalcEmulator *emu,
                                 struct slotdialog *slotdlg)
{
	GtkWidget *parent, *dlg, *vbox, *vbox2, *sw, *tv, *lbl;
	GtkListStore *prgmstore, *slotstore;
	GtkTreeIter iter;
	GtkCellRenderer *cell;
	GtkTreeViewColumn *col;
	int i, j, slot;
	int used[TI81_SLOT_MAX + 1];
	char *slotstr, *namestr;
	char *slotlabel[TI81_SLOT_MAX + 1];

	if (emu->ewin)
		parent = emu->ewin->window;
	else
		parent = NULL;

	/* Generate list of existing programs */

	slotstore = gtk_list_store_new(1, G_TYPE_STRING);
	slotdlg->slot_model = GTK_TREE_MODEL(slotstore);

	for (i = 0; i <= TI81_SLOT_MAX; i++) {
		slot = display_index_to_slot(i);
		slotstr = ti81_program_slot_to_string(slot);
		namestr = ti81_program_name_to_string(slotdlg->info[slot].name);
	
		if (slotdlg->info[slot].size == 0) {
			slotlabel[slot] = g_strdup(slotstr);
			used[slot] = 0;
		}
		else if (namestr && namestr[0]) {
			slotlabel[slot] = g_strdup_printf(_("%s (in use: %s)"),
			                                  slotstr, namestr);
			used[slot] = 1;
		}
		else {
			slotlabel[slot] = g_strdup_printf(_("%s (in use)"), slotstr);
			used[slot] = 1;
		}

		gtk_list_store_append(slotstore, &iter);
		gtk_list_store_set(slotstore, &iter, 0, slotlabel[slot], -1);
		g_free(slotstr);
		g_free(namestr);
	}

	/* Assign default slots to files */

	for (i = 0; i < slotdlg->nfiles; i++) {
		slot = guess_slot(slotdlg->filenames[i]);
		if (slotdlg->slots[i] < 0)
			slotdlg->slots[i] = slot;
		if (slot >= 0)
			used[slot] = 1;
	}

	for (i = 0; i < slotdlg->nfiles; i++) {
		if (slotdlg->slots[i] < 0) {
			for (j = 0; j <= TI81_SLOT_MAX; j++) {
				slot = display_index_to_slot(j);
				if (!used[slot]) {
					slotdlg->slots[i] = slot;
					used[slot] = 1;
					break;
				}
			}
		}

		if (slotdlg->slots[i] < 0)
			slotdlg->slots[i] = TI81_SLOT_1;
	}

	/* Generate list of filenames and assigned slots */

	prgmstore = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	slotdlg->prgm_model = GTK_TREE_MODEL(prgmstore);

	for (i = 0; i < slotdlg->nfiles; i++) {
		namestr = g_filename_display_basename(slotdlg->filenames[i]);
		slot = slotdlg->slots[i];

		gtk_list_store_append(prgmstore, &iter);
		gtk_list_store_set(prgmstore, &iter,
		                   0, namestr,
		                   1, slotlabel[slot],
		                   -1);
		g_free(namestr);
	}

	for (i = 0; i <= TI81_SLOT_MAX; i++)
		g_free(slotlabel[i]);

	/* Create tree view */

	tv = gtk_tree_view_new_with_model(slotdlg->prgm_model);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tv), TRUE);

	cell = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes
		(_("File"), cell, "text", 0, NULL);
	gtk_tree_view_column_set_expand(col, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

	cell = gtk_cell_renderer_combo_new();
	g_object_set(cell, "model", slotstore, "text-column", 0,
	             "editable", TRUE, "has-entry", FALSE, NULL);
	col = gtk_tree_view_column_new_with_attributes
		(_("Slot"), cell, "text", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

	g_signal_connect(cell, "edited", G_CALLBACK(slot_edited), slotdlg);

	/* Create dialog */

	dlg = gtk_dialog_new_with_buttons(_("Select Program Slots"),
	                                  GTK_WINDOW(parent), GTK_DIALOG_MODAL,
	                                  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	                                  GTK_STOCK_OK, GTK_RESPONSE_OK,
	                                  NULL);
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dlg),
	                                        GTK_RESPONSE_OK,
	                                        GTK_RESPONSE_CANCEL,
	                                        -1);
	gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_OK);

	gtk_window_set_default_size(GTK_WINDOW(dlg), -1, 250);

	sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
	                               GTK_POLICY_NEVER,
	                               GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
	                                    GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(sw), tv);

	vbox = gtk_vbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);

	lbl = gtk_label_new(_("Select a slot where each program should be"
	                      " loaded.  If a program slot is already in use,"
	                      " its contents will be overwritten."));
	gtk_misc_set_alignment(GTK_MISC(lbl), 0.0, 0.0);
	gtk_label_set_line_wrap(GTK_LABEL(lbl), TRUE);
	gtk_label_set_width_chars(GTK_LABEL(lbl), 45);
	gtk_box_pack_start(GTK_BOX(vbox), lbl, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);
	gtk_widget_show_all(vbox);

	vbox2 = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
	gtk_box_pack_start(GTK_BOX(vbox2), vbox, TRUE, TRUE, 0);

	if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)
		send_files(emu, slotdlg->filenames, slotdlg->slots);

	gtk_widget_destroy(dlg);
}

/* Check status of existing programs */
static gboolean check_prog_slots_main(TilemCalcEmulator *emu, gpointer data)
{
	struct slotdialog *slotdlg = data;
	int i;

	tilem_em_wake_up(emu, TRUE);
	for (i = 0; i <= TI81_SLOT_MAX; i++)
		ti81_get_program_info(emu->calc, i, &slotdlg->info[i]);

	return TRUE;
}

static void check_prog_slots_finished(TilemCalcEmulator *emu, gpointer data,
                                      gboolean cancelled)
{
	struct slotdialog *slotdlg = data;

	if (!cancelled)
		prompt_program_slots(emu, slotdlg);

	g_free(slotdlg->slots);
	g_strfreev(slotdlg->filenames);
	g_slice_free(struct slotdialog, slotdlg);
}


#define PAT_TI81       "*.prg"
#define PAT_TI73       "*.73?"
#define PAT_TI73_NUM   "*.73n;*.73l;*.73m;*.73i"
#define PAT_TI82       "*.82?"
#define PAT_TI82_NUM   "*.82n;*.82l;*.82m;*.82i"
#define PAT_TI82_TEXT  "*.82s;*.82y;*.82p"
#define PAT_TI83       "*.83?"
#define PAT_TI83_NUM   "*.83n;*.83l;*.83m;*.83i"
#define PAT_TI83_TEXT  "*.83s;*.83y;*.83p"
#define PAT_TI83P      "*.8x?;*.8xgrp"
#define PAT_TI83P_NUM  "*.8xn;*.8xl;*.8xm;*.8xi"
#define PAT_TI83P_TEXT "*.8xs;*.8xy;*.8xp"
#define PAT_TI85       "*.85?"
#define PAT_TI86       "*.86?"
#define PAT_TIG        "*.tig"

#define FLT_TI81       _("TI-81 programs"), PAT_TI81
#define FLT_TI73       _("TI-73 files"), PAT_TI73
#define FLT_TI82       _("TI-82 files"), PAT_TI82
#define FLT_TI83       _("TI-83 files"), PAT_TI83
#define FLT_TI83P      _("TI-83 Plus files"), PAT_TI83P
#define FLT_TI85       _("TI-85 files"), PAT_TI85
#define FLT_TI86       _("TI-86 files"), PAT_TI86
#define FLT_TIG        _("TIGroup files"), PAT_TIG
#define FLT_ALL        _("All files"), "*"

#define DESC_COMPAT _("All compatible files")

#define FLT_TI73_COMPAT    DESC_COMPAT, (PAT_TI73 ";" PAT_TIG ";" \
                                         PAT_TI82_NUM ";" \
                                         PAT_TI83_NUM ";" \
                                         PAT_TI83P_NUM)

#define FLT_TI82_COMPAT    DESC_COMPAT, (PAT_TI82 ";" PAT_TIG ";" \
                                         PAT_TI83_TEXT ";" PAT_TI83_NUM ";" \
                                         PAT_TI83P_TEXT ";" PAT_TI83P_NUM ";" \
                                         PAT_TI73_NUM)

#define FLT_TI83_COMPAT    DESC_COMPAT, (PAT_TI83 ";" PAT_TIG ";" \
                                         PAT_TI82_TEXT ";" PAT_TI82_NUM ";" \
                                         PAT_TI83P_TEXT ";" PAT_TI83P_NUM ";" \
                                         PAT_TI73_NUM)

#define FLT_TI83P_COMPAT   DESC_COMPAT, (PAT_TI83P ";" PAT_TIG ";" \
                                         PAT_TI82_TEXT ";" PAT_TI82_NUM ";" \
                                         PAT_TI83_TEXT ";" PAT_TI83_NUM ";" \
                                         PAT_TI73_NUM)

#define FLT_TI8586_COMPAT  DESC_COMPAT, (PAT_TI85 ";" PAT_TI86 ";" PAT_TIG)

static char ** prompt_link_files(const char *title,
                                 GtkWindow *parent,
                                 const char *dir,
                                 int model)
{
	switch (model) {
	case TILEM_CALC_TI73:
		return prompt_open_files(title, parent, dir,
		                         FLT_TI73_COMPAT, FLT_TI73,
		                         FLT_TI82, FLT_TI83, FLT_TI83P,
		                         FLT_TIG, FLT_ALL, NULL);
	case TILEM_CALC_TI81:
		return prompt_open_files(title, parent, dir,
		                         FLT_TI81, FLT_ALL, NULL);
	case TILEM_CALC_TI82:
		return prompt_open_files(title, parent, dir,
		                         FLT_TI82_COMPAT, FLT_TI73,
		                         FLT_TI82, FLT_TI83, FLT_TI83P,
		                         FLT_TIG, FLT_ALL, NULL);
	case TILEM_CALC_TI83:
	case TILEM_CALC_TI76:
		return prompt_open_files(title, parent, dir,
		                         FLT_TI83_COMPAT, FLT_TI73,
		                         FLT_TI82, FLT_TI83, FLT_TI83P,
		                         FLT_TIG, FLT_ALL, NULL);
	case TILEM_CALC_TI83P:
	case TILEM_CALC_TI83P_SE:
	case TILEM_CALC_TI84P:
	case TILEM_CALC_TI84P_SE:
	case TILEM_CALC_TI84P_NSPIRE:
		return prompt_open_files(title, parent, dir,
		                         FLT_TI83P_COMPAT, FLT_TI73,
		                         FLT_TI82, FLT_TI83, FLT_TI83P,
		                         FLT_TIG, FLT_ALL, NULL);
	case TILEM_CALC_TI85:
	case TILEM_CALC_TI86:
		return prompt_open_files(title, parent, dir,
		                         FLT_TI8586_COMPAT, FLT_TI85,
		                         FLT_TI86, FLT_TIG, FLT_ALL, NULL);
	default:
		return prompt_open_files(title, parent, dir, FLT_ALL, NULL);
	}
}

/* Load a list of files through the GUI.  The list of filenames must
   end with NULL. */
void load_files(TilemEmulatorWindow *ewin, char **filenames)
{
	struct slotdialog *slotdlg;
	int i;

	g_return_if_fail(ewin->emu->calc != NULL);

	if (ewin->emu->calc->hw.model_id == TILEM_CALC_TI81) {
		slotdlg = g_slice_new0(struct slotdialog);
		slotdlg->filenames = g_strdupv(filenames);
		slotdlg->nfiles = g_strv_length(filenames);
		slotdlg->slots = g_new(int, slotdlg->nfiles);
		for (i = 0; i < slotdlg->nfiles; i++)
			slotdlg->slots[i] = TI81_SLOT_AUTO;
		tilem_calc_emulator_begin(ewin->emu, &check_prog_slots_main,
		                          &check_prog_slots_finished, slotdlg);
	}
	else {
		send_files(ewin->emu, filenames, NULL);
	}
}

static int get_cmdline_slot(const char *str, const char **name)
{
	char *e;
	int n;

	n = strtol(str, &e, 10);
	if (*e == '=') {
		*name = e + 1;
		return n;
	}

	if (g_ascii_isalpha(str[0]) && str[1] == '=') {
		*name = str + 2;
		return TI81_SLOT_A + g_ascii_toupper(str[0]) - 'A';
	}

	if (str[0] == '@' && str[1] == '=') {
		*name = str + 2;
		return TI81_SLOT_THETA;
	}

	if (!g_ascii_strncasecmp(str, "theta=", 6)) {
		*name = str + 6;
		return TI81_SLOT_THETA;
	}

	*name = str;
	return TI81_SLOT_AUTO;
}

/* Load a list of files from the command line.  Filenames may begin
   with an optional slot designation. */
void load_files_cmdline(TilemEmulatorWindow *ewin, char **filenames)
{
	struct slotdialog *slotdlg;
	int i;
	gboolean need_prompt = FALSE;
	const char *name;

	g_return_if_fail(ewin->emu->calc != NULL);

	slotdlg = g_slice_new0(struct slotdialog);
	slotdlg->nfiles = g_strv_length(filenames);
	slotdlg->slots = g_new(int, slotdlg->nfiles);
	slotdlg->filenames = g_new0(char *, slotdlg->nfiles + 1);

	for (i = 0; i < slotdlg->nfiles; i++) {
		slotdlg->slots[i] = get_cmdline_slot(filenames[i], &name);
		slotdlg->filenames[i] = g_strdup(name);

		if (slotdlg->slots[i] < 0)
			need_prompt = TRUE;
	}

	if (need_prompt && ewin->emu->calc->hw.model_id == TILEM_CALC_TI81) {
		tilem_calc_emulator_begin(ewin->emu, &check_prog_slots_main,
		                          &check_prog_slots_finished, slotdlg);
	}
	else {
		send_files(ewin->emu, slotdlg->filenames, slotdlg->slots);
		g_free(slotdlg->slots);
		g_strfreev(slotdlg->filenames);
		g_slice_free(struct slotdialog, slotdlg);
	}
}

/* Prompt user to load a file */
void load_file_dialog(TilemEmulatorWindow *ewin)
{
	char **filenames, *dir;

	tilem_config_get("upload",
	                 "sendfile_recentdir/f", &dir,
	                 NULL);

	filenames = prompt_link_files(_("Send File"),
	                              GTK_WINDOW(ewin->window),
	                              dir, ewin->emu->calc->hw.model_id);
	g_free(dir);

	if (!filenames || !filenames[0]) {
		g_free(filenames);
		return;
	}

	dir = g_path_get_dirname(filenames[0]);
	tilem_config_set("upload",
	                 "sendfile_recentdir/f", dir,
	                 NULL);
	g_free(dir);

	load_files(ewin, filenames);
	g_strfreev(filenames);
}
