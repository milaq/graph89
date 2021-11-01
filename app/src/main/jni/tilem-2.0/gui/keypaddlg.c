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
#include <gtk/gtk.h>
#include <ticalcs.h>
#include <tilem.h>
#include <scancodes.h>

#include "gui.h"

#define NGROUPS 7
#define NKEYS 8

/* Check-button toggled */
static void group_toggled(GtkToggleButton *btn, gpointer data)
{
	TilemKeypadDialog *kpdlg = data;
	TilemCalcEmulator *emu;
	int i;
	gboolean state;

	if (kpdlg->refreshing)
		return;

	g_return_if_fail(kpdlg->dbg != NULL);
	g_return_if_fail(kpdlg->dbg->emu != NULL);
	emu = kpdlg->dbg->emu;

	state = gtk_toggle_button_get_active(btn);

	for (i = 0; i < NGROUPS; i++) {
		if (GTK_WIDGET(btn) == kpdlg->output[i]) {
			tilem_calc_emulator_lock(emu);
			if (state)
				emu->calc->keypad.group &= ~(1 << i);
			else
				emu->calc->keypad.group |= (1 << i);
			tilem_calc_emulator_unlock(emu);

			tilem_keypad_dialog_refresh(kpdlg);
			return;
		}
	}

	g_return_if_reached();
}

/* Key toggled */
static void key_toggled(GtkToggleButton *btn, gpointer data)
{
	TilemKeypadDialog *kpdlg = data;
	TilemCalcEmulator *emu;
	int i, j, k;
	gboolean state;

	if (kpdlg->refreshing)
		return;

	g_return_if_fail(kpdlg->dbg != NULL);
	g_return_if_fail(kpdlg->dbg->emu != NULL);
	emu = kpdlg->dbg->emu;

	state = gtk_toggle_button_get_active(btn);

	for (i = 0; i < NGROUPS; i++) {
		for (j = 0; j < NKEYS; j++) {
			if (GTK_WIDGET(btn) == kpdlg->keys[i][j]) {
				k = i * 8 + j + 1;
				if (state)
					tilem_calc_emulator_press_key(emu, k);
				else
					tilem_calc_emulator_release_key(emu, k);
				return;
			}
		}
	}

	g_return_if_reached();
}

/* Create a new TilemKeypadDialog. */
TilemKeypadDialog *tilem_keypad_dialog_new(TilemDebugger *dbg)
{
	TilemKeypadDialog *kpdlg;
	GtkWidget *tbl1, *tbl2, *hbox, *vbox, *btn, *lbl;
	int i, j;
	char buf[20];

	g_return_val_if_fail(dbg != NULL, NULL);

	kpdlg = g_slice_new0(TilemKeypadDialog);
	kpdlg->dbg = dbg;

	kpdlg->window = gtk_dialog_new_with_buttons
		(_("Keypad"), NULL, 0,
		 GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
		 NULL);

	g_signal_connect(kpdlg->window, "delete-event",
	                 G_CALLBACK(gtk_widget_hide_on_delete), NULL);

	tbl1 = gtk_table_new(NGROUPS, NKEYS, TRUE);
	hbox = gtk_hbox_new(TRUE, 0);
	vbox = gtk_vbox_new(TRUE, 0);

	/* Keypad buttons (labels will be filled in, and buttons
	   shown/hidden, by tilem_keypad_dialog_calc_changed())

	   Buttons are displayed right to left, top to bottom; this
	   way, the layout of groups 1-5 roughly corresponds to the
	   physical layout of the keys, and the "input" value can be
	   read across the bottom as a binary number. */

	for (i = 0; i < NGROUPS; i++) {
		for (j = 0; j < NKEYS; j++) {
			btn = gtk_toggle_button_new_with_label("");
			kpdlg->keys[i][j] = btn;
			gtk_table_attach(GTK_TABLE(tbl1), btn,
			                 NKEYS - j - 1, NKEYS - j,
			                 i, i + 1,
			                 GTK_FILL, GTK_FILL, 2, 2);

			g_signal_connect(btn, "toggled",
			                 G_CALLBACK(key_toggled), kpdlg);

			gtk_widget_set_no_show_all(btn, TRUE);
		}
	}

	/* Check buttons for key groups (output bits) */

	for (i = 0; i < NGROUPS; i++) {
		g_snprintf(buf, sizeof(buf), _("Group %d"), i);
		btn = gtk_check_button_new_with_label(buf);
		kpdlg->output[i] = btn;
		gtk_box_pack_start(GTK_BOX(vbox), btn, FALSE, TRUE, 2);

		g_signal_connect(btn, "toggled",
		                 G_CALLBACK(group_toggled), kpdlg);
	}

	/* Labels for input bits */

	for (j = NKEYS - 1; j >= 0; j--) {
		kpdlg->input[j] = lbl = gtk_label_new("");
		gtk_box_pack_start(GTK_BOX(hbox), lbl, FALSE, TRUE, 2);
	}

	tbl2 = gtk_table_new(3, 2, FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(tbl2), 6);
	gtk_table_set_row_spacings(GTK_TABLE(tbl2), 12);
	gtk_table_set_col_spacings(GTK_TABLE(tbl2), 12);

	lbl = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(lbl), _("<b>Scan Groups</b>"));
	gtk_table_attach(GTK_TABLE(tbl2), lbl, 0, 1, 0, 1,
	                 GTK_FILL, GTK_FILL, 0, 0);

	lbl = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(lbl), _("<b>Keys</b>"));
	gtk_table_attach(GTK_TABLE(tbl2), lbl, 1, 2, 0, 1,
	                 GTK_FILL, GTK_FILL, 0, 0);

	lbl = gtk_label_new(_("Input Value:"));
	gtk_table_attach(GTK_TABLE(tbl2), lbl, 0, 1, 2, 3,
	                 GTK_FILL, GTK_FILL, 0, 0);

	gtk_table_attach(GTK_TABLE(tbl2), vbox, 0, 1, 1, 2,
	                 GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(tbl2), tbl1, 1, 2, 1, 2,
	                 GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(tbl2), hbox, 1, 2, 2, 3,
	                 GTK_FILL, GTK_FILL, 0, 0);

	gtk_widget_show_all(tbl2);

	vbox = gtk_dialog_get_content_area(GTK_DIALOG(kpdlg->window));
	gtk_box_pack_start(GTK_BOX(vbox), tbl2, FALSE, FALSE, 0);

	tilem_keypad_dialog_calc_changed(kpdlg);

	return kpdlg;
}

/* Free a TilemKeypadDialog. */
void tilem_keypad_dialog_free(TilemKeypadDialog *kpdlg)
{
	g_return_if_fail(kpdlg != NULL);
	if (kpdlg->window)
		gtk_widget_destroy(kpdlg->window);
	g_slice_free(TilemKeypadDialog, kpdlg);
}

/* New calculator loaded. */
void tilem_keypad_dialog_calc_changed(TilemKeypadDialog *kpdlg)
{
	TilemCalc *calc;
	int i, j, k;
	GtkWidget *btn, *lbl;

	g_return_if_fail(kpdlg != NULL);
	g_return_if_fail(kpdlg->dbg != NULL);
	g_return_if_fail(kpdlg->dbg->emu != NULL);
	g_return_if_fail(kpdlg->dbg->emu->calc != NULL);
	calc = kpdlg->dbg->emu->calc;

	for (i = 0; i < NGROUPS; i++) {
		for (j = 0; j < NKEYS; j++) {
			btn = kpdlg->keys[i][j];
			k = i * 8 + j + 1;
			if (k != TILEM_KEY_ON
			    && calc->hw.keynames[k - 1] != NULL) {
				lbl = gtk_bin_get_child(GTK_BIN(btn));
				gtk_label_set_text(GTK_LABEL(lbl),
				                   calc->hw.keynames[k - 1]);
				gtk_widget_show(btn);
			}
			else {
				gtk_widget_hide(btn);
			}
		}
	}

	tilem_keypad_dialog_refresh(kpdlg);
}

/* Refresh key states. */
void tilem_keypad_dialog_refresh(TilemKeypadDialog *kpdlg)
{
	int i, j;
	byte keys[NGROUPS], inval, outval;
	TilemCalcEmulator *emu;
	GtkWidget *btn, *lbl;

	g_return_if_fail(kpdlg != NULL);
	g_return_if_fail(kpdlg->dbg != NULL);
	g_return_if_fail(kpdlg->dbg->emu != NULL);
	emu = kpdlg->dbg->emu;

	if (kpdlg->refreshing)
		return;

	kpdlg->refreshing = TRUE;

	tilem_calc_emulator_lock(emu);
	for (i = 0; i < NGROUPS; i++)
		keys[i] = emu->calc->keypad.keysdown[i];
	outval = emu->calc->keypad.group;
	inval = tilem_keypad_read_keys(emu->calc);
	tilem_calc_emulator_unlock(emu);

	for (i = 0; i < NGROUPS; i++) {
		for (j = 0; j < NKEYS; j++) {
			btn = kpdlg->keys[i][j];
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btn),
			                             (keys[i] & (1 << j)));
		}
	}


	for (i = 0; i < NGROUPS; i++) {
		btn = kpdlg->output[i];
		if (emu->paused) {
			gtk_widget_set_sensitive(btn, TRUE);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btn),
			                             !(outval & (1 << i)));
		}
		else {
			gtk_widget_set_sensitive(btn, FALSE);
		}
	}

	for (j = 0; j < NKEYS; j++) {
		lbl = kpdlg->input[j];
		gtk_label_set_text(GTK_LABEL(lbl), 
		                   (emu->paused
		                    ? (inval & (1 << j) ? "1" : "0")
		                    : ""));

	}

	kpdlg->refreshing = FALSE;
}
