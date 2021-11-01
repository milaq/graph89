/* Hey EMACS -*- linux-c -*- */
/* $Id: dbg_mem.c 2825 2009-05-06 19:48:47Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Liévin
 *  Copyright (c) 2005, Romain Liévin
 *  Copyright (c) 2006-2007, Kevin Kofler
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
#include "mem_map.h"

#define FORCE_REFRESH
#define DUMP_SIZE       128

enum { 
	    COL_ADDR, 
        COL_0, COL_1, COL_2, COL_3,
        COL_4, COL_5, COL_6, COL_7,
        COL_8, COL_9, COL_A, COL_B,
        COL_C, COL_D, COL_E, COL_F,
		COL_ASCII,

        COL_S0, COL_S1, COL_S2, COL_S3, // green or white background (selection)
        COL_S4, COL_S5, COL_S6, COL_S7,
        COL_S8, COL_S9, COL_SA, COL_SB,
        COL_SC, COL_SD, COL_SE, COL_SF,

        COL_EDITABLE,   // editable cell
		COL_GRAY,       // left and right column in gray
        COL_FONT,       // courier font for everyone
        COL_COLOR,      // red or black foreground (changes)
};
#define CLIST_NVCOLS	(18)
#define CLIST_NCOLS		(18 + 20)

static gint column2index(GtkWidget *list, GtkTreeViewColumn * column)
{
	gint i;

	for (i = 0; i < CLIST_NVCOLS; i++) {
		GtkTreeViewColumn *col;

		col = gtk_tree_view_get_column(GTK_TREE_VIEW(list), i);
		if (col == column)
			return i;
	}

	return -1;
}

static void renderer_edited(GtkCellRendererText * cell,
			    const gchar * path_string,
			    const gchar * new_text, gpointer user_data)
{
    GtkWidget *list = user_data;
	GtkTreeView *view = GTK_TREE_VIEW(list);
	GtkTreeModel *model = gtk_tree_view_get_model(view);
	GtkListStore *store = GTK_LIST_STORE(model);

    GtkTreeViewColumn *column;
    GtkTreeIter iter;
	GtkTreePath *path;	
	gint col;
    gchar *str_addr;
    gchar *str_data = (char *)new_text;
    int addr, data, i;
	gchar *ascii, *utf;
	gsize bw;

    // get column
    gtk_tree_view_get_cursor(view, &path, &column);
    if(!path || !column)
        return;

    // get iterator
	if (!gtk_tree_model_get_iter(model, &iter, path))
		return;

    // get address
	col = column2index(list, column);
    gtk_tree_model_get(model, &iter, COL_ADDR, &str_addr, COL_ASCII, &ascii, -1);

    // check for new value
    if((strlen(str_data) % 2) != 0)
    {
        gtk_tree_path_free(path);
        return;
    }

    for(i = 0; i < (int)strlen(str_data); i++)
        if(!isxdigit(str_data[i]))
            {
                gtk_tree_path_free(path);
                return;
            }

    // set new value(s) and update memory
    for(i = 0; (i < (int)strlen(str_data)/2) && ((col+i) <= COL_F); i++)
    {
        char digits[3];
		int offset = col - COL_0 + i;

        strncpy(digits, &new_text[2*i], 2); 
        digits[2] = '\0';

        sscanf(str_addr, "%x", &addr);
        sscanf(digits, "%x", &data);
		mem_wr_byte(addr + offset, (uint8_t)data);

		// don't rely on typed value
		data = mem_rd_byte(addr + offset);
		sprintf(digits, "%02x", data);
		ascii[offset] = (isprint(data) && !iscntrl(data) ? data : '.');

		gtk_list_store_set(store, &iter, col+i, digits, -1);
		dbgstack_refresh_window();	// refresh stack, too
    }

	// and ascii area, too
	utf = g_locale_to_utf8(ascii, -1, NULL, &bw, NULL);
	gtk_list_store_set(store, &iter, COL_ASCII, utf, -1);

    g_free(str_addr);
	gtk_tree_path_free(path);
}

static void refresh_page(int page, int offset);

static gboolean
on_treeview_key_press_event            (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);
static gboolean
on_treeview_btn_press_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

static GtkWidget* clist_create(GtkListStore **st)
{
	GtkWidget *list;
	GtkTreeView *view;
	GtkListStore *store;
	GtkTreeModel *model;
	GtkCellRenderer *renderer;
	GtkTreeSelection *selection;
	
	const gchar *text[CLIST_NVCOLS] = { 
            _("Address"), 
            "+0", "+1", "+2", "+3", "+4", "+5", "+6", "+7",
            "+8", "+9", "+A", "+B", "+C", "+D", "+E", "+F",
			"ASCII"
    };
    gint i;
	
	store = gtk_list_store_new(CLIST_NCOLS,
				G_TYPE_STRING, 
                G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
                G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
                G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
                G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
				G_TYPE_STRING,

                GDK_TYPE_COLOR, GDK_TYPE_COLOR, GDK_TYPE_COLOR, GDK_TYPE_COLOR,
                GDK_TYPE_COLOR, GDK_TYPE_COLOR, GDK_TYPE_COLOR, GDK_TYPE_COLOR,
                GDK_TYPE_COLOR, GDK_TYPE_COLOR, GDK_TYPE_COLOR, GDK_TYPE_COLOR,
                GDK_TYPE_COLOR, GDK_TYPE_COLOR, GDK_TYPE_COLOR, GDK_TYPE_COLOR,
				G_TYPE_BOOLEAN, GDK_TYPE_COLOR, G_TYPE_STRING, GDK_TYPE_COLOR,
				-1
            );
    model = GTK_TREE_MODEL(store);
	
	list = gtk_tree_view_new_with_model(model);
	view = GTK_TREE_VIEW(list);
  
    gtk_tree_view_set_model(view, model); 
    gtk_tree_view_set_headers_visible(view, TRUE);
	gtk_tree_view_set_rules_hint(view, TRUE);
  
	i = COL_ADDR;
	renderer = gtk_cell_renderer_text_new();
	set_renderer_pad(renderer);
	gtk_tree_view_insert_column_with_attributes(view, -1, 
            text[i], renderer, 
            "text", i,
			"font", COL_FONT,
			"foreground-gdk", COL_GRAY,
			NULL);

    for (i = COL_0; i <= COL_F; i++)
    {
    	renderer = gtk_cell_renderer_text_new();
		set_renderer_pad(renderer);
        gtk_tree_view_insert_column_with_attributes(view, -1, 
            text[i], renderer, 
            "text", i, 
			"font", COL_FONT,
            "editable", COL_EDITABLE,
            "foreground-gdk", COL_COLOR,
            "background-gdk", COL_S0 + i - COL_0,
            NULL);

        g_signal_connect(G_OBJECT(renderer), "edited",
			 G_CALLBACK(renderer_edited), list);
    }

	i = COL_ASCII;
	renderer = gtk_cell_renderer_text_new();
	set_renderer_pad(renderer);
	gtk_tree_view_insert_column_with_attributes(view, -1, 
            text[i], renderer, 
            "text", i,
			"font", COL_FONT,
			"foreground-gdk", COL_GRAY,
			NULL);
    
    for (i = 0; i < CLIST_NVCOLS; i++) 
    {
		GtkTreeViewColumn *col;
		
		col = gtk_tree_view_get_column(view, i);
		gtk_tree_view_column_set_resizable(col, TRUE);
	}
	
	selection = gtk_tree_view_get_selection(view);
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);

    g_signal_connect(G_OBJECT(list), "key_press_event",
			 G_CALLBACK(on_treeview_key_press_event), list);
    g_signal_connect(G_OBJECT(list), "button_press_event",
			 G_CALLBACK(on_treeview_btn_press_event), list);

	*st = store;
	return list;
}

static void clist_populate(GtkListStore *store, uint32_t start, int length)
{
    GtkTreeIter iter;
    int i;
    gchar *str;
    char ascii[17];
    uint32_t a;
	GdkColor gray, black, red, white;
	gboolean success;
    GdkColor *color = &black;

    //static uint8_t old[DUMP_SIZE] = { 0 };
    //static uint8_t *old_ptr;

	gdk_color_parse("DarkGray", &gray);
	gdk_colormap_alloc_colors(gdk_colormap_get_system(), &gray, 1,
				  FALSE, FALSE, &success);

	gdk_color_parse("Black", &black);
	gdk_colormap_alloc_colors(gdk_colormap_get_system(), &black, 1,
				  FALSE, FALSE, &success);

	gdk_color_parse("Red", &red);
	gdk_colormap_alloc_colors(gdk_colormap_get_system(), &red, 1,
				  FALSE, FALSE, &success);

    gdk_color_parse("White", &white);
	gdk_colormap_alloc_colors(gdk_colormap_get_system(), &white, 1,
				  FALSE, FALSE, &success);

    //old_ptr = old;
    for(a = start; a < start+length; a += 0x10)
    {
		uint32_t addr = a & 0xffffff;
		uint8_t mem;

		char *utf;
		gsize bw;

        gtk_list_store_append(store, &iter);

		str = g_strdup_printf("%06x", addr);
		gtk_list_store_set(store, &iter, 
			COL_ADDR, str, 
			COL_GRAY, &gray,
			-1);
		g_free(str);

		if(options3.dbg_font_type)
			gtk_list_store_set(store, &iter, COL_FONT, options3.dbg_font_name, -1);
		else
			gtk_list_store_set(store, &iter, COL_FONT, "courier", -1);

		for(i = COL_0; i <= COL_F; i++)
		{
			if(addr + (i-COL_0) != 0x60000f)
				mem = mem_rd_byte(addr + (i-COL_0)); 
			else
				mem = 0xff;

			str = g_strdup_printf("%02x", mem);
			ascii[i-COL_0] = (isprint(mem) && !iscntrl(mem) ? mem : '.');
/*
            if(*old_ptr != *mem_ptr)
            {
                *old_ptr++ = *mem_ptr;
                color = &red;
                printf("$");
            }
            else
            {
                old_ptr++;
                color = &black;
                printf(".");
            }
*/
			gtk_list_store_set(store, &iter, 
				i, str, 
				COL_EDITABLE, TRUE, 
                COL_COLOR, color,
                i + CLIST_NVCOLS - COL_0, &white,
				-1);

			g_free(str);            
        }
	 
		ascii[16] = '\0';
		utf = g_locale_to_utf8(ascii, -1, NULL, &bw, NULL);
		gtk_list_store_set(store, &iter, COL_ASCII, utf, -1);
    }
}

static void clist_refresh(GtkListStore *store, uint32_t start, int length)
{
	gtk_list_store_clear(store);
	clist_populate(store, start, length);
}

static void notebook_add_page(GtkWidget *notebook, const char* tab_name)
{
	GtkListStore *store;
	GtkWidget *label;
	GtkWidget *child;
	GtkNotebook *nb = GTK_NOTEBOOK(notebook);
	gint page = gtk_notebook_get_current_page(nb);
	uint32_t addr;
	
	label = gtk_label_new(tab_name);
	gtk_widget_show(label);

    child = clist_create(&store);

	// display normal
	sscanf(tab_name, "%06x", &addr);
   	clist_populate(store, addr, DUMP_SIZE);

	gtk_widget_show(child);

	gtk_notebook_insert_page(nb, child, label, page);
	gtk_notebook_set_current_page(nb, page);

    gtk_widget_grab_focus(child);
}

static GtkWidget *notebook;

/*
	Display memory window
*/
GtkWidget* dbgmem_create_window(void)
{
	GladeXML *xml = NULL;
	GtkWidget *dbox;
	
	xml = glade_xml_new
		(tilp_paths_build_glade("dbg_mem-2.glade"), "dbgmem_window",
		 PACKAGE);
	if (!xml)
		g_error("GUI loading failed !\n");
	glade_xml_signal_autoconnect(xml);
	
	dbox = glade_xml_get_widget(xml, "dbgmem_window");
	if(options3.transient)
		gtk_window_set_transient_for(GTK_WINDOW(dbox), GTK_WINDOW(main_wnd));

    notebook = glade_xml_get_widget(xml, "notebook1");
    gtk_notebook_popup_enable(GTK_NOTEBOOK(notebook));
    
	notebook_add_page(notebook, "0x000000");

	return dbox;
}

GtkWidget* dbgmem_display_window(void)
{ 
#ifdef WND_STATE
	if(!options3.mem.minimized)
	{
		gtk_window_resize(GTK_WINDOW(dbgw.mem), options3.mem.rect.w, options3.mem.rect.h);
		gtk_window_move(GTK_WINDOW(dbgw.mem), options3.mem.rect.x, options3.mem.rect.y);
	}
	else
		gtk_window_iconify(GTK_WINDOW(dbgw.mem));
#endif

	if(!GTK_WIDGET_VISIBLE(dbgw.mem) && !options3.mem.closed)
		gtk_widget_show(dbgw.mem);

    return dbgw.mem;
}

void dbgmem_refresh_window(void)
{
	WND_TMR_START();

	if(!options3.mem.closed)
	{
        GtkNotebook *nb = GTK_NOTEBOOK(notebook);
	    gint page = gtk_notebook_get_current_page(nb);

        refresh_page(page, 0);
	}

	WND_TMR_STOP("Memory Refresh Time");
}

void dbgmem_add_tab(uint32_t addr)
{
    gchar *str;
	
	str = g_strdup_printf("%06x", addr);
	notebook_add_page(notebook, str);
	g_free(str);
}


GLADE_CB void
dbgmem_button1_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkNotebook *nb = GTK_NOTEBOOK(notebook);
	uint32_t addr = 0;
	gchar *str;
	
	if(display_dbgmem_address(&addr) == -1)
		return;
	
	str = g_strdup_printf("%06x", addr);
	notebook_add_page(notebook, str);
	g_free(str);

	gtk_widget_set_sensitive(GTK_WIDGET(button), 
		gtk_notebook_get_n_pages(nb) > 1 ? TRUE : FALSE);
}

GLADE_CB void
dbgmem_button2_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkNotebook *nb = GTK_NOTEBOOK(notebook);
	gint page = gtk_notebook_get_current_page(nb);
	
	if(gtk_notebook_get_n_pages(nb) > 1)
		gtk_notebook_remove_page(nb, page);

	gtk_widget_set_sensitive(GTK_WIDGET(button), 
		gtk_notebook_get_n_pages(nb) > 1 ? TRUE : FALSE);
}

GLADE_CB void
dbgmem_button3_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkNotebook *nb = GTK_NOTEBOOK(notebook);
	gint page = gtk_notebook_get_current_page(nb);

	refresh_page(page, -0x10);
}

GLADE_CB void
dbgmem_button4_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkNotebook *nb = GTK_NOTEBOOK(notebook);
	gint page = gtk_notebook_get_current_page(nb);

	refresh_page(page, +0x10);
}

GLADE_CB void
dbgmem_button5_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkNotebook *nb = GTK_NOTEBOOK(notebook);
	gint page = gtk_notebook_get_current_page(nb);

	refresh_page(page, 0);
}

/***** Mem Map popup menu *****/

static void memmap_menu_activate(GtkMenuItem* menuitem, gpointer user_data)
{
	guint32 addr = GPOINTER_TO_INT(user_data);
	gchar *str;
	
	str = g_strdup_printf("%06x", addr);
	notebook_add_page(notebook, str);
	g_free(str);
}

static GtkWidget* memmap_menu(void)
{
	GtkWidget *menu;
	GtkWidget *item;
	int result;
	MEM_MAP **ptr, **array = NULL;


	menu = gtk_menu_new();
	g_object_set_data_full(G_OBJECT(menu), "memmap_menu",
			       gtk_widget_ref(menu),
			       (GDestroyNotify)g_object_unref);

	// (re)load mem map
	result = ti68k_debug_load_memmap(inst_paths.misc_dir);
	if(result == -1)
	{
		return menu;
	}

	array = memmap_array();
	if(array == NULL)
		return menu;
	
	for(ptr = array; *ptr; ptr++)
	{
		MEM_MAP *s = *ptr;
		char *label;

		label = g_strdup_printf("%06x-%06x : %s", s->addr, s->addr + s->size - 1, s->name);

		item = gtk_menu_item_new_with_label(label);
		g_object_set_data_full(G_OBJECT(menu), "c_drive",
					   gtk_widget_ref(item),
					   (GDestroyNotify)g_object_unref);
		gtk_widget_show(item);

		gtk_container_add(GTK_CONTAINER(menu), item);
		g_signal_connect((gpointer)item, "activate",
					   G_CALLBACK(memmap_menu_activate),
					   GINT_TO_POINTER(s->addr));

		g_free(label);
	}

	return menu;
}

GLADE_CB void
dbgmem_button6_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *menu;
	guint butt = 0;
	guint32 time;

	time = gtk_get_current_event_time();
	menu = memmap_menu();
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, butt, time);
	gtk_widget_show(menu);
}

GLADE_CB void
on_notebook1_switch_page               (GtkNotebook     *notebook,
                                        GtkNotebookPage *page,
                                        guint            page_num,
                                        gpointer         user_data)
{
	refresh_page(page_num, 0);
}

/***** Misc *****/

static void refresh_page(int page, int offset)
{
	GtkNotebook *nb = GTK_NOTEBOOK(notebook);
	GtkWidget *tab;
	GtkWidget *label;
	G_CONST_RETURN gchar *text;
	uint32_t addr, len = DUMP_SIZE;

	GList *l, *elt;
	GtkWidget *list;
	GtkTreeView *view;
	GtkTreeModel *model;
	GtkListStore *store;
	gchar *str;

    gint i;

	// retrieve addr by tab name
	tab = gtk_notebook_get_nth_page(nb, page);
	label = gtk_notebook_get_tab_label(nb, tab);
	text = gtk_label_get_text(GTK_LABEL(label));

	// get list pointer (we have 1 child)
	l = gtk_container_get_children(GTK_CONTAINER(nb));
	elt = g_list_nth(l, page);
	list = GTK_WIDGET(elt->data);
	view = GTK_TREE_VIEW(list);
	model = gtk_tree_view_get_model(view);
	store = GTK_LIST_STORE(model);

    // get new address
	sscanf(text, "%x", &addr);
	len = DUMP_SIZE;

	addr += offset;
	addr &= 0xffffff;

    // refresh only if mem changed (speed-up)
#ifndef FORCE_REFRESH
    if(!offset)
    {
        static uint8_t old_array[DUMP_SIZE] = { 0 };
        gint diff = 0;

        // can't use memcmp due to banking
        for(i = 0; i < DUMP_SIZE; i++)
        {
			uint8_t old = old_array[i];
			uint8_t mem = mem_rd_byte(addr + i);

            if(old != mem)
            {
                old = mem;
                diff = !0;
            }
        }

        if(!diff) return;
    }
#endif

    // refresh tab
	str = g_strdup_printf("%06x", addr);
	gtk_label_set_text(GTK_LABEL(label), str);
	g_free(str);

    // and list
   	clist_refresh(store, addr, len <= DUMP_SIZE ? len : DUMP_SIZE);

    // set column
	for(i = COL_0; i <= COL_F; i++)
    {
        GtkTreeViewColumn *col;

		col = gtk_tree_view_get_column(view, i);
        str = g_strdup_printf("%X", (addr + i - 1) & 0xf);
        gtk_tree_view_column_set_title(col, str);
        g_free(str);
    }
}

/***** Popup menu *****/

/*
	Display popup menu (right click)
*/
static GtkWidget* display_dbgmem_popup_menu(void)
{
	GladeXML *xml2;
	GtkWidget *menu;

	xml2 = glade_xml_new
	    (tilp_paths_build_glade("dbg_mem-2.glade"), "dbgmem_popup",
	     PACKAGE);
	if (!xml2)
		g_error(_("%s: GUI loading failed!\n"), __FILE__);
	glade_xml_signal_autoconnect(xml2);

	menu = glade_xml_get_widget(xml2, "dbgmem_popup");
	return menu;
}

static gboolean
on_treeview_btn_press_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    GtkTreeView *view = GTK_TREE_VIEW(widget);
	GtkTreePath *path;
	GtkTreeViewColumn *column;
    gboolean ret;

    switch (event->type) 
    {
    case GDK_BUTTON_PRESS:	// third button clicked
        if (event->button == 3) 
        {
            GdkEventButton *bevent;
            GtkWidget *menu;

            //--- set cell focus
            gint tx = (gint) event->x;
	        gint ty = (gint) event->y;
	        gint cx, cy;

	        ret = gtk_tree_view_get_path_at_pos(view, tx, ty, &path, &column, &cx, &cy);
            gtk_tree_view_set_cursor(view, path, column, FALSE);    // select cell
            gtk_tree_view_row_activated(view, path, column);        // show selection
            gtk_tree_path_free(path);
            //printf("%i %i %i %i (%i)\n", tx, ty, cx, cy, ret);
            //---

		    bevent = (GdkEventButton *) (event);
            menu = display_dbgmem_popup_menu();

		    gtk_menu_popup(GTK_MENU(menu),
				       NULL, NULL, NULL, NULL,
				       bevent->button, bevent->time);
	        gtk_widget_show(menu);

		    return TRUE;
	    }
	    break;
    default:
        break;
    }

    return FALSE;
}

GLADE_CB void
on_find1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GtkNotebook *nb = GTK_NOTEBOOK(notebook);
    gint page = gtk_notebook_get_current_page(nb);
	GtkWidget *tab;
	GtkWidget *label;
	G_CONST_RETURN gchar *text;
	uint32_t addr;

	tab = gtk_notebook_get_nth_page(nb, page);
	label = gtk_notebook_get_tab_label(nb, tab);
	text = gtk_label_get_text(GTK_LABEL(label));
    sscanf(text, "%06x", &addr);

    display_dbgmem_search(addr);
}


GLADE_CB void
on_find_next1_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


GLADE_CB void
on_go_to_address2_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkNotebook *nb = GTK_NOTEBOOK(notebook);
	gint page = gtk_notebook_get_current_page(nb);

    uint32_t addr = 0;
	gchar *str;
	
	if(display_dbgmem_address(&addr) == -1)
		return;

    gtk_notebook_remove_page(nb, page);
	
	str = g_strdup_printf("%06x", addr);
	notebook_add_page(notebook, str);
	g_free(str);
}

static uint32_t column2address         (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GtkNotebook *nb = GTK_NOTEBOOK(notebook);
    gint page = gtk_notebook_get_current_page(nb);

	GList *l, *elt;
	GtkWidget *list;
	GtkTreeView *view;
	GtkTreeModel *model;
	GtkListStore *store;
	gchar *str;

    GtkTreePath *path;
    GtkTreeViewColumn *column;
    GtkTreeIter iter;
    gint col;
    uint32_t addr;

	// get list pointer (we have 1 child)
	l = gtk_container_get_children(GTK_CONTAINER(nb));
	elt = g_list_nth(l, page);
	list = GTK_WIDGET(elt->data);
	view = GTK_TREE_VIEW(list);
	model = gtk_tree_view_get_model(view);
	store = GTK_LIST_STORE(model);

    // get column
    gtk_tree_view_get_cursor(view, &path, &column);
    if(!path || !column)
        return -1;

    // get iterator
	if (!gtk_tree_model_get_iter(model, &iter, path))
		return -1;

    // get address
	col = column2index(list, column);
    gtk_tree_model_get(model, &iter, COL_ADDR, &str, -1);
    sscanf(str, "%06x", &addr);

	return (addr + (col-1));
}

GLADE_CB void
on_dissassemble1_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    uint32_t addr = column2address(menuitem, user_data);
    
    // populate code at this address
    dbgcode_disasm_at(addr);
}

GLADE_CB void
on_disassemble_indirect1_activate              (GtkMenuItem     *menuitem,
                                                gpointer         user_data)
{
	uint32_t addr = column2address(menuitem, user_data);

	// populate code at the address contained at this address
	dbgcode_disasm_at(mem_rd_long(addr));
}

GLADE_CB void
on_dbgmem_view_memory1_activate                (GtkMenuItem     *menuitem,
                                                gpointer         user_data)
{
	uint32_t addr = column2address(menuitem, user_data);
	gchar *str;

	// open a new tab at the address contained at this address	
	str = g_strdup_printf("%06x", mem_rd_long(addr));
	notebook_add_page(notebook, str);
	g_free(str);
}

static void search_next(void);

GLADE_CB gboolean
on_dbgmem_key_press_event              (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data)
{
	GtkNotebook *nb = GTK_NOTEBOOK(notebook);

    switch(event->keyval) 
	{
	case GDK_F1:
		dbgmem_button1_clicked(NULL, NULL);
		return TRUE;
	case GDK_F2:
		dbgmem_button2_clicked(NULL, NULL);
		return TRUE;
	case GDK_F3:
		dbgmem_button3_clicked(NULL, NULL);
		return TRUE;
	case GDK_F4:
		dbgmem_button4_clicked(NULL, NULL);
		return TRUE;
	case GDK_F5:
		dbgmem_button5_clicked(NULL, NULL);
		return TRUE;
	case GDK_F6:
		dbgmem_button6_clicked(NULL, NULL);
		return TRUE;
	case GDK_F7:
		gtk_notebook_next_page(nb);
		return TRUE;
	case GDK_F8:
		gtk_notebook_prev_page(nb);
		return TRUE;
	default:
		return FALSE;
	}

    return FALSE;
}

static gboolean
on_treeview_key_press_event            (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data)
{
    GtkTreeView *view = GTK_TREE_VIEW(widget);
	GtkTreeModel *model = gtk_tree_view_get_model(view);
    GtkTreeIter iter;
    GtkTreePath *path;
    GtkTreeViewColumn *column;

    gchar *str;
    gchar *row;
    gint row_idx, row_max;
    uint32_t min, max;
    gint n;

    GtkNotebook *nb = GTK_NOTEBOOK(notebook);
	gint page = gtk_notebook_get_current_page(nb);

    // get min address
    gtk_tree_model_get_iter_first(model, &iter);
    gtk_tree_model_get(model, &iter, COL_ADDR, &str, -1);
    sscanf(str, "%x", &min);

    // get max address
    n = gtk_tree_model_iter_n_children(model, NULL);
    gtk_tree_model_iter_nth_child(model, &iter, NULL, n-1);
    gtk_tree_model_get(model, &iter, COL_ADDR, &str, -1);
    sscanf(str, "%x", &max);

    // retrieve cursor
    gtk_tree_view_get_cursor(view, &path, &column);
    if(path == NULL)
        return FALSE;

    // get row
    row_idx = row_max = -1;
    row = gtk_tree_path_to_string(path);
    sscanf(row, "%i", &row_idx);
    g_free(row);
    row_max = gtk_tree_model_iter_n_children(model, NULL) - 1;
    //printf("row_idx = %i\n", row_idx);

    switch(event->keyval) 
	{
    case GDK_Up:
        if(row_max == -1)
            break;

        if(row_idx > 0)
            break;

        refresh_page(page, -0x10);

        return FALSE;

    case GDK_Down:
        if(row_max == -1)
            break;

        if(row_idx < row_max)
            break;

        refresh_page(page, +0x10);

		str = g_strdup_printf("%i", row_max);
        path = gtk_tree_path_new_from_string(str);
		g_free(str);
        gtk_tree_view_set_cursor(view, path, NULL, FALSE);
        
        return FALSE;

    case GDK_Page_Up:
        if(row_max == -1)
            break;

        if(row_idx > 0)
            break;

        refresh_page(page, -DUMP_SIZE);

        path = gtk_tree_path_new_from_string("0");
        gtk_tree_view_set_cursor(view, path, NULL, FALSE);

        return FALSE;

    case GDK_Page_Down:
        if(row_max == -1)
            break;

        if(row_idx < row_max)
            break;

        refresh_page(page, +DUMP_SIZE);

        str = g_strdup_printf("%i", row_max);
        path = gtk_tree_path_new_from_string(str);
		g_free(str);
        gtk_tree_view_set_cursor(view, path, NULL, FALSE);

        return FALSE;

    case GDK_n:
		search_next();
		return TRUE;

    case GDK_a:
    case GDK_A:
        if(event->state & GDK_CONTROL_MASK)
        {
            on_go_to_address2_activate((GtkMenuItem *)widget, user_data);
            return TRUE;
        }

    case GDK_f:
    case GDK_F:
        if(event->state & GDK_CONTROL_MASK)
        {
            on_find1_activate((GtkMenuItem *)widget, user_data);
            return TRUE;
        }

	case GDK_plus:
		dbgmem_button1_clicked(NULL, NULL);
		return FALSE;

	case GDK_minus:
		dbgmem_button2_clicked(NULL, NULL);
		return FALSE;

	default:
		return FALSE;
	}

    return FALSE;
}

/*
	Type address in a box.
*/
gint display_dbgmem_address(uint32_t *addr)
{
	GladeXML *xml;
	GtkWidget *dbox;
	GtkWidget *entry;
	gint result;
	gchar *str;
	gint ret = -1;
	
	xml = glade_xml_new
		(tilp_paths_build_glade("dbg_mem-2.glade"), "dbgmem_address", PACKAGE);
	if (!xml)
		g_error(_("%s: GUI loading failed!\n"), __FILE__);
	glade_xml_signal_autoconnect(xml);
	
	entry = glade_xml_get_widget(xml, "entry1");
	str = g_strdup_printf("0x%06x", *addr);
	gtk_entry_set_text(GTK_ENTRY(entry), str);
	g_free(str);

	gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1);
	
	dbox = glade_xml_get_widget(xml, "dbgmem_address");	
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dbox), GTK_RESPONSE_OK,
	                                        GTK_RESPONSE_CANCEL,-1);
	result = gtk_dialog_run(GTK_DIALOG(dbox));
	
	switch (result) 
	{
	case GTK_RESPONSE_OK:
		str = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
		sscanf(str, "%x", addr);
		ret = 0;
		break;
	default:
		break;
	}

	gtk_widget_destroy(dbox);
	return ret;
}

/*
    Search string/data

  Note: 'casse' is a French'ized word taken from the English word 'case'. It means 
  'case sensitive' or in good French 'respect des majuscules'.
*/

static gint search_engine(char *str, int ascii, int casse, uint32_t *address, int *length)
{
    uint8_t *data;
    uint16_t len;
    gchar *tmp;
    gint i;
    uint32_t addr;

    data = (uint8_t *) g_strdup((gchar *) str);
    len = strlen((char *) data);

    // ASCII mode ?
    if(!ascii)
    {
        char *p, *q;

        // compact string
        p = str;
        q = tmp = g_strdup(str);

        while(*p)
        {
            if(*p != ' ')
                *q++ = *p++;
            else
                p++;
        }
        *q = '\0';

        // and converts
        len = strlen(tmp) / 2;
        for(i = 0; i < len; i++)
        {
            char digits[3];
            unsigned int temp;

            strncpy(digits, &tmp[2*i], 2); 
            digits[2] = '\0';
            sscanf(digits, "%02x", &temp);
            data[i] = temp;
        }

        g_free(tmp);
    }

    // search (we don't use memcmp because mem space may be not contiguous)
    i = 0;
    for(addr = *address; addr <= 0xffffff; addr++)
    {
		if(mem_rd_byte(addr) == data[i])
            i++;
        else
            i = 0;

        if(i == len)
            break;
    }

    if(i == len)
    {
        *address = addr - len + 1;
        *length = len;
        return !0;
    }

    return 0;
}

#define IS_BOUNDED(a,v,b) (((a) <= (v)) && ((v) <= (b)))

static gint search_highlight(uint32_t blk_beg, uint32_t blk_end, int state)
{
    GtkNotebook *nb = GTK_NOTEBOOK(notebook);
    gint page = gtk_notebook_get_current_page(nb);
	GtkWidget *tab;
	GtkWidget *label;
	G_CONST_RETURN gchar *text;
	uint32_t tab_adr, addr, offset;
    gint i;
    gint start, stop;

	GList *l, *elt;
	GtkWidget *list;
	GtkTreeView *view;
	GtkTreeModel *model;
	GtkListStore *store;
    GtkTreeIter iter;
    gboolean valid;

    GdkColor white, green;
	gboolean success;

	gdk_color_parse("White", &white);
	gdk_colormap_alloc_colors(gdk_colormap_get_system(), &white, 1,
				  FALSE, FALSE, &success);

	gdk_color_parse("Green", &green);
	gdk_colormap_alloc_colors(gdk_colormap_get_system(), &green, 1,
				  FALSE, FALSE, &success);

	// retrieve addr by tab name
	tab = gtk_notebook_get_nth_page(nb, page);
	label = gtk_notebook_get_tab_label(nb, tab);
	text = gtk_label_get_text(GTK_LABEL(label));
    sscanf(text, "%06x", &tab_adr);

	// get list pointer (we have 1 child)
	l = gtk_container_get_children(GTK_CONTAINER(nb));
	elt = g_list_nth(l, page);
	list = GTK_WIDGET(elt->data);
	view = GTK_TREE_VIEW(list);
	model = gtk_tree_view_get_model(view);
	store = GTK_LIST_STORE(model);

    // scroll mem
    if(!IS_BOUNDED(tab_adr, blk_beg, tab_adr + DUMP_SIZE))
    {
        GtkNotebook *nb = GTK_NOTEBOOK(notebook);
	    gint page = gtk_notebook_get_current_page(nb);

        offset = (blk_beg - tab_adr) & 0xfffff0;
        refresh_page(page, offset);

        while(gtk_events_pending()) gtk_main_iteration();
        tab_adr += offset;
        tab_adr &= 0xffffff;
    }

    // change background color
    for(valid = gtk_tree_model_get_iter_first(model, &iter), addr = tab_adr;
        valid && (addr - tab_adr < DUMP_SIZE); 
        valid = gtk_tree_model_iter_next(model, &iter), addr += 0x10)
    {
        // clear selection
        for(i = 0; i < 16; i++)
            gtk_list_store_set(store, &iter, i + COL_S0, &white, -1);

        if(addr + 16 <= blk_beg)
            continue;

        if(addr > blk_end)
            continue;

        start = !addr ? blk_beg : blk_beg % addr;
        if(start > 15) start = 0;

        stop = !addr ? blk_end : blk_end % addr;
        if(stop > 15) stop = 16;

        // set selection
        for(i = start; (i < 16) && (i < stop); i++)
            gtk_list_store_set(store, &iter, i + COL_S0, &green, -1);
    }

    return 0;
}

// searching engine context
static gchar *old_str = NULL;
static gint ascii = !0;
static gint casse = !0;
static uint32_t blk_adr = 0x000000;
static gint blk_len = 0;

static void search_next(void)
{
    if(search_engine(old_str, ascii, casse, &blk_adr, &blk_len))
    {
        search_highlight(blk_adr, blk_adr + blk_len, !0);
        while(gtk_events_pending()) gtk_main_iteration();
    }

    blk_adr += blk_len;
}

gint display_dbgmem_search(uint32_t addr)
{
	GladeXML *xml;
	GtkWidget *dbox;
	GtkWidget *entry, *check1, *check2;
	gint result;
	gchar *str;
	gint ret = -1;
	
	xml = glade_xml_new
		(tilp_paths_build_glade("dbg_mem-2.glade"), "dbgmem_search", PACKAGE);
	if (!xml)
		g_error(_("%s: GUI loading failed!\n"), __FILE__);
	glade_xml_signal_autoconnect(xml);
	
    if(old_str == NULL)
         old_str = g_strdup("");

	entry = glade_xml_get_widget(xml, "entry1");
	gtk_entry_set_text(GTK_ENTRY(entry), old_str);

    check1 = glade_xml_get_widget(xml, "checkbutton1");    
    check2 = glade_xml_get_widget(xml, "checkbutton2");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check2), TRUE);

	dbox = glade_xml_get_widget(xml, "dbgmem_search");
	/* Find, Find Next, Cancel */
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dbox), GTK_RESPONSE_OK,
	                                        GTK_RESPONSE_APPLY, GTK_RESPONSE_CANCEL,-1);
	
    
    for(result = 0;;)
    {
        result = gtk_dialog_run(GTK_DIALOG(dbox));

        if((result == GTK_RESPONSE_CANCEL) || 
            (result == GTK_RESPONSE_DELETE_EVENT))
            break;

        if(result == GTK_RESPONSE_OK)
        {
            blk_adr = addr;	//0x000000;
            blk_len = 0;
        }

        //printf("searching at $%x\n", blk_adr);

        // find first/next
	    str = (gchar *)gtk_entry_get_text(GTK_ENTRY(entry));

        g_free(old_str);
        old_str = g_strdup(str);

        ascii = GTK_TOGGLE_BUTTON(check1)->active;
        casse = GTK_TOGGLE_BUTTON(check2)->active;

        gtk_widget_set_sensitive(entry, FALSE);
        while(gtk_events_pending()) gtk_main_iteration();

	    if(search_engine(old_str, ascii, casse, &blk_adr, &blk_len))
        {
            search_highlight(blk_adr, blk_adr + blk_len, !0);
            while(gtk_events_pending()) gtk_main_iteration();
        }

        gtk_widget_set_sensitive(entry, TRUE);
        while(gtk_events_pending()) gtk_main_iteration();

        blk_adr += blk_len;
        blk_adr &= 0xffffff;
	}

	gtk_widget_destroy(dbox);
	return ret;
}
