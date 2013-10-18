/*
 * TilEm II
 *
 * Copyright (c) 2010-2011 Thibault Duponchelle
 * Copyright (c) 2010-2011 Benjamin Moody
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
#include <errno.h>
#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <ticalcs.h>
#include <tilem.h>
#include <tilemdb.h>

#include "gui.h"
#include "memmodel.h"
#include "fixedtreeview.h"

static int get_column_index(GtkWidget *view, GtkTreeViewColumn *col)
{
	GList *cols;
	int i;

	cols = gtk_tree_view_get_columns(GTK_TREE_VIEW(view));
	i = g_list_index(cols, col);
	g_list_free(cols);
	return i;
}

/* Determine current position in the memory view. */
static void get_mem_view_position(GtkWidget *mem_view, dword *row_addr,
                                  dword *col_addr, gboolean *cur_hex)
{
	GtkTreePath *path;
	GtkTreeViewColumn *col;
	GtkTreeModel *model;
	TilemMemModel *mm;
	const int *indices;
	int n;

	*row_addr = *col_addr = (dword) -1;
	*cur_hex = FALSE;

	gtk_tree_view_get_cursor(GTK_TREE_VIEW(mem_view), &path, &col);
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(mem_view));
	if (!TILEM_IS_MEM_MODEL(model))
		return;

	mm = TILEM_MEM_MODEL(model);

	if (!path)
		return;

	indices = gtk_tree_path_get_indices(path);
	*row_addr = mm->start_addr + indices[0] * mm->row_size;

	n = get_column_index(mem_view, col);
	if (n > 0 && n <= mm->row_size) {
		*col_addr = *row_addr + n - 1;
		*cur_hex = TRUE;
	}
	else if (n > mm->row_size && n < mm->row_size * 2) {
		*col_addr = *row_addr + n - mm->row_size - 1;
		*cur_hex = FALSE;
	}

	gtk_tree_path_free(path);
}

static void addr_to_pos(TilemMemModel *mm, dword addr,
                        int *rownum, int *colnum)
{
	if (addr < mm->start_addr)
		addr += mm->wrap_addr;
	addr -= mm->start_addr;
	if (rownum) *rownum = (addr / mm->row_size);
	if (colnum) *colnum = (addr % mm->row_size);
}

/* Move memory view cursor */
static void set_mem_view_position(GtkWidget *mem_view, dword row_addr,
                                  dword col_addr, gboolean cur_hex)
{
	int rownum, colnum;
	GtkTreePath *path = NULL;
	GtkTreeViewColumn *col = NULL;
	GtkTreeModel *model;
	TilemMemModel *mm;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(mem_view));
	if (!TILEM_IS_MEM_MODEL(model))
		return;

	mm = TILEM_MEM_MODEL(model);

	if (col_addr != (dword) -1) {
		addr_to_pos(mm, col_addr, &rownum, &colnum);
		path = gtk_tree_path_new_from_indices(rownum, -1);

		if (!cur_hex)
			colnum += mm->row_size;

		col = gtk_tree_view_get_column(GTK_TREE_VIEW(mem_view),
		                               colnum + 1);
	}
	else if (row_addr != (dword) -1) {
		addr_to_pos(mm, row_addr, &rownum, NULL);
		path = gtk_tree_path_new_from_indices(rownum, -1);
	}

	if (path) {
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(mem_view),
		                         path, col, FALSE);
		gtk_tree_path_free(path);
	}
}

/* Cell edited in memory view */
static void hex_cell_edited(GtkCellRendererText *renderer,
                            gchar *pathstr, gchar *text,
                            gpointer data)
{
	GtkTreeView *mem_view = data;
	TilemDebugger *dbg;
	GtkTreeModel *model;
	GtkTreePath *path;
	GtkTreeIter iter;
	byte *bptr = NULL;
	int col;
	int value;
	char *end;

	value = strtol(text, &end, 16);
	if (end == text || *end != 0)
		return;

	dbg = g_object_get_data(G_OBJECT(mem_view), "tilem-debugger");
	g_return_if_fail(dbg != NULL);

	col = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(renderer),
	                                        "tilem-mem-column"));

	model = gtk_tree_view_get_model(mem_view);
	path = gtk_tree_path_new_from_string(pathstr);
	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_path_free(path);

	gtk_tree_model_get(model, &iter, MM_COL_BYTE_PTR(col), &bptr, -1);
	g_return_if_fail(bptr != NULL);

	*bptr = (byte) value;
	tilem_debugger_refresh(dbg, TRUE);
}

/* Create the GtkTreeView to show the memory */
GtkWidget *tilem_debugger_mem_view_new(TilemDebugger *dbg)
{
	GtkCellRenderer     *renderer;
	GtkTreeViewColumn   *column;
	GtkWidget           *treeview;

	/* Create the memory list tree view and set title invisible */
	treeview = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), FALSE);
	gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(treeview), TRUE);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(treeview), TRUE);

	g_object_set_data(G_OBJECT(treeview), "tilem-debugger", dbg);

	/* Create the columns */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes
		("ADDR", renderer, "text", MM_COL_ADDRESS(0), NULL);

	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_expand(column, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

	return treeview;
}

static void create_columns(GtkWidget *mem_view, int width)
{
	GtkCellRenderer     *renderer;
	GtkTreeViewColumn   *column;
	int i;

	for (i = 0; i < width; i++) {
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes
			(NULL, renderer,
			 "text", MM_COL_HEX(i),
			 "editable", MM_COL_EDITABLE(i),
			 NULL);

		g_object_set_data(G_OBJECT(renderer), "tilem-mem-column",
		                  GINT_TO_POINTER(i));
		g_signal_connect(renderer, "edited",
		                 G_CALLBACK(hex_cell_edited), mem_view);

		gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
		gtk_tree_view_column_set_expand(column, (i == width - 1));
		gtk_tree_view_append_column(GTK_TREE_VIEW(mem_view), column);
	}

	for (i = 0; i < width; i++) {
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes
			(NULL, renderer, "text", MM_COL_CHAR(i), NULL);

		gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
		gtk_tree_view_column_set_expand(column, (i == width - 1));
		gtk_tree_view_append_column(GTK_TREE_VIEW(mem_view), column);
	}
}

static dword translate_addr(TilemCalcEmulator *emu, dword a, gboolean ptol)
{
	if (!emu->calc || a == (dword) -1)
		return a;
	if (ptol)
		return (*emu->calc->hw.mem_ptol)(emu->calc, a);
	else
		return (*emu->calc->hw.mem_ltop)(emu->calc, a & 0xffff);
}

void tilem_debugger_mem_view_configure(GtkWidget *mem_view,
                                       TilemCalcEmulator *emu,
                                       int rowsize, int start,
                                       gboolean logical)
{
	GtkTreeModel *model;
	dword row_addr, col_addr;
	gboolean cur_hex;
	GList *cols, *l;
	int old_rowsize;

	get_mem_view_position(mem_view, &row_addr, &col_addr, &cur_hex);

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(mem_view));
	if (TILEM_IS_MEM_MODEL(model)
	    && TILEM_MEM_MODEL(model)->use_logical != logical) {
		tilem_calc_emulator_lock(emu);
		row_addr = translate_addr(emu, row_addr, logical);
		col_addr = translate_addr(emu, col_addr, logical);
		tilem_calc_emulator_unlock(emu);
	}

	cols = gtk_tree_view_get_columns(GTK_TREE_VIEW(mem_view));
	old_rowsize = (g_list_length(cols) - 1) / 2;
	if (old_rowsize != rowsize)
		for (l = g_list_next(cols); l; l = l->next)
			gtk_tree_view_remove_column(GTK_TREE_VIEW(mem_view),
			                            l->data);
	g_list_free(cols);

	model = tilem_mem_model_new(emu, rowsize, start, logical);
	gtk_tree_view_set_model(GTK_TREE_VIEW(mem_view), model);
	g_object_unref(model);

	if (old_rowsize != rowsize)
		create_columns(mem_view, rowsize);

	fixed_tree_view_init(mem_view, MM_COLUMNS_PER_BYTE,
	                     MM_COL_ADDRESS_0, "DD:DDDD ",
	                     MM_COL_HEX_0, "DD ",
	                     MM_COL_CHAR_0, "M ",
	                     MM_COL_EDITABLE_0, TRUE,
	                     -1);

	set_mem_view_position(mem_view, row_addr, col_addr, cur_hex);
}
