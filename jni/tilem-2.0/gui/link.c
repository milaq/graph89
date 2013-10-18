/*
 * TilEm II
 *
 * Copyright (c) 2010-2012 Thibault Duponchelle
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
#include <errno.h>
#include <glib/gstdio.h>
#include <ticalcs.h>
#include <ticonv.h>
#include <string.h>
#include <tilem.h>
#include <scancodes.h>

#include "gui.h"
#include "emucore.h"
#include "ti81prg.h"
#include "msgbox.h"

/*
 * Modified to run in Android OS. Dritan Hashorva 2012
 */


/**************** Internal link emulation ****************/

/* Open cable */
static int ilp_open(CableHandle* cbl)
{
	TilemCalcEmulator* emu = cbl->priv;

	tilem_em_lock(emu);

	if (emu->ilp_active) {
		fprintf(stderr, _("INTERNAL ERROR: cable already opened\n"));
		tilem_em_unlock(emu);
		return 1;
	}

	emu->ilp_active = TRUE;
	tilem_linkport_graylink_reset(emu->calc);
	tilem_em_unlock(emu);
	return 0;
}

/* Close cable */
static int ilp_close(CableHandle* cbl)
{
	TilemCalcEmulator* emu = cbl->priv;

	tilem_em_lock(emu);

	if (!emu->ilp_active) {
		fprintf(stderr, _("INTERNAL ERROR: cable already closed\n"));
		tilem_em_unlock(emu);
		return 1;
	}

	emu->ilp_active = FALSE;
	emu->calc->linkport.linkemu = TILEM_LINK_EMULATOR_NONE;
	tilem_linkport_graylink_reset(emu->calc);
	tilem_em_unlock(emu);
	return 0;
}

/* Reset cable */
static int ilp_reset(CableHandle* cbl)
{
	TilemCalcEmulator* emu = cbl->priv;

	tilem_em_lock(emu);
	tilem_linkport_graylink_reset(emu->calc);
	tilem_em_unlock(emu);
	return 0;
}

/* Send data to calc */
static int ilp_send(CableHandle* cbl, uint8_t* data, uint32_t count)
{
	TilemCalcEmulator* emu = cbl->priv;
	int timeout = cbl->timeout * 100000;

	tilem_em_lock(emu);
	while (count > 0) {
		if (tilem_em_send_byte(emu, data[0], timeout, TRUE)) {
			tilem_em_unlock(emu);
			return ERROR_WRITE_TIMEOUT;
		}
		data++;
		count--;
	}
	tilem_em_unlock(emu);
	return 0;
}

/* cool-down period required after receiving and before sending */
#define COOLDOWN 10000

/* Receive data from calc */
static int ilp_recv(CableHandle* cbl, uint8_t* data, uint32_t count)
{
	TilemCalcEmulator* emu = cbl->priv;
	int timeout = cbl->timeout * 100000;
	int value;

	tilem_em_lock(emu);
	while (count > 0) {
		value = tilem_em_get_byte(emu, timeout, TRUE);
		if (value < 0) {
			tilem_em_unlock(emu);
			return ERROR_READ_TIMEOUT;
		}
		data[0] = value;
		data++;
		count--;
	}
	tilem_em_delay(emu, COOLDOWN, TRUE);
	tilem_em_unlock(emu);
	return 0;
}

/* Check if ready */
static int ilp_check(CableHandle* cbl, int* status)
{
	TilemCalcEmulator* emu = cbl->priv;

	tilem_em_lock(emu);

	*status = STATUS_NONE;
	if (emu->calc->linkport.lines)
		*status |= STATUS_RX;
	if (emu->calc->linkport.extlines)
		*status |= STATUS_TX;

	tilem_em_unlock(emu);
	return 0;
}

/* Open a cable */
static CableHandle* internal_link_handle_new(TilemCalcEmulator* emu)
{
	CableHandle* cbl;

	cbl = ticables_handle_new(CABLE_ILP, PORT_0);
	if (!cbl)
		return NULL;

	cbl->priv = emu;
	cbl->cable->open = ilp_open;
	cbl->cable->close = ilp_close;
	cbl->cable->reset = ilp_reset;
	cbl->cable->send = ilp_send;
	cbl->cable->recv = ilp_recv;
	cbl->cable->check = ilp_check;

	return cbl;
}

/**************** Automatic link menu ****************/

/* Run a key (wait, press, wait; release; wait) */
static void run_with_key(TilemCalcEmulator* emu, int key)
{
	tilem_em_delay(emu, 50000, TRUE);
	tilem_keypad_press_key(emu->calc, key);
	tilem_em_delay(emu, 50000, TRUE);
	tilem_keypad_release_key(emu->calc, key);
	tilem_em_delay(emu, 50000, TRUE);
}

/* Automatically press key to be in the receive mode (ti82 and ti85) */
static void prepare_for_link_send(TilemCalcEmulator* emu)
{
	tilem_em_lock(emu);
	tilem_em_wake_up(emu, TRUE);
	if (emu->calc->hw.model_id == TILEM_CALC_TI82) {
		run_with_key(emu, TILEM_KEY_2ND);
		run_with_key(emu, TILEM_KEY_MODE);
		run_with_key(emu, TILEM_KEY_2ND);
		run_with_key(emu, TILEM_KEY_GRAPHVAR);
		run_with_key(emu, TILEM_KEY_RIGHT);
		run_with_key(emu, TILEM_KEY_ENTER);
	}
	else if (emu->calc->hw.model_id == TILEM_CALC_TI85) {
		run_with_key(emu, TILEM_KEY_MODE);
		run_with_key(emu, TILEM_KEY_MODE);
		run_with_key(emu, TILEM_KEY_MODE);
		run_with_key(emu, TILEM_KEY_2ND);
		run_with_key(emu, TILEM_KEY_GRAPHVAR);
		run_with_key(emu, TILEM_KEY_WINDOW);
	}
	tilem_em_unlock(emu);
}

/* Automatically press key to be in the send mode (ti82 and ti85) */
static void prepare_for_link_receive(TilemCalcEmulator *emu)
{
	tilem_em_lock(emu);
	tilem_em_wake_up(emu, TRUE);
	if (emu->calc->hw.model_id == TILEM_CALC_TI82) {
		run_with_key(emu, TILEM_KEY_2ND);
		run_with_key(emu, TILEM_KEY_MODE);
		run_with_key(emu, TILEM_KEY_2ND);
		run_with_key(emu, TILEM_KEY_GRAPHVAR);
		run_with_key(emu, TILEM_KEY_ENTER);
		tilem_em_delay(emu, 10000000, TRUE);
		run_with_key(emu, TILEM_KEY_RIGHT);
		run_with_key(emu, TILEM_KEY_ENTER);
	}
	else if (emu->calc->hw.model_id == TILEM_CALC_TI85) {
		run_with_key(emu, TILEM_KEY_MODE);
		run_with_key(emu, TILEM_KEY_MODE);
		run_with_key(emu, TILEM_KEY_MODE);
		run_with_key(emu, TILEM_KEY_2ND);
		run_with_key(emu, TILEM_KEY_GRAPHVAR);
		run_with_key(emu, TILEM_KEY_YEQU);
		run_with_key(emu, TILEM_KEY_GRAPH);
		tilem_em_delay(emu, 10000000, TRUE);
		run_with_key(emu, TILEM_KEY_ZOOM);
		tilem_em_delay(emu, 10000000, TRUE);
		run_with_key(emu, TILEM_KEY_YEQU);
	}
	tilem_em_unlock(emu);
}

/**************** Calc handle ****************/

static GStaticPrivate current_emu_key = G_STATIC_PRIVATE_INIT;

/* ticalcs progress bar callback */
static void pbar_do_update()
{
	TilemCalcEmulator *emu = g_static_private_get(&current_emu_key);
	CalcUpdate *upd = emu->link_update;
	gdouble frac;

	if (upd->max1 > 0 && upd->max2 > 0)
		frac = ((gdouble) upd->cnt1 / upd->max1 + upd->cnt2) / upd->max2;
	else if (upd->max1 > 0)
		frac = ((gdouble) upd->cnt1 / upd->max1);
	else if (upd->max2 > 0)
		frac = ((gdouble) upd->cnt2 / upd->max2);
	else
		frac = -1.0;

	tilem_em_set_progress(emu, frac, upd->text);
}

/* Get the calc model (compatible for ticalcs) */
int get_calc_model(TilemCalc *calc)
{
	return model_to_calcmodel(calc->hw.model_id);
}

/* Create a calc handle */
void begin_link(TilemCalcEmulator *emu, CableHandle **cbl, CalcHandle **ch,
                const char *title)
{
	tilem_em_unlock(emu);

	*cbl = internal_link_handle_new(emu);

	emu->link_update->max1 = 0;
	emu->link_update->max2 = 0;
	emu->link_update->text[0] = 0;

	emu->link_update->pbar = &pbar_do_update;
	emu->link_update->label = &pbar_do_update;

	g_static_private_set(&current_emu_key, emu, NULL);

	tilem_em_set_progress_title(emu, title);

	*ch = ticalcs_handle_new(get_calc_model(emu->calc));
	if (!*ch) {
		g_critical(_("unsupported calc"));
		return;
	}

	ticalcs_update_set(*ch, emu->link_update);
	ticalcs_cable_attach(*ch, *cbl);
}

/* Destroy calc handle */
void end_link(TilemCalcEmulator *emu, CableHandle *cbl, CalcHandle *ch)
{
	tilem_em_set_progress_title(emu, NULL);

	ticalcs_cable_detach(ch);
	ticalcs_handle_del(ch);
	ticables_handle_del(cbl);

	tilem_em_lock(emu);
}

/**************** Error messages ****************/

static char * get_tilibs_error(int errcode)
{
	char *p = NULL;

	if (!ticalcs_error_get(errcode, &p)
	    || !ticables_error_get(errcode, &p)
	    || !tifiles_error_get(errcode, &p))
		return p;
	else
		return g_strdup_printf(_("Unknown error (%d)"), errcode);
}

static char * get_ti81_error(int errcode)
{
	switch (errcode) {
	case TI81_ERR_FILE_IO:
		return g_strdup(_("File I/O error"));

	case TI81_ERR_INVALID_FILE:
		return g_strdup(_("Not a valid PRG file"));

	case TI81_ERR_MEMORY:
		return g_strdup(_("The calculator does not have enough free memory"
		                  " to load the program."));

	case TI81_ERR_SLOTS_FULL:
		return g_strdup(_("All calculator program slots are in use. "
		                  " You must delete an existing program before"
		                  " loading a new program."));

	case TI81_ERR_BUSY:
		return g_strdup(_("The calculator is currently busy.  Please"
		                  " exit to the home screen before loading"
		                  " programs."));

	default:
		return g_strdup_printf(_("Unknown error code (%d)"), errcode);
	}
}

void show_error(TilemCalcEmulator *emu,
                       const char *title, const char *message)
{

}

/**************** Sending files ****************/

/* Send a file to TI-81 */
static gboolean send_file_ti81(TilemCalcEmulator *emu, struct TilemSendFileInfo *sf)
{
	TI81Program *prgm = NULL;
	FILE *f;
	int errnum;
	sf->error_message = NULL;

	f = g_fopen(sf->filename, "rb");
	if (!f) {
		sf->error_message = g_strdup_printf
			(_("Failed to open %s for reading: %s"),
			 sf->display_name, g_strerror(errno));
		return FALSE;
	}

	if (ti81_read_prg_file(f, &prgm)) {
		sf->error_message = g_strdup_printf
			(_("The file %s is not a valid TI-81 program file."),
			 sf->display_name);
		fclose(f);
		return FALSE;
	}

	fclose(f);

	tilem_em_wake_up(emu, TRUE);

	prgm->info.slot = sf->slot;
	errnum = ti81_load_program(emu->calc, prgm);
	ti81_program_free(prgm);

	if (errnum && !emu->task_abort)
		sf->error_message = get_ti81_error(errnum);
	return (errnum == 0);
}

/* Get application name */
static gboolean get_app_name(const FlashContent *flashc, char *name)
{
	int i;
	const unsigned char *data;
	unsigned int type, length;

	if (flashc->num_pages < 1 || flashc->pages[0]->size < 6
	    || flashc->pages[0]->data[0] != 0x80
	    || flashc->pages[0]->data[1] != 0x0f)
		return FALSE;

	i = 6;
	data = flashc->pages[0]->data;
	while (i < flashc->pages[0]->size && i < 128) {
		type = (data[i] << 8 | (data[i + 1] & 0xf0));
		length = data[i + 1] & 0x0f;
		i += 2;

		if (length == 0x0d) {
			length = data[i];
			i++;
		}
		else if (length == 0x0e) {
			length = (data[i] << 8 | data[i + 1]);
			i += 2;
		}
		else if (length == 0x0f) {
			return FALSE;
		}

		if (type == 0x8070)
			return FALSE;

		if (type == 0x8040) {
			memcpy(name, data + i, length > 8 ? 8 : length);
			return TRUE;
		}

		 i += length;
	}
	return FALSE;
}


/* Try to delete an existing Flash app before we send a replacement */
static void try_delete_app(CalcHandle *ch, const FlashContent *flashc)
{
	VarRequest vr;

	/* TI-73 does not support remote deletion */
	if (ch->model == CALC_TI73)
		return;

	memset(&vr, 0, sizeof(VarRequest));
	if (!get_app_name(flashc, vr.name))
		return;

	/* Why does this use type 0x14 and not 0x24?  I don't know. */
	vr.type = 0x14;
	ticalcs_calc_del_var(ch, &vr);
	/* if an error occurs, ignore it */
}

int sync_clock_tilem(TilemCalcEmulator *emu)
{
	time_t tt;
	struct tm *lt;
	CalcClock tc;
	int err;

	time(&tt);
	lt = localtime(&tt);
	tc.day = lt->tm_mday;
	tc.month = lt->tm_mon + 1;
	tc.year = lt->tm_year + 1900;
	tc.hours = lt->tm_hour;
	tc.minutes = lt->tm_min;
	tc.seconds = lt->tm_sec;

	CableHandle *cbl;
	CalcHandle *ch;
	begin_link(emu, &cbl, &ch, NULL);
	prepare_for_link_send(emu);
	ticalcs_calc_set_clock(ch, &tc);
	end_link(emu, cbl, ch);
}

/* Send a file using ticalcs2 */
static gboolean send_file_linkport(TilemCalcEmulator *emu, struct TilemSendFileInfo *sf)
{
	CalcModel model;
	FileClass cls;
	CableHandle *cbl;
	CalcHandle *ch;
	FileContent *filec;
	BackupContent *backupc;
	FlashContent *flashc;
	TigContent *tigc;
	CalcMode mode;
	int e;
	char *desc;

	model = get_calc_model(emu->calc);
	cls = tifiles_file_get_class(sf->filename);

	desc = g_strdup_printf(_("Sending %s"), sf->display_name);

	/* Read input file */

	switch (cls) {
	case TIFILE_SINGLE:
	case TIFILE_GROUP:
	case TIFILE_REGULAR:
		filec = tifiles_content_create_regular(model);
		e = tifiles_file_read_regular(sf->filename, filec);
		if (!e) {
			begin_link(emu, &cbl, &ch, desc);
			if (sf->first)
				prepare_for_link_send(emu);
			mode = (sf->last ? MODE_SEND_LAST_VAR : MODE_NORMAL);
			e = ticalcs_calc_send_var(ch, mode, filec);
			end_link(emu, cbl, ch);
		}
		tifiles_content_delete_regular(filec);
		break;

	case TIFILE_BACKUP:
		backupc = tifiles_content_create_backup(model);
		e = tifiles_file_read_backup(sf->filename, backupc);
		if (!e) {
			begin_link(emu, &cbl, &ch, desc);
			prepare_for_link_send(emu);
			e = ticalcs_calc_send_backup(ch, backupc);
			end_link(emu, cbl, ch);
		}
		tifiles_content_delete_backup(backupc);
		break;

	case TIFILE_FLASH:
	case TIFILE_OS:
	case TIFILE_APP:
		flashc = tifiles_content_create_flash(model);
		e = tifiles_file_read_flash(sf->filename, flashc);
		if (!e) {
			begin_link(emu, &cbl, &ch, desc);
			ticables_options_set_timeout(cbl, 30 * 10);
			prepare_for_link_send(emu);
			if (tifiles_file_is_os(sf->filename))
				e = ticalcs_calc_send_os(ch, flashc);
			else if (tifiles_file_is_app(sf->filename)) {
				try_delete_app(ch, flashc);
				e = ticalcs_calc_send_app(ch, flashc);
			}
			else
				e = ticalcs_calc_send_cert(ch, flashc);
			end_link(emu, cbl, ch);
		}
		tifiles_content_delete_flash(flashc);
		break;

	case TIFILE_TIGROUP:
		tigc = tifiles_content_create_tigroup(model, 0);
		e = tifiles_file_read_tigroup(sf->filename, tigc);
		if (!e) {
			begin_link(emu, &cbl, &ch, desc);
			prepare_for_link_send(emu);
			e = ticalcs_calc_send_tigroup(ch, tigc, TIG_ALL);
			end_link(emu, cbl, ch);
		}
		tifiles_content_delete_tigroup(tigc);
		break;

	default:
		g_free(desc);
		sf->error_message = g_strdup_printf
			(_("The file %s is not a valid program or"
			   " variable file."),
			 sf->display_name);
		return FALSE;
	}

	g_free(desc);
	if (e && !emu->task_abort)
		sf->error_message = get_tilibs_error(e);
	return (e == 0);
}

gboolean send_file_main(TilemCalcEmulator *emu, gpointer data)
{
	struct TilemSendFileInfo *sf = data;
	/*emu->ilp.finished_cond = g_cond_new(); */

	if (emu->calc->hw.model_id == TILEM_CALC_TI81)
		return send_file_ti81(emu, sf);
	else
		return send_file_linkport(emu, sf);
}

static void send_file_finished(TilemCalcEmulator *emu, gpointer data,
                               gboolean cancelled)
{
	struct TilemSendFileInfo *sf = data;

	if (sf->error_message && !cancelled)
		show_error(emu, _("Unable to send file"), sf->error_message);

	g_free(sf->filename);
	g_free(sf->display_name);
	g_free(sf->error_message);
	g_slice_free(struct TilemSendFileInfo, sf);
	
	/*g_cond_broadcast(emu->ilp.finished_cond);*/
	
	
}

int tilem_link_send_file(TilemCalcEmulator *emu, const char *filename,
                          int slot, gboolean first, gboolean last)
{
	struct TilemSendFileInfo *sf;

	sf = g_slice_new0(struct TilemSendFileInfo);
	sf->filename = g_strdup(filename);
	sf->display_name = g_filename_display_basename(filename);
	sf->slot = slot;
	sf->first = first;
	sf->last = last;

	return !send_file_main(emu, sf);
//	tilem_calc_emulator_begin(emu, &send_file_main,
	//                          &send_file_finished, sf);
}

/**************** Get directory listing ****************/

/* Make a copy of a TilemVarEntry */
TilemVarEntry *tilem_var_entry_copy(const TilemVarEntry *tve)
{
	TilemVarEntry *nve;

	g_return_val_if_fail(tve != NULL, NULL);

	nve = g_slice_new(TilemVarEntry);
	*nve = *tve;

	if (tve->ve) {
		nve->ve = g_slice_new(VarEntry);
		*nve->ve = *tve->ve;
		nve->ve->data = g_memdup(tve->ve->data, tve->ve->size);
	}
	if (tve->name_str)
		nve->name_str = g_strdup(tve->name_str);
	if (tve->type_str)
		nve->type_str = g_strdup(tve->type_str);
	if (tve->slot_str)
		nve->slot_str = g_strdup(tve->slot_str);
	if (tve->file_ext)
		nve->file_ext = g_strdup(tve->file_ext);
	if (tve->filetype_desc)
		nve->filetype_desc = g_strdup(tve->filetype_desc);

	return nve;
}

/* Free a TilemVarEntry */
void tilem_var_entry_free(TilemVarEntry *tve)
{
	g_return_if_fail(tve != NULL);
	if (tve->ve) {
		g_free(tve->ve->data);
		g_slice_free(VarEntry, tve->ve);
	}
	g_free(tve->name_str);
	g_free(tve->type_str);
	g_free(tve->slot_str);
	g_free(tve->file_ext);
	g_free(tve->filetype_desc);
	g_slice_free(TilemVarEntry, tve);
}

struct dirlistinfo {
	GSList *list;
	char *error_message;
	gboolean aborted;
	gboolean no_gui;
};

/* Convert tifiles VarEntry into a TilemVarEntry */
static TilemVarEntry *convert_ve(TilemCalcEmulator *emu, CalcHandle *ch,
                                 VarEntry *ve, gboolean is_flash)
{
	TilemVarEntry *tve = g_slice_new0(TilemVarEntry);
	CalcModel tfmodel = ch->model;
	const char *model_str;
	const char *type_str;
	const char *fext;

	tve->model = emu->calc->hw.model_id;

	tve->ve = g_slice_new(VarEntry);
	*tve->ve = *ve;
	if (ve->data)
		tve->ve->data = g_memdup(ve->data, ve->size);

	tve->size = ve->size;
	tve->archived = (ve->attr & ATTRB_ARCHIVED ? TRUE : FALSE);
	tve->can_group = TRUE;

	tve->name_str = ticonv_varname_to_utf8(tfmodel, ve->name, ve->type);
	g_strchomp(tve->name_str);
	tve->type_str = g_strdup(tifiles_vartype2string(tfmodel, ve->type));
	fext = tifiles_vartype2fext(tfmodel, ve->type);
	tve->file_ext = g_ascii_strdown(fext, -1);

	model_str = ch->calc->fullname;
	type_str = tifiles_vartype2type(tfmodel, ve->type);
	tve->filetype_desc = g_strdup_printf(_("%s %s files"), model_str, type_str);

	tve->can_group = !is_flash;

	return tve;
}

/* Convert a complete directory listing */
static void convert_dir_list(TilemCalcEmulator *emu, CalcHandle *ch,
                             GNode *root, gboolean is_flash, GSList **list)
{
	GNode *dir, *var;
	VarEntry *ve;
	TilemVarEntry *tve;

	if (!root)
		return;

	for (dir = root->children; dir; dir = dir->next) {
		for (var = dir->children; var; var = var->next) {
			ve = var->data;
			tve = convert_ve(emu, ch, ve, is_flash);
			*list = g_slist_prepend(*list, tve);
		}
	}
}

/* Request directory listing using ticalcs */
static gboolean get_dirlist_silent(TilemCalcEmulator *emu,
                                   struct dirlistinfo *dl)
{
	CableHandle *cbl;
	CalcHandle *ch;
	GNode *vars = NULL, *apps = NULL;
	GSList *list = NULL;
	int e = 0;

	begin_link(emu, &cbl, &ch, _("Reading variable list"));
	prepare_for_link_receive(emu);

	if (ticalcs_calc_features(ch) & OPS_DIRLIST) {
		e = ticalcs_calc_get_dirlist(ch, &vars, &apps);
		if (!e) {
			convert_dir_list(emu, ch, vars, FALSE, &list);
			convert_dir_list(emu, ch, apps, TRUE, &list);
		}
		ticalcs_dirlist_destroy(&vars);
		ticalcs_dirlist_destroy(&apps);
	}

	end_link(emu, cbl, ch);

	dl->list = g_slist_reverse(list);

	dl->aborted = emu->task_abort;

	if (e && !emu->task_abort)
		dl->error_message = get_tilibs_error(e);
	return (e == 0);
}

/* Transfer variables non-silently using ticalcs */
static gboolean get_dirlist_nonsilent(TilemCalcEmulator *emu,
                                      struct dirlistinfo *dl)
{
	CableHandle *cbl;
	CalcHandle *ch;
	FileContent *fc;
	VarEntry *head_entry = NULL;
	TilemVarEntry *tve;
	GSList *list = NULL;
	int e, i;

	begin_link(emu, &cbl, &ch, _("Receiving variables"));
	prepare_for_link_receive(emu);

	fc = tifiles_content_create_regular(ch->model);
	e = ticalcs_calc_recv_var_ns(ch, MODE_BACKUP, fc, &head_entry);
	if (!e) {
		for (i = 0; i < fc->num_entries; i++) {
			tve = convert_ve(emu, ch, fc->entries[i], FALSE);
			list = g_slist_prepend(list, tve);
		}
	}
	if (head_entry)
		tifiles_ve_delete(head_entry);
	tifiles_content_delete_regular(fc);

	end_link(emu, cbl, ch);

	dl->list = g_slist_reverse(list);

	dl->aborted = emu->task_abort;

	if (e && !emu->task_abort)
		dl->error_message = get_tilibs_error(e);
	return (e == 0);
}

/* Get TI-81 directory listing */
static gboolean get_dirlist_ti81(TilemCalcEmulator *emu,
                                 struct dirlistinfo *dl)
{
	int i, slot;
	TI81ProgInfo info;
	GSList *list = NULL;
	TilemVarEntry *tve;
	int e;

	tilem_em_wake_up(emu, TRUE);

	for (i = 0; i <= TI81_SLOT_MAX; i++) {
		/* put Prgm0 after Prgm9, the way it appears in the menu */
		if (i < 9)
			slot = i + 1;
		else if (i == 9)
			slot = 0;
		else
			slot = i;

		if ((e = ti81_get_program_info(emu->calc, slot, &info)))
			break;

		if (info.size == 0)
			continue;

		tve = g_slice_new0(TilemVarEntry);
		tve->model = TILEM_CALC_TI81;
		tve->slot = info.slot;
		tve->name_str = ti81_program_name_to_string(info.name);
		tve->slot_str = ti81_program_slot_to_string(info.slot);
		tve->file_ext = g_strdup("prg");
		tve->filetype_desc = g_strdup(_("TI-81 programs"));
		tve->size = info.size;
		tve->archived = FALSE;
		tve->can_group = FALSE;

		list = g_slist_prepend(list, tve);
	}

	dl->list = g_slist_reverse(list);

	if (e && !emu->task_abort)
		dl->error_message = get_ti81_error(e);
	return (e == 0);
}

static gboolean get_dirlist_main(TilemCalcEmulator *emu, gpointer data)
{
	switch (emu->calc->hw.model_id) {
	case TILEM_CALC_TI81:
		return get_dirlist_ti81(emu, data);

	case TILEM_CALC_TI82:
	case TILEM_CALC_TI85:
		return get_dirlist_nonsilent(emu, data);

	default:
		return get_dirlist_silent(emu, data);
	}
}

static void get_dirlist_finished(TilemCalcEmulator *emu, gpointer data,
                                 gboolean cancelled)
{
/*	GSList *l;
	struct dirlistinfo *dl = data;

	if (dl->error_message && !cancelled)
		show_error(emu, _("Unable to receive variable list"),
		           dl->error_message);
	else if (!cancelled && !dl->aborted && emu->ewin && !dl->no_gui) {
		if (!emu->rcvdlg)
			emu->rcvdlg = tilem_receive_dialog_new(emu);
		tilem_receive_dialog_update(emu->rcvdlg, dl->list);
		dl->list = NULL;
	}

	if (!dl->no_gui && emu->rcvdlg)
		emu->rcvdlg->refresh_pending = FALSE;

	for (l = dl->list; l; l = l->next)
		tilem_var_entry_free(l->data);

	g_slist_free(dl->list);
	g_slice_free(struct dirlistinfo, dl);*/
}

void tilem_link_get_dirlist(TilemCalcEmulator *emu)
{
	struct dirlistinfo *dl = g_slice_new0(struct dirlistinfo);
	tilem_calc_emulator_begin(emu, &get_dirlist_main,
	                          &get_dirlist_finished, dl);
}

/**************** Receiving files ****************/

static gboolean write_output(FileContent **vars, FlashContent **apps,
                             const char *filename, gboolean output_tig,
                             char **error_message)
{
	FileContent *group = NULL;
	TigContent *tig = NULL;
	int e, nvars, napps;

	for (nvars = 0; vars && vars[nvars]; nvars++)
		;
	for (napps = 0; apps && apps[napps]; napps++)
		;

	g_return_val_if_fail(nvars > 0 || napps > 0, FALSE);

	if (output_tig) {
		e = tifiles_tigroup_contents(vars, apps, &tig);
		if (!e)
			e = tifiles_file_write_tigroup(filename, tig);
	}
	else if (nvars > 1 && napps == 0) {
		e = tifiles_group_contents(vars, &group);
		if (!e)
			e = tifiles_file_write_regular(filename, group, NULL);
	}
	else if (nvars == 0 && napps == 1) {
		e = tifiles_file_write_flash(filename, apps[0]);
	}
	else if (nvars == 1 && napps == 0) {
		e = tifiles_file_write_regular(filename, vars[0], NULL);
	}
	else {
		*error_message = g_strdup
			(_("Applications cannot be saved in an XXg group"
			   " file.  Try using TIG format or saving apps"
			   " individually."));
		return FALSE;
	}

	if (e)
		*error_message = get_tilibs_error(e);

	if (tig)
		tifiles_content_delete_tigroup(tig);
	if (group)
		tifiles_content_delete_regular(group);

	return (e == 0);
}

static gboolean receive_files_silent(TilemCalcEmulator* emu,
                                     struct TilemReceiveFileInfo *rf)
{
	CableHandle *cbl;
	CalcHandle *ch;
	FileContent **vars, *filec;
	FlashContent **apps, *flashc;
	GSList *l;
	TilemVarEntry *tve;
	int e, i, nvars, napps;

	g_return_val_if_fail(rf->entries != NULL, FALSE);

	i = g_slist_length(rf->entries);

	vars = g_new0(FileContent *, i + 1);
	apps = g_new0(FlashContent *, i + 1);
	nvars = napps = 0;

	begin_link(emu, &cbl, &ch, _("Receiving variables"));

	for (l = rf->entries; l; l = l->next) {
		tve = l->data;

		if (tve->ve->type == tifiles_flash_type(ch->model)) {
			flashc = tifiles_content_create_flash(ch->model);
			e = ticalcs_calc_recv_app(ch, flashc, tve->ve);
			apps[napps++] = flashc;
		}
		else {
			filec = tifiles_content_create_regular(ch->model);
			e = ticalcs_calc_recv_var(ch, MODE_NORMAL,
			                          filec, tve->ve);
			vars[nvars++] = filec;
		}
		if (e)
			break;
	}

	if (e && !emu->task_abort)
		rf->error_message = get_tilibs_error(e);

	end_link(emu, cbl, ch);

	if (!e)
		e = !write_output(vars, apps, rf->destination,
		                  rf->output_tig, &rf->error_message);

	for (i = 0; i < nvars; i++)
		tifiles_content_delete_regular(vars[i]);
	for (i = 0; i < napps; i++)
		tifiles_content_delete_flash(apps[i]);

	return (e == 0);
}

static gboolean receive_files_ti81(TilemCalcEmulator* emu,
                                   struct TilemReceiveFileInfo *rf)
{
	TilemVarEntry *tve;
	TI81Program *prgm = NULL;
	int e;
	FILE *f;
	char *dname;

	g_return_val_if_fail(rf->entries != NULL, FALSE);

	if (rf->entries->next) {
		rf->error_message = g_strdup
			(_("TI-81 programs cannot be saved in a group file."
			   " Try saving programs individually."));
		return FALSE;
	}

	tve = rf->entries->data;
	e = ti81_get_program(emu->calc, tve->slot, &prgm);
	if (e) {
		rf->error_message = get_ti81_error(e);
		return FALSE;
	}

	f = g_fopen(rf->destination, "wb");
	if (!f) {
		e = errno;
		dname = g_filename_display_basename(rf->destination);
		rf->error_message = g_strdup_printf
			(_("Failed to open %s for writing: %s"),
			 dname, g_strerror(e));
		g_free(dname);
		ti81_program_free(prgm);
		return FALSE;
	}

	e = ti81_write_prg_file(f, prgm);
	if (fclose(f) || e) {
		e = errno;
		dname = g_filename_display_basename(rf->destination);
		rf->error_message = g_strdup_printf
			(_("Error writing %s: %s"),
			 dname, g_strerror(e));
		g_free(dname);
		ti81_program_free(prgm);
		return FALSE;
	}

	ti81_program_free(prgm);
	return TRUE;
}

static gboolean receive_files_nonsilent(TilemCalcEmulator *emu,
                                        struct TilemReceiveFileInfo *rf)
{
	const TilemVarEntry *tve;
	FileContent **vars, *fc;
	int i, nvars;
	GSList *l;
	gboolean status;

	nvars = g_slist_length(rf->entries);

	vars = g_new0(FileContent *, nvars + 1);
	i = 0;
	for (l = rf->entries; l; l = l->next) {
		tve = l->data;
		g_return_val_if_fail(tve->ve != NULL, FALSE);
		g_return_val_if_fail(tve->ve->data != NULL, FALSE);

		/* avoid copying variable data */
		fc = tifiles_content_create_regular(get_calc_model(emu->calc));
		fc->entries = g_new(VarEntry *, 1);
		fc->num_entries = 1;
		fc->entries[0] = tve->ve;
		vars[i++] = fc;
	}

	status = write_output(vars, NULL, rf->destination, rf->output_tig,
	                      &rf->error_message);

	for (i = 0; i < nvars; i++) {
		if (vars[i]) {
			vars[i]->num_entries = 0;
			g_free(vars[i]->entries);
			vars[i]->entries = NULL;
			tifiles_content_delete_regular(vars[i]);
		}
	}
	g_free(vars);

	return status;
}

static gboolean receive_files_main(TilemCalcEmulator *emu, gpointer data)
{
	struct TilemReceiveFileInfo *rf = data;
	TilemVarEntry *tve;

	g_return_val_if_fail(rf->entries != NULL, FALSE);

	tve = rf->entries->data;

	if (emu->calc->hw.model_id == TILEM_CALC_TI81)
		return receive_files_ti81(emu, rf);
	else if (tve->ve && tve->ve->data)
		return receive_files_nonsilent(emu, rf);
	else
		return receive_files_silent(emu, rf);
}

static void receive_files_finished(TilemCalcEmulator *emu, gpointer data,
                                   gboolean cancelled)
{
	struct TilemReceiveFileInfo *rf = data;
	GSList *l;

	if (rf->error_message && !cancelled)
		show_error(emu, _("Unable to save file"), rf->error_message);

	g_free(rf->destination);
	g_free(rf->error_message);
	for (l = rf->entries; l; l = l->next)
		tilem_var_entry_free(l->data);
	g_slist_free(rf->entries);
	g_slice_free(struct TilemReceiveFileInfo, rf);
}

void tilem_link_receive_group(TilemCalcEmulator *emu,
                              GSList *entries,
                              const char *destination)
{
	struct TilemReceiveFileInfo *rf;
	GSList *l;
	TilemVarEntry *tve;
	const char *p;
	gboolean output_tig = FALSE;

	g_return_if_fail(emu != NULL);
	g_return_if_fail(emu->calc != NULL);
	g_return_if_fail(entries != NULL);
	g_return_if_fail(destination != NULL);

	for (l = entries; l; l = l->next) {
		tve = l->data;
		g_return_if_fail(emu->calc->hw.model_id == tve->model);
	}

	p = strrchr(destination, '.');
	if (p && (!g_ascii_strcasecmp(p, ".tig")
	          || !g_ascii_strcasecmp(p, ".zip")))
		output_tig = TRUE;

	rf = g_slice_new0(struct TilemReceiveFileInfo);
	rf->destination = g_strdup(destination);
	rf->output_tig = output_tig;

	tve = entries->data;
	if (tve->ve && tve->ve->data) {
		/* non-silent: we already have the data; save the file
		   right now */
		rf->entries = entries;
		receive_files_nonsilent(emu, rf);
		rf->entries = NULL;
		receive_files_finished(emu, rf, FALSE);
	}
	else {
		/* silent: retrieve and save data in background */
		for (l = entries; l; l = l->next) {
			tve = tilem_var_entry_copy(l->data);
			rf->entries = g_slist_prepend(rf->entries, tve);
		}
		rf->entries = g_slist_reverse(rf->entries);

		tilem_calc_emulator_begin(emu, &receive_files_main,
		                          &receive_files_finished, rf);
	}
}

void tilem_link_receive_file(TilemCalcEmulator *emu,
                             const TilemVarEntry *varentry,
                             const char* destination)
{
	GSList *l;
	l = g_slist_prepend(NULL, (gpointer) varentry);
	tilem_link_receive_group(emu, l, destination);
	g_slist_free(l);
}

/**************** Receive matching files ****************/

struct recmatchinfo {
	char *pattern;
	char *destdir;
	struct dirlistinfo *dl;
	struct TilemReceiveFileInfo *rf;
};

static gboolean receive_matching_main(TilemCalcEmulator *emu, gpointer data)
{
	struct recmatchinfo *rm = data;
	GSList *l, *selected = NULL;
	TilemVarEntry *tve;
	char *defname, *defname_r, *defname_l;
	GPatternSpec *pat;
	gboolean status = TRUE;

	if (!get_dirlist_main(emu, rm->dl))
		return FALSE;

	/* Find variables that match the pattern */

	pat = g_pattern_spec_new(rm->pattern);

	for (l = rm->dl->list; l; l = l->next) {
		tve = l->data;

		defname = get_default_filename(tve);
		defname_r = g_utf8_normalize(defname, -1, G_NORMALIZE_NFKD);

		if (g_pattern_match(pat, strlen(defname_r), defname_r, NULL))
			selected = g_slist_prepend(selected, tve);

		g_free(defname);
		g_free(defname_r);
	}
	
	g_pattern_spec_free(pat);

	if (!selected) {
		rm->rf->error_message = g_strdup_printf
			(_("Variable %s not found"), rm->pattern);
		return FALSE;
	}

	/* Receive variables */

	selected = g_slist_reverse(selected);

	for (l = selected; l; l = l->next) {
		tve = l->data;

		g_free(rm->rf->destination);

		defname = get_default_filename(tve);
		defname_l = utf8_to_filename(defname);
		rm->rf->destination = g_build_filename(rm->destdir,
		                                       defname_l, NULL);
		g_free(defname);
		g_free(defname_l);

		rm->rf->entries = g_slist_prepend(NULL, tve);
		status = receive_files_main(emu, rm->rf);
		g_slist_free(rm->rf->entries);
		rm->rf->entries = NULL;

		if (!status)
			break;
	}

	g_slist_free(selected);

	return status;
}

static void receive_matching_finished(TilemCalcEmulator *emu, gpointer data,
                                      gboolean cancelled)
{
	struct recmatchinfo *rm = data;

	get_dirlist_finished(emu, rm->dl, cancelled);
	receive_files_finished(emu, rm->rf, cancelled);
	g_free(rm->pattern);
	g_free(rm->destdir);
	g_slice_free(struct recmatchinfo, rm);
}

/* Receive variables with names matching a pattern.  PATTERN is a
   glob-like pattern in UTF-8.  Files will be written out to
   DESTDIR. */
void tilem_link_receive_matching(TilemCalcEmulator *emu,
                                 const char *pattern,
                                 const char *destdir)
{
	struct recmatchinfo *rm;

	g_return_if_fail(emu != NULL);
	g_return_if_fail(pattern != NULL);
	g_return_if_fail(destdir != NULL);

	rm = g_slice_new0(struct recmatchinfo);
	rm->pattern = g_utf8_normalize(pattern, -1, G_NORMALIZE_NFKD);
	rm->destdir = g_strdup(destdir);

	rm->dl = g_slice_new0(struct dirlistinfo);
	rm->dl->no_gui = TRUE;
	rm->rf = g_slice_new0(struct TilemReceiveFileInfo);

	tilem_calc_emulator_begin(emu, &receive_matching_main,
	                          &receive_matching_finished, rm);
}
