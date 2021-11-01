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

#include <gtk/gtk.h>

/* TilemMemModel object: an efficient GtkTreeModel interface for
   viewing and editing calculator's memory */

G_BEGIN_DECLS

/* Column numbers (repeated for each byte in a given row) */
enum {
	MM_COL_ADDRESS_0 = 0,   /* Address (hexadecimal) */
	MM_COL_HEX_0,           /* Byte value (hexadecimal) */
	MM_COL_CHAR_0,          /* Byte value (character, converted to
	                           UTF-8) */
	MM_COL_BYTE_PTR_0,	/* Pointer to corresponding memory byte */
	MM_COL_EDITABLE_0,      /* TRUE if byte is editable */
	MM_COLUMNS_PER_BYTE
};

#define MM_COL_ADDRESS(n) (MM_COL_ADDRESS_0 + (n) * MM_COLUMNS_PER_BYTE)
#define MM_COL_HEX(n) (MM_COL_HEX_0 + (n) * MM_COLUMNS_PER_BYTE)
#define MM_COL_CHAR(n) (MM_COL_CHAR_0 + (n) * MM_COLUMNS_PER_BYTE)
#define MM_COL_BYTE_PTR(n) (MM_COL_BYTE_PTR_0 + (n) * MM_COLUMNS_PER_BYTE)
#define MM_COL_EDITABLE(n) (MM_COL_EDITABLE_0 + (n) * MM_COLUMNS_PER_BYTE)

/* GObject stuff */

#define TILEM_TYPE_MEM_MODEL           (tilem_mem_model_get_type())
#define TILEM_MEM_MODEL(obj)           (G_TYPE_CHECK_INSTANCE_CAST((obj), TILEM_TYPE_MEM_MODEL, TilemMemModel))
#define TILEM_MEM_MODEL_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST((cls), TILEM_TYPE_MEM_MODEL, TilemMemModelClass))
#define TILEM_IS_MEM_MODEL(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj), TILEM_TYPE_MEM_MODEL))
#define TILEM_IS_MEM_MODEL_CLASS(cls)  (G_TYPE_CHECK_CLASS_TYPE((cls), TILEM_TYPE_MEM_MODEL))
#define TILEM_MEM_MODEL_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), TILEM_TYPE_MEM_MODEL, TilemMemModelClass))

typedef struct _TilemMemModel {
	GObject parent;
	TilemCalcEmulator *emu;
	gint stamp;
	int row_size;
	int num_rows;
	dword start_addr;
	dword wrap_addr;
	gboolean use_logical;
	GQueue *cache;
} TilemMemModel;

typedef struct _TilemMemModelClass {
	GObjectClass parent_class;
} TilemMemModelClass;

GType tilem_mem_model_get_type(void) G_GNUC_CONST;

GtkTreeModel * tilem_mem_model_new(TilemCalcEmulator* emu,
                                   int rowsize, dword start,
                                   gboolean logical);

void tilem_mem_model_clear_cache(TilemMemModel *mm);

G_END_DECLS
