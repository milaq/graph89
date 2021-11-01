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
#include <gdk/gdkkeysyms.h>
#include <ticalcs.h>
#include <tilem.h>
#include <tilemdb.h>

#include "gui.h"
#include "disasmview.h"
#include "fixedtreeview.h"

/**************** Add/edit breakpoint dialog ****************/

struct hex_entry {
	GtkWidget *addr_label;
	GtkWidget *addr_entry;
	GtkWidget *page_label;
	GtkWidget *page_entry;
};

struct breakpoint_dlg {
	TilemDebugger *dbg;

	GtkWidget *dlg;
	GtkWidget *box;

	GtkWidget *type_combo;
	GtkWidget *access_cb[3];
	GtkWidget *single_rb;
	GtkWidget *range_rb;
	GtkWidget *access_label;
	GtkWidget *address_label;

	struct hex_entry start;
	struct hex_entry end;
};

static const struct {
	char abbrev;
	const char *desc;
	const char *value_label;
	int use_pages;
	guint access_mask;
} type_info[] = {
	{ 'M', N_("Memory address (logical)"), N_("Address"), 0, 7 },
	{ 'M', N_("Memory address (absolute)"), N_("Address"), 1, 7 },
	{ 'P', N_("I/O port"), N_("Port Number"), 0, 6 },
	{ 'I', N_("Z80 instruction"), N_("Opcode"), 0, 0 }
};

/* Determine currently selected address type */
static guint get_bp_type(struct breakpoint_dlg *bpdlg)
{
	int i = gtk_combo_box_get_active(GTK_COMBO_BOX(bpdlg->type_combo));
	return (i < 0 ? 0 : i);
}

/* Format address as a string */
static void hex_entry_set_value(struct hex_entry *he, TilemDebugger *dbg,
                                int type, dword value)
{
	const TilemCalc *calc;
	char buf[20];
	unsigned int page;

	g_return_if_fail(dbg->emu != NULL);
	g_return_if_fail(dbg->emu->calc != NULL);

	calc = dbg->emu->calc;

	switch (type) {
	case TILEM_DB_BREAK_LOGICAL:
		g_snprintf(buf, sizeof(buf), "%04X", value);
		break;

	case TILEM_DB_BREAK_PHYSICAL:
		if (value >= calc->hw.romsize) {
			value -= calc->hw.romsize;
			page = (value >> 14) + calc->hw.rampagemask;
		}
		else {
			page = (value >> 14);
		}

		g_snprintf(buf, sizeof(buf), "%02X", page);
		gtk_entry_set_text(GTK_ENTRY(he->page_entry), buf);

		g_snprintf(buf, sizeof(buf), "%04X", value & 0x3fff);
		break;

	case TILEM_DB_BREAK_PORT:
		g_snprintf(buf, sizeof(buf), "%02X", value);
		break;

	case TILEM_DB_BREAK_OPCODE:
		if (value < 0x100)
			g_snprintf(buf, sizeof(buf), "%02X", value);
		else if (value < 0x10000)
			g_snprintf(buf, sizeof(buf), "%04X", value);
		else if (value < 0x1000000)
			g_snprintf(buf, sizeof(buf), "%06X", value);
		else
			g_snprintf(buf, sizeof(buf), "%08X", value);
		break;

	default:
		g_return_if_reached();
	}

	gtk_entry_set_text(GTK_ENTRY(he->addr_entry), buf);
}

/* Parse contents of entry */
static gboolean parse_num(TilemDebugger *dbg, const char *s, dword *a)
{
	const char *n;
	char *e;

	g_return_val_if_fail(s != NULL, FALSE);

	if (s[0] == '$')
		n = s + 1;
	else if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
		n = s + 2;
	else
		n = s;

	*a = strtol(n, &e, 16);
	if (e != n) {
		if (*e == 'h' || *e == 'H')
			e++;
		if (*e == 0)
			return TRUE;
	}

	if (dbg->dasm && tilem_disasm_get_label(dbg->dasm, s, a))
		return TRUE;

	return FALSE;
}

/* Parse user input from hex entry */
static gboolean hex_entry_parse_value(struct hex_entry *he, TilemDebugger *dbg,
                                      int type, dword *value)
{
	const TilemCalc *calc = dbg->emu->calc;
	dword page;
	const char *s;

	g_return_val_if_fail(calc != NULL, 0);

	s = gtk_entry_get_text(GTK_ENTRY(he->addr_entry));
	if (!parse_num(dbg, s, value))
		return FALSE;

	if (type != TILEM_DB_BREAK_PHYSICAL)
		return TRUE;

	s = gtk_entry_get_text(GTK_ENTRY(he->page_entry));
	if (!parse_num(dbg, s, &page))
		return FALSE;

	*value &= 0x3fff;

	if (page >= calc->hw.rampagemask) {
		*value += ((page - calc->hw.rampagemask) << 14);
		*value %= calc->hw.ramsize;
		*value += calc->hw.romsize;
	}
	else {
		*value += (page << 14);
		*value %= calc->hw.romsize;
	}

	return TRUE;
}

/* Parse input fields and check if they make sense */
static gboolean parse_input(struct breakpoint_dlg *bpdlg,
                            TilemDebugBreakpoint *bp)
{
	int i;
	dword addr0, addr1;

	bp->mask = bp->start = bp->end = 0xffffffff;
	bp->type = get_bp_type(bpdlg);
	bp->mode = 0;

	for (i = 0; i < 3; i++)
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(bpdlg->access_cb[i])))
			bp->mode += (1 << i);

	bp->mode &= type_info[bp->type].access_mask;
	if (bp->type == TILEM_DB_BREAK_OPCODE)
		bp->mode = TILEM_DB_BREAK_EXEC;
	else if (bp->mode == 0)
		return FALSE;

	if (!hex_entry_parse_value(&bpdlg->start, bpdlg->dbg,
	                           bp->type, &addr0))
		return FALSE;

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(bpdlg->range_rb))) {
		if (!hex_entry_parse_value(&bpdlg->end, bpdlg->dbg,
		                           bp->type, &addr1))
			return FALSE;
	}
	else {
		addr1 = addr0;
	}

	if (bp->type == TILEM_DB_BREAK_LOGICAL)
		bp->mask = 0xffff;
	else if (bp->type == TILEM_DB_BREAK_PORT)
		bp->mask = 0xff;
	else
		bp->mask = 0xffffffff;

	bp->start = addr0 & bp->mask;
	bp->end = addr1 & bp->mask;
	if (bp->end < bp->start)
		return FALSE;

	return TRUE;
}

/* Check if input fields are valid, and enable/disable OK response as
   appropriate */
static void validate(struct breakpoint_dlg *bpdlg)
{
	TilemDebugBreakpoint tmpbp;

	if (parse_input(bpdlg, &tmpbp))
		gtk_dialog_set_response_sensitive(GTK_DIALOG(bpdlg->dlg),
		                                  GTK_RESPONSE_OK, TRUE);
	else
		gtk_dialog_set_response_sensitive(GTK_DIALOG(bpdlg->dlg),
		                                  GTK_RESPONSE_OK, FALSE);
}

/* Enable/disable check buttons for access mode */
static void set_access_mask(struct breakpoint_dlg *bpdlg, guint mask)
{
	int i;

	if (mask)
		gtk_widget_show(bpdlg->access_label);
	else
		gtk_widget_hide(bpdlg->access_label);

	for (i = 0; i < 3; i++) {
		if (mask & (1 << i))
			gtk_widget_show(bpdlg->access_cb[i]);
		else
			gtk_widget_hide(bpdlg->access_cb[i]);
	}
}

/* Combo box changed */
static void addr_type_changed(G_GNUC_UNUSED GtkComboBox *combo, gpointer data)
{
	struct breakpoint_dlg *bpdlg = data;
	int type = get_bp_type(bpdlg);
	gboolean range = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(bpdlg->range_rb));
	char *s;

	s = g_strdup_printf("<b>%s</b>", _(type_info[type].value_label));
	gtk_label_set_markup(GTK_LABEL(bpdlg->address_label), s);
	g_free(s);

	set_access_mask(bpdlg, type_info[type].access_mask);

	if (type_info[type].use_pages) {
		gtk_widget_show(bpdlg->start.page_label);
		gtk_widget_show(bpdlg->start.page_entry);
	}
	else {
		gtk_widget_hide(bpdlg->start.page_label);
		gtk_widget_hide(bpdlg->start.page_entry);
	}

	if (range) {
		gtk_label_set_text_with_mnemonic(GTK_LABEL(bpdlg->start.addr_label),
		                                 _("_Start:"));
		gtk_widget_show(bpdlg->end.addr_label);
		gtk_widget_show(bpdlg->end.addr_entry);
	}
	else {
		gtk_label_set_text_with_mnemonic(GTK_LABEL(bpdlg->start.addr_label),
		                                 _("_Value:"));
		gtk_widget_hide(bpdlg->end.addr_label);
		gtk_widget_hide(bpdlg->end.addr_entry);
	}

	if (type_info[type].use_pages && range) {
		gtk_widget_show(bpdlg->end.page_label);
		gtk_widget_show(bpdlg->end.page_entry);
	}
	else {
		gtk_widget_hide(bpdlg->end.page_label);
		gtk_widget_hide(bpdlg->end.page_entry);
	}

	validate(bpdlg);
}

/* Access mode changed */
static void access_changed(G_GNUC_UNUSED GtkToggleButton *tb, gpointer data)
{
	struct breakpoint_dlg *bpdlg = data;
	validate(bpdlg);
}

/* Single/range mode changed */
static void range_mode_changed(G_GNUC_UNUSED GtkToggleButton *tb, gpointer data)
{
	struct breakpoint_dlg *bpdlg = data;
	addr_type_changed(NULL, bpdlg);
}

/* Text of entry changed */
static void entry_edited(G_GNUC_UNUSED GtkEntry *entry,
                         gpointer data)
{
	struct breakpoint_dlg *bpdlg = data;
	validate(bpdlg);
}

/* Key presssed in entry */
static gboolean entry_key_event(G_GNUC_UNUSED GtkWidget *entry,
                                GdkEventKey *ev, gpointer data)
{
	struct breakpoint_dlg *bpdlg = data;
	TilemDebugBreakpoint tmpbp;

	if (ev->state & GDK_MODIFIER_MASK)
		return FALSE;

	if (ev->keyval != GDK_Return
	    && ev->keyval != GDK_KP_Enter
	    && ev->keyval != GDK_ISO_Enter)
		return FALSE;

	if (parse_input(bpdlg, &tmpbp))
		gtk_dialog_response(GTK_DIALOG(bpdlg->dlg), GTK_RESPONSE_OK);
	else
		gtk_widget_child_focus(bpdlg->box, GTK_DIR_TAB_FORWARD);

	return TRUE;
}

static void init_hex_entry(struct breakpoint_dlg *bpdlg,
                           struct hex_entry *he, const char *label,
                           GtkTable *tbl, int ypos)
{
	GtkWidget *align, *lbl;

	he->addr_label = lbl = gtk_label_new_with_mnemonic(label);
	gtk_misc_set_alignment(GTK_MISC(lbl), LABEL_X_ALIGN, 0.5);
	align = gtk_alignment_new(0.5, 0.5, 1.0, 1.0);
	gtk_alignment_set_padding(GTK_ALIGNMENT(align), 0, 0, 12, 6);
	gtk_container_add(GTK_CONTAINER(align), lbl);
	gtk_table_attach(tbl, align, 0, 1, ypos, ypos + 1,
	                 GTK_FILL, GTK_FILL, 0, 0);

	he->addr_entry = gtk_entry_new();
	gtk_label_set_mnemonic_widget(GTK_LABEL(lbl), he->addr_entry);
	gtk_table_attach(tbl, he->addr_entry, 1, 2, ypos, ypos + 1,
	                 GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);

	g_signal_connect(he->addr_entry, "changed",
	                 G_CALLBACK(entry_edited), bpdlg);
	g_signal_connect(he->addr_entry, "key-press-event",
	                 G_CALLBACK(entry_key_event), bpdlg);

	he->page_label = lbl = gtk_label_new(_("Page:"));
	gtk_misc_set_alignment(GTK_MISC(lbl), LABEL_X_ALIGN, 0.5);
	gtk_table_attach(tbl, lbl, 2, 3, ypos, ypos + 1,
	                 GTK_FILL, GTK_FILL, 6, 0);

	he->page_entry = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(he->page_entry), 5);
	gtk_label_set_mnemonic_widget(GTK_LABEL(lbl), he->page_entry);
	gtk_table_attach(tbl, he->page_entry, 3, 4, ypos, ypos + 1,
	                 GTK_FILL, GTK_FILL, 0, 0);

	g_signal_connect(he->page_entry, "changed",
	                 G_CALLBACK(entry_edited), bpdlg);
	g_signal_connect(he->page_entry, "key-press-event",
	                 G_CALLBACK(entry_key_event), bpdlg);
}

static gboolean edit_breakpoint(TilemDebugger *dbg,
                                GtkWindow *parent_window,
                                const char *title,
                                TilemDebugBreakpoint *bp,
                                gboolean edit_existing,
                                gboolean edit_endaddr)
{
	GtkWidget *dlg, *vbox, *frame, *tbl, *hbox, *lbl, *combo, *cb, *rb;
	struct breakpoint_dlg bpdlg;
	gsize i;

	g_return_val_if_fail(bp != NULL, FALSE);
	g_return_val_if_fail(dbg != NULL, FALSE);
	g_return_val_if_fail(dbg->emu != NULL, FALSE);
	g_return_val_if_fail(dbg->emu->calc != NULL, FALSE);

	bpdlg.dbg = dbg;

	dlg = gtk_dialog_new_with_buttons(title, parent_window,
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

	bpdlg.dlg = dlg;

	bpdlg.box = gtk_vbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(bpdlg.box), 6);

	tbl = gtk_table_new(2, 2, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(tbl), 6);
	gtk_table_set_col_spacings(GTK_TABLE(tbl), 6);

	/* Breakpoint type */

	lbl = gtk_label_new_with_mnemonic(_("Breakpoint _type:"));
	gtk_misc_set_alignment(GTK_MISC(lbl), LABEL_X_ALIGN, 0.5);
	gtk_table_attach(GTK_TABLE(tbl), lbl, 0, 1, 0, 1,
	                 GTK_FILL, GTK_FILL, 0, 0);

	combo = gtk_combo_box_new_text();
	for (i = 0; i < G_N_ELEMENTS(type_info); i++)
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo), _(type_info[i].desc));

	bpdlg.type_combo = combo;
	gtk_label_set_mnemonic_widget(GTK_LABEL(lbl), combo);
	gtk_table_attach(GTK_TABLE(tbl), combo, 1, 2, 0, 1,
	                 GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);

	gtk_combo_box_set_active(GTK_COMBO_BOX(combo), bp->type);

	/* Access mode */

	bpdlg.access_label = lbl = gtk_label_new(_("Break when:"));
	gtk_misc_set_alignment(GTK_MISC(lbl), LABEL_X_ALIGN, 0.5);
	gtk_table_attach(GTK_TABLE(tbl), lbl, 0, 1, 1, 2,
	                 GTK_FILL, GTK_FILL, 0, 0);

	hbox = gtk_hbox_new(FALSE, 6);

	cb = gtk_check_button_new_with_mnemonic(_("_Reading"));
	gtk_box_pack_start(GTK_BOX(hbox), cb, FALSE, FALSE, 0);
	bpdlg.access_cb[2] = cb;

	cb = gtk_check_button_new_with_mnemonic(_("_Writing"));
	gtk_box_pack_start(GTK_BOX(hbox), cb, FALSE, FALSE, 0);
	bpdlg.access_cb[1] = cb;

	cb = gtk_check_button_new_with_mnemonic(_("E_xecuting"));
	gtk_box_pack_start(GTK_BOX(hbox), cb, FALSE, FALSE, 0);
	bpdlg.access_cb[0] = cb;

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(bpdlg.access_cb[0]),
	                             bp->mode & 1);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(bpdlg.access_cb[1]),
	                             bp->mode & 2);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(bpdlg.access_cb[2]),
	                             bp->mode & 4);

	gtk_table_attach(GTK_TABLE(tbl), hbox, 1, 2, 1, 2,
	                 GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);

	frame = new_frame(_("Breakpoint Condition"), tbl);
	gtk_box_pack_start(GTK_BOX(bpdlg.box), frame, FALSE, FALSE, 0);

	/* Addresses */

	tbl = gtk_table_new(3, 4, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(tbl), 6);

	hbox = gtk_hbox_new(FALSE, 6);

	rb = gtk_radio_button_new_with_mnemonic(NULL, _("Si_ngle"));
	gtk_box_pack_start(GTK_BOX(hbox), rb, FALSE, FALSE, 0);
	bpdlg.single_rb = rb;

	rb = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(rb), _("R_ange"));
	gtk_box_pack_start(GTK_BOX(hbox), rb, FALSE, FALSE, 0);
	bpdlg.range_rb = rb;

	if (edit_existing && bp->end != bp->start)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb), TRUE);

	gtk_table_attach(GTK_TABLE(tbl), hbox, 0, 2, 0, 1,
	                 GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);

	init_hex_entry(&bpdlg, &bpdlg.start, _("S_tart:"), GTK_TABLE(tbl), 1);
	init_hex_entry(&bpdlg, &bpdlg.end, _("_End:"), GTK_TABLE(tbl), 2);

	frame = new_frame(_("Address"), tbl);
	bpdlg.address_label = gtk_frame_get_label_widget(GTK_FRAME(frame));
	gtk_box_pack_start(GTK_BOX(bpdlg.box), frame, FALSE, FALSE, 0);
	gtk_widget_show_all(bpdlg.box);

	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
	gtk_box_pack_start(GTK_BOX(vbox), bpdlg.box, FALSE, FALSE, 0);

	if (edit_existing) {
		hex_entry_set_value(&bpdlg.start, dbg, bp->type, bp->start);
		hex_entry_set_value(&bpdlg.end, dbg, bp->type, bp->end);
	}

	g_signal_connect(combo, "changed",
	                 G_CALLBACK(addr_type_changed), &bpdlg);
	g_signal_connect(bpdlg.access_cb[0], "toggled",
	                 G_CALLBACK(access_changed), &bpdlg);
	g_signal_connect(bpdlg.access_cb[1], "toggled",
	                 G_CALLBACK(access_changed), &bpdlg);
	g_signal_connect(bpdlg.access_cb[2], "toggled",
	                 G_CALLBACK(access_changed), &bpdlg);
	g_signal_connect(bpdlg.single_rb, "toggled",
	                 G_CALLBACK(range_mode_changed), &bpdlg);

	addr_type_changed(NULL, &bpdlg);

	if (edit_existing && edit_endaddr && bp->end != bp->start)
		gtk_widget_grab_focus(bpdlg.end.addr_entry);
	else
		gtk_widget_grab_focus(bpdlg.start.addr_entry);

	do {
		if (gtk_dialog_run(GTK_DIALOG(dlg)) != GTK_RESPONSE_OK) {
			gtk_widget_destroy(dlg);
			return FALSE;
		}
	} while (!parse_input(&bpdlg, bp));

	gtk_widget_destroy(dlg);
	return TRUE;
}

/**************** Breakpoint list dialog ****************/

enum {
	COL_BP,
	COL_START,
	COL_END,
	COL_TYPE_STR,
	COL_START_STR,
	COL_END_STR,
	COL_ENABLED,
	N_COLUMNS
};

struct bplist_dlg {
	TilemDebugger *dbg;
	GtkWidget *dlg;
	GtkListStore *store;
	GtkWidget *treeview;
	GtkWidget *remove_btn;
	GtkWidget *edit_btn;
	GtkWidget *clear_btn;
};

/* Convert address into a displayable string */
static void format_address(TilemDebugger *dbg,
                           char *buf, int bufsize,
                           int type, dword value)
{
	const TilemCalc *calc;
	unsigned int page;

	g_return_if_fail(dbg->emu != NULL);
	g_return_if_fail(dbg->emu->calc != NULL);

	calc = dbg->emu->calc;

	switch (type) {
	case TILEM_DB_BREAK_LOGICAL:
		g_snprintf(buf, bufsize, "%04X", value);
		break;

	case TILEM_DB_BREAK_PHYSICAL:
		if (value >= calc->hw.romsize) {
			value -= calc->hw.romsize;
			page = (value >> 14) + calc->hw.rampagemask;
		}
		else {
			page = (value >> 14);
		}

		g_snprintf(buf, bufsize, "%02X:%04X", page, value & 0x3fff);
		break;

	case TILEM_DB_BREAK_PORT:
		g_snprintf(buf, bufsize, "%02X", value);
		break;

	case TILEM_DB_BREAK_OPCODE:
		if (value < 0x100)
			g_snprintf(buf, bufsize, "%02X", value);
		else if (value < 0x10000)
			g_snprintf(buf, bufsize, "%04X", value);
		else if (value < 0x1000000)
			g_snprintf(buf, bufsize, "%06X", value);
		else
			g_snprintf(buf, bufsize, "%08X", value);
		break;

	default:
		g_return_if_reached();
	}
}

/* Store breakpoint properties in tree model */
static void set_iter_from_bp(struct bplist_dlg *bpldlg, GtkTreeIter *iter,
                             const TilemDebugBreakpoint *bp)
{
	char tbuf[5], sbuf[10], ebuf[10];
	int i, j;

	g_return_if_fail(bp != NULL);

	/* FIXME: i18n */
	tbuf[0] = type_info[bp->type].abbrev;
	j = 1;
	for (i = 0; i < 3; i++)
		if (bp->mode & (4 >> i))
			tbuf[j++] = "RWX"[i];
	tbuf[j] = 0;

	format_address(bpldlg->dbg, sbuf, sizeof(sbuf), bp->type, bp->start);
	format_address(bpldlg->dbg, ebuf, sizeof(ebuf), bp->type, bp->end);

	gtk_list_store_set(bpldlg->store, iter,
	                   COL_BP, bp,
	                   COL_START, bp->start,
	                   COL_END, bp->end,
	                   COL_TYPE_STR, tbuf,
	                   COL_START_STR, sbuf,
	                   COL_END_STR, ebuf,
	                   COL_ENABLED, !bp->disabled,
	                   -1);
}

/* Get breakpoint pointer for the given tree model row */
static TilemDebugBreakpoint *get_iter_bp(struct bplist_dlg *bpldlg,
                                         GtkTreeIter *iter)
{
	gpointer ptr;
	gtk_tree_model_get(GTK_TREE_MODEL(bpldlg->store), iter,
	                   COL_BP, &ptr, -1);
	return (TilemDebugBreakpoint *) ptr;
}

/* Set buttons sensitive or insensitive depending on whether any BPs
   are selected */
static void update_buttons(struct bplist_dlg *bpldlg)
{
	GtkTreeSelection *sel;
	gboolean any_sel;

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(bpldlg->treeview));

	any_sel = gtk_tree_selection_get_selected(sel, NULL, NULL);
	gtk_widget_set_sensitive(bpldlg->remove_btn, any_sel);
	gtk_widget_set_sensitive(bpldlg->edit_btn, any_sel);

	gtk_widget_set_sensitive(bpldlg->clear_btn,
	                         bpldlg->dbg->breakpoints != NULL);
}

/* "Add breakpoint" button clicked */
static void add_clicked(G_GNUC_UNUSED GtkButton *btn, gpointer data)
{
	struct bplist_dlg *bpldlg = data;
	TilemDebugBreakpoint tmpbp, *newbp;
	TilemDebugger *dbg = bpldlg->dbg;
	GtkTreeIter iter;

	memset(&tmpbp, 0, sizeof(tmpbp));
	tmpbp.type = dbg->last_bp_type;
	tmpbp.mode = dbg->last_bp_mode;

	if (!edit_breakpoint(dbg, GTK_WINDOW(bpldlg->dlg),
	                     _("Add Breakpoint"), &tmpbp, FALSE, FALSE))
		return;

	dbg->last_bp_type = tmpbp.type;
	dbg->last_bp_mode = tmpbp.mode;

	newbp = tilem_debugger_add_breakpoint(dbg, &tmpbp);
	gtk_list_store_append(bpldlg->store, &iter);
	set_iter_from_bp(bpldlg, &iter, newbp);

	update_buttons(bpldlg);
}

/* "Remove breakpoint" button clicked */
static void remove_clicked(G_GNUC_UNUSED GtkButton *btn, gpointer data)
{
	struct bplist_dlg *bpldlg = data;
	GtkTreeSelection *sel;
	GtkTreeIter iter;
	TilemDebugBreakpoint *bp;

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(bpldlg->treeview));

	if (!gtk_tree_selection_get_selected(sel, NULL, &iter))
		return;

	bp = get_iter_bp(bpldlg, &iter);
	g_return_if_fail(bp != NULL);
	gtk_list_store_remove(bpldlg->store, &iter);
	tilem_debugger_remove_breakpoint(bpldlg->dbg, bp);

	update_buttons(bpldlg);
}

/* Edit an existing breakpoint */
static void edit_row(struct bplist_dlg *bpldlg, GtkTreeIter *iter,
                     gboolean edit_endaddr)
{
	TilemDebugBreakpoint *bp, tmpbp;

	bp = get_iter_bp(bpldlg, iter);
	g_return_if_fail(bp != NULL);
	tmpbp = *bp;

	if (!edit_breakpoint(bpldlg->dbg, GTK_WINDOW(bpldlg->dlg),
	                     _("Edit Breakpoint"), &tmpbp,
	                     TRUE, edit_endaddr))
		return;

	tmpbp.disabled = 0;
	tilem_debugger_change_breakpoint(bpldlg->dbg, bp, &tmpbp);
	set_iter_from_bp(bpldlg, iter, bp);

	update_buttons(bpldlg);
}

/* "Edit breakpoint" button clicked */
static void edit_clicked(G_GNUC_UNUSED GtkButton *btn, gpointer data)
{
	struct bplist_dlg *bpldlg = data;
	GtkTreeSelection *sel;
	GtkTreeIter iter;

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(bpldlg->treeview));

	if (!gtk_tree_selection_get_selected(sel, NULL, &iter))
		return;

	edit_row(bpldlg, &iter, FALSE);
}

/* "Clear breakpoints" button clicked */
static void clear_clicked(G_GNUC_UNUSED GtkButton *btn, gpointer data)
{
	struct bplist_dlg *bpldlg = data;
	GtkWidget *dlg;
	TilemDebugBreakpoint *bp;

	dlg = gtk_message_dialog_new(GTK_WINDOW(bpldlg->dlg),
	                             GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION,
	                             GTK_BUTTONS_NONE,
	                             _("Clear all breakpoints?"));
	gtk_message_dialog_format_secondary_markup
		(GTK_MESSAGE_DIALOG(dlg),
		 _("All existing breakpoints will be deleted and"
		   " cannot be restored."));
	gtk_dialog_add_buttons(GTK_DIALOG(dlg),
	                       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	                       GTK_STOCK_CLEAR, GTK_RESPONSE_ACCEPT,
	                       NULL);
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dlg),
	                                        GTK_RESPONSE_ACCEPT,
	                                        GTK_RESPONSE_CANCEL,
	                                        -1);

	if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
		while (bpldlg->dbg->breakpoints) {
			bp = bpldlg->dbg->breakpoints->data;
			tilem_debugger_remove_breakpoint(bpldlg->dbg, bp);
		}
		gtk_list_store_clear(bpldlg->store);
		update_buttons(bpldlg);
	}

	gtk_widget_destroy(dlg);
}

/* Row activated (double-clicked, usually) */
static void row_activated(G_GNUC_UNUSED GtkTreeView *treeview,
                          GtkTreePath *path,
                          GtkTreeViewColumn *col,
                          gpointer data)
{
	struct bplist_dlg *bpldlg = data;
	GtkTreeIter iter;
	int cnum = gtk_tree_view_column_get_sort_column_id(col);

	if (gtk_tree_model_get_iter(GTK_TREE_MODEL(bpldlg->store),
	                            &iter, path))
		edit_row(bpldlg, &iter, (cnum == COL_END));
}

/* Toggle button clicked for a breakpoint */
static void enabled_toggled(G_GNUC_UNUSED GtkCellRendererToggle *cell,
                            gchar *pathstr, gpointer data)
{
	struct bplist_dlg *bpldlg = data;
	GtkTreePath *path;
	GtkTreeIter iter;
	TilemDebugBreakpoint *bp, tmpbp;

	path = gtk_tree_path_new_from_string(pathstr);
	if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(bpldlg->store),
	                             &iter, path)) {
		gtk_tree_path_free(path);
		return;
	}
	gtk_tree_path_free(path);

	bp = get_iter_bp(bpldlg, &iter);
	g_return_if_fail(bp != NULL);
	tmpbp = *bp;
	tmpbp.disabled = !tmpbp.disabled;
	tilem_debugger_change_breakpoint(bpldlg->dbg, bp, &tmpbp);
	set_iter_from_bp(bpldlg, &iter, bp);
}

/* Selection changed */
static void selection_changed(G_GNUC_UNUSED GtkTreeSelection *sel,
                              gpointer data)
{
	struct bplist_dlg *bpldlg = data;
	update_buttons(bpldlg);
}

/* Show a dialog letting the user add, remove, and edit breakpoints */
void tilem_debugger_edit_breakpoints(TilemDebugger *dbg)
{
	struct bplist_dlg bpldlg;
	GtkWidget *dlg, *hbox, *treeview, *sw, *bbox, *btn, *vbox, *vbox2,
		*invalid_cb, *undoc_cb;
	GtkListStore *store;
	GtkTreeViewColumn *col;
	GtkCellRenderer *cell;
	GtkTreeIter iter;
	GSList *l;
	GtkTreeSelection *sel;
	unsigned int flags;

	g_return_if_fail(dbg != NULL);
	g_return_if_fail(dbg->emu != NULL);
	g_return_if_fail(dbg->emu->calc != NULL);

	bpldlg.dbg = dbg;

	dlg = gtk_dialog_new_with_buttons(_("Breakpoints"),
	                                  GTK_WINDOW(dbg->window),
	                                  GTK_DIALOG_MODAL,
	                                  GTK_STOCK_CLOSE,
	                                  GTK_RESPONSE_ACCEPT,
	                                  NULL);

	gtk_window_set_default_size(GTK_WINDOW(dlg), -1, 300);

	store = gtk_list_store_new(N_COLUMNS,
	                           G_TYPE_POINTER,
	                           G_TYPE_INT,
	                           G_TYPE_INT,
	                           G_TYPE_STRING,
	                           G_TYPE_STRING,
	                           G_TYPE_STRING,
	                           G_TYPE_BOOLEAN);

	bpldlg.dlg = dlg;
	bpldlg.store = store;

	vbox = gtk_vbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);

	hbox = gtk_hbox_new(FALSE, 6);

	for (l = dbg->breakpoints; l; l = l->next) {
		gtk_list_store_append(store, &iter);
		set_iter_from_bp(&bpldlg, &iter, l->data);
	}

	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), TRUE);
	gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(treeview), TRUE);
	gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(treeview), TRUE);
	bpldlg.treeview = treeview;

	fixed_tree_view_init(treeview, 0,
	                     COL_TYPE_STR, "MRWX ",
	                     COL_START_STR, "DD:DDDD ",
	                     COL_END_STR, "DD:DDDD ",
	                     COL_ENABLED, TRUE,
	                     -1);

	g_signal_connect(treeview, "row-activated",
	                 G_CALLBACK(row_activated), &bpldlg);

	/* Enabled/type column */

	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, _("Type"));
	gtk_tree_view_column_set_sort_column_id(col, COL_TYPE_STR);
	gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);

	cell = gtk_cell_renderer_toggle_new();
	gtk_tree_view_column_pack_start(col, cell, FALSE);
	gtk_tree_view_column_set_attributes(col, cell,
	                                    "active", COL_ENABLED,
	                                    NULL);
	g_signal_connect(cell, "toggled",
	                 G_CALLBACK(enabled_toggled), &bpldlg);

	cell = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, cell, TRUE);
	gtk_tree_view_column_set_attributes(col, cell,
	                                    "text", COL_TYPE_STR,
	                                    NULL);

	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), col);

	/* Start column */

	cell = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes
		(_("Start"), cell, "text", COL_START_STR, NULL);
	gtk_tree_view_column_set_sort_column_id(col, COL_START);
	gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);

	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), col);

	/* End column */

	cell = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes
		(_("End"), cell, "text", COL_END_STR, NULL);
	gtk_tree_view_column_set_sort_column_id(col, COL_END);
	gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);

	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), col);

	sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
	                               GTK_POLICY_NEVER,
	                               GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
	                                    GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(sw), treeview);

	gtk_box_pack_start(GTK_BOX(hbox), sw, TRUE, TRUE, 0);

	/* Buttons */

	bbox = gtk_vbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_START);
	gtk_box_set_spacing(GTK_BOX(bbox), 6);

	btn = gtk_button_new_from_stock(GTK_STOCK_ADD);
	g_signal_connect(btn, "clicked",
	                 G_CALLBACK(add_clicked), &bpldlg);
	gtk_container_add(GTK_CONTAINER(bbox), btn);

	btn = gtk_button_new_from_stock(GTK_STOCK_REMOVE);
	g_signal_connect(btn, "clicked",
	                 G_CALLBACK(remove_clicked), &bpldlg);
	gtk_container_add(GTK_CONTAINER(bbox), btn);
	bpldlg.remove_btn = btn;

	btn = gtk_button_new_from_stock(GTK_STOCK_EDIT);
	g_signal_connect(btn, "clicked",
	                 G_CALLBACK(edit_clicked), &bpldlg);
	gtk_container_add(GTK_CONTAINER(bbox), btn);
	bpldlg.edit_btn = btn;

	btn = gtk_button_new_from_stock(GTK_STOCK_CLEAR);
	g_signal_connect(btn, "clicked",
	                 G_CALLBACK(clear_clicked), &bpldlg);
	gtk_container_add(GTK_CONTAINER(bbox), btn);
	bpldlg.clear_btn = btn;

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
	g_signal_connect(sel, "changed",
	                 G_CALLBACK(selection_changed), &bpldlg);

	update_buttons(&bpldlg);

	gtk_box_pack_start(GTK_BOX(hbox), bbox, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

	invalid_cb = gtk_check_button_new_with_mnemonic
		(_("Break on _invalid instructions"));
	undoc_cb = gtk_check_button_new_with_mnemonic
		(_("Break on _undocumented instructions"));

	tilem_calc_emulator_lock(dbg->emu);
	flags = dbg->emu->calc->z80.emuflags;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(invalid_cb),
	                             (flags & TILEM_Z80_BREAK_INVALID));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(undoc_cb),
	                             (flags & TILEM_Z80_BREAK_UNDOCUMENTED));
	tilem_calc_emulator_unlock(dbg->emu);

	gtk_box_pack_start(GTK_BOX(vbox), invalid_cb, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), undoc_cb, FALSE, FALSE, 0);

	gtk_widget_show_all(vbox);

	gtk_widget_grab_focus(treeview);

	vbox2 = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
	gtk_box_pack_start(GTK_BOX(vbox2), vbox, TRUE, TRUE, 0);

	gtk_dialog_run(GTK_DIALOG(dlg));

	tilem_calc_emulator_lock(dbg->emu);
	flags = dbg->emu->calc->z80.emuflags;
	flags &= ~(TILEM_Z80_BREAK_INVALID | TILEM_Z80_BREAK_UNDOCUMENTED);
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(invalid_cb)))
		flags |= TILEM_Z80_BREAK_INVALID;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(undoc_cb)))
		flags |= TILEM_Z80_BREAK_UNDOCUMENTED;
	dbg->emu->calc->z80.emuflags = flags;
	tilem_calc_emulator_unlock(dbg->emu);

	gtk_widget_destroy(dlg);
}
