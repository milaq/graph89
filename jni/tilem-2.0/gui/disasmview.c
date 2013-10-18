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
#include <string.h>
#include <gtk/gtk.h>
#include <ticalcs.h>
#include <tilem.h>
#include <tilemdb.h>

#include "gui.h"
#include "disasmview.h"

G_DEFINE_TYPE(TilemDisasmView, tilem_disasm_view, GTK_TYPE_TREE_VIEW);

/*
  This is a HORRIBLE kludge.  Don't ever do anything like this.  ;)

  We want a widget that has the look and feel of a GtkTreeView.  But
  our "data model" isn't consistent with a GtkTreeModel, since it
  changes depending on where we are.

  This widget keeps track of how high each row will be once rendered,
  and uses that to construct a GtkListStore with the appropriate
  number of rows to fill the window.  We also override the move-cursor
  signal so that we can handle the boundaries.
 */

/* Model columns */
enum {
	COL_POSITION,
	COL_ADDRESS,
	COL_MNEMONIC,
	COL_ARGUMENTS,
	COL_SHOW_MNEMONIC,
	COL_ICON,
	NUM_COLUMNS
};

static GtkTreeViewClass *parent_class;

/* We define two "positions" for each actual address; the second is
   used if there's a label to be displayed at that address. */

#define POS_TO_ADDR(x) ((x) >> 1)
#define ADDR_TO_POS(x) ((x) << 1)

/* Disassembly */

/* Convert physical to logical address; if address is not currently
   mapped, use the bank-A address */
static dword default_ptol(TilemCalc *calc, dword addr)
{
	dword addr_l;

	g_return_val_if_fail(calc != NULL, 0);

	addr_l = (*calc->hw.mem_ptol)(calc, addr);
	if (addr_l == 0xffffffff)
		addr_l = (addr & 0x3fff) | 0x4000;

	return addr_l;
}

/* Check for a label at the given address (physical or logical
   depending on the mode of the DisasmView) */
static const char *get_label(TilemDisasmView *dv, TilemCalc *calc,
                             dword addr)
{
	g_return_val_if_fail(calc != NULL, NULL);
	g_return_val_if_fail(dv->dbg->dasm != NULL, NULL);

	if (!dv->use_logical)
		addr = default_ptol(calc, addr);

	return tilem_disasm_get_label_at_address(dv->dbg->dasm, addr);
}

/* Disassemble a line */
static void disassemble(TilemDisasmView *dv, TilemCalc *calc, dword pos,
                        dword *nextpos, char **mnemonic, char **args)
{
	dword addr = POS_TO_ADDR(pos);
	const char *lbl;
	char buf[500], *p;

	g_return_if_fail(calc != NULL);
	g_return_if_fail(dv->dbg->dasm != NULL);

	if (!(pos & 1) && (lbl = get_label(dv, calc, addr))) {
		if (mnemonic) {
			*mnemonic = NULL;
			*args = g_strdup_printf(_("%s:"), lbl);
		}

		if (nextpos)
			*nextpos = pos + 1;
	}
	else if (mnemonic) {
		tilem_disasm_disassemble(dv->dbg->dasm, calc,
		                         !dv->use_logical, addr,
		                         &addr, buf, sizeof(buf));

		p = strchr(buf, '\t');
		if (p) {
			*mnemonic = g_strndup(buf, p - buf);
			*args = g_strdup(p + 1);
		}
		else {
			*mnemonic = g_strdup(buf);
			*args = NULL;
		}

		if (nextpos)
			*nextpos = ADDR_TO_POS(addr);
	}
	else {
		tilem_disasm_disassemble(dv->dbg->dasm, calc,
		                         !dv->use_logical, addr,
		                         &addr, NULL, 0);
		if (nextpos)
			*nextpos = ADDR_TO_POS(addr);
	}
}

/* Get "next" position */
static dword get_next_pos(TilemDisasmView *dv, TilemCalc *calc, dword pos)
{
	disassemble(dv, calc, pos, &pos, NULL, NULL);
	return pos;
}

/* Get "previous" position */
static dword get_prev_pos(TilemDisasmView *dv, TilemCalc *calc, dword pos)
{
	dword addr = POS_TO_ADDR(pos), a2;

	g_return_val_if_fail(calc != NULL, 0);

	if (pos & 1) {
		return pos - 1;
	}
	else {
		a2 = tilem_disasm_guess_prev_address(dv->dbg->dasm, calc,
		                                     !dv->use_logical, addr);

		if (a2 != addr && get_label(dv, calc, a2))
			return ADDR_TO_POS(a2) + 1;
		else
			return ADDR_TO_POS(a2);
	}
}

/* Convert physical to logical position */
static dword pos_ptol(TilemCalc *calc, dword pos)
{
	dword addr;

	g_return_val_if_fail(calc != NULL, 0);

	if (pos == (dword) -1)
		return pos;

	addr = default_ptol(calc, POS_TO_ADDR(pos));
	return ADDR_TO_POS(addr) + (pos & 1);
}

/* Convert logical to physical position */
static dword pos_ltop(TilemCalc *calc, dword pos)
{
	dword addr;

	g_return_val_if_fail(calc != NULL, 0);

	if (pos == (dword) -1)
		return pos;

	addr = (*calc->hw.mem_ltop)(calc, POS_TO_ADDR(pos));
	return ADDR_TO_POS(addr) + (pos & 1);
}

/* Icons */

static GdkPixbuf *get_icon(TilemDisasmView *dv, gboolean ispc, gboolean isbp)
{
	const char *name;

	if (ispc && isbp)
		name = "tilem-disasm-break-pc";
	else if (isbp)
		name = "tilem-disasm-break";
	else if (ispc)
		name = "tilem-disasm-pc";
	else
		return NULL;

	return gtk_widget_render_icon(GTK_WIDGET(dv), name,
	                              GTK_ICON_SIZE_MENU, NULL);
}

/* List model management */

/* Create a new list store for disassembly */
static GtkTreeModel * new_dasm_model()
{
	GtkListStore *store;

	g_assert(NUM_COLUMNS == 6);
	store = gtk_list_store_new(6,
	                           G_TYPE_INT,
	                           G_TYPE_STRING,
	                           G_TYPE_STRING,
	                           G_TYPE_STRING,
	                           G_TYPE_BOOLEAN,
	                           GDK_TYPE_PIXBUF);

	return GTK_TREE_MODEL(store);
}

/* Append dummy data to the model; used for sizing */
static void append_dummy_line(TilemDisasmView *dv, GtkTreeModel *model,
                              GtkTreeIter *iter)
{
	GtkTreeIter iter1;
	GdkPixbuf *icon;

	gtk_list_store_append(GTK_LIST_STORE(model), &iter1);

	icon = get_icon(dv, TRUE, FALSE);

	gtk_list_store_set(GTK_LIST_STORE(model), &iter1,
	                   COL_ICON, icon,
	                   COL_ADDRESS, "DD:DDDD",
	                   COL_MNEMONIC, "ROM_CALL",
	                   COL_ARGUMENTS, "_fnord",
	                   COL_SHOW_MNEMONIC, TRUE,
	                   -1);

	if (icon)
		g_object_unref(icon);

	if (iter)
		*iter = iter1;
}

/* Check if given logical address is a breakpoint (according to
   current mapping) */
static TilemDebugBreakpoint *find_bp_logical(TilemDebugger *dbg,
                                             TilemCalc *calc,
                                             dword addr)
{
	GSList *l;
	TilemDebugBreakpoint *bp;
	dword pa = (*calc->hw.mem_ltop)(calc, addr);

	for (l = dbg->breakpoints; l; l = l->next) {
		bp = l->data;
		if (!(bp->mode & TILEM_DB_BREAK_EXEC))
			continue;

		if (bp->type == TILEM_DB_BREAK_LOGICAL
		    && bp->start <= addr
		    && bp->end >= addr
		    && !bp->disabled)
			return bp;

		if (bp->type == TILEM_DB_BREAK_PHYSICAL
		    && bp->start <= pa
		    && bp->end >= pa
		    && !bp->disabled)
			return bp;
	}

	return NULL;
}

/* Check if given physical address is a breakpoint (according to
   current mapping) */
static TilemDebugBreakpoint *find_bp_physical(TilemDebugger *dbg,
                                              TilemCalc *calc,
                                              dword addr)
{
	GSList *l;
	TilemDebugBreakpoint *bp;
	dword la, pa;
	int i, mapped[4];

	/* NOTE: this assumes that bits 0-13 are unaffected by the
	   mapping!  This is true for all current models, but might
	   need to be changed in the future */
	for (i = 0; i < 4; i++) {
		la = (i << 14) + (addr & 0x3fff);
		pa = (*calc->hw.mem_ltop)(calc, la);
		mapped[i] = (addr == pa);
	}

	for (l = dbg->breakpoints; l; l = l->next) {
		bp = l->data;
		if (!(bp->mode & TILEM_DB_BREAK_EXEC))
			continue;

		if (bp->type == TILEM_DB_BREAK_PHYSICAL
		    && bp->start <= addr
		    && bp->end >= addr
		    && !bp->disabled)
			return bp;

		if (bp->type == TILEM_DB_BREAK_LOGICAL
		    && !bp->disabled) {
			for (i = 0; i < 4; i++) {
				la = (i << 14) + (addr & 0x3fff);
				if (bp->start <= la
				    && bp->end >= la
				    && mapped[i])
					return bp;
			}
		}
	}

	return NULL;
}

/* Check if line has a breakpoint set */
static TilemDebugBreakpoint *find_line_bp(TilemDisasmView *dv, dword pos)
{
	TilemDebugBreakpoint *bp;
	dword addr = POS_TO_ADDR(pos);
	TilemCalc *calc;

	tilem_calc_emulator_lock(dv->dbg->emu);
	calc = dv->dbg->emu->calc;

	if (dv->use_logical)
		bp = find_bp_logical(dv->dbg, calc, addr);
	else
		bp = find_bp_physical(dv->dbg, calc, addr);

	tilem_calc_emulator_unlock(dv->dbg->emu);

	return bp;
}

/* Enable breakpoint on the given line */
static void enable_line_bp(TilemDisasmView *dv, dword pos)
{
	TilemDebugBreakpoint tmpbp;

	if (find_line_bp(dv, pos))
		return;

	tmpbp.type = (dv->use_logical
	              ? TILEM_DB_BREAK_LOGICAL
	              : TILEM_DB_BREAK_PHYSICAL);
	tmpbp.mode = TILEM_DB_BREAK_EXEC;
	tmpbp.start = POS_TO_ADDR(pos);
	tmpbp.end = POS_TO_ADDR(pos);
	tmpbp.mask = (dv->use_logical ? 0xffff : 0xffffffff);
	tmpbp.disabled = 0;
	tilem_debugger_add_breakpoint(dv->dbg, &tmpbp);
}

/* Disable breakpoint on the given line */
static void disable_line_bp(TilemDisasmView *dv, dword pos)
{
	TilemDebugBreakpoint *bp, tmpbp;

	if (!(bp = find_line_bp(dv, pos)))
		return;

	if (bp->mode != TILEM_DB_BREAK_EXEC || bp->start != bp->end) {
		/* special breakpoint; do not delete it, just disable it */
		tmpbp = *bp;
		tmpbp.disabled = 1;
		tilem_debugger_change_breakpoint(dv->dbg, bp, &tmpbp);
	}
	else {
		/* regular breakpoint */
		tilem_debugger_remove_breakpoint(dv->dbg, bp);
	}
}

/* Append a line to the dasm model */
static void append_dasm_line(TilemDisasmView *dv, TilemCalc *calc,
                             GtkTreeModel *model, GtkTreeIter *iter,
                             dword pos, dword *nextpos)
{
	GtkTreeIter iter1;
	char *astr, *mnem, *args;
	dword addr, pc;
	gboolean ispc;
	TilemDebugBreakpoint *bp;
	GdkPixbuf *icon;

	g_return_if_fail(calc != NULL);

	gtk_list_store_append(GTK_LIST_STORE(model), &iter1);

	addr = POS_TO_ADDR(pos);
	astr = tilem_format_addr(dv->dbg, addr, !dv->use_logical);

	disassemble(dv, calc, pos, nextpos, &mnem, &args);

	if (!mnem)
		bp = NULL;
	else if (dv->use_logical)
		bp = find_bp_logical(dv->dbg, calc, addr);
	else
		bp = find_bp_physical(dv->dbg, calc, addr);

	if (!mnem || !dv->dbg->emu->paused) {
		ispc = FALSE;
	}
	else {
		pc = calc->z80.r.pc.w.l;
		if (!dv->use_logical)
			pc = (*calc->hw.mem_ltop)(calc, pc);
		ispc = (addr == pc);
	}

	icon = get_icon(dv, ispc, (bp != NULL));

	gtk_list_store_set(GTK_LIST_STORE(model), &iter1,
	                   COL_POSITION, (int) pos,
	                   COL_ADDRESS, astr,
	                   COL_MNEMONIC, mnem,
	                   COL_SHOW_MNEMONIC, (mnem ? TRUE : FALSE),
	                   COL_ARGUMENTS, args,
	                   COL_ICON, icon,
	                   -1);

	if (icon)
		g_object_unref(icon);

	g_free(astr);
	g_free(mnem);
	g_free(args);

	if (iter)
		*iter = iter1;
}

/* Refresh the view by creating and populating a new model */
static void refresh_disassembly(TilemDisasmView *dv, dword pos, int nlines,
                                dword selectpos)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *selectpath = NULL;
	TilemCalc *calc;
	dword nextpos;
	int i;

	model = new_dasm_model();

	dv->startpos = pos;

	tilem_calc_emulator_lock(dv->dbg->emu);
	calc = dv->dbg->emu->calc;

	if (!calc)
		nlines = 0;

	for (i = 0; i < nlines; i++) {
		append_dasm_line(dv, calc, model, &iter, pos, &nextpos);

		if (pos == selectpos)
			selectpath = gtk_tree_model_get_path(model, &iter);

		pos = nextpos;
	}

	tilem_calc_emulator_unlock(dv->dbg->emu);

	dv->endpos = pos;
	dv->nlines = nlines;

	gtk_tree_view_set_model(GTK_TREE_VIEW(dv), model);
	g_object_unref(model);

	if (selectpath) {
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(dv), selectpath,
		                         NULL, FALSE);
		gtk_tree_path_free(selectpath);
	}
}

/* Determine the (absolute) position and (display-relative) line
   number of the cursor, if any */
static gboolean get_cursor_line(TilemDisasmView *dv, dword *pos,
                                int *linenum)
{
	GtkTreePath *path;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gint *i, p;

	gtk_tree_view_get_cursor(GTK_TREE_VIEW(dv), &path, NULL);
	if (!path) {
		if (pos) *pos = (dword) -1;
		if (linenum) *linenum = -1;
		return FALSE;
	}

	if (pos) {
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(dv));
		if (gtk_tree_model_get_iter(model, &iter, path)) {
			gtk_tree_model_get(model, &iter,
			                   COL_POSITION, &p, -1);	
			*pos = p;
		}
		else {
			*pos = (dword) -1;
		}
	}

	if (linenum) {
		i = gtk_tree_path_get_indices(path);
		*linenum = i[0];
	}

	gtk_tree_path_free(path);

	return TRUE;
}

/* Size allocation */

/* Get the desired height for the tree view (based on size of the data
   we've inserted into the model) */
static int get_parent_request_height(GtkWidget *w)
{
	GtkRequisition req;
	(*GTK_WIDGET_CLASS(parent_class)->size_request)(w, &req);
	return req.height;
}

/* Widget is assigned a size and position */
static void tilem_disasm_view_size_allocate(GtkWidget *w,
                                            GtkAllocation *alloc)
{
	TilemDisasmView *dv;
	GtkTreeModel *model;
	dword curpos;
	int n, height1, height2;

	g_return_if_fail(TILEM_IS_DISASM_VIEW(w));
	dv = TILEM_DISASM_VIEW(w);

	(*GTK_WIDGET_CLASS(parent_class)->size_allocate)(w, alloc);

	if (alloc->height < 1)
		return;

	get_cursor_line(dv, &curpos, NULL);

	/* Calculate line height */
	if (!dv->line_height) {
		model = new_dasm_model();

		append_dummy_line(dv, model, NULL);
		gtk_tree_view_set_model(GTK_TREE_VIEW(dv), model);
		height1 = get_parent_request_height(w);

		append_dummy_line(dv, model, NULL);
		height2 = get_parent_request_height(w);

		dv->line_height = height2 - height1;
		dv->base_height = height1 - dv->line_height;

		g_object_unref(model);

		dv->nlines = 0;

		if (dv->line_height <= 0) {
			dv->line_height = 0;
			return;
		}
	}

	n = (alloc->height - dv->base_height) / dv->line_height;

	if (n < 1)
		n = 1;

	if (n != dv->nlines)
		refresh_disassembly(dv, dv->startpos, n, curpos);
}

/* Get widget's desired size */
static void tilem_disasm_view_size_request(GtkWidget *w, GtkRequisition *req)
{
	(*GTK_WIDGET_CLASS(parent_class)->size_request)(w, req);
	req->height = 100;	/* ignore requested height */
}

/* Widget style set */
static void tilem_disasm_view_style_set(GtkWidget *w, GtkStyle *oldstyle)
{
	TilemDisasmView *dv;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GList *cols, *cp;
	GtkTreeViewColumn *col;
	int width;

	g_return_if_fail(TILEM_IS_DISASM_VIEW(w));
	dv = TILEM_DISASM_VIEW(w);

	(*GTK_WIDGET_CLASS(parent_class)->style_set)(w, oldstyle);

	/* line height must be recalculated */
	dv->line_height = 0;

	/* set column widths based on a dummy model */

	model = new_dasm_model();
	append_dummy_line(dv, model, &iter);

	cols = gtk_tree_view_get_columns(GTK_TREE_VIEW(dv));
	for (cp = cols; cp; cp = cp->next) {
		col = cp->data;
		gtk_tree_view_column_cell_set_cell_data(col, model, &iter,
		                                        FALSE, FALSE);
		gtk_tree_view_column_cell_get_size(col, NULL, NULL, NULL,
		                                   &width, NULL);
		gtk_tree_view_column_set_fixed_width(col, width + 2);
	}
	g_list_free(cols);

	g_object_unref(model);
}

/* Cursor movement commands */

/* Move up by COUNT lines */
static gboolean move_up_lines(TilemDisasmView *dv, int count)
{
	TilemCalc *calc;
	dword pos;
	int linenum;

	if (!get_cursor_line(dv, NULL, &linenum))
		linenum = 0;

	if (linenum >= count)
		return FALSE;

	tilem_calc_emulator_lock(dv->dbg->emu);
	calc = dv->dbg->emu->calc;

	pos = dv->startpos;
	count -= linenum;
	while (count > 0) {
		pos = get_prev_pos(dv, calc, pos);
		count--;
	}

	tilem_calc_emulator_unlock(dv->dbg->emu);

	refresh_disassembly(dv, pos, dv->nlines, pos);

	return TRUE;
}

/* Move down by COUNT lines */
static gboolean move_down_lines(TilemDisasmView *dv, int count)
{
	TilemCalc *calc;
	dword startpos, selpos;
	int linenum;

	if (!get_cursor_line(dv, NULL, &linenum))
		linenum = -1;

	if (linenum + count < dv->nlines)
		return FALSE;

	tilem_calc_emulator_lock(dv->dbg->emu);
	calc = dv->dbg->emu->calc;

	startpos = get_next_pos(dv, calc, dv->startpos);
	selpos = dv->endpos;
	count -= dv->nlines - linenum;

	while (count > 0) {
		startpos = get_next_pos(dv, calc, startpos);
		selpos = get_next_pos(dv, calc, selpos);
		count--;
	}

	tilem_calc_emulator_unlock(dv->dbg->emu);

	refresh_disassembly(dv, startpos, dv->nlines, selpos);

	return TRUE;
}

/* Move up by COUNT pages */
static void move_up_pages(TilemDisasmView *dv, int count)
{
	TilemCalc *calc;
	int i;
	dword pos;

	tilem_calc_emulator_lock(dv->dbg->emu);
	calc = dv->dbg->emu->calc;

	pos = dv->startpos;

	while (count > 0) {
		for (i = 0; i < dv->nlines; i++)
			pos = get_prev_pos(dv, calc, pos);
		count--;
	}

	tilem_calc_emulator_unlock(dv->dbg->emu);
	refresh_disassembly(dv, pos, dv->nlines, pos - 1);
}

/* Move down by COUNT pages */
static void move_down_pages(TilemDisasmView *dv, int count)
{
	TilemCalc *calc;
	int i;
	dword pos;

	tilem_calc_emulator_lock(dv->dbg->emu);
	calc = dv->dbg->emu->calc;

	pos = dv->endpos;
	count--;

	while (count > 0) {
		for (i = 0; i < dv->nlines; i++)
			pos = get_next_pos(dv, calc, pos);
		count--;
	}

	tilem_calc_emulator_unlock(dv->dbg->emu);
	refresh_disassembly(dv, pos, dv->nlines, pos - 1);
}

/* Move down by COUNT bytes */
static void move_bytes(TilemDisasmView *dv, int count)
{
	dword pos, addr;
	const TilemCalc *calc = dv->dbg->emu->calc;

	g_return_if_fail(calc != NULL);

	if (!get_cursor_line(dv, &pos, NULL))
		pos = dv->startpos;

	addr = POS_TO_ADDR(pos);

	if (dv->use_logical)
		addr = (addr + count) & 0xffff;
	else {
		addr += calc->hw.romsize + calc->hw.ramsize + count;
		addr %= calc->hw.romsize + calc->hw.ramsize;
	}

	pos = ADDR_TO_POS(addr);
	refresh_disassembly(dv, pos, dv->nlines, pos);
}

/* Move the cursor (action signal) */
static gboolean tilem_disasm_view_move_cursor(GtkTreeView *tv,
                                              GtkMovementStep step,
                                              gint count)
{
	TilemDisasmView *dv;

	g_return_val_if_fail(TILEM_IS_DISASM_VIEW(tv), FALSE);
	dv = TILEM_DISASM_VIEW(tv);

	if (!dv->dbg->emu->calc)
		return FALSE;

	switch (step) {
	case GTK_MOVEMENT_DISPLAY_LINES:
		if (count < 0) {
			if (move_up_lines(dv, -count))
				return TRUE;
		}
		else {
			if (move_down_lines(dv, count))
				return TRUE;
		}
		break;

	case GTK_MOVEMENT_PARAGRAPHS:
	case GTK_MOVEMENT_PARAGRAPH_ENDS:
	case GTK_MOVEMENT_PAGES:
		if (count < 0)
			move_up_pages(dv, -count);
		else
			move_down_pages(dv, count);
		return TRUE;

	case GTK_MOVEMENT_BUFFER_ENDS:
		move_bytes(dv, count * 0x4000);
		return TRUE;

	case GTK_MOVEMENT_LOGICAL_POSITIONS:
	case GTK_MOVEMENT_VISUAL_POSITIONS:
	case GTK_MOVEMENT_WORDS:
	case GTK_MOVEMENT_DISPLAY_LINE_ENDS:
	case GTK_MOVEMENT_HORIZONTAL_PAGES:
	default:
		break;
	}

	return (*GTK_TREE_VIEW_CLASS(parent_class)->move_cursor)(tv, step, count);
}

/* Scroll view by a fixed number of bytes. */
void tilem_disasm_view_scroll_bytes(TilemDisasmView *dv, int n)
{
	g_return_if_fail(TILEM_IS_DISASM_VIEW(dv));
	move_bytes(dv, n);
}

/* Popup menu */

static void toggle_bp(G_GNUC_UNUSED GtkCheckMenuItem *item, gpointer data)
{
	TilemDisasmView *dv = data;
	tilem_disasm_view_toggle_breakpoint(dv);
}

void tilem_disasm_view_toggle_breakpoint(TilemDisasmView *dv)
{
	dword curpos;

	g_return_if_fail(TILEM_IS_DISASM_VIEW(dv));

	get_cursor_line(dv, &curpos, NULL);
	if (curpos == (dword) -1)
		return;

	if (find_line_bp(dv, curpos))
		disable_line_bp(dv, curpos);
	else
		enable_line_bp(dv, curpos);
}

static void prompt_go_to(G_GNUC_UNUSED GtkMenuItem *item, gpointer data)
{
	TilemDisasmView *dv = data;
	GtkWidget *window;
	dword curpos, addr;

	window = gtk_widget_get_toplevel(GTK_WIDGET(dv));

	get_cursor_line(dv, &curpos, NULL);
	addr = POS_TO_ADDR(curpos);

	if (tilem_prompt_address(dv->dbg, GTK_WINDOW(window),
	                         _("Go to Address"), _("Address:"),
	                         &addr, !dv->use_logical,
	                         (curpos != (dword) -1)))
		tilem_disasm_view_go_to_address(dv, addr, dv->use_logical);
}

static void go_to_pc(G_GNUC_UNUSED GtkMenuItem *item, gpointer data)
{
	TilemDisasmView *dv = data;
	TilemCalc *calc;
	dword pc;

	g_return_if_fail(dv->dbg != NULL);
	g_return_if_fail(dv->dbg->emu != NULL);
	g_return_if_fail(dv->dbg->emu->calc != NULL);

	tilem_calc_emulator_lock(dv->dbg->emu);
	calc = dv->dbg->emu->calc;
	pc = calc->z80.r.pc.w.l;
	tilem_calc_emulator_unlock(dv->dbg->emu);

	tilem_disasm_view_go_to_address(dv, pc, TRUE);
}

/* Determine where to pop up menu (if not activated by a mouse event) */
static void place_menu(GtkMenu *menu, gint *x, gint *y,
                       gboolean *push_in, gpointer data)
{
	TilemDisasmView *dv = data;
	GtkTreePath *path;
	GdkRectangle rect;
	GdkWindow *win;
	GdkScreen *screen;
	int n;

	win = gtk_tree_view_get_bin_window(GTK_TREE_VIEW(dv));
	gdk_window_get_origin(win, x, y);

	gtk_tree_view_get_cursor(GTK_TREE_VIEW(dv), &path, NULL);
	if (path) {
		gtk_tree_view_get_cell_area(GTK_TREE_VIEW(dv), path, NULL, &rect);
		gtk_tree_path_free(path);
		*y += rect.y + rect.height;
	}

	screen = gdk_drawable_get_screen(win);
	n = gdk_screen_get_monitor_at_point(screen, *x, *y);
	gtk_menu_set_monitor(menu, n);

	*push_in = FALSE;
}

/* Create and show the popup menu */
static void show_popup_menu(TilemDisasmView *dv, GdkEventButton *event)
{
	GtkWidget *menu, *item;
	dword curpos;

	if (dv->popup_menu)
		gtk_widget_destroy(dv->popup_menu);
	dv->popup_menu = menu = gtk_menu_new();

	/* Enable/disable breakpoint */

	item = gtk_check_menu_item_new_with_mnemonic(_("_Breakpoint Here"));

	get_cursor_line(dv, &curpos, NULL);
	if (curpos == (dword) -1)
		gtk_widget_set_sensitive(item, FALSE);
	else if (find_line_bp(dv, curpos))
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), TRUE);

	g_signal_connect(item, "toggled",
	                 G_CALLBACK(toggle_bp), dv);

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	gtk_widget_show(item);

	item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	gtk_widget_show(item);

	/* Jump to address */

	item = gtk_menu_item_new_with_mnemonic(_("_Go to Address..."));
	g_signal_connect(item, "activate", G_CALLBACK(prompt_go_to), dv);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	gtk_widget_show(item);

	item = gtk_menu_item_new_with_mnemonic(_("Go to P_C"));
	g_signal_connect(item, "activate", G_CALLBACK(go_to_pc), dv);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	gtk_widget_show(item);

	if (event)
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
		               event->button, event->time);
	else
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, &place_menu, dv,
		               0, gtk_get_current_event_time());
}

/* Button pressed */
static gboolean tilem_disasm_view_button_press(GtkWidget *w,
                                               GdkEventButton *event)
{
	g_return_val_if_fail(TILEM_IS_DISASM_VIEW(w), FALSE);

	(*GTK_WIDGET_CLASS(parent_class)->button_press_event)(w, event);

	if (event->button == 3)
		show_popup_menu(TILEM_DISASM_VIEW(w), event);

	return TRUE;
}

/* Key pressed to activate context menu */
static gboolean tilem_disasm_view_popup_menu(GtkWidget *w)
{
	g_return_val_if_fail(TILEM_IS_DISASM_VIEW(w), FALSE);
	show_popup_menu(TILEM_DISASM_VIEW(w), NULL);
	return TRUE;
}

/* Row activated (double-clicked) */
static void tilem_disasm_view_row_activated(GtkTreeView *tv, GtkTreePath *path,
                                            GtkTreeViewColumn *col)
{
	TilemDisasmView *dv = TILEM_DISASM_VIEW(tv);
	GtkTreeModel *model;
	GtkTreeIter iter;
	gint pos;

	model = gtk_tree_view_get_model(tv);
	if (!gtk_tree_model_get_iter(model, &iter, path))
		return;

	gtk_tree_model_get(model, &iter, COL_POSITION, &pos, -1);

	if (col == dv->icon_column) {
		if (find_line_bp(dv, pos))
			disable_line_bp(dv, pos);
		else
			enable_line_bp(dv, pos);
	}
}

/* Unrealize widget */
static void tilem_disasm_view_unrealize(GtkWidget *w)
{
	TilemDisasmView *dv = TILEM_DISASM_VIEW(w);

	if (dv->popup_menu)
		gtk_widget_destroy(dv->popup_menu);
	dv->popup_menu = NULL;

	(*GTK_WIDGET_CLASS(parent_class)->unrealize)(w);
}

/* Initialize a new TilemDisasmView */
static void tilem_disasm_view_init(TilemDisasmView *dv)
{
	GtkTreeView *tv = GTK_TREE_VIEW(dv);
	GtkCellRenderer *cell;
	GtkTreeViewColumn *col;

	dv->use_logical = TRUE;

	gtk_tree_view_set_enable_search(tv, FALSE);
	gtk_tree_view_set_fixed_height_mode(tv, TRUE);
	gtk_tree_view_set_headers_visible(tv, FALSE);

	cell = gtk_cell_renderer_pixbuf_new();
	col = gtk_tree_view_column_new_with_attributes(NULL, cell,
	                                               "pixbuf", COL_ICON,
	                                               NULL);
	gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column(tv, col);
	dv->icon_column = col;

	cell = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Addr"), cell,
	                                               "text", COL_ADDRESS,
	                                               NULL);
	gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column(tv, col);

	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, _("Disassembly"));

	cell = gtk_cell_renderer_text_new();
	g_object_set(cell, "xpad", 10, NULL);
	gtk_tree_view_column_pack_start(col, cell, FALSE);
	gtk_tree_view_column_set_attributes(col, cell,
	                                    "text", COL_MNEMONIC,
	                                    "visible", COL_SHOW_MNEMONIC,
	                                    NULL);

	cell = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, cell, TRUE);
	gtk_tree_view_column_set_attributes(col, cell,
	                                    "text", COL_ARGUMENTS,
	                                    NULL);

	gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_expand(col, TRUE);
	gtk_tree_view_append_column(tv, col);
}

static const char default_style[] =
	"style \"tilem-disasm-default\" { font_name = \"Monospace\" } "
	"widget \"*.TilemDisasmView\" style:application \"tilem-disasm-default\"";

/* Initialize the TilemDisasmView class */
static void tilem_disasm_view_class_init(TilemDisasmViewClass *klass)
{
	GtkTreeViewClass *tv_class = GTK_TREE_VIEW_CLASS(klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

	gtk_rc_parse_string(default_style);

	parent_class = g_type_class_peek_parent(klass);

	widget_class->style_set = &tilem_disasm_view_style_set;
	widget_class->size_request = &tilem_disasm_view_size_request;
	widget_class->size_allocate = &tilem_disasm_view_size_allocate;
	widget_class->button_press_event = &tilem_disasm_view_button_press;
	widget_class->popup_menu = &tilem_disasm_view_popup_menu;
	widget_class->unrealize = &tilem_disasm_view_unrealize;
	tv_class->move_cursor = &tilem_disasm_view_move_cursor;
	tv_class->row_activated = &tilem_disasm_view_row_activated;
}

GtkWidget * tilem_disasm_view_new(TilemDebugger *dbg)
{
	TilemDisasmView *dv;

	g_return_val_if_fail(dbg != NULL, NULL);

	dv = g_object_new(TILEM_TYPE_DISASM_VIEW, NULL);
	dv->dbg = dbg;

	return GTK_WIDGET(dv);
}

/* Select memory addressing mode. */
void tilem_disasm_view_set_logical(TilemDisasmView *dv, gboolean logical)
{
	dword start, curpos;
	TilemCalc *calc;

	g_return_if_fail(TILEM_IS_DISASM_VIEW(dv));
	g_return_if_fail(dv->dbg->emu->calc != NULL);

	get_cursor_line(dv, &curpos, NULL);

	if (logical && !dv->use_logical) {
		tilem_calc_emulator_lock(dv->dbg->emu);
		calc = dv->dbg->emu->calc;
		curpos = pos_ptol(calc, curpos);
		start = pos_ptol(calc, dv->startpos);
		tilem_calc_emulator_unlock(dv->dbg->emu);

		dv->use_logical = TRUE;
		refresh_disassembly(dv, start, dv->nlines, curpos);
	}
	else if (!logical && dv->use_logical) {
		tilem_calc_emulator_lock(dv->dbg->emu);
		calc = dv->dbg->emu->calc;
		curpos = pos_ltop(calc, curpos);
		start = pos_ltop(calc, dv->startpos);
		tilem_calc_emulator_unlock(dv->dbg->emu);

		dv->use_logical = FALSE;
		refresh_disassembly(dv, start, dv->nlines, curpos);
	}
}

/* Refresh contents of view. */
void tilem_disasm_view_refresh(TilemDisasmView *dv)
{
	dword curpos;
	g_return_if_fail(TILEM_IS_DISASM_VIEW(dv));
	get_cursor_line(dv, &curpos, NULL);
	refresh_disassembly(dv, dv->startpos, dv->nlines, curpos);
}

/* Find tree path for the given position */
static GtkTreePath *find_path_for_position(GtkTreeModel *model, int pos)
{
	gint p;
	GtkTreeIter iter;

	if (!gtk_tree_model_get_iter_first(model, &iter))
		return NULL;

	do {
		gtk_tree_model_get(model, &iter, COL_POSITION, &p, -1);
		if (p == pos) {
			return gtk_tree_model_get_path(model, &iter);
		}
	} while (gtk_tree_model_iter_next(model, &iter));

	return NULL;
}

/* Highlight the specified address. */
void tilem_disasm_view_go_to_address(TilemDisasmView *dv, dword addr,
                                     gboolean logical)
{
	dword pos;
	GtkTreeModel *model;
	GtkTreePath *path;
	TilemCalc *calc;

	g_return_if_fail(TILEM_IS_DISASM_VIEW(dv));

	tilem_calc_emulator_lock(dv->dbg->emu);
	calc = dv->dbg->emu->calc;

	if (logical) {
		addr &= 0xffff;
		if (dv->use_logical)
			pos = ADDR_TO_POS(addr);
		else
			pos = pos_ltop(calc, ADDR_TO_POS(addr));
	}
	else {
		if (dv->use_logical) {
			addr = (*calc->hw.mem_ptol)(calc, addr);
			if (addr == (dword) -1) {
				tilem_calc_emulator_unlock(dv->dbg->emu);
				return;
			}
		}
		pos = ADDR_TO_POS(addr);
	}

	tilem_calc_emulator_unlock(dv->dbg->emu);

	if (pos >= dv->startpos && pos < dv->endpos) {
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(dv));
		path = find_path_for_position(model, pos);
		if (path) {
			gtk_tree_view_set_cursor(GTK_TREE_VIEW(dv), path,
			                         NULL, FALSE);
			gtk_tree_path_free(path);
			return;
		}
	}

	refresh_disassembly(dv, pos, dv->nlines, pos);
}

/* Get currently selected address. */
gboolean tilem_disasm_view_get_cursor(TilemDisasmView *dv, dword *addr,
                                      gboolean *is_logical)
{
	dword pos;

	g_return_val_if_fail(TILEM_IS_DISASM_VIEW(dv), FALSE);

	if (is_logical) *is_logical = dv->use_logical;

	if (!get_cursor_line(dv, &pos, NULL))
		return FALSE;

	if (addr) *addr = POS_TO_ADDR(pos);
	return TRUE;
}

