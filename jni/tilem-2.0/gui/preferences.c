/*
 * TilEm II
 *
 * Copyright (c) 2011-2012 Benjamin Moody
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
#include "files.h"
#include "filedlg.h"

/* Convert to an absolute path */
static char *canonicalize_filename(const char *name)
{
#ifdef G_OS_WIN32
	static const char delim[] = "/\\";
#else
	static const char delim[] = G_DIR_SEPARATOR_S;
#endif
	char *result, **parts, *p;
	int i;

	if (name == NULL || g_path_is_absolute(name))
		return g_strdup(name);

	result = g_get_current_dir();
	parts = g_strsplit_set(name, delim, -1);
	for (i = 0; parts[i]; i++) {
		if (!strcmp(parts[i], "..")) {
			p = g_path_get_dirname(result);
			g_free(result);
			result = p;
		}
		else if (strcmp(parts[i], ".")
		         && strcmp(parts[i], "")) {
			p = g_build_filename(result, parts[i], NULL);
			g_free(result);
			result = p;
		}
	}
	g_strfreev(parts);
	return result;
}

/* check if two file names are equivalent (of course, if this fails,
   it doesn't necessarily mean the files are distinct) */
static gboolean file_names_equal(const char *a, const char *b)
{
	char *ca, *cb;
	gboolean status;
	
	if (a == NULL && b == NULL)
		return TRUE;
	else if (a == NULL || b == NULL)
		return FALSE;

	ca = canonicalize_filename(a);
	cb = canonicalize_filename(b);
	status = !strcmp(ca, cb);
	g_free(ca);
	g_free(cb);
	return status;
}

static void save_skin_name(TilemEmulatorWindow *ewin)
{
	const char *model = ewin->emu->calc->hw.name;
	char *base, *shared;

	/* don't save pref unless skin was actually loaded */
	if (!ewin->skin_file_name || !ewin->skin)
		return;

	/* if file is stored in shared skins directory, save
	   only the relative path; otherwise, save the
	   absolute path */
	base = g_path_get_basename(ewin->skin_file_name);
	shared = get_shared_file_path("skins", base, NULL);

	if (file_names_equal(shared, ewin->skin_file_name))
		tilem_config_set(model,
		                 "skin/f", base,
		                 NULL);
	else
		tilem_config_set(model,
		                 "skin/f", ewin->skin_file_name,
		                 NULL);

	g_free(base);
	g_free(shared);
}

static void speed_changed(GtkToggleButton *btn, gpointer data)
{
	TilemEmulatorWindow *ewin = data;
	gboolean setting = gtk_toggle_button_get_active(btn);
	tilem_calc_emulator_set_limit_speed(ewin->emu, setting);
	tilem_config_set("emulation",
	                 "limit_speed/b", setting,
	                 NULL);
}

static void grayscale_changed(GtkToggleButton *btn, gpointer data)
{
	TilemEmulatorWindow *ewin = data;
	gboolean setting = gtk_toggle_button_get_active(btn);
	tilem_calc_emulator_set_grayscale(ewin->emu, setting);
	tilem_emulator_window_refresh_lcd(ewin);
	tilem_config_set("emulation",
	                 "grayscale/b", setting,
	                 NULL);
}

static void smooth_changed(GtkToggleButton *btn, gpointer data)
{
	TilemEmulatorWindow *ewin = data;
	gboolean setting = gtk_toggle_button_get_active(btn);
	ewin->lcd_smooth_scale = setting;
	tilem_emulator_window_refresh_lcd(ewin);
	tilem_config_set("settings",
	                 "smooth_scaling/b", setting,
	                 NULL);
}


static void skin_enable_changed(GtkToggleButton *btn, gpointer data)
{
	TilemEmulatorWindow *ewin = data;
	gboolean enable = gtk_toggle_button_get_active(btn);

	if (ewin->skin_disabled == !enable)
		return;

	tilem_emulator_window_set_skin_disabled(ewin, !enable);
	tilem_config_set("settings",
	                 "skin_disabled/b", !enable,
	                 NULL);

	save_skin_name(ewin);
}

static void skin_file_changed(GtkWidget *fe, gpointer data)
{
	TilemEmulatorWindow *ewin = data;
	char *fname = file_entry_get_filename(fe);

	if (fname && !file_names_equal(fname, ewin->skin_file_name)) {
		tilem_emulator_window_set_skin(ewin, fname);
		save_skin_name(ewin);
		g_free(fname);
	}
}

/* Run preferences dialog. */
void tilem_preferences_dialog(TilemEmulatorWindow *ewin)
{
	GtkWidget *dlg, *vbox1, *vbox2, *frame, *slow_rb, *fast_rb,
		*grayscale_cb, *smooth_cb, *hbox, *skin_cb, *skin_entry;

	g_return_if_fail(ewin != NULL);
	g_return_if_fail(ewin->emu != NULL);

	dlg = gtk_dialog_new_with_buttons(_("Preferences"),
	                                  GTK_WINDOW(ewin->window),
	                                  GTK_DIALOG_MODAL,
	                                  GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
	                                  NULL);

	vbox1 = gtk_vbox_new(FALSE, 12);
	gtk_container_set_border_width(GTK_CONTAINER(vbox1), 6);

	/* Emulation speed */

	vbox2 = gtk_vbox_new(FALSE, 6);

	slow_rb = gtk_radio_button_new_with_mnemonic
		(NULL, _("_Limit to actual calculator speed"));
	gtk_box_pack_start(GTK_BOX(vbox2), slow_rb, FALSE, FALSE, 0);

	fast_rb = gtk_radio_button_new_with_mnemonic_from_widget
		(GTK_RADIO_BUTTON(slow_rb), _("As _fast as possible"));
	gtk_box_pack_start(GTK_BOX(vbox2), fast_rb, FALSE, FALSE, 0);

	if (!ewin->emu->limit_speed)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fast_rb), TRUE);

	g_signal_connect(slow_rb, "toggled",
	                 G_CALLBACK(speed_changed), ewin);

	frame = new_frame(_("Emulation Speed"), vbox2);
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 0);

	/* Display settings */

	vbox2 = gtk_vbox_new(FALSE, 6);

	grayscale_cb = gtk_check_button_new_with_mnemonic(_("Emulate _grayscale"));
	gtk_box_pack_start(GTK_BOX(vbox2), grayscale_cb, FALSE, FALSE, 0);

	if (ewin->emu->grayscale)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(grayscale_cb), TRUE);

	g_signal_connect(grayscale_cb, "toggled",
	                 G_CALLBACK(grayscale_changed), ewin);

	smooth_cb = gtk_check_button_new_with_mnemonic(_("Use _smooth scaling"));
	gtk_box_pack_start(GTK_BOX(vbox2), smooth_cb, FALSE, FALSE, 0);

	if (ewin->lcd_smooth_scale)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(smooth_cb), TRUE);

	g_signal_connect(smooth_cb, "toggled",
	                 G_CALLBACK(smooth_changed), ewin);

	hbox = gtk_hbox_new(FALSE, 6);

	skin_cb = gtk_check_button_new_with_mnemonic(_("Use s_kin:"));
	gtk_box_pack_start(GTK_BOX(hbox), skin_cb, FALSE, FALSE, 0);

	skin_entry = file_entry_new(_("Select Skin"),
	                            _("Skin files"), "*.skn",
	                            _("All files"), "*",
	                            NULL);
	gtk_box_pack_start(GTK_BOX(hbox), skin_entry, TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);

	if (!ewin->skin_disabled)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(skin_cb), TRUE);

	if (ewin->skin_file_name)
		file_entry_set_filename(skin_entry, ewin->skin_file_name);

	g_signal_connect(skin_cb, "toggled",
	                 G_CALLBACK(skin_enable_changed), ewin);

	g_signal_connect(skin_entry, "selection-changed",
	                 G_CALLBACK(skin_file_changed), ewin);

	frame = new_frame(_("Display"), vbox2);
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 0);

	vbox2 = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
	gtk_box_pack_start(GTK_BOX(vbox2), vbox1, FALSE, FALSE, 0);
	gtk_widget_show_all(vbox1);

	gtk_dialog_run(GTK_DIALOG(dlg));
	gtk_widget_destroy(dlg);
}
