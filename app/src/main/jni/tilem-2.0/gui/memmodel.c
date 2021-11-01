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

#include "gui.h"
#include "memmodel.h"
#include "charmap.h"

/* GTK+ requires us to supply several property values for every byte,
   and we might have hundreds of bytes that need to be refreshed each
   time the window is repainted.  To avoid locking and unlocking the
   calc for every call to tilem_mem_model_get_value(), we can retrieve
   information for an entire block of memory and keep it in cache.

   For each address, we cache the current byte value (8 bits), whether
   or not it is editable (1 bit), and its physical address (up to 22
   bits for current calculator models), so everything fits in a
   guint32.  For future models, this scheme might need to be
   modified. */

#define CACHE_BLOCK_SIZE 256
#define CACHE_NUM_BLOCKS 16

typedef struct {
	dword address;
	guint32 *info;
} MemModelCacheBlock;

/* Check if a given physical address is editable (i.e., located in RAM
   or in a non-protected Flash sector) */
static gboolean address_editable(TilemCalc *calc, dword a)
{
	int start, end, i;

	if (a >= calc->hw.romsize)
		/* address is in RAM */
		return TRUE;

	if (!(calc->hw.flags & TILEM_CALC_HAS_FLASH))
		/* calc does not use Flash */
		return FALSE;

	/* address is in Flash -> check if sector is protected */
	start = 0;
	end = calc->hw.nflashsectors;
	while (start < end) {
		i = (start + end) / 2;
		if (a < calc->hw.flashsectors[i].start)
			end = i;
		else if (a >= (calc->hw.flashsectors[i].start
		               + calc->hw.flashsectors[i].size))
			start = i + 1;
		else
			return !(calc->hw.flashsectors[i].protectgroup
			         & ~calc->flash.overridegroup);
	}

	g_return_val_if_reached(FALSE);
}

/* Copy calc memory contents into cache. */
static void fill_cache_block(TilemMemModel *mm, MemModelCacheBlock *cb)
{
	TilemCalc *calc;
	dword i, addr, phys;
	byte value, editable;

	g_return_if_fail(mm->emu != NULL);

	tilem_calc_emulator_lock(mm->emu);
	calc = mm->emu->calc;
	if (!calc) {
		tilem_calc_emulator_unlock(mm->emu);
		return;
	}

	for (i = 0; i < CACHE_BLOCK_SIZE; i++) {
		addr = (cb->address + i) % mm->wrap_addr;

		if (mm->use_logical)
			phys = (*calc->hw.mem_ltop)(calc, addr);
		else
			phys = addr;

		editable = address_editable(calc, phys);
		value = calc->mem[phys];

		cb->info[i] = (value
		               | (editable << 8)
		               | (phys << 9));
	}

	tilem_calc_emulator_unlock(mm->emu);
}

/* Retrieve info for given address. */
static guint32 get_mem_info(TilemMemModel *mm, dword addr)
{
	GList *l;
	MemModelCacheBlock *cb;
	dword start, index;

	start = addr & ~(CACHE_BLOCK_SIZE - 1);
	index = addr & (CACHE_BLOCK_SIZE - 1);

	for (l = mm->cache->head; l; l = l->next) {
		cb = l->data;
		if (cb->address == start) {
			if (l->prev) {
				/* Move this cache block to the start
				   of the list */
				g_queue_unlink(mm->cache, l);
				g_queue_push_head_link(mm->cache, l);
			}

			return cb->info[index];
		}
	}

	/* Data not found in cache; drop the least recently used block
	   and retrieve the requested block from the calc */
	l = g_queue_pop_tail_link(mm->cache);
	g_queue_push_head_link(mm->cache, l);
	cb = l->data;
	cb->address = start;
	fill_cache_block(mm, cb);
	return cb->info[index];
}

/* Get address's byte value. */
static byte get_value(TilemMemModel *mm, dword addr)
{
	return (get_mem_info(mm, addr) & 0xff);
}

/* Get address's editability. */
static gboolean get_editable(TilemMemModel *mm, dword addr)
{
	return ((get_mem_info(mm, addr) >> 8) & 1);
}

/* Get address's corresponding physical address. */
static dword get_phys_addr(TilemMemModel *mm, dword addr)
{
	return (get_mem_info(mm, addr) >> 9);
}

/* Clear cache.  This function should be called any time something
   happens that might affect memory contents. */
void tilem_mem_model_clear_cache(TilemMemModel *mm)
{
	GList *l;
	MemModelCacheBlock *cb;

	g_return_if_fail(TILEM_IS_MEM_MODEL(mm));

	for (l = mm->cache->head; l; l = l->next) {
		cb = l->data;
		cb->address = (dword) -1;
	}
}

/* Get flags for the model */
static GtkTreeModelFlags
tilem_mem_model_get_flags(G_GNUC_UNUSED GtkTreeModel *model)
{
	return (GTK_TREE_MODEL_LIST_ONLY | GTK_TREE_MODEL_ITERS_PERSIST);
}

/* Get the number of columns */
static int
tilem_mem_model_get_n_columns(GtkTreeModel *model)
{
	TilemMemModel *mm;
	g_return_val_if_fail(TILEM_IS_MEM_MODEL(model), 0);
	mm = TILEM_MEM_MODEL(model);
	return (MM_COLUMNS_PER_BYTE * mm->row_size);
}

/* Get type of data for the given column.  Currently all columns are
   strings. */
static GType
tilem_mem_model_get_column_type(G_GNUC_UNUSED GtkTreeModel *model,
                                int index)
{
	index %= MM_COLUMNS_PER_BYTE;

	switch (index) {
	case MM_COL_ADDRESS_0:
	case MM_COL_HEX_0:
	case MM_COL_CHAR_0:
		return G_TYPE_STRING;

	case MM_COL_BYTE_PTR_0:
		return G_TYPE_POINTER;

	case MM_COL_EDITABLE_0:
		return G_TYPE_BOOLEAN;

	default:
		g_return_val_if_reached(G_TYPE_INVALID);
	}
}

/* Get an iterator pointing to the nth row */
static gboolean get_nth_iter(GtkTreeModel *model, GtkTreeIter *iter, int n)
{
	TilemMemModel *mm;

	g_return_val_if_fail(TILEM_IS_MEM_MODEL(model), FALSE);
	mm = TILEM_MEM_MODEL(model);

	if (n >= mm->num_rows)
		return FALSE;

	iter->stamp = mm->stamp;
	iter->user_data = GINT_TO_POINTER(n);
	iter->user_data2 = NULL;
	iter->user_data3 = NULL;
	return TRUE;
}

/* Get row number for the given iterator */
static int get_row_number(GtkTreeModel *model, GtkTreeIter *iter)
{
	TilemMemModel *mm;
	int n;

	g_return_val_if_fail(TILEM_IS_MEM_MODEL(model), 0);
	mm = TILEM_MEM_MODEL(model);
	g_return_val_if_fail(iter != NULL, 0);
	g_return_val_if_fail(iter->stamp == mm->stamp, 0);
	n = GPOINTER_TO_INT(iter->user_data);
	g_return_val_if_fail(n < mm->num_rows, 0);
	return n;
}

/* Get iterator for a given path */
static gboolean tilem_mem_model_get_iter(GtkTreeModel *model,
                                         GtkTreeIter *iter,
                                         GtkTreePath *path)
{
	int *indices;

	if (gtk_tree_path_get_depth(path) != 1)
		return FALSE;

	indices = gtk_tree_path_get_indices(path);
	return get_nth_iter(model, iter, indices[0]);
}

/* Get path for an iterator */
static GtkTreePath * tilem_mem_model_get_path(GtkTreeModel *model,
                                              GtkTreeIter *iter)
{
	int n;
	n = get_row_number(model, iter);
	return gtk_tree_path_new_from_indices(n, -1);
}

/* Get next (sibling) iterator */
static gboolean tilem_mem_model_iter_next(GtkTreeModel *model,
                                          GtkTreeIter *iter)
{
	int n;
	n = get_row_number(model, iter);
	return get_nth_iter(model, iter, n + 1);
}

/* Check if iterator has a child */
static gboolean
tilem_mem_model_iter_has_child(G_GNUC_UNUSED GtkTreeModel *model,
			       G_GNUC_UNUSED GtkTreeIter *iter)
{
	return FALSE;
}

/* Get number of children (iter = NULL means get number of root
   nodes) */
static gint tilem_mem_model_iter_n_children(GtkTreeModel *model,
                                            GtkTreeIter *iter)
{
	TilemMemModel *mm;

	g_return_val_if_fail(TILEM_IS_MEM_MODEL(model), 0);
	mm = TILEM_MEM_MODEL(model);

	if (iter)
		return 0;
	else
		return (mm->num_rows);
}

/* Get nth child (parent = NULL means get nth root node */
static gboolean tilem_mem_model_iter_nth_child( GtkTreeModel *model,
                                               GtkTreeIter *iter,
                                               GtkTreeIter *parent,
                                               gint n)
{
	G_GNUC_UNUSED TilemMemModel* mm;

	g_return_val_if_fail(TILEM_IS_MEM_MODEL(model), FALSE);
	mm = TILEM_MEM_MODEL(model);

	if (parent)
		return FALSE;
	else
		return get_nth_iter(model, iter, n);
}

/* Get first child */
static gboolean tilem_mem_model_iter_children(GtkTreeModel *model,
                                              GtkTreeIter *iter,
                                              GtkTreeIter *parent)
{
	return tilem_mem_model_iter_nth_child(model, iter, parent, 0);
}

/* Get parent */
static gboolean tilem_mem_model_iter_parent(G_GNUC_UNUSED GtkTreeModel *model,
                                            G_GNUC_UNUSED GtkTreeIter *iter,
                                            G_GNUC_UNUSED GtkTreeIter *child)
{
	return FALSE;
}

/* Retrieve value for a given column */
static void tilem_mem_model_get_value(GtkTreeModel *model,
                                      GtkTreeIter *iter,
                                      gint column,
                                      GValue *value)
{
	TilemMemModel *mm;
	dword n, addr, phys;
	TilemCalc *calc;
	char buf[100], *s;

	g_return_if_fail(TILEM_IS_MEM_MODEL(model));
	mm = TILEM_MEM_MODEL(model);

	g_return_if_fail(mm->emu != NULL);
	g_return_if_fail(mm->emu->calc != NULL);

	n = get_row_number(model, iter);

	calc = mm->emu->calc;

	addr = (mm->start_addr
	        + n * mm->row_size
	        + column / MM_COLUMNS_PER_BYTE) % mm->wrap_addr;

	column %= MM_COLUMNS_PER_BYTE;

	switch (column) {
	case MM_COL_ADDRESS_0:
		s = tilem_format_addr(mm->emu->dbg, addr, !mm->use_logical);
		g_value_init(value, G_TYPE_STRING);
		g_value_set_string(value, s);
		g_free(s);
		break;

	case MM_COL_HEX_0:
		g_snprintf(buf, sizeof(buf), "%02X", get_value(mm, addr));
		g_value_init(value, G_TYPE_STRING);
		g_value_set_string(value, buf);
		break;

	case MM_COL_CHAR_0:
		s = ti_to_unicode(calc->hw.model_id, get_value(mm, addr));
		g_value_init(value, G_TYPE_STRING);
		g_value_set_string(value, s);
		g_free(s);
		break;

	case MM_COL_BYTE_PTR_0:
		phys = get_phys_addr(mm, addr);
		g_value_init(value, G_TYPE_POINTER);
		g_value_set_pointer(value, &calc->mem[phys]);
		break;

	case MM_COL_EDITABLE_0:
		g_value_init(value, G_TYPE_BOOLEAN);
		g_value_set_boolean(value, get_editable(mm, addr));
		break;
	}
}

static void tilem_mem_model_init(TilemMemModel *mm)
{
	int i;
	MemModelCacheBlock *cb;

	mm->stamp = g_random_int();
	mm->row_size = 1;

	mm->cache = g_queue_new();
	for (i = 0; i < CACHE_NUM_BLOCKS; i++) {
		cb = g_slice_new(MemModelCacheBlock);
		cb->address = (dword) -1;
		cb->info = g_new(guint32, CACHE_BLOCK_SIZE);
		g_queue_push_head(mm->cache, cb);
	}
}

static void tilem_mem_model_finalize(GObject *obj)
{
	TilemMemModel *mm;
	MemModelCacheBlock *cb;

	g_return_if_fail(TILEM_IS_MEM_MODEL(obj));
	mm = TILEM_MEM_MODEL(obj);	                 

	while ((cb = g_queue_pop_head(mm->cache))) {
		g_free(cb->info);
		g_slice_free(MemModelCacheBlock, cb);
	}
}

static void tilem_mem_model_class_init(TilemMemModelClass *klass)
{
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	obj_class->finalize = &tilem_mem_model_finalize;
}

static void tilem_mem_tree_model_init(GtkTreeModelIface *iface)
{
	iface->get_flags = &tilem_mem_model_get_flags;
	iface->get_n_columns = &tilem_mem_model_get_n_columns;
	iface->get_column_type = &tilem_mem_model_get_column_type;
	iface->get_iter = &tilem_mem_model_get_iter;
	iface->get_path = &tilem_mem_model_get_path;
	iface->get_value = &tilem_mem_model_get_value;
	iface->iter_next = &tilem_mem_model_iter_next;
	iface->iter_children = &tilem_mem_model_iter_children;
	iface->iter_has_child = &tilem_mem_model_iter_has_child;
	iface->iter_n_children = &tilem_mem_model_iter_n_children;
	iface->iter_nth_child = &tilem_mem_model_iter_nth_child;
	iface->iter_parent = &tilem_mem_model_iter_parent;
}

GType tilem_mem_model_get_type(void)
{
	static GType type = 0;

	static const GTypeInfo type_info = {
		sizeof(TilemMemModelClass),
		NULL,
		NULL,
		(GClassInitFunc) tilem_mem_model_class_init,
		NULL,
		NULL,
		sizeof(TilemMemModel),
		0,
		(GInstanceInitFunc) tilem_mem_model_init,
		NULL
	};

	static const GInterfaceInfo tree_model_info = {
		(GInterfaceInitFunc) tilem_mem_tree_model_init,
		NULL,
		NULL
	};

	if (!type) {
		type = g_type_register_static(G_TYPE_OBJECT, "TilemMemModel",
					      &type_info, 0);
		g_type_add_interface_static(type, GTK_TYPE_TREE_MODEL,
					    &tree_model_info);
	}

	return type;
}

GtkTreeModel * tilem_mem_model_new(TilemCalcEmulator* emu,
                                   int rowsize, dword start,
                                   gboolean logical)
{
	TilemMemModel* mm;

	g_return_val_if_fail(emu != NULL, NULL);
	g_return_val_if_fail(emu->calc != NULL, NULL);
	g_return_val_if_fail(rowsize > 0, NULL);

	mm = g_object_new(TILEM_TYPE_MEM_MODEL, NULL);

	mm->emu = emu;
	mm->row_size = rowsize;
	mm->start_addr = start;

	if (logical) {
		mm->use_logical = TRUE;
		mm->wrap_addr = 0x10000;
	}
	else {
		mm->use_logical = FALSE;
		mm->wrap_addr = (emu->calc->hw.romsize
		                 + emu->calc->hw.ramsize);
	}

	mm->num_rows = (mm->wrap_addr + rowsize - 1) / rowsize;

	return GTK_TREE_MODEL(mm);
}
