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

/*
 * Modified to run in Android OS. Dritan Hashorva 2013
 */


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <glib/gstdio.h>
#include <ticalcs.h>
#include <tilem.h>

#include "gui.h"
#include "emucore.h"
#include "msgbox.h"
#include "filedlg.h"

#define MILLISEC_PER_FRAME 30
#define MICROSEC_PER_FRAME (MILLISEC_PER_FRAME * 1000)

#define GRAY_WINDOW_SIZE 4
#define GRAY_SAMPLE_INT 200


/* Lock emulator.  Notify the core loop that we wish to do so - note
   that if the core is running at full speed, it keeps the mutex
   locked almost all the time, so we need to explicitly ask it to
   yield.  (This may not be necessary with all mutex implementations,
   but definitely is necessary with some.) */
void tilem_calc_emulator_lock(TilemCalcEmulator *emu)
{
	g_atomic_int_inc(&emu->calc_lock_waiting);
	g_mutex_lock(emu->calc_mutex);
}

/* Unlock emulator and (if no other threads are waiting to lock it)
   wake up the core thread.  This also serves to resume emulation if
   the calculator has been powered off. */
void tilem_calc_emulator_unlock(TilemCalcEmulator *emu)
{
	if (g_atomic_int_dec_and_test(&emu->calc_lock_waiting))
		g_cond_signal(emu->calc_wakeup_cond);
	g_mutex_unlock(emu->calc_mutex);
}

static gboolean refresh_lcd(gpointer data)
{
	TilemCalcEmulator* emu = data;

	if (emu->ewin)
	//	tilem_emulator_window_refresh_lcd(emu->ewin);

	return FALSE;
}

static void tmr_screen_update(TilemCalc *calc, void *data)
{
/*	TilemCalcEmulator *emu = data;
	dword old_stamp;

	g_mutex_lock(emu->lcd_mutex);

	old_stamp = emu->lcd_buffer->stamp;

	if (emu->glcd)
		tilem_gray_lcd_get_frame(emu->glcd, emu->lcd_buffer);
	else
		tilem_lcd_get_frame(calc, emu->lcd_buffer);

	if (emu->anim) {
		if (emu->anim_grayscale) {
			tilem_animation_append_frame(emu->anim,
			                             emu->lcd_buffer,
			                             MILLISEC_PER_FRAME);
		}
		else {
			tilem_lcd_get_frame(calc, emu->tmp_lcd_buffer);
			tilem_animation_append_frame(emu->anim,
			                             emu->tmp_lcd_buffer,
			                             MILLISEC_PER_FRAME);
		}
	}

	if (!emu->lcd_update_pending && emu->lcd_buffer->stamp != old_stamp) {
		emu->lcd_update_pending = TRUE;
		g_idle_add_full(G_PRIORITY_DEFAULT, &refresh_lcd, emu, NULL);
	}

	g_mutex_unlock(emu->lcd_mutex); */
}

static void cancel_animation(TilemCalcEmulator *emu)
{

}


static void link_update_nop()
{
}

TilemCalcEmulator *tilem_calc_emulator_new()
{
	TilemCalcEmulator *emu = g_new0(TilemCalcEmulator, 1);
	CalcUpdate *update;
	int rate, channels;
	double latency, volume;
	char *driver;

	emu->calc_mutex = g_mutex_new();
	emu->calc_wakeup_cond = g_cond_new();
	emu->lcd_mutex = g_mutex_new();

	emu->task_queue = g_queue_new();
	emu->task_finished_cond = g_cond_new();

	emu->timer = g_timer_new();

	emu->pbar_mutex = g_mutex_new();

	update = g_new0(CalcUpdate, 1);
	update->start = &link_update_nop;
	update->stop = &link_update_nop;
	update->refresh = &link_update_nop;
	update->pbar = &link_update_nop;
	update->label = &link_update_nop;
	emu->link_update = update;

	emu->ext_cable_in = -1;
	emu->ext_cable_out = -1;

	if (rate <= 0) rate = DEFAULT_AUDIO_RATE;
	if (channels < 1 || channels > 2) channels = DEFAULT_AUDIO_CHANNELS;
	if (latency <= 0.0) latency = DEFAULT_AUDIO_LATENCY;
	if (volume <= 0.0) volume = DEFAULT_AUDIO_VOLUME;



	return emu;
}

void tilem_calc_emulator_free(TilemCalcEmulator *emu)
{
	g_return_if_fail(emu != NULL);

	tilem_calc_emulator_cancel_tasks(emu);

	tilem_calc_emulator_lock(emu);
	cancel_animation(emu);
	emu->exiting = TRUE;
	tilem_calc_emulator_unlock(emu);

	if (emu->z80_thread)
		g_thread_join(emu->z80_thread);

	g_free(emu->key_queue);

	g_free(emu->rom_file_name);
	g_free(emu->state_file_name);

	g_mutex_free(emu->calc_mutex);
	g_mutex_free(emu->lcd_mutex);
	g_cond_free(emu->calc_wakeup_cond);

	g_cond_free(emu->task_finished_cond);
	g_queue_free(emu->task_queue);

	g_timer_destroy(emu->timer);

	g_mutex_free(emu->pbar_mutex);
	g_free(emu->link_update);


	if (emu->lcd_buffer)
		tilem_lcd_buffer_free(emu->lcd_buffer);
	if (emu->tmp_lcd_buffer)
		tilem_lcd_buffer_free(emu->tmp_lcd_buffer);
	//	tilem_audio_filter_free(emu->audio_filter);
	if (emu->glcd)
		tilem_gray_lcd_free(emu->glcd);
	if (emu->calc)
		tilem_calc_free(emu->calc);

	g_free(emu);
}

static char *get_sav_name(const char *romname)
{
	char *dname, *bname, *sname, *suff;

	dname = g_path_get_dirname(romname);
	bname = g_path_get_basename(romname);

	if ((suff = strrchr(bname, '.')))
		*suff = 0;
	sname = g_strconcat(dname, G_DIR_SEPARATOR_S, bname, ".sav", NULL);

	g_free(dname);
	g_free(bname);
	return sname;
}

gboolean tilem_calc_emulator_load_state(TilemCalcEmulator *emu,
                                        const char *romfname,
                                        const char *statefname,
                                        int model, GError **err)
{
	/*
	const char *modelname;
	FILE *romfile, *savfile;
	char *rname = NULL, *sname = NULL;
	TilemCalc *calc;
	char *dname;
	int errnum;

	g_return_val_if_fail(emu != NULL, FALSE);
	g_return_val_if_fail(err == NULL || *err == NULL, FALSE);

	tilem_calc_emulator_cancel_tasks(emu);

	if (romfname)
		rname = g_strdup(romfname);
	if (!sname && statefname)
		sname = g_strdup(statefname);

	if (!rname && model) {
		modelname = model_to_name(model);
		g_return_val_if_fail(modelname != NULL, FALSE);
		if (sname) g_free(sname);
		tilem_config_get(modelname,
		                 "rom_file/f", &rname,
		                 "state_file/f", &sname,
		                 NULL);
	}

	if (!rname) {
		g_set_error(err, TILEM_EMULATOR_ERROR,
		            TILEM_EMULATOR_ERROR_NO_ROM,
		            _("No ROM file specified"));
		g_free(rname);
		g_free(sname);
		return FALSE;
	}

	romfile = g_fopen(rname, "rb");
	if (!romfile) {
		errnum = errno;
		dname = g_filename_display_basename(rname);
		g_set_error(err, G_FILE_ERROR,
		            g_file_error_from_errno(errnum),
		            _("Unable to open %s for reading: %s"),
		            dname, g_strerror(errnum));
		g_free(dname);
		g_free(rname);
		g_free(sname);
		return FALSE;
	}

	if (!sname)
		sname = get_sav_name(rname);

	savfile = g_fopen(sname, "rb");

	if (!savfile && errno != ENOENT) {
		errnum = errno;
		dname = g_filename_display_basename(sname);
		g_set_error(err, G_FILE_ERROR,
		            g_file_error_from_errno(errnum),
		            _("Unable to open %s for reading: %s"),
		            dname, g_strerror(errnum));
		g_free(dname);
		g_free(rname);
		g_free(sname);
		fclose(romfile);
		return FALSE;
	}



	if (!model && savfile)
		model = tilem_get_sav_type(savfile);


	if (!model) {
		model = tilem_guess_rom_type(romfile);
		if (model) {
			model = choose_rom_popup(get_toplevel(emu),
						 rname, model);
		}
		else {
			dname = g_filename_display_basename(rname);
			g_set_error(err, TILEM_EMULATOR_ERROR,
			            TILEM_EMULATOR_ERROR_INVALID_ROM,
			            _("The file %s is not a recognized"
			              " calculator ROM file."),
			            dname);
			g_free(dname);
		}
	}

	if (!model) {
		fclose(romfile);
		if (savfile) fclose(savfile);
		g_free(rname);
		g_free(sname);
		return FALSE;
	}


	calc = tilem_calc_new(model);
	if (tilem_calc_load_state(calc, romfile, savfile)) {
		g_set_error(err, TILEM_EMULATOR_ERROR,
		            TILEM_EMULATOR_ERROR_INVALID_STATE,
		            _("The specified ROM or state file is invalid."));
		fclose(romfile);
		if (savfile) fclose(savfile);
		g_free(rname);
		g_free(sname);
		return FALSE;
	}

	if (!savfile) {
		savfile = g_fopen(sname, "wb");
		if (savfile)
			fprintf(savfile, "MODEL = %s\n", calc->hw.name);
	}

	fclose(romfile);
	if (savfile) fclose(savfile);


	tilem_calc_emulator_lock(emu);

	cancel_animation(emu);

	if (emu->audio_filter)
		tilem_audio_filter_free(emu->audio_filter);
 	if (emu->glcd)
		tilem_gray_lcd_free(emu->glcd);
	if (emu->calc)
		tilem_calc_free(emu->calc);

	emu->calc = calc;
	emu->lcd_buffer = tilem_lcd_buffer_new();
	emu->tmp_lcd_buffer = tilem_lcd_buffer_new();

	if (emu->grayscale)
		emu->glcd = tilem_gray_lcd_new(calc, GRAY_WINDOW_SIZE,
		                               GRAY_SAMPLE_INT);
	else
		emu->glcd = NULL;

	emu->audio_filter = NULL;

	tilem_z80_add_timer(calc, MICROSEC_PER_FRAME,
	                    MICROSEC_PER_FRAME, 1,
	                    &tmr_screen_update, emu);

	tilem_calc_emulator_unlock(emu);

	if (emu->rom_file_name)
		g_free(emu->rom_file_name);
	emu->rom_file_name = rname;

	if (emu->state_file_name)
		g_free(emu->state_file_name);
	emu->state_file_name = sname;

	tilem_keybindings_init(emu, calc->hw.name);

	if (emu->ewin)
		tilem_emulator_window_calc_changed(emu->ewin);
	if (emu->dbg)
		tilem_debugger_calc_changed(emu->dbg);

	if (emu->rcvdlg)
		tilem_receive_dialog_free(emu->rcvdlg);
	emu->rcvdlg = NULL;
*/
	return TRUE;
}

gboolean tilem_calc_emulator_revert_state(TilemCalcEmulator *emu, GError **err)
{
	FILE *romfile, *savfile;
	char *dname;
	int errnum = 0;
	gboolean status = TRUE;

	g_return_val_if_fail(emu != NULL, FALSE);
	g_return_val_if_fail(emu->calc != NULL, FALSE);
	g_return_val_if_fail(emu->rom_file_name != NULL, FALSE);
	g_return_val_if_fail(emu->state_file_name != NULL, FALSE);
	g_return_val_if_fail(err == NULL || *err == NULL, FALSE);

	/* Open ROM file */

	if (emu->calc->hw.flags & TILEM_CALC_HAS_FLASH) {
		romfile = g_fopen(emu->rom_file_name, "rb");
		if (!romfile) {
			errnum = errno;
			dname = g_filename_display_basename(emu->rom_file_name);
			g_set_error(err, G_FILE_ERROR,
			            g_file_error_from_errno(errnum),
			            _("Unable to open %s for reading: %s"),
			            dname, g_strerror(errnum));
			g_free(dname);
			return FALSE;
		}
	}
	else {
		romfile = NULL;
	}

	/* Open state file */

	savfile = g_fopen(emu->state_file_name, "rb");
	if (!savfile) {
		errnum = errno;
		dname = g_filename_display_basename(emu->state_file_name);
		g_set_error(err, G_FILE_ERROR,
		            g_file_error_from_errno(errnum),
		            _("Unable to open %s for reading: %s"),
		            dname, g_strerror(errnum));
		g_free(dname);
		if (romfile) fclose(romfile);
		return FALSE;
	}

	/* Read state */

	tilem_calc_emulator_lock(emu);

	if (tilem_calc_load_state(emu->calc, romfile, savfile)) {
		g_set_error(err, TILEM_EMULATOR_ERROR,
		            TILEM_EMULATOR_ERROR_INVALID_STATE,
		            _("The specified ROM or state file is invalid."));
		status = FALSE;
	}

	tilem_calc_emulator_unlock(emu);

	if (emu->dbg)
	//	tilem_debugger_refresh(emu->dbg, TRUE);

	if (romfile) fclose(romfile);
	fclose(savfile);
	return status;
}


void tilem_calc_emulator_reset(TilemCalcEmulator *emu)
{
	g_return_if_fail(emu != NULL);
	g_return_if_fail(emu->calc != NULL);

	tilem_calc_emulator_lock(emu);
	tilem_calc_reset(emu->calc);
	tilem_calc_emulator_unlock(emu);

//	if (emu->dbg)
		//tilem_debugger_refresh(emu->dbg, TRUE);
}

void tilem_calc_emulator_pause(TilemCalcEmulator *emu)
{
	g_return_if_fail(emu != NULL);

	tilem_calc_emulator_lock(emu);
	emu->paused = TRUE;
	tilem_calc_emulator_unlock(emu);
}

void tilem_calc_emulator_run(TilemCalcEmulator *emu)
{
	g_return_if_fail(emu != NULL);
	g_return_if_fail(emu->calc != NULL);

	tilem_calc_emulator_lock(emu);
	emu->paused = FALSE;
	tilem_calc_emulator_unlock(emu);

	if (!emu->z80_thread)
		emu->z80_thread = g_thread_create(&tilem_em_main, emu, TRUE, NULL);
}

void tilem_calc_emulator_set_limit_speed(TilemCalcEmulator *emu,
                                         gboolean limit)
{
	emu->limit_speed = limit;
}

void tilem_calc_emulator_set_grayscale(TilemCalcEmulator *emu,
                                       gboolean grayscale)
{
	emu->grayscale = grayscale;

	if (grayscale && emu->calc && !emu->glcd) {
		tilem_calc_emulator_lock(emu);
		emu->glcd = tilem_gray_lcd_new(emu->calc, GRAY_WINDOW_SIZE,
		                               GRAY_SAMPLE_INT);
		tilem_calc_emulator_unlock(emu);
	}
	else if (!grayscale && emu->glcd) {
		tilem_calc_emulator_lock(emu);
		tilem_gray_lcd_free(emu->glcd);
		emu->glcd = NULL;
		tilem_calc_emulator_unlock(emu);
	}
}

void tilem_calc_emulator_set_audio(TilemCalcEmulator *emu,
                                   gboolean enable)
{

}

void tilem_calc_emulator_set_audio_volume(TilemCalcEmulator *emu,
                                          double volume)
{
/*	tilem_calc_emulator_lock(emu);
	emu->audio_volume = volume;
	if (emu->audio_filter)
		tilem_audio_filter_set_volume(emu->audio_filter, volume);
	tilem_calc_emulator_unlock(emu);*/
}

void tilem_calc_emulator_set_audio_options(TilemCalcEmulator *emu,
                                           const TilemAudioOptions *options)
{

}

void tilem_calc_emulator_set_link_cable(TilemCalcEmulator *emu,
                                        const CableOptions *options)
{
	tilem_calc_emulator_lock(emu);
	if (options)
		emu->ext_cable_options = *options;
	else
		emu->ext_cable_options.model = CABLE_NUL;
	emu->ext_cable_changed = TRUE;
	tilem_calc_emulator_unlock(emu);
}

/* If currently recording a macro, record a keypress */
static void record_key(TilemCalcEmulator* emu, int code)
{
/*	char* codechar;
	int type = 0;

	if (emu->isMacroRecording) {
		codechar = g_strdup_printf("%04d", code);
		tilem_macro_add_action(emu->macro, type, codechar);     
		g_free(codechar);
	}*/
}

void tilem_calc_emulator_press_key(TilemCalcEmulator *emu, int key)
{
	/*g_return_if_fail(emu != NULL);

	if (key == 0)
		return;

	tilem_calc_emulator_lock(emu);
	tilem_keypad_press_key(emu->calc, key);
	tilem_calc_emulator_unlock(emu);

	record_key(emu, key);*/
}

void tilem_calc_emulator_release_key(TilemCalcEmulator *emu, int key)
{
	g_return_if_fail(emu != NULL);

	if (key == 0)
		return;

	tilem_calc_emulator_lock(emu);
	tilem_keypad_release_key(emu->calc, key);
	tilem_calc_emulator_unlock(emu);

}

static gboolean refresh_kpd(gpointer data)
{
	TilemCalcEmulator *emu = data;

	return FALSE;
}

/* Timer callback for key sequences */
static void tmr_key_queue(TilemCalc* calc, void* data)
{
	TilemCalcEmulator *emu = data;
	int nextkey;

	if (emu->key_queue_pressed) {
		if (emu->key_queue_len > 0 || !emu->key_queue_hold) {
			tilem_keypad_release_key(calc, emu->key_queue_cur);
			emu->key_queue_pressed = 0;
			emu->key_queue_cur = 0;
			tilem_z80_set_timer(calc, emu->key_queue_timer,
			                    50000, 0, 1);
		}
		else {
			tilem_z80_remove_timer(calc, emu->key_queue_timer);
			emu->key_queue_timer = 0;
		}
	}
	else {
		if (emu->key_queue_len > 0) {
			nextkey = emu->key_queue[--emu->key_queue_len];
			tilem_keypad_press_key(calc, nextkey);
			emu->key_queue_pressed = 1;
			emu->key_queue_cur = nextkey;
			tilem_z80_set_timer(calc, emu->key_queue_timer,
			                    20000, 0, 1);
		}
		else {
			tilem_z80_remove_timer(calc, emu->key_queue_timer);
			emu->key_queue_timer = 0;
		}
	}

	g_idle_add(&refresh_kpd, emu);
}

static void queue_keys(TilemCalcEmulator *emu, const byte *keys, int nkeys)
{
	byte *q;
	int i;

	q = g_new(byte, emu->key_queue_len + nkeys);

	for (i = 0; i < nkeys; i++) {
		q[nkeys - i - 1] = keys[i];
		record_key(emu, keys[i]);
	}

	if (emu->key_queue_len)
		memcpy(q + nkeys, emu->key_queue, emu->key_queue_len);

	g_free(emu->key_queue);
	emu->key_queue = q;
	emu->key_queue_len += nkeys;
	emu->key_queue_hold = 1;

	if (!emu->key_queue_timer) {
		emu->key_queue_timer
			= tilem_z80_add_timer(emu->calc, 1, 0, 1,
			                      &tmr_key_queue, emu);
	}
}

void tilem_calc_emulator_queue_keys(TilemCalcEmulator *emu,
                                    const byte *keys, int nkeys)
{
	g_return_if_fail(emu != NULL);
	g_return_if_fail(keys != NULL);
	g_return_if_fail(nkeys > 0);

	tilem_calc_emulator_lock(emu);
	queue_keys(emu, keys, nkeys);
	tilem_calc_emulator_unlock(emu);
}

void tilem_calc_emulator_release_queued_key(TilemCalcEmulator *emu)
{
	g_return_if_fail(emu != NULL);

	tilem_calc_emulator_lock(emu);
	if (emu->key_queue_timer) {
		emu->key_queue_hold = 0;
	}
	else if (emu->key_queue_pressed) {
		tilem_keypad_release_key(emu->calc, emu->key_queue_cur);
		emu->key_queue_pressed = 0;
		emu->key_queue_cur = 0;
	}
	tilem_calc_emulator_unlock(emu);
}

gboolean tilem_calc_emulator_press_or_queue(TilemCalcEmulator *emu,
                                            int key)
{
	byte b;
	gboolean status;

	g_return_val_if_fail(emu != NULL, FALSE);

	tilem_calc_emulator_lock(emu);

	if (emu->key_queue_timer) {
		b = key;
		queue_keys(emu, &b, 1);
		status = TRUE;
	}
	else {
		tilem_keypad_press_key(emu->calc, key);
		status = FALSE;
	}
	tilem_calc_emulator_unlock(emu);

	return status;
}

TilemAnimation * tilem_calc_emulator_get_screenshot(TilemCalcEmulator *emu,
                                                    gboolean grayscale)
{
/*	TilemAnimation *anim;

	g_return_val_if_fail(emu != NULL, NULL);
	g_return_val_if_fail(emu->calc != NULL, NULL);

	anim = tilem_animation_new(emu->calc->hw.lcdwidth,
	                           emu->calc->hw.lcdheight);

	tilem_calc_emulator_lock(emu);

	if (grayscale && emu->glcd)
		tilem_gray_lcd_get_frame(emu->glcd, emu->tmp_lcd_buffer);
	else
		tilem_lcd_get_frame(emu->calc, emu->tmp_lcd_buffer);

	tilem_animation_append_frame(anim, emu->tmp_lcd_buffer, 1);

	tilem_calc_emulator_unlock(emu);

	return anim;*/
}

void tilem_calc_emulator_begin_animation(TilemCalcEmulator *emu,
                                         gboolean grayscale)
{
/*	g_return_if_fail(emu != NULL);
	g_return_if_fail(emu->calc != NULL);

	tilem_calc_emulator_lock(emu);
	cancel_animation(emu);
	emu->anim = tilem_animation_new(emu->calc->hw.lcdwidth,
	                                emu->calc->hw.lcdheight);
	emu->anim_grayscale = grayscale;
	tilem_calc_emulator_unlock(emu);*/
}

TilemAnimation * tilem_calc_emulator_end_animation(TilemCalcEmulator *emu)
{
return NULL;
}

/* Prompt for a ROM file to open */
int tilem_calc_emulator_prompt_open_rom(TilemCalcEmulator *emu)
{
/*	char *dir, *filename;
	GError *err = NULL;
	const char *modelname;

	if (emu->rom_file_name)
		dir = g_path_get_dirname(emu->rom_file_name);
	else
		dir = g_get_current_dir();

	filename = prompt_open_file(_("Open Calculator"), GTK_WINDOW(get_toplevel(emu)),
	                            dir, _("ROM files"), "*.rom;*.clc;*.bin",
	                            _("All files"), "*", NULL);
	g_free(dir);
	if (!filename)
		return 0;

	if (tilem_calc_emulator_load_state(emu, filename, NULL,
	                                   0, &err)) {
		modelname = emu->calc->hw.name;
		tilem_config_set(modelname,
		                 "rom_file/f", emu->rom_file_name,
		                 "state_file/f", emu->state_file_name,
		                 NULL);
		tilem_config_set("recent", "last_model/s", modelname, NULL);
	}
	g_free(filename);

	if (err) {

		g_error_free(err);
		return -1;
	}
	else {
		return 1;
	}*/
}

/* Run slowly to play macro (used instead run_with_key() function) */
void run_with_key_slowly(TilemCalc* calc, int key)
{
	tilem_z80_run_time(calc, 5000000, NULL); /* Wait */
	tilem_keypad_press_key(calc, key); /* Press */
	tilem_z80_run_time(calc, 10000, NULL); /* Wait (don't forget to wait) */
	tilem_keypad_release_key(calc, key); /* Release */
	tilem_z80_run_time(calc, 50, NULL); /* Wait */
}
