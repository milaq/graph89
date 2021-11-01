/*
 * TilEm II
 *
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
#include <gtk/gtk.h>
#include <ticalcs.h>
#include <tilem.h>

#include "gui.h"

static const struct {
	CableModel model;
	const char *description;
} cable_types[] = {
	{ CABLE_GRY, N_("Gray TI Graph Link") },
	{ CABLE_BLK, N_("Black TI Graph Link") },
	{ CABLE_SLV, N_("Silver TI Graph Link (USB)") },
	{ CABLE_PAR, N_("Parallel Link") }};

/* Global lock used for external ticables operations (defined in
   emucore.c) */
G_LOCK_EXTERN(tilem_ticables_io);

struct link_setup_dlg {
	int usb_port_num;
	int usb_port_count;

	CableModel models[4];

	/* button to open audio settings */
	GtkWidget *audio_setup_btn;

	/* table containing settings for external cable */
	GtkWidget *tbl;

	/* port and timeout widgets */
	GtkWidget *port_lbl;
	GtkWidget *port_sb;
	GtkWidget *timeout_lbl;
	GtkWidget *timeout_sb;
};

static void audio_toggled(GtkToggleButton *tb, gpointer data)
{
	struct link_setup_dlg *lsdlg = data;

	/* set audio options insensitive unless 'audio' is selected */
	if (gtk_toggle_button_get_active(tb))
		gtk_widget_set_sensitive(lsdlg->audio_setup_btn, TRUE);
	else
		gtk_widget_set_sensitive(lsdlg->audio_setup_btn, FALSE);
}

static void audio_setup_clicked(G_GNUC_UNUSED GtkButton *btn, void *data)
{
	TilemEmulatorWindow *ewin = data;
	tilem_audio_setup_dialog(ewin);
}

static void ext_toggled(GtkToggleButton *tb, gpointer data)
{
	struct link_setup_dlg *lsdlg = data;

	/* set external cable options insensitive unless 'external' is
	   selected */
	if (gtk_toggle_button_get_active(tb))
		gtk_widget_set_sensitive(lsdlg->tbl, TRUE);
	else
		gtk_widget_set_sensitive(lsdlg->tbl, FALSE);
}

static void ext_type_changed(GtkComboBox *combo, gpointer data)
{
	struct link_setup_dlg *lsdlg = data;
	int i = gtk_combo_box_get_active(combo);
	CableModel m = (i < 0 ? CABLE_NUL : lsdlg->models[i]);

	/* set port option insensitive if SilverLink is selected and
	   only one USB cable is present (make things less confusing
	   for the user in 99% of situations) */

	if (m == CABLE_SLV && lsdlg->usb_port_count < 2) {
		gtk_widget_set_sensitive(lsdlg->port_lbl, FALSE);
		gtk_widget_set_sensitive(lsdlg->port_sb, FALSE);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(lsdlg->port_sb),
		                          lsdlg->usb_port_num);
	}
	else {
		gtk_widget_set_sensitive(lsdlg->port_lbl, TRUE);
		gtk_widget_set_sensitive(lsdlg->port_sb, TRUE);
	}

	/* set timeout option insensitive for "raw" cables */

	if (m == CABLE_BLK || m == CABLE_PAR) {
		gtk_widget_set_sensitive(lsdlg->timeout_lbl, FALSE);
		gtk_widget_set_sensitive(lsdlg->timeout_sb, FALSE);
	}
	else {
		gtk_widget_set_sensitive(lsdlg->timeout_lbl, TRUE);
		gtk_widget_set_sensitive(lsdlg->timeout_sb, TRUE);
	}
}

/* Check if a given cable type is supported by the ticables library. */
static gboolean cable_supported(CableModel model)
{
	CableHandle *h;

#ifndef HAVE_TICABLES_CABLE_SET_RAW
	/* Disable black/parallel cables if raw API is not available */
	if (model == CABLE_BLK || model == CABLE_PAR)
		return FALSE;
#endif

	h = ticables_handle_new(model, 1);
	if (!h)
		return FALSE;
	ticables_handle_del(h);
	return TRUE;
}

void tilem_link_setup_dialog(TilemEmulatorWindow *ewin)
{
	GtkWidget *dlg, *lbl, *vbox, *vbox2, *hbox, *frame, *align;
	GtkWidget *none_rb, *virtual_rb, *external_rb, *audio_rb, *type_combo;
	struct link_setup_dlg lsdlg;
	CableOptions cur_opts;
	gboolean audio = FALSE;
	char *modelstr;
	int *usbpids = NULL, nusbpids;
	int portnum, timeout, i, j;
	const char *s;

	/* Scan for USB devices */

	G_LOCK(tilem_ticables_io);

	lsdlg.usb_port_num = 1;
	lsdlg.usb_port_count = 0;
	if (!ticables_get_usb_devices(&usbpids, &nusbpids)) {
		for (i = 0; i < nusbpids; i++) {
			if (usbpids[i] == PID_TIGLUSB) {
				lsdlg.usb_port_num = i + 1;
				lsdlg.usb_port_count++;
			}
		}
	}
	if (usbpids)
		free(usbpids);

	G_UNLOCK(tilem_ticables_io);

	/* If a link cable is currently connected, display the current
	   settings.  Otherwise display the settings for the most
	   recently connected external cable. */

	cur_opts = ewin->emu->ext_cable_options;

	if (cur_opts.model == CABLE_NUL || cur_opts.model == CABLE_TIE) {
		tilem_config_get("external_link",
		                 "model/s", &modelstr,
		                 "port/i", &portnum,
		                 "timeout/i", &timeout,
		                 NULL);
		if (modelstr)
			cur_opts.model = ticables_string_to_model(modelstr);
		else
			cur_opts.model = CABLE_SLV;
		g_free(modelstr);

		if (cur_opts.model == CABLE_SLV)
			portnum = lsdlg.usb_port_num;
		else if (portnum < 1 || portnum > 4)
			portnum = 1;
		cur_opts.port = portnum;

		if (timeout < 1)
			timeout = DFLT_TIMEOUT;
		cur_opts.timeout = timeout;
	}

	dlg = gtk_dialog_new_with_buttons(_("Link Port"),
	                                  GTK_WINDOW(ewin->window),
	                                  GTK_DIALOG_MODAL,
	                                  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	                                  GTK_STOCK_OK, GTK_RESPONSE_OK,
	                                  NULL);
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dlg),
	                                        GTK_RESPONSE_OK,
	                                        GTK_RESPONSE_CANCEL,
	                                        -1);
	gtk_dialog_set_default_response(GTK_DIALOG(dlg),
	                                GTK_RESPONSE_OK);

	vbox = gtk_vbox_new(FALSE, 12);

	none_rb = gtk_radio_button_new_with_mnemonic
		(NULL, _("_Disconnected"));
	gtk_box_pack_start(GTK_BOX(vbox), none_rb, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 12);
	audio_rb = gtk_radio_button_new_with_mnemonic_from_widget
		(GTK_RADIO_BUTTON(none_rb), _("Connected to _speakers"));
	gtk_box_pack_start(GTK_BOX(hbox), audio_rb, TRUE, TRUE, 0);

	lsdlg.audio_setup_btn = gtk_button_new_with_mnemonic(_("Ad_vanced..."));
	g_signal_connect(lsdlg.audio_setup_btn, "clicked",
	                 G_CALLBACK(audio_setup_clicked), ewin);

	gtk_box_pack_start(GTK_BOX(hbox), lsdlg.audio_setup_btn,
	                   FALSE, FALSE, 0);
#ifndef ENABLE_AUDIO
	gtk_widget_set_no_show_all(hbox, TRUE);
#endif
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	virtual_rb = gtk_radio_button_new_with_mnemonic_from_widget
		(GTK_RADIO_BUTTON(none_rb),
		 _("Connected to another _emulator (virtual cable)"));
	gtk_box_pack_start(GTK_BOX(vbox), virtual_rb, FALSE, FALSE, 0);

	vbox2 = gtk_vbox_new(FALSE, 6);

	external_rb = gtk_radio_button_new_with_mnemonic_from_widget
		(GTK_RADIO_BUTTON(none_rb),
		 _("Connected to an e_xternal link cable"));
	gtk_box_pack_start(GTK_BOX(vbox2), external_rb, FALSE, FALSE, 0);

	lsdlg.tbl = gtk_table_new(3, 2, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(lsdlg.tbl), 6);
	gtk_table_set_col_spacings(GTK_TABLE(lsdlg.tbl), 6);

	lbl = gtk_label_new_with_mnemonic(_("Cable _type:"));
	gtk_misc_set_alignment(GTK_MISC(lbl), LABEL_X_ALIGN, 0.5);
	gtk_table_attach(GTK_TABLE(lsdlg.tbl), lbl, 0, 1, 0, 1,
	                 GTK_FILL, GTK_FILL, 0, 0);

	type_combo = gtk_combo_box_new_text();
	for (i = j = 0; i < (int) G_N_ELEMENTS(cable_types); i++) {
		if (!cable_supported(cable_types[i].model))
			continue;

		gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo),
		                          _(cable_types[i].description));

		lsdlg.models[j] = cable_types[i].model;
		if (lsdlg.models[j] == cur_opts.model)
			gtk_combo_box_set_active(GTK_COMBO_BOX(type_combo), j);
		j++;
	}
	gtk_label_set_mnemonic_widget(GTK_LABEL(lbl), type_combo);
	gtk_table_attach(GTK_TABLE(lsdlg.tbl), type_combo, 1, 2, 0, 1,
	                 GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);

	lbl = gtk_label_new_with_mnemonic(_("_Port:"));
	lsdlg.port_lbl = lbl;
	gtk_misc_set_alignment(GTK_MISC(lbl), LABEL_X_ALIGN, 0.5);
	gtk_table_attach(GTK_TABLE(lsdlg.tbl), lbl, 0, 1, 1, 2,
	                 GTK_FILL, GTK_FILL, 0, 0);

	lsdlg.port_sb = gtk_spin_button_new_with_range(1.0, 4.0, 1.0);
	gtk_entry_set_activates_default(GTK_ENTRY(lsdlg.port_sb), TRUE);
	gtk_label_set_mnemonic_widget(GTK_LABEL(lbl), lsdlg.port_sb);
	gtk_table_attach(GTK_TABLE(lsdlg.tbl), lsdlg.port_sb, 1, 2, 1, 2,
	                 GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);

	lbl = gtk_label_new_with_mnemonic(_("Ti_meout (sec):"));
	lsdlg.timeout_lbl = lbl;
	gtk_misc_set_alignment(GTK_MISC(lbl), LABEL_X_ALIGN, 0.5);
	gtk_table_attach(GTK_TABLE(lsdlg.tbl), lbl, 0, 1, 2, 3,
	                 GTK_FILL, GTK_FILL, 0, 0);

	lsdlg.timeout_sb = gtk_spin_button_new_with_range(0.1, 60.0, 0.1);
	gtk_entry_set_activates_default(GTK_ENTRY(lsdlg.timeout_sb), TRUE);
	gtk_label_set_mnemonic_widget(GTK_LABEL(lbl), lsdlg.timeout_sb);
	gtk_table_attach(GTK_TABLE(lsdlg.tbl), lsdlg.timeout_sb, 1, 2, 2, 3,
	                 GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);

	align = gtk_alignment_new(0.5, 0.5, 1.0, 1.0);
	gtk_alignment_set_padding(GTK_ALIGNMENT(align), 0, 0, 12, 0);
	gtk_container_add(GTK_CONTAINER(align), lsdlg.tbl);
	gtk_box_pack_start(GTK_BOX(vbox2), align, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), vbox2, FALSE, FALSE, 0);

#ifdef G_OS_WIN32
	if (!ticables_is_usb_enabled()) {
		GtkWidget *hbox, *icon;
		const char *url = "http://lpg.ticalc.org/prj_tilp/";
		char *link, *msg;

		if (!gtk_check_version(2, 18, 0))
			link = g_strdup_printf("<a href=\"%s\">%s</a>",
			                       url, url);
		else
			link = g_strdup(url);
		msg = g_strdup_printf
			(_("USB drivers not found. You must install TiLP\n"
			   "(%s) in order to use\n"
			   "an external link cable."), link);

		hbox = gtk_hbox_new(FALSE, 0);
		icon = gtk_image_new_from_stock(GTK_STOCK_DIALOG_WARNING,
		                                GTK_ICON_SIZE_LARGE_TOOLBAR);
		gtk_box_pack_start(GTK_BOX(hbox), icon, FALSE, FALSE, 6);

		lbl = gtk_label_new(msg);
		gtk_label_set_use_markup(GTK_LABEL(lbl), TRUE);
		gtk_label_set_selectable(GTK_LABEL(lbl), TRUE);
		gtk_misc_set_alignment(GTK_MISC(lbl), 0.0, 0.5);
		gtk_box_pack_start(GTK_BOX(hbox), lbl, FALSE, FALSE, 0);
		g_free(link);
		g_free(msg);

		gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);

		gtk_widget_set_no_show_all(align, TRUE);
		gtk_widget_set_sensitive(external_rb, FALSE);
	}
#endif

	frame = new_frame(_("Link Port"), vbox);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 6);
	gtk_widget_show_all(frame);

	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(lsdlg.port_sb),
	                          cur_opts.port);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(lsdlg.timeout_sb),
	                          cur_opts.timeout * 0.1);

	if (ewin->emu->enable_audio)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(audio_rb), TRUE);
	else if (ewin->emu->ext_cable_options.model == CABLE_NUL)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(none_rb), TRUE);
	else if (ewin->emu->ext_cable_options.model == CABLE_TIE)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(virtual_rb), TRUE);
	else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(external_rb), TRUE);

	g_signal_connect(audio_rb, "toggled",
	                 G_CALLBACK(audio_toggled), &lsdlg);
	g_signal_connect(external_rb, "toggled",
	                 G_CALLBACK(ext_toggled), &lsdlg);
	g_signal_connect(type_combo, "changed",
	                 G_CALLBACK(ext_type_changed), &lsdlg);

	audio_toggled(GTK_TOGGLE_BUTTON(audio_rb), &lsdlg);
	ext_toggled(GTK_TOGGLE_BUTTON(external_rb), &lsdlg);
	ext_type_changed(GTK_COMBO_BOX(type_combo), &lsdlg);

	if (gtk_dialog_run(GTK_DIALOG(dlg)) != GTK_RESPONSE_OK) {
		gtk_widget_destroy(dlg);
		return;
	}

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(virtual_rb))) {
		/* virtual link */
		cur_opts.model = CABLE_TIE;
		cur_opts.port = 0;
		cur_opts.timeout = DFLT_TIMEOUT;
	}
	else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(external_rb))) {
		i = gtk_combo_box_get_active(GTK_COMBO_BOX(type_combo));
		if (i < 0) {
			cur_opts.model = CABLE_NUL;
		}
		else {
			cur_opts.model = lsdlg.models[i];
			cur_opts.port = gtk_spin_button_get_value_as_int
				(GTK_SPIN_BUTTON(lsdlg.port_sb));
			cur_opts.timeout = gtk_spin_button_get_value
				(GTK_SPIN_BUTTON(lsdlg.timeout_sb)) * 10.0 + 0.5;

			s = ticables_model_to_string(cur_opts.model);
			tilem_config_set("external_link",
			                 "model/s", s,
			                 "port/i", cur_opts.port,
			                 "timeout/i", cur_opts.timeout,
			                 NULL);
		}
	}
	else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(audio_rb))) {
		cur_opts.model = CABLE_NUL;
		audio = TRUE;
	}
	else {
		cur_opts.model = CABLE_NUL;
	}

	tilem_calc_emulator_set_audio(ewin->emu, audio);
	tilem_calc_emulator_set_link_cable(ewin->emu, &cur_opts);
	gtk_widget_destroy(dlg);
	return;
}
