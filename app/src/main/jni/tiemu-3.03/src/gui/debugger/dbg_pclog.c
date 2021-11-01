/* Hey EMACS -*- linux-c -*- */
/* $Id: dbg_pclog.c 2818 2009-05-02 19:46:04Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Liévin
 *  Copyright (c) 2005, Romain Liévin
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <tiemuconfig.h>
#endif

#include <gtk/gtk.h>
#include <glade/glade.h>

#include "intl.h"
#include "paths.h"
#include "support.h"
#include "ti68k_int.h"
#include "struct.h"
#include "dbg_wnds.h"

enum { 
	    COL_ADDR, COL_FONT
};
#define CLIST_NVCOLS	(1)		// 1 visible columns
#define CLIST_NCOLS		(2)		// 1 real columns

static GtkListStore* clist_create(GtkWidget *widget)
{
	GtkTreeView *view = GTK_TREE_VIEW(widget);
	GtkListStore *store;
	GtkTreeModel *model;
	GtkCellRenderer *renderer;
	GtkTreeSelection *selection;
    const gchar *text[CLIST_NVCOLS] = { _("Address") };
    gint i;
	
	store = gtk_list_store_new(CLIST_NCOLS,
				G_TYPE_STRING, G_TYPE_STRING,
				-1
            );
    model = GTK_TREE_MODEL(store);
	
    gtk_tree_view_set_model(view, model); 
    gtk_tree_view_set_headers_visible(view, TRUE);
	gtk_tree_view_set_rules_hint(view, FALSE);
  
	for(i = COL_ADDR; i <= COL_ADDR; i++)
	{
		renderer = gtk_cell_renderer_text_new();
		set_renderer_pad(renderer);
		gtk_tree_view_insert_column_with_attributes(view, -1, 
            text[i], renderer, 
			"font", COL_FONT,
            "text", i,
			NULL);
	}
    
    for (i = 0; i < CLIST_NVCOLS; i++) 
    {
		GtkTreeViewColumn *col;
		
		col = gtk_tree_view_get_column(view, i);
		gtk_tree_view_column_set_resizable(col, TRUE);
	}
	
	selection = gtk_tree_view_get_selection(view);
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);

	return store;
}

static void clist_populate(GtkListStore *store)
{
    int i;

    for(i = 0; i < logger.pclog_size; i++)
    {
        GtkTreeIter iter;
        uint32_t addr;
        gchar *str;

        addr = logger.pclog_buf[(logger.pclog_ptr + i) % logger.pclog_size];
        str = g_strdup_printf("%06x", addr);
    
        gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 
		COL_ADDR, str, 
		-1);

		if(options3.dbg_font_type)
			gtk_list_store_set(store, &iter, COL_FONT, options3.dbg_font_name, -1);
		
		g_free(str);
    }
}

static void clist_refresh(GtkListStore *store)
{
	gtk_list_store_clear(store);
	clist_populate(store);
}

static GtkListStore *store = NULL;

/*
	Display registers window
*/
GtkWidget* dbgpclog_create_window(void)
{
	GladeXML *xml = NULL;
	GtkWidget *dbox;
    GtkWidget *data;
	
	xml = glade_xml_new
		(tilp_paths_build_glade("dbg_pclog-2.glade"), "dbgpclog_window",
		 PACKAGE);
	if (!xml)
		g_error(_("%s: GUI loading failed!\n"), __FILE__);
	glade_xml_signal_autoconnect(xml);
	
	dbox = glade_xml_get_widget(xml, "dbgpclog_window");
	if(options3.transient)
		gtk_window_set_transient_for(GTK_WINDOW(dbox), GTK_WINDOW(main_wnd));

	data = glade_xml_get_widget(xml, "treeview1");
    store = clist_create(data);
	clist_populate(store);

	gtk_tree_view_expand_all(GTK_TREE_VIEW(data));

	return dbox;
}

GtkWidget* dbgpclog_display_window(void)
{
#ifdef WND_STATE
	if(!options3.pclog.minimized)
	{
		gtk_window_resize(GTK_WINDOW(dbgw.pclog), options3.pclog.rect.w, options3.pclog.rect.h);
		gtk_window_move(GTK_WINDOW(dbgw.pclog), options3.pclog.rect.x, options3.pclog.rect.y);
	}
	else
		gtk_window_iconify(GTK_WINDOW(dbgw.pclog));
#endif

	if(!GTK_WIDGET_VISIBLE(dbgw.pclog) && !options3.pclog.closed)
		gtk_widget_show(dbgw.pclog);

	return dbgw.pclog;
}

void dbgpclog_refresh_window(void)
{
	WND_TMR_START();

	if(!options3.pclog.closed)
	{
		clist_refresh(store);
	}

	WND_TMR_STOP("Pclog Refresh Time");
}

GLADE_CB gboolean
on_pclog_button_press_event        (GtkWidget       *widget,
                                    GdkEventButton  *event,
                                    gpointer         user_data)
{		
	GtkWidget *list = GTK_WIDGET(widget);
	GtkTreeView *view = GTK_TREE_VIEW(list);
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GList *l;

	// is double click ?
	if(event->type != GDK_2BUTTON_PRESS)
		return FALSE;
	
	// get selection
	selection = gtk_tree_view_get_selection(view);
	l = gtk_tree_selection_get_selected_rows(selection, &model);
	if(l != NULL)
	{
		GtkTreeIter iter;
		GtkTreePath *path = l->data;
        gchar** row_text = g_malloc0((CLIST_NVCOLS + 1) * sizeof(gchar *));
        uint32_t addr;
		
		// get address
		gtk_tree_model_get_iter(model, &iter, path);
		gtk_tree_model_get(model, &iter, COL_ADDR, &row_text[COL_ADDR], -1);

		// populate code
		sscanf(row_text[COL_ADDR], "%x", &addr);
		dbgcode_disasm_at(addr);

        g_strfreev(row_text);
    }

	// free selection
	g_list_foreach (l, (GFunc)gtk_tree_path_free, NULL);
	g_list_free (l);

    return FALSE;
}
