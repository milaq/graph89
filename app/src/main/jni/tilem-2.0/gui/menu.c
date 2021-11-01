/*
 * TilEm II
 *
 * Copyright (c) 2010-2011 Thibault Duponchelle
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
#include <gtk/gtk.h>
#include <ticalcs.h>
#include <tilem.h>

#include "gui.h"
#include "msgbox.h"

static void action_send_file(G_GNUC_UNUSED GtkAction *act, gpointer data)
{
	TilemEmulatorWindow *ewin = data;
	load_file_dialog(ewin);
}

static void action_receive_file(G_GNUC_UNUSED GtkAction *act, gpointer data)
{
	TilemEmulatorWindow *ewin = data;
	//tilem_rcvmenu_new(ewin->emu);
	//get_var(ewin->emu);
	popup_receive_menu(ewin);
}

static void action_link_setup(G_GNUC_UNUSED GtkAction *act, gpointer data)
{
	TilemEmulatorWindow *ewin = data;
	tilem_link_setup_dialog(ewin);
}

static void action_start_debugger(G_GNUC_UNUSED GtkAction *act, gpointer data)
{
	TilemEmulatorWindow *ewin = data;
	launch_debugger(ewin);
}

static void action_open_calc(G_GNUC_UNUSED GtkAction *act, gpointer data)
{
	TilemEmulatorWindow *ewin = data;
	tilem_calc_emulator_prompt_open_rom(ewin->emu);
}

static void action_save_calc(G_GNUC_UNUSED GtkAction *act, G_GNUC_UNUSED gpointer data)
{
	TilemEmulatorWindow *ewin = data;
	GError *err = NULL;

	if (!tilem_calc_emulator_save_state(ewin->emu, &err)) {
		messagebox01(GTK_WINDOW(ewin->window), GTK_MESSAGE_ERROR,
		             "Unable to save calculator state",
		             "%s", err->message);
		g_error_free(err);
	}
}

static void action_revert_calc(G_GNUC_UNUSED GtkAction *act, G_GNUC_UNUSED gpointer data)
{
	TilemEmulatorWindow *ewin = data;
	GError *err = NULL;

	if (!tilem_calc_emulator_revert_state(ewin->emu, &err)) {
		messagebox01(GTK_WINDOW(ewin->window), GTK_MESSAGE_ERROR,
		             _("Unable to load calculator state"),
		             "%s", err->message);
		g_error_free(err);
	}
}

static void action_reset_calc(G_GNUC_UNUSED GtkAction *act, gpointer data)
{
	TilemEmulatorWindow *ewin = data;
	tilem_calc_emulator_reset(ewin->emu);
}

static void action_begin_macro(G_GNUC_UNUSED GtkAction *act, gpointer data)
{
	TilemEmulatorWindow *ewin = data;
	tilem_macro_start(ewin->emu);
}

static void action_end_macro(G_GNUC_UNUSED GtkAction *act, gpointer data)
{
	TilemEmulatorWindow *ewin = data;
	tilem_macro_stop(ewin->emu);
	/* tilem_macro_print(ewin->emu->macro); */
}

static void action_play_macro(G_GNUC_UNUSED GtkAction *act, gpointer data)
{
	TilemEmulatorWindow *ewin = data;
	tilem_macro_play(ewin->emu);
}

static void action_open_macro(G_GNUC_UNUSED GtkAction *act, G_GNUC_UNUSED gpointer data)
{
	TilemEmulatorWindow *ewin = data;
	tilem_macro_load(ewin->emu, NULL);
}

/* I will improve macro creation by saving it firstly into a macro object
 * Save macro will only be done if user choose to save it */
static void action_save_macro(G_GNUC_UNUSED GtkAction *act, G_GNUC_UNUSED gpointer data)
{
	TilemEmulatorWindow *ewin = data;
	tilem_macro_write_file(ewin->emu);
}

static void action_screenshot(G_GNUC_UNUSED GtkAction *act, gpointer data)
{
	TilemEmulatorWindow *ewin = data;
	popup_screenshot_window(ewin);
}

static void action_quick_screenshot(G_GNUC_UNUSED GtkAction *act,
                                    gpointer data)
{
	TilemEmulatorWindow *ewin = data;
	quick_screenshot(ewin);
}

static void action_preferences(G_GNUC_UNUSED GtkAction *act,
                               gpointer data)
{
	TilemEmulatorWindow *ewin = data;
	tilem_preferences_dialog(ewin);
}

static void action_about(G_GNUC_UNUSED GtkAction *act,
                                    G_GNUC_UNUSED gpointer data)
{
	show_about();
}

static void action_quit(G_GNUC_UNUSED GtkAction *act,
                        G_GNUC_UNUSED gpointer data)
{
	
	TilemEmulatorWindow *ewin = data;
	gtk_widget_destroy(ewin->window);
}

static const GtkActionEntry main_action_ents[] =
	{{ "send-file",
	   GTK_STOCK_OPEN, N_("Send _File..."), "<ctrl>O",
	   N_("Send a program or variable file to the calculator"),
	   G_CALLBACK(action_send_file) },
	 { "receive-file",
	   GTK_STOCK_SAVE_AS, N_("Re_ceive File..."), "<ctrl>S",
	   N_("Receive a program or variable from the calculator"),
	   G_CALLBACK(action_receive_file) },
	 { "link-setup",
	   GTK_STOCK_CONNECT, N_("_Link Cable..."), "<ctrl>L",
	   N_("Connect to an external link cable"),
	   G_CALLBACK(action_link_setup) },

	 { "open-calc",
	   GTK_STOCK_OPEN, N_("_Open Calculator..."), "<shift><ctrl>O",
	   N_("Open a calculator ROM file"),
	   G_CALLBACK(action_open_calc) },
	 { "save-calc",
	   GTK_STOCK_SAVE, N_("_Save Calculator"), "<shift><ctrl>S",
	   N_("Save current calculator state"),
	   G_CALLBACK(action_save_calc) },
	 { "revert-calc",
	   GTK_STOCK_REVERT_TO_SAVED, N_("Re_vert Calculator State"), 0,
	   N_("Revert to saved calculator state"),
	   G_CALLBACK(action_revert_calc) },
	 { "reset-calc",
	   GTK_STOCK_CLEAR, N_("_Reset Calculator"), "<shift><ctrl>Delete",
	   N_("Reset the calculator"),
	   G_CALLBACK(action_reset_calc) },

	 { "start-debugger",
	   0, N_("_Debugger"), "Pause",
	   N_("Pause emulation and start the debugger"),
	   G_CALLBACK(action_start_debugger) },

	 { "begin-macro",
	   GTK_STOCK_MEDIA_RECORD, N_("_Record"), 0,
	   N_("Begin recording a macro"),
	   G_CALLBACK(action_begin_macro) },
	 { "end-macro",
	   GTK_STOCK_MEDIA_STOP, N_("S_top"), 0,
	   N_("Begin recording a macro"),
	   G_CALLBACK(action_end_macro) },
	 { "play-macro",
	   GTK_STOCK_MEDIA_PLAY, N_("_Play"), 0,
	   N_("Play back the current macro"),
	   G_CALLBACK(action_play_macro) },
	 { "open-macro",
	   GTK_STOCK_OPEN, N_("_Open Macro File..."), "",
	   N_("Load a macro from a file"),
	   G_CALLBACK(action_open_macro) },
	 { "save-macro",
	   GTK_STOCK_SAVE_AS, N_("_Save Macro File..."), "",
	   N_("Save current macro to a file"),
	   G_CALLBACK(action_save_macro) },

	 { "screenshot",
	   0, N_("S_creenshot..."), "<ctrl>Print",
	   N_("Save a screenshot"),
	   G_CALLBACK(action_screenshot) },
	 { "quick-screenshot",
	   0, N_("_Quick Screenshot"), "<shift><ctrl>Print",
	   N_("Save a screenshot using default settings"),
	   G_CALLBACK(action_quick_screenshot) },

	 { "preferences",
	   GTK_STOCK_PREFERENCES, 0, 0,
	   N_("Edit emulator settings"),
	   G_CALLBACK(action_preferences) },

	 { "about",
	   GTK_STOCK_ABOUT, N_("_About"), "",
	   N_("Print some informations about TilEm 2 and its authors"),
	   G_CALLBACK(action_about) },

	 { "quit",
	   GTK_STOCK_QUIT, N_("_Quit"), "<ctrl>Q",
	   N_("Quit the application"),
	   G_CALLBACK(action_quit) }};

static GtkWidget *add_item(GtkWidget *menu, GtkAccelGroup *accelgrp,
                           GtkActionGroup *actions, const char *name)
{
	GtkAction *action;
	GtkWidget *item;

	action = gtk_action_group_get_action(actions, name);
	g_return_val_if_fail(action != NULL, NULL);

	gtk_action_set_accel_group(action, accelgrp);
	item = gtk_action_create_menu_item(action);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	gtk_widget_show(item);
	return item;
}

static GtkWidget *add_separator(GtkWidget *menu)
{
	GtkWidget *item;
	item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	gtk_widget_show(item);
	return item;
}

static GtkWidget *add_submenu(GtkWidget *menu, const char *label)
{
	GtkWidget *item, *submenu;

	item = gtk_menu_item_new_with_mnemonic(label);
	submenu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	gtk_widget_show(item);
	return submenu;
}

/* Build the menu */
void build_menu(TilemEmulatorWindow* ewin)
{
	GtkActionGroup *acts;
	GtkAccelGroup *ag;
	GtkWidget *menu, *submenu;

	ewin->actions = acts = gtk_action_group_new("Emulator");
	gtk_action_group_set_translation_domain(acts, GETTEXT_PACKAGE);
	gtk_action_group_add_actions(ewin->actions, main_action_ents,
	                             G_N_ELEMENTS(main_action_ents), ewin);

	ag = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(ewin->window), ag);

	ewin->popup_menu = menu = gtk_menu_new();

	add_item(menu, ag, acts, "send-file");
	add_item(menu, ag, acts, "receive-file");
	add_item(menu, ag, acts, "link-setup");
	add_separator(menu);

	add_item(menu, ag, acts, "open-calc");
	add_item(menu, ag, acts, "save-calc");
	add_item(menu, ag, acts, "revert-calc");
	add_item(menu, ag, acts, "reset-calc");
	add_separator(menu);

	add_item(menu, ag, acts, "start-debugger");
	
	submenu = add_submenu(menu, _("_Macro"));
	add_item(submenu, ag, acts, "begin-macro");
	add_item(submenu, ag, acts, "end-macro");
	add_item(submenu, ag, acts, "play-macro");
	add_separator(submenu);
	add_item(submenu, ag, acts, "open-macro");
	add_item(submenu, ag, acts, "save-macro");

	add_item(menu, ag, acts, "screenshot");
	add_item(menu, ag, acts, "quick-screenshot");
	add_separator(menu);

	add_item(menu, ag, acts, "preferences");
	add_separator(menu);

	add_item(menu, ag, acts, "about");
	add_item(menu, ag, acts, "quit");
}	
