/* Hey EMACS -*- linux-c -*- */
/* $Id: dbg_iop.c 2818 2009-05-02 19:46:04Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *Copyright (c) 2001-2003, Romain Lievin
 *Copyright (c) 2003, Julien Blache
 *Copyright (c) 2004, Romain Liévin
 *Copyright (c) 2005, Romain Liévin
 *
 *This program is free software; you can redistribute it and/or modify
 *it under the terms of the GNU General Public License as published by
 *the Free Software Foundation; either version 2 of the License, or
 *(at your option) any later version.
 *
 *This program is distributed in the hope that it will be useful,
 *but WITHOUT ANY WARRANTY; without even the implied warranty of
 *MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *GNU General Public License for more details.
 *
 *You should have received a copy of the GNU General Public License
 *along with this program; if not, write to the Free Software
 *Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <tiemuconfig.h>
#endif

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "intl.h"
#include "paths.h"
#include "support.h"
#include "ti68k_int.h"
#include "struct.h"
#include "dbg_wnds.h"
#include "iodefs.h"

enum 
{
	    COL_NAME, COL_VALUE, COL_ADDR, COL_MASK, 
		COL_FONT, COL_EDIT, COL_S, COL_BTNVIS, COL_BTNACT,
};
#define CTREE_NVCOLS	(4)		// 2 visible columns
#define CTREE_NCOLS		(9)		// 5 real columns

// return value as string
static char* rd_mem_as_str(IO_DEF *t)
{
	switch(t->size)
	{
		case 1: return g_strdup_printf("%02x", mem_rd_byte(t->addr)); break;
		case 2: return g_strdup_printf("%04x", mem_rd_word(t->addr)); break;
		case 4: return g_strdup_printf("%08x", mem_rd_long(t->addr)); break;
		default: return g_strdup("???"); break;
	}
	return g_strdup("");
}

static int rd_bit(IO_DEF *s, int bit_num)
{
	switch(s->size)
	{
	case 1: return mem_rd_byte(s->addr) & (1 << bit_num);
	break;
	case 2: return mem_rd_word(s->addr) & (1 << bit_num);
	break;
	case 4: return mem_rd_long(s->addr) & (1 << bit_num);
	break;
	}

	return -1;
}

// check for valid hexadecimal value
static int validate_value(const char *str, int ndigits)
{
	int i;
	
	if((int)strlen(str) > ndigits)
	 	return 0;
	
	for(i = 0; (i < ndigits) && (i < (int)strlen(str)); i++)
	{
		if(!isxdigit(str[i]))
			return 0;
	}
	
	return !0;
}

// called when cell has been edited
static void renderer_edited(GtkCellRendererText *cell,
			    const gchar *path_string,
			    const gchar *new_text, gpointer user_data)
{
	GtkWidget *tree = user_data;
	GtkTreeView *view = GTK_TREE_VIEW(tree);
	GtkTreeModel *model = gtk_tree_view_get_model(view);
	GtkTreeStore *store = GTK_TREE_STORE(model);

	GtkTreePath *path = gtk_tree_path_new_from_string(path_string);
	GtkTreeIter iter, child;
	
	IO_DEF *s;
	gchar *str;
	uint32_t value;
	gboolean valid;

	if (!gtk_tree_model_get_iter(model, &iter, path))
		return;
		
	gtk_tree_model_get(model, &iter, COL_VALUE, &str, COL_S, &s, -1);
	sscanf(str, "%x", &value);
	g_free(str);

	// change value in memory
	if(validate_value(new_text, 2 *s->size))
	{
		sscanf(new_text, "%x", &value);			

		switch(s->size)
		{
		case 1: mem_wr_byte(s->addr, (uint8_t )value); break;
		case 2: mem_wr_word(s->addr, (uint16_t)value); break;
		case 4: mem_wr_long(s->addr, (uint32_t)value); break;
		default: break;
		}
	}

	// and change displayed value (don't rely on typed value !)
	str = rd_mem_as_str(s);
	gtk_tree_store_set(store, &iter, COL_VALUE, str, -1);
	g_free(str);

	// update bits (children nodes)
	for(valid = gtk_tree_model_iter_children(model, &child, &iter);
        valid; 
        valid = gtk_tree_model_iter_next(model, &child))
    {
		gchar* bit_adr;
		int	   bit_num;

		gtk_tree_model_get(model, &child, COL_NAME, &str, COL_ADDR, &bit_adr, -1);
		sscanf(bit_adr, "%i", &bit_num);
		gtk_tree_store_set(store, &child, COL_BTNACT, rd_bit(s, bit_num), -1);
	}
	
	gtk_tree_path_free(path);
}

// called when a check button has been toggled
static void renderer_toggled(GtkCellRendererToggle *cell,
			     gchar *path_string, gpointer user_data)
{
	GtkWidget *tree = user_data;
	GtkTreeView *view = GTK_TREE_VIEW(tree);
	GtkTreeModel *model = gtk_tree_view_get_model(view);
	GtkTreeStore *store = GTK_TREE_STORE(model);

	GtkTreePath *path;
	GtkTreeIter parent, iter;
	IO_DEF *s;
	gboolean state, result;
	gchar* bit_str;
	gint bit_num;
	gchar* str;

	path = gtk_tree_path_new_from_string(path_string);

	if (!gtk_tree_model_get_iter(model, &iter, path))
		return;
	if (!gtk_tree_model_get_iter(model, &iter, path))
		return;
		
	gtk_tree_model_get(model, &iter, 
		COL_BTNACT, &state, 
		COL_S, &s, 
		COL_ADDR, &bit_str, 
		-1);

	// change button state
	state = !state;
	sscanf(bit_str, "%i", &bit_num);

	// change value in memory
	switch(s->size)
	{
	case 1:
		if(state)
			mem_wr_byte(s->addr, (uint8_t)(mem_rd_byte(s->addr) | (1 << bit_num)));
		else
			mem_wr_byte(s->addr, (uint8_t)(mem_rd_byte(s->addr) & ~(1 << bit_num)));
		break;
	case 2:
		if(state)
			mem_wr_word(s->addr, (uint16_t)(mem_rd_word(s->addr) | (1 << bit_num)));
		else
			mem_wr_word(s->addr, (uint16_t)(mem_rd_word(s->addr) & ~(1 << bit_num)));
		break;
	case 4:
		if(state)
			mem_wr_long(s->addr, mem_rd_long(s->addr) | (1 << bit_num));
		else
			mem_wr_long(s->addr, mem_rd_long(s->addr) & ~(1 << bit_num));
		break;
	}

	// and change displayed value (parent node)
	gtk_tree_store_set(store, &iter, COL_BTNACT, state, -1);
	g_free(bit_str);

	result = gtk_tree_model_iter_parent(model, &parent, &iter);
	if(result)
	{
		str = rd_mem_as_str(s);
		gtk_tree_store_set(store, &parent, COL_VALUE, str, -1);
		g_free(str);
	}
}

static gboolean select_func(GtkTreeSelection *selection,
			    GtkTreeModel *model,
			    GtkTreePath *path,
			    gboolean path_currently_selected,
			    gpointer data)
{
	return TRUE;
}

static GtkTreeStore* ctree_create(GtkWidget *widget)
{
	GtkTreeView *view = GTK_TREE_VIEW(widget);
	GtkTreeStore *store;
	GtkTreeModel *model;
	GtkCellRenderer *renderer;
	GtkTreeSelection *selection;
	GtkTreeViewColumn *column;
    gint i;
	
	store = gtk_tree_store_new(CTREE_NCOLS,
				G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
				G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_POINTER, 
				G_TYPE_BOOLEAN, G_TYPE_BOOLEAN,
				-1
            );
    model = GTK_TREE_MODEL(store);
	
    gtk_tree_view_set_model(view, model); 
    gtk_tree_view_set_headers_visible(view, TRUE);
	gtk_tree_view_set_rules_hint(view, TRUE);
  
	// col 1
	renderer = gtk_cell_renderer_text_new();
	set_renderer_pad(renderer);
	gtk_tree_view_insert_column_with_attributes(view, -1, 
            _("Name"), renderer, 
            "text", COL_NAME,
			"font", COL_FONT,
			NULL);

	// col 2
	column = gtk_tree_view_column_new();
	gtk_tree_view_append_column(view, column);
	gtk_tree_view_column_set_title(column, _("Value"));

	renderer = gtk_cell_renderer_toggle_new();
	set_renderer_pad(renderer);
	gtk_tree_view_column_pack_start(GTK_TREE_VIEW_COLUMN(column), renderer, FALSE);
	gtk_tree_view_column_set_attributes(GTK_TREE_VIEW_COLUMN(column),
					    renderer,
					    "active", COL_BTNACT,
					    "visible", COL_BTNVIS, 
						NULL);
	g_signal_connect(G_OBJECT(renderer), "toggled", G_CALLBACK(renderer_toggled), widget);

	renderer = gtk_cell_renderer_text_new();
	set_renderer_pad(renderer);
	gtk_tree_view_column_pack_start(GTK_TREE_VIEW_COLUMN(column), renderer, FALSE);	
	gtk_tree_view_column_set_attributes(GTK_TREE_VIEW_COLUMN(column),
					    renderer,
					    "text", COL_VALUE,
						"editable", COL_EDIT,
						"font", COL_FONT, 
						NULL);	
	g_signal_connect(G_OBJECT(renderer), "edited", G_CALLBACK(renderer_edited), widget);

	// col 3
	renderer = gtk_cell_renderer_text_new();
	set_renderer_pad(renderer);
	gtk_tree_view_insert_column_with_attributes(view, -1, 
            _("Address"), renderer, 
            "text", COL_ADDR,
			"font", COL_FONT,
			NULL);

	// col 4
	renderer = gtk_cell_renderer_text_new();
	set_renderer_pad(renderer);
	gtk_tree_view_insert_column_with_attributes(view, -1, 
            _("Mask"), renderer, 
            "text", COL_MASK,
			"font", COL_FONT,
			NULL);
    
    for (i = 0; i < CTREE_NVCOLS; i++) 
    {
		GtkTreeViewColumn *col;
		
		col = gtk_tree_view_get_column(view, i);
		gtk_tree_view_column_set_resizable(col, TRUE);
	}
	
	selection = gtk_tree_view_get_selection(view);
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
	gtk_tree_selection_set_select_function(selection, select_func, NULL, NULL);

	return store;
}

static void ctree_populate(GtkTreeStore *store)
{
	GNode* node0;
	GtkTreeIter iter0, iter1, iter2;
	int i, j, k;
	int result;

	// (re)load I/O ports
	result = ti68k_debug_load_iodefs(inst_paths.misc_dir);
	if(result == -1)
	{
		gtk_tree_store_clear(store);
		return;				// already loaded
	}

	node0 = iodefs_tree();
	if(node0 == NULL)
		return;

	// parse sections
	for (i = 0; i < (int)g_node_n_children(node0); i++) 
	{
		GNode *node1 = g_node_nth_child(node0, i);
		IO_DEF *s = (IO_DEF *)(node1->data);

		gtk_tree_store_append(store, &iter0, NULL);
		gtk_tree_store_set(store, &iter0, COL_NAME, s->name, -1);
		if(options3.dbg_font_type)
			gtk_tree_store_set(store, &iter0, COL_FONT, options3.dbg_font_name, -1);

		// parse registers
		for (j = 0; j < (int)g_node_n_children(node1); j++) 
		{
			GNode *node2 = g_node_nth_child(node1, j);
			IO_DEF *t = (IO_DEF *)(node2->data);

			gchar **row_text = g_malloc0((CTREE_NCOLS + 1) *sizeof(gchar *));

			row_text[0] = g_strdup(t->name);
			row_text[1] = rd_mem_as_str(t);
			row_text[2] = g_strdup_printf("%06x", t->addr);
			row_text[3] = g_strdup(t->all_bits ? "" : t->bit_str);

			gtk_tree_store_append(store, &iter1, &iter0);
			gtk_tree_store_set(store, &iter1, 
					   COL_NAME, row_text[0],
					   COL_VALUE, row_text[1], 
					   COL_ADDR,  row_text[2], 
					   COL_MASK,  row_text[3],
					   COL_S, (gpointer)t,
					   COL_EDIT, TRUE,
					   COL_BTNVIS, FALSE,
					   COL_BTNACT, FALSE,
					   -1);

			if(options3.dbg_font_type)
				gtk_tree_store_set(store, &iter1, COL_FONT, options3.dbg_font_name, -1);

			g_strfreev(row_text);

			// parse bits
			for(k = t->nbits-1; k >= 0 ; k--)
			{
				row_text = g_malloc0((CTREE_NCOLS + 1) *sizeof(gchar *));

				row_text[0] = g_strdup(t->bit_name[k]);
				row_text[2] = g_strdup_printf("%i", t->bits[k]);
				gtk_tree_store_append(store, &iter2, &iter1);
				gtk_tree_store_set(store, &iter2, 
					COL_NAME, row_text[0],
					COL_ADDR, row_text[2], 
					COL_S, (gpointer)t, 
					COL_EDIT, FALSE,
					COL_BTNVIS, TRUE,
					COL_BTNACT, rd_bit(t, t->bits[k]),					
					-1);
			}
		}
	}
}

static void ctree_refresh(GtkTreeStore *store)
{
	gtk_tree_store_clear(store);
	ctree_populate(store);
}

static GtkTreeStore *store;

/*
	Display io ports window
*/
GtkWidget* dbgiop_create_window(void)
{
	GladeXML *xml = NULL;
	GtkWidget *dbox;
    GtkWidget *data;	
	
	xml = glade_xml_new
		(tilp_paths_build_glade("dbg_ioports-2.glade"), "dbgioports_window",
		 PACKAGE);
	if (!xml)
		g_error(_("%s: GUI loading failed!\n"), __FILE__);
	glade_xml_signal_autoconnect(xml);
	
	dbox = glade_xml_get_widget(xml, "dbgioports_window");
	if(options3.transient)
		gtk_window_set_transient_for(GTK_WINDOW(dbox), GTK_WINDOW(main_wnd));

	data = glade_xml_get_widget(xml, "treeview1");
    store = ctree_create(data);
	ctree_populate(store);

	gtk_tree_view_collapse_all(GTK_TREE_VIEW(data));

	return dbox;
}

GtkWidget* dbgiop_display_window(void)
{
#ifdef WND_STATE
	if(!options3.iop.minimized)
	{
		gtk_window_resize(GTK_WINDOW(dbgw.iop), options3.iop.rect.w, options3.iop.rect.h);
		gtk_window_move(GTK_WINDOW(dbgw.iop), options3.iop.rect.x, options3.iop.rect.y);
	}
	else
		gtk_window_iconify(GTK_WINDOW(dbgw.iop));
#endif
    
	if(!GTK_WIDGET_VISIBLE(dbgw.iop) && !options3.iop.closed)
		gtk_widget_show(dbgw.iop);

	return dbgw.iop;
}

void dbgiop_refresh_window(void)
{
	WND_TMR_START();

	if(!options3.iop.closed)
	{
		ctree_refresh(store);
	}

	WND_TMR_STOP("Iop Refresh Time");
}
