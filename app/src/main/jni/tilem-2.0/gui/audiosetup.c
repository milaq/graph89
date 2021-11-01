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
#include <string.h>
#include <math.h>
#include <gtk/gtk.h>
#include <ticalcs.h>
#include <tilem.h>

#include "gui.h"

struct audio_setup_dlg {
	TilemCalcEmulator *emu;

	gboolean no_update;
	GtkWidget *vol_scl;
	GtkWidget *driver_combo;
	GtkWidget *rate_sb;
	GtkWidget *channels_sb;
	GtkWidget *latency_sb;
	guint update_timer;

	gboolean save_volume;
	gboolean save_driver;
	gboolean save_rate;
	gboolean save_channels;
	gboolean save_latency;
};

static double linear_to_db(double v)
{
	return 20.0 * log10(v);
}

static double db_to_linear(double v)
{
	return exp(v * 0.05 * G_LN10);
}

static char *format_volume(G_GNUC_UNUSED GtkScale *scl, gdouble value,
                           G_GNUC_UNUSED void *data)
{
	return g_strdup_printf(_("%+.2g dB"), value);
}

/* Update GUI to reflect current settings if device is active, or
   preferred settings if device is not active */
static gboolean update_settings(gpointer data)
{
	struct audio_setup_dlg *asdlg = data;
	TilemAudioOptions opts;

	tilem_calc_emulator_lock(asdlg->emu);
	if (asdlg->emu->audio_device)
		tilem_audio_device_get_options(asdlg->emu->audio_device, &opts);
	else
		opts = asdlg->emu->audio_options;
	tilem_calc_emulator_unlock(asdlg->emu);

	asdlg->no_update = TRUE;
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(asdlg->channels_sb),
	                          opts.channels);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(asdlg->rate_sb),
	                          opts.rate * 0.001);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(asdlg->latency_sb),
	                          opts.latency * 1000.0);
	asdlg->no_update = FALSE;

	asdlg->update_timer = 0;
	return FALSE;
}

/* After user changes settings, wait 1 second, then update the GUI to
   reflect the actual settings of the current output device */
static void update_later(struct audio_setup_dlg *asdlg)
{
	if (!asdlg->emu->enable_audio)
		return;
	if (asdlg->update_timer)
		g_source_remove(asdlg->update_timer);
	asdlg->update_timer = g_timeout_add(1000, &update_settings, asdlg);
}

static void volume_changed(GtkRange *rng, void *data)
{
	struct audio_setup_dlg *asdlg = data;
	double v = gtk_range_get_value(rng);

	if (asdlg->no_update)
		return;

	tilem_calc_emulator_set_audio_volume(asdlg->emu, db_to_linear(v));
	asdlg->save_volume = TRUE;
}

static void channels_changed(GtkSpinButton *sb, void *data)
{
	struct audio_setup_dlg *asdlg = data;
	TilemAudioOptions opts = asdlg->emu->audio_options;
	double v = gtk_spin_button_get_value(sb);

	if (asdlg->no_update)
		return;

	opts.channels = v + 0.5;
	tilem_calc_emulator_set_audio_options(asdlg->emu, &opts);
	update_later(asdlg);
	asdlg->save_channels = TRUE;
}

static void driver_changed(GtkComboBox *cmb, void *data)
{
	struct audio_setup_dlg *asdlg = data;
	TilemAudioOptions opts = asdlg->emu->audio_options;

	if (asdlg->no_update)
		return;

	if (gtk_combo_box_get_active(cmb) > 0)
		opts.driver = gtk_combo_box_get_active_text(cmb);
	else
		opts.driver = NULL;
	tilem_calc_emulator_set_audio_options(asdlg->emu, &opts);
	g_free(opts.driver);
	asdlg->save_driver = TRUE;
}

static void rate_changed(GtkSpinButton *sb, void *data)
{
	struct audio_setup_dlg *asdlg = data;
	TilemAudioOptions opts = asdlg->emu->audio_options;
	double v = gtk_spin_button_get_value(sb);

	if (asdlg->no_update)
		return;

	opts.rate = (v * 1000.0) + 0.5;
	tilem_calc_emulator_set_audio_options(asdlg->emu, &opts);
	update_later(asdlg);
	asdlg->save_rate = TRUE;
}

static void latency_changed(GtkSpinButton *sb, void *data)
{
	struct audio_setup_dlg *asdlg = data;
	TilemAudioOptions opts = asdlg->emu->audio_options;
	double v = gtk_spin_button_get_value(sb);

	if (asdlg->no_update)
		return;

	opts.latency = v * 0.001;
	tilem_calc_emulator_set_audio_options(asdlg->emu, &opts);
	update_later(asdlg);
	asdlg->save_latency = TRUE;
}

#define MINVOLUME -40.0
#define MAXVOLUME 10.0
#define MINRATE 8.000
#define MAXRATE 192.000

void tilem_audio_setup_dialog(TilemEmulatorWindow *ewin)
{
	struct audio_setup_dlg asdlg;
	GtkWidget *dlg, *tbl, *lbl, *vbox, *frame;
	const char * const *drivers;
	const TilemAudioOptions *opts;
	int i;

	memset(&asdlg, 0, sizeof(asdlg));
	asdlg.emu = ewin->emu;

	dlg = gtk_dialog_new_with_buttons(_("Audio Properties"),
	                                  GTK_WINDOW(ewin->window),
	                                  GTK_DIALOG_MODAL,
	                                  GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
	                                  NULL);

	tbl = gtk_table_new(5, 2, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(tbl), 6);
	gtk_table_set_col_spacings(GTK_TABLE(tbl), 6);

	lbl = gtk_label_new_with_mnemonic(_("_Volume:"));
	gtk_misc_set_alignment(GTK_MISC(lbl), LABEL_X_ALIGN, 0.5);
	gtk_table_attach(GTK_TABLE(tbl), lbl, 0, 1, 0, 1,
	                 GTK_FILL, GTK_FILL, 0, 0);

	asdlg.vol_scl = gtk_hscale_new_with_range(MINVOLUME, MAXVOLUME, 1.0);
	gtk_label_set_mnemonic_widget(GTK_LABEL(lbl), asdlg.vol_scl);
	gtk_table_attach(GTK_TABLE(tbl), asdlg.vol_scl, 1, 2, 0, 1,
	                 GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 0);

	lbl = gtk_label_new_with_mnemonic(_("C_hannels:"));
	gtk_misc_set_alignment(GTK_MISC(lbl), LABEL_X_ALIGN, 0.5);
	gtk_table_attach(GTK_TABLE(tbl), lbl, 0, 1, 1, 2,
	                 GTK_FILL, GTK_FILL, 0, 0);

	asdlg.channels_sb = gtk_spin_button_new_with_range(1.0, 2.0, 1.0);
	gtk_label_set_mnemonic_widget(GTK_LABEL(lbl), asdlg.channels_sb);
	gtk_table_attach(GTK_TABLE(tbl), asdlg.channels_sb, 1, 2, 1, 2,
	                 GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 0);

	lbl = gtk_label_new_with_mnemonic(_("_Driver:"));
	gtk_misc_set_alignment(GTK_MISC(lbl), LABEL_X_ALIGN, 0.5);
	gtk_table_attach(GTK_TABLE(tbl), lbl, 0, 1, 2, 3,
	                 GTK_FILL, GTK_FILL, 0, 0);

	asdlg.driver_combo = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(asdlg.driver_combo),
	                          _("Automatic"));
	if (!ewin->emu->audio_options.driver)
		gtk_combo_box_set_active(GTK_COMBO_BOX(asdlg.driver_combo), 0);

	drivers = tilem_audio_device_list_drivers();
	for (i = 0; drivers && drivers[i]; i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(asdlg.driver_combo),
		                          drivers[i]);

		if (ewin->emu->audio_options.driver
		    && !strcmp(ewin->emu->audio_options.driver, drivers[i]))
			gtk_combo_box_set_active
				(GTK_COMBO_BOX(asdlg.driver_combo), i + 1);
	}
	gtk_label_set_mnemonic_widget(GTK_LABEL(lbl), asdlg.driver_combo);
	gtk_table_attach(GTK_TABLE(tbl), asdlg.driver_combo, 1, 2, 2, 3,
	                 GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 0);

	lbl = gtk_label_new_with_mnemonic(_("Sampling _rate (kHz):"));
	gtk_misc_set_alignment(GTK_MISC(lbl), LABEL_X_ALIGN, 0.5);
	gtk_table_attach(GTK_TABLE(tbl), lbl, 0, 1, 3, 4,
	                 GTK_FILL, GTK_FILL, 0, 0);

	asdlg.rate_sb = gtk_spin_button_new_with_range(MINRATE, MAXRATE, 1.0);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(asdlg.rate_sb), 3);
	gtk_label_set_mnemonic_widget(GTK_LABEL(lbl), asdlg.rate_sb);
	gtk_table_attach(GTK_TABLE(tbl), asdlg.rate_sb, 1, 2, 3, 4,
	                 GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 0);

	lbl = gtk_label_new_with_mnemonic(_("_Latency (ms):"));
	gtk_misc_set_alignment(GTK_MISC(lbl), LABEL_X_ALIGN, 0.5);
	gtk_table_attach(GTK_TABLE(tbl), lbl, 0, 1, 4, 5,
	                 GTK_FILL, GTK_FILL, 0, 0);

	asdlg.latency_sb = gtk_spin_button_new_with_range(1.0, 1000.0, 1.0);
	gtk_label_set_mnemonic_widget(GTK_LABEL(lbl), asdlg.latency_sb);
	gtk_table_attach(GTK_TABLE(tbl), asdlg.latency_sb, 1, 2, 4, 5,
	                 GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 0);

	frame = new_frame(_("Audio Properties"), tbl);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 6);
	gtk_widget_show_all(frame);

	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);

	gtk_range_set_value(GTK_RANGE(asdlg.vol_scl),
	                    linear_to_db(ewin->emu->audio_volume));

	g_signal_connect(asdlg.vol_scl, "format-value",
	                 G_CALLBACK(format_volume), NULL);
	g_signal_connect(asdlg.vol_scl, "value-changed",
	                 G_CALLBACK(volume_changed), &asdlg);
	g_signal_connect(asdlg.channels_sb, "value-changed",
	                 G_CALLBACK(channels_changed), &asdlg);
	g_signal_connect(asdlg.driver_combo, "changed",
	                 G_CALLBACK(driver_changed), &asdlg);
	g_signal_connect(asdlg.rate_sb, "value-changed",
	                 G_CALLBACK(rate_changed), &asdlg);
	g_signal_connect(asdlg.latency_sb, "value-changed",
	                 G_CALLBACK(latency_changed), &asdlg);
	update_settings(&asdlg);

	gtk_dialog_run(GTK_DIALOG(dlg));
	gtk_widget_destroy(dlg);

	if (asdlg.update_timer)
		g_source_remove(asdlg.update_timer);

	/* save manually-entered values (do not save values that the
	   user didn't touch, and use the values originally entered
	   rather than those obtained by the device) */

	opts = &asdlg.emu->audio_options;
	if (asdlg.save_driver)
		tilem_config_set("audio", "driver/s",
		                 (opts->driver ? opts->driver : ""),
		                 NULL);
	if (asdlg.save_channels)
		tilem_config_set("audio", "channels/i", opts->channels, NULL);
	if (asdlg.save_rate)
		tilem_config_set("audio", "rate/i", opts->rate, NULL);
	if (asdlg.save_latency)
		tilem_config_set("audio", "latency/r", opts->latency, NULL);
	if (asdlg.save_volume)
		tilem_config_set("audio", "volume/r",
		                 asdlg.emu->audio_volume, NULL);
}
