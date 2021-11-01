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

G_BEGIN_DECLS

#define TILEM_TYPE_DISASM_VIEW           (tilem_disasm_view_get_type())
#define TILEM_DISASM_VIEW(obj)           (G_TYPE_CHECK_INSTANCE_CAST((obj), TILEM_TYPE_DISASM_VIEW, TilemDisasmView))
#define TILEM_DISASM_VIEW_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST((cls), TILEM_TYPE_DISASM_VIEW, TilemDisasmViewClass))
#define TILEM_IS_DISASM_VIEW(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj), TILEM_TYPE_DISASM_VIEW))
#define TILEM_IS_DISASM_VIEW_CLASS(cls)  (G_TYPE_CHECK_CLASS_TYPE((cls), TILEM_TYPE_DISASM_VIEW))
#define TILEM_DISASM_VIEW_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), TILEM_TYPE_DISASM_VIEW, TilemDisasmViewClass))

typedef struct _TilemDisasmView {
	GtkTreeView parent;

	TilemDebugger *dbg;

	gboolean use_logical;

	int base_height;  /* base height of tree view */
	int line_height;  /* height of each row */

	dword startpos;   /* position at start of window */
	dword endpos;     /* position at end of window */
	int nlines;       /* number of lines visible */

	GtkTreeViewColumn *icon_column;
	GtkWidget *popup_menu;
} TilemDisasmView;

typedef struct _TilemDisasmViewClass {
	GtkTreeViewClass parent_class;
} TilemDisasmViewClass;

GType tilem_disasm_view_get_type(void) G_GNUC_CONST;

/* Create a new TilemDisasmView. */
GtkWidget * tilem_disasm_view_new(TilemDebugger *dbg);

/* Select memory addressing mode. */
void tilem_disasm_view_set_logical(TilemDisasmView *dv, gboolean logical);

/* Refresh contents of view. */
void tilem_disasm_view_refresh(TilemDisasmView *dv);

/* Highlight the specified address. */
void tilem_disasm_view_go_to_address(TilemDisasmView *dv, dword addr,
                                     gboolean logical);

/* Scroll view by a fixed number of bytes. */
void tilem_disasm_view_scroll_bytes(TilemDisasmView *dv, int n);

/* Get currently selected address. */
gboolean tilem_disasm_view_get_cursor(TilemDisasmView *dv, dword *addr,
                                      gboolean *is_logical);

/* Toggle breakpoint at selected address. */
void tilem_disasm_view_toggle_breakpoint(TilemDisasmView *dv);

G_END_DECLS
