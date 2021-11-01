/*
 * TilEm II
 *
 * Copyright (c) 2011 Benjamin Moody
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
#include <tilemdb.h>

#include "gui.h"

char * tilem_format_addr(TilemDebugger *dbg, dword addr, gboolean physical)
{
	dword page, addr_l;

	g_return_val_if_fail(dbg != NULL, NULL);
	g_return_val_if_fail(dbg->emu != NULL, NULL);
	g_return_val_if_fail(dbg->emu->calc != NULL, NULL);

	if (!physical)
		return g_strdup_printf("%04X", addr);

	if (addr >= dbg->emu->calc->hw.romsize)
		page = (((addr - dbg->emu->calc->hw.romsize) >> 14)
		        + dbg->emu->calc->hw.rampagemask);
	else
		page = addr >> 14;

	addr_l = (*dbg->emu->calc->hw.mem_ptol)(dbg->emu->calc, addr);
	if (addr_l == 0xffffffff)
		addr_l = (addr & 0x3fff) | 0x4000;

	return g_strdup_printf("%02X:%04X", page, addr_l);
}

static gboolean parse_hex(const char *string, dword *value)
{
	const char *n;
	char *e;
	dword a;

	if (string[0] == '$')
		n = string + 1;
	else if (string[0] == '0' && (string[1] == 'x' || string[1] == 'X'))
		n = string + 2;
	else
		n = string;

	a = strtol(n, &e, 16);
	if (value)
		*value = a;

	if (e == n)
		return FALSE;

	if (*e == 'h' || *e == 'H')
		e++;

	return (*e == 0);
}

gboolean tilem_parse_paged_addr(TilemDebugger *dbg, const char *pagestr,
                                const char *offsstr, dword *value)
{
	dword page, offs;

	g_return_val_if_fail(dbg != NULL, FALSE);
	g_return_val_if_fail(dbg->emu != NULL, FALSE);
	g_return_val_if_fail(dbg->emu->calc != NULL, FALSE);

	if (!parse_hex(pagestr, &page))
		return FALSE;
	if (!tilem_parse_addr(dbg, offsstr, &offs, NULL))
		return FALSE;

	offs &= 0x3fff;
	if (page & dbg->emu->calc->hw.rampagemask) {
		page &= ~dbg->emu->calc->hw.rampagemask;
		offs += (offs << 14);
		if (offs > dbg->emu->calc->hw.ramsize)
			return FALSE;
		offs += dbg->emu->calc->hw.romsize;
	}
	else {
		offs += (page << 14);
		if (offs > dbg->emu->calc->hw.romsize)
			return FALSE;
	}

	if (value) *value = offs;
	return TRUE;
}

gboolean tilem_parse_addr(TilemDebugger *dbg, const char *string,
                          dword *value, gboolean *physical)
{
	const char *offstr;
	char *pagestr;

	g_return_val_if_fail(dbg != NULL, FALSE);
	g_return_val_if_fail(dbg->emu != NULL, FALSE);
	g_return_val_if_fail(dbg->emu->calc != NULL, FALSE);

	if (parse_hex(string, value)) {
		if (physical) *physical = FALSE;
		return TRUE;
	}

	if (physical && (offstr = strchr(string, ':'))) {
		pagestr = g_strndup(string, offstr - string);
		offstr++;
		if (tilem_parse_paged_addr(dbg, pagestr, offstr, value)) {
			*physical = TRUE;
			return TRUE;
		}
	}

	if (dbg->dasm && tilem_disasm_get_label(dbg->dasm, string, value)) {
		if (physical) *physical = FALSE;
		return TRUE;
	}

	return FALSE;
}

struct addrdlg {
	GtkWidget *dlg;
	TilemDebugger *dbg;
	gboolean physical;
};

static void edited(GtkEntry *entry, gpointer data)
{
	struct addrdlg *adlg = data;
	const char *text;
	gboolean valid, phys;
	
	text = gtk_entry_get_text(entry);
	valid = tilem_parse_addr(adlg->dbg, text, NULL,
	                         adlg->physical ? &phys : NULL);
	gtk_dialog_set_response_sensitive(GTK_DIALOG(adlg->dlg),
	                                  GTK_RESPONSE_OK,
	                                  valid);
}

gboolean tilem_prompt_address(TilemDebugger *dbg, GtkWindow *parent,
                              const char *title, const char *prompt,
                              dword *value, gboolean physical,
                              gboolean usedefault)
{
	GtkWidget *dlg, *hbox, *vbox, *lbl, *ent;
	struct addrdlg adlg;
	const char *text;
	gboolean phys;
	char *s;

	g_return_val_if_fail(dbg != NULL, FALSE);
	g_return_val_if_fail(dbg->emu != NULL, FALSE);
	g_return_val_if_fail(dbg->emu->calc != NULL, FALSE);

	dlg = gtk_dialog_new_with_buttons(title, parent, GTK_DIALOG_MODAL,
	                                  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	                                  GTK_STOCK_OK, GTK_RESPONSE_OK,
	                                  NULL);
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dlg),
	                                        GTK_RESPONSE_OK,
	                                        GTK_RESPONSE_CANCEL,
	                                        -1);
	gtk_dialog_set_default_response(GTK_DIALOG(dlg),
	                                GTK_RESPONSE_OK);

	hbox = gtk_hbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 6);

	lbl = gtk_label_new(prompt);
	gtk_box_pack_start(GTK_BOX(hbox), lbl, FALSE, FALSE, 0);

	ent = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(ent), TRUE);
	gtk_box_pack_start(GTK_BOX(hbox), ent, TRUE, TRUE, 0);

	if (usedefault) {
		s = tilem_format_addr(dbg, *value, physical);
		gtk_entry_set_text(GTK_ENTRY(ent), s);
		g_free(s);
	}

	adlg.dlg = dlg;
	adlg.dbg = dbg;
	adlg.physical = physical;

	g_signal_connect(ent, "changed",
	                 G_CALLBACK(edited), &adlg);
	edited(GTK_ENTRY(ent), &adlg);

	gtk_widget_show_all(hbox);
	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	if (gtk_dialog_run(GTK_DIALOG(dlg)) != GTK_RESPONSE_OK) {
		gtk_widget_destroy(dlg);
		return FALSE;
	}

	text = gtk_entry_get_text(GTK_ENTRY(ent));
	if (!tilem_parse_addr(dbg, text, value, physical ? &phys : NULL)) {
		gtk_widget_destroy(dlg);
		return FALSE;
	}

	if (physical && !phys) {
		tilem_calc_emulator_lock(dbg->emu);
		*value &= 0xffff;
		*value = (*dbg->emu->calc->hw.mem_ltop)(dbg->emu->calc, *value);
		tilem_calc_emulator_unlock(dbg->emu);
	}

	gtk_widget_destroy(dlg);
	return TRUE;
}
