/* Hey EMACS -*- linux-c -*- */
/* $Id: dbg_stack.c 2825 2009-05-06 19:48:47Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Liévin
 *  Copyright (c) 2005, Romain Liévin, Kevin Kofler
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
#include <gdk/gdkkeysyms.h>
#include <stdlib.h>

#include "intl.h"
#include "paths.h"
#include "support.h"
#include "ti68k_int.h"
#include "struct.h"
#include "dbg_wnds.h"

enum { 
	    COL_ADDR, COL_DATA, COL_FONT
};
#define CLIST_NVCOLS	(2)		// 1 visible columns
#define CLIST_NCOLS		(3)		// 1 real columns

#define DUMP_SIZE       10

static GtkListStore* clist_create(GtkWidget *widget)
{
	GtkTreeView *view = GTK_TREE_VIEW(widget);
	GtkListStore *store;
	GtkTreeModel *model;
	GtkCellRenderer *renderer;
	GtkTreeSelection *selection;
    const gchar *text[CLIST_NVCOLS] = { _("Address"), _("Value") };
    gint i;
	
	store = gtk_list_store_new(CLIST_NCOLS,
				G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
				-1
            );
    model = GTK_TREE_MODEL(store);
	
    gtk_tree_view_set_model(view, model); 
    gtk_tree_view_set_headers_visible(view, FALSE);
	gtk_tree_view_set_rules_hint(view, FALSE);
  
	for(i = COL_ADDR; i <= COL_DATA; i++)
	{
		renderer = gtk_cell_renderer_text_new();
		set_renderer_pad(renderer);
		gtk_tree_view_insert_column_with_attributes(view, -1, 
            text[i], renderer, 
            "text", i,
            "font", COL_FONT,
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

#define	TARGET_SP	1
#define TARGET_FP	2

static void clist_populate(GtkListStore *store, gint target, gint offset)
{
    int i;
    GtkTreeIter iter;
    gchar *str;

    uint32_t sp;
    uint16_t data;

	if(target == TARGET_SP)
		ti68k_register_get_sp(&sp);
	else if(target == TARGET_FP)
		ti68k_register_get_addr(6, &sp);

    for(i = 0+(offset>>1); i < DUMP_SIZE+(offset>>1); i++)
    {
        gtk_list_store_append(store, &iter);

        str = g_strdup_printf(/*"0x%06x "*/"(%c%i):", /*sp + 2*i, */i > 0 ? '+' : i < 0 ? '-' : ' ', abs(2*i));
		gtk_list_store_set(store, &iter, COL_ADDR, str, -1);
        g_free(str);

		data = mem_rd_word(sp+2*i);	
		str = g_strdup_printf("%04x", data);
        gtk_list_store_set(store, &iter, COL_DATA, str, -1);
        g_free(str);

		if(options3.dbg_font_type)
			gtk_list_store_set(store, &iter, COL_FONT, options3.dbg_font_name, -1);

    }
}

static GtkWidget *label;
static GtkWidget *notebook;

static void clist_refresh(GtkListStore *store, gint target)
{
	gtk_list_store_clear(store);
	clist_populate(store, target, -4);

    if(ti68k_debug_is_supervisor())
        gtk_label_set_text(GTK_LABEL(label), "SSP");
    else
        gtk_label_set_text(GTK_LABEL(label), "USP");
}

static GtkListStore *store1 = NULL;
static GtkListStore *store2 = NULL;

/*
	Display registers window
*/
GtkWidget* dbgstack_create_window(void)
{
	GladeXML *xml = NULL;
	GtkWidget *dbox;
    GtkWidget *data;
	
	xml = glade_xml_new
		(tilp_paths_build_glade("dbg_stack-2.glade"), "dbgstack_window",
		 PACKAGE);
	if (!xml)
		g_error(_("%s: GUI loading failed!\n"), __FILE__);
	glade_xml_signal_autoconnect(xml);
	
	dbox = glade_xml_get_widget(xml, "dbgstack_window");
	if(options3.transient)
		gtk_window_set_transient_for(GTK_WINDOW(dbox), GTK_WINDOW(main_wnd));

	notebook = glade_xml_get_widget(xml, "notebook1");
    gtk_notebook_popup_enable(GTK_NOTEBOOK(notebook));

	data = glade_xml_get_widget(xml, "treeview1");
    store1 = clist_create(data);
	clist_populate(store1, TARGET_SP, 0);

	data = glade_xml_get_widget(xml, "treeview2");
    store2 = clist_create(data);
	clist_populate(store2, TARGET_FP, 0);

    label = glade_xml_get_widget(xml, "label2");

	gtk_tree_view_expand_all(GTK_TREE_VIEW(data));

	return dbox;
}

GtkWidget* dbgstack_display_window(void)
{
#ifdef WND_STATE
	if(!options3.stack.minimized)
	{
		gtk_window_resize(GTK_WINDOW(dbgw.stack), options3.stack.rect.w, options3.stack.rect.h);
		gtk_window_move(GTK_WINDOW(dbgw.stack), options3.stack.rect.x, options3.stack.rect.y);
	}
	else
		gtk_window_iconify(GTK_WINDOW(dbgw.stack));
#endif

	if(!GTK_WIDGET_VISIBLE(dbgw.stack) && !options3.stack.closed)
		gtk_widget_show(dbgw.stack);

	return dbgw.stack;
}

void dbgstack_refresh_window(void)
{
	WND_TMR_START();

	if(!options3.stack.closed)
	{
		clist_refresh(store1, TARGET_SP);
		clist_refresh(store2, TARGET_FP);
	}

	WND_TMR_STOP("Heap Refresh Time");
}

GLADE_CB gboolean
on_dbgstack_key_press_event           (GtkWidget       *widget,
                                       GdkEventKey     *event,
                                       gpointer         user_data)
{
    GtkTreeView *view = GTK_TREE_VIEW(widget);
	GtkTreeModel *model = gtk_tree_view_get_model(view);
	GtkListStore *store = GTK_LIST_STORE(model);
    GtkTreeIter iter;
    GtkTreePath *path;
	
	gchar *str;
    gchar *row;
    gint row_idx, row_max;
    uint32_t min, max;
    gint n;

	// get min address
    gtk_tree_model_get_iter_first(model, &iter);
    gtk_tree_model_get(model, &iter, COL_ADDR, &str, -1);
    sscanf(str, "(%i)", &min);

    // get max address
    n = gtk_tree_model_iter_n_children(model, NULL);
    gtk_tree_model_iter_nth_child(model, &iter, NULL, n-1);
    gtk_tree_model_get(model, &iter, COL_ADDR, &str, -1);
    sscanf(str, "(%i)", &max);

	// retrieve cursor
    gtk_tree_view_get_cursor(view, &path, NULL);
    if(path == NULL)
        return FALSE;

	// get row
    row_idx = row_max = -1;
    row = gtk_tree_path_to_string(path);
    sscanf(row, "%i", &row_idx);
    g_free(row);
    row_max = gtk_tree_model_iter_n_children(model, NULL) - 1;
    //printf("row_idx = %i, row_max = %i\n", row_idx, row_max);
    
    // bind our key
	switch(event->keyval) 
	{
    case GDK_Up:
        if(row_max == -1)
            break;
        if(row_idx > 0)
            break;

        gtk_list_store_clear(store);
        clist_populate(store, TARGET_SP, min-2);

		path = gtk_tree_path_new_from_string("0");
        gtk_tree_view_set_cursor(view, path, NULL, FALSE);

        return TRUE;

    case GDK_Down:
        if(row_max == -1)
            break;
        if(row_idx < row_max)
            break;

		gtk_list_store_clear(store);
        clist_populate(store, TARGET_SP, min+2);

        str = g_strdup_printf("%i", row_max);
        path = gtk_tree_path_new_from_string(str);
		g_free(str);
        gtk_tree_view_set_cursor(view, path, NULL, FALSE);

        return TRUE;

    case GDK_Page_Up:
        if(row_max == -1)
            break;

        if(row_idx > 0)
            break;

		 gtk_list_store_clear(store);
        clist_populate(store, TARGET_SP, min-10);

		path = gtk_tree_path_new_from_string("0");
        gtk_tree_view_set_cursor(view, path, NULL, FALSE);

        return TRUE;

    case GDK_Page_Down:
        if(row_max == -1)
            break;

        if(row_idx < row_max)
            break;

        gtk_list_store_clear(store);
        clist_populate(store, TARGET_SP, min+10);

        str = g_strdup_printf("%i", row_max);
        path = gtk_tree_path_new_from_string(str);
		g_free(str);
        gtk_tree_view_set_cursor(view, path, NULL, FALSE);

        return TRUE;

	default:
		return FALSE;
	}

	return FALSE;
}

static GtkWidget* display_dbgstack_popup_menu(void);
static uint32_t value = 0;

GLADE_CB gboolean
on_dbgstack_button_press_event     (GtkWidget       *widget,
                                    GdkEventButton  *event,
                                    gpointer         user_data)
{
	GtkWidget *list = GTK_WIDGET(widget);
	GtkTreeView *view = GTK_TREE_VIEW(list);
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GList *l;

	// get selection
	selection = gtk_tree_view_get_selection(view);
	l = gtk_tree_selection_get_selected_rows(selection, &model);
	if(l != NULL)
	{
		GtkTreeIter iter;
		GtkTreePath *path = l->data;
		gchar** row_text = g_malloc0((CLIST_NVCOLS + 1) * sizeof(gchar *));
		uint16_t hi, lo;
		
		// get address
		gtk_tree_model_get_iter(model, &iter, path);
		gtk_tree_model_get(model, &iter, COL_DATA, &row_text[COL_DATA], -1);
		sscanf(row_text[COL_DATA], "%x", &hi);

		if(gtk_tree_model_iter_next(model, &iter) == FALSE)
			return FALSE;
		gtk_tree_model_get(model, &iter, COL_DATA, &row_text[COL_DATA], -1);
		sscanf(row_text[COL_DATA], "%x", &lo);

		value = (hi << 16) | lo;

		g_strfreev(row_text);
	}

	if (l && event->type == GDK_2BUTTON_PRESS)
	{	
		dbgmem_add_tab(value);

		g_list_foreach (l, (GFunc)gtk_tree_path_free, NULL);
		g_list_free (l);

		return TRUE;
	}
	else if(l && event->button == 3)
	{
		GdkEventButton *bevent;
        GtkWidget *menu;  

        // popup menu
       	bevent = (GdkEventButton *) (event);
        menu = display_dbgstack_popup_menu();

		gtk_menu_popup(GTK_MENU(menu),
				   NULL, NULL, NULL, NULL,
				   bevent->button, bevent->time);
	    gtk_widget_show(menu);

		g_list_foreach (l, (GFunc)gtk_tree_path_free, NULL);
		g_list_free (l);

		return TRUE;
	}

    return FALSE;
}

/***** Popup menu *****/

/*
	Display popup menu (right click)
*/
static GtkWidget* display_dbgstack_popup_menu(void)
{
	GladeXML *xml;
	GtkWidget *menu;

	xml = glade_xml_new
	    (tilp_paths_build_glade("dbg_stack-2.glade"), "dbgstack_popup",
	     PACKAGE);
	if (!xml)
		g_error(_("%s: GUI loading failed!\n"), __FILE__);
	glade_xml_signal_autoconnect(xml);

	menu = glade_xml_get_widget(xml, "dbgstack_popup");
	return menu;
}

GLADE_CB void
on_dbgstack_view_memory1_activate      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	dbgmem_add_tab(value);
}
