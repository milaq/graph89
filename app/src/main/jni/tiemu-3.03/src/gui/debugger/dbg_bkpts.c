/* Hey EMACS -*- linux-c -*- */
/* $Id: dbg_bkpts.c 2825 2009-05-06 19:48:47Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Liévin
 *  Copyright (c) 2005-2006, Romain Liévin, Kevin Kofler
 *  Copyright (c) 2007, Romain Liévin
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
#include <string.h>

#include "intl.h"
#include "paths.h"
#include "support.h"
#include "ti68k_int.h"
#include "struct.h"
#include "dbg_wnds.h"
#include "bkpts.h"
#include "handles.h"

static GladeXML *xml = NULL;
static GtkWidget *wnd = NULL;

enum { 
	    COL_SYMBOL, COL_TYPE, COL_STATUS, COL_START, COL_END, COL_MODE,
		COL_DATA, COL_FONT
};
#define CLIST_NVCOLS	(6)		// 7 visible columns
#define CLIST_NCOLS		(8)		// 7 real columns

static GtkListStore* clist_create(GtkWidget *widget)
{
	GtkTreeView *view = GTK_TREE_VIEW(widget);
	GtkListStore *store;
	GtkTreeModel *model;
	GtkCellRenderer *renderer;
	GtkTreeSelection *selection;
    const gchar *text[CLIST_NVCOLS] = { 
		_("Symbol"), _("Type"), _("Status"), _("Start"), _("End"), _("Mode")
    };
    gint i;
	
	store = gtk_list_store_new(CLIST_NCOLS,
				G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
				G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_STRING,
				-1
            );
    model = GTK_TREE_MODEL(store);
	
    gtk_tree_view_set_model(view, model); 
    gtk_tree_view_set_headers_visible(view, TRUE);
	gtk_tree_view_set_rules_hint(view, FALSE);
  
	for(i = COL_SYMBOL; i <= COL_MODE; i++)
	{
		renderer = gtk_cell_renderer_text_new();
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
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);

	return store;
}

static GList** bkpts_mem_access[6] = {
	&bkpts.mem_rb, &bkpts.mem_rw, &bkpts.mem_rl, 
	&bkpts.mem_wb, &bkpts.mem_ww, &bkpts.mem_wl, 
};
static  GList** bkpts_mem_range[2] = { 
	&bkpts.mem_rng_r, &bkpts.mem_rng_w,
};
static const int bkpts_memacc_rw[6] = {
    BK_READ_BYTE, BK_READ_WORD, BK_READ_LONG,
    BK_WRITE_BYTE, BK_WRITE_WORD, BK_WRITE_LONG, 
};
static const int bkpts_memrng_rw[2] = {
	BK_READ, BK_WRITE
};

static void clist_populate(GtkListStore *store)
{
	GList *l;
    GtkTreeIter iter, iter2;
	gint i;

	GtkTreeModel *model = GTK_TREE_MODEL(store);
	gboolean valid, valid2, removed;
	uint32_t mode, mode2;

	// Code breakpoints
	for(l = bkpts.code; l != NULL; l = g_list_next(l))
	{
		uint32_t addr = GPOINTER_TO_INT(l->data);
		gchar *str;
		
		str = g_strdup_printf("0x%06x", BKPT_ADDR(addr));
		
		gtk_list_store_append(store, &iter);

		gtk_list_store_set(store, &iter, 
		COL_SYMBOL, str, 
		COL_TYPE, ti68k_bkpt_type_to_string(BK_TYPE_CODE),
		COL_STATUS, BKPT_IS_ENABLED(addr) ? _("enabled") : _("disabled"),
		COL_START, str,
		COL_END, "",		
        COL_MODE, BKPT_IS_TMP(addr) ? _("one-shot") : "",
		COL_DATA, l->data,
		-1);
/*
		if(options3.dbg_font_type)
			gtk_list_store_set(store, &iter, COL_FONT, options3.dbg_font_name, -1);
*/		
		g_free(str);
	}

	// Vector breakpoints
	for(l = bkpts.exception; l != NULL; l = g_list_next(l))
	{
		gint n = GPOINTER_TO_INT(l->data);
		gchar *str1, *str2;
		gint bn = BKPT_ADDR(n);
		
		str1 = g_strdup_printf("#%i - %s", bn, ti68k_exception_to_string(bn));
		str2 = g_strdup_printf("0x%06x", 4 * bn);
		
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 
		COL_SYMBOL, str1, 
		COL_TYPE, ti68k_bkpt_type_to_string(BK_TYPE_EXCEPTION),
		COL_STATUS, BKPT_IS_ENABLED(n) ? _("enabled") : _("disabled"),
		COL_START, str2,
		COL_END, "",		
		COL_MODE, "",
		COL_DATA, l->data,
		-1);
/*
		if(options3.dbg_font_type)
			gtk_list_store_set(store, &iter, COL_FONT, options3.dbg_font_name, -1);
*/		
		g_free(str1);
		g_free(str2);
	}
	
	// Memory access breakpoints
	for(i = 0; i < 6; i++)
	{
		for(l = *bkpts_mem_access[i]; l != NULL; l = g_list_next(l))
		{
			uint32_t addr = GPOINTER_TO_INT(l->data);
			gchar *str;
			
			str = g_strdup_printf("0x%06x", BKPT_ADDR(addr));
			
			gtk_list_store_append(store, &iter);
			gtk_list_store_set(store, &iter, 
			COL_SYMBOL, str, 
			COL_TYPE, ti68k_bkpt_type_to_string(BK_TYPE_ACCESS),
			COL_STATUS, BKPT_IS_ENABLED(addr) ? _("enabled") : _("disabled"),
			COL_START, str,
			COL_END, "",
            COL_MODE, ti68k_bkpt_mode_to_string(BK_TYPE_ACCESS, bkpts_memacc_rw[i]),
			COL_DATA, l->data,
			-1);
/*
			if(options3.dbg_font_type)
				gtk_list_store_set(store, &iter, COL_FONT, options3.dbg_font_name, -1);
*/			
			g_free(str);
		}
	}
	
	// Memory range breakpoints
	for(i = 0; i < 2; i++)
	{
		for(l = *(bkpts_mem_range[i]); l != NULL; l = g_list_next(l))
		{
			ADDR_RANGE *s = l->data;
			gchar *str1, *str2;
			
			str1 = g_strdup_printf("0x%06x", BKPT_ADDR(s->val1));
			str2 = g_strdup_printf("0x%06x", BKPT_ADDR(s->val2));
			
			gtk_list_store_append(store, &iter);
			gtk_list_store_set(store, &iter, 
			COL_SYMBOL, str1, 
			COL_TYPE, ti68k_bkpt_type_to_string(BK_TYPE_RANGE),
			COL_STATUS, BKPT_IS_ENABLED(s->val1) ? _("enabled") : _("disabled"),
			COL_START, str1,
			COL_END, str2,
            COL_MODE, ti68k_bkpt_mode_to_string(BK_TYPE_RANGE, bkpts_memrng_rw[i]),
			COL_DATA, l->data,
			-1);
/*
			if(options3.dbg_font_type)
				gtk_list_store_set(store, &iter, COL_FONT, options3.dbg_font_name, -1);
*/			
			g_free(str1);
			g_free(str2);
		}
	}
	
	// Bit change breakpoints
	for(l = bkpts.bits; l != NULL; l = g_list_next(l))
	{
		ADDR_BIT *s = l->data;
		gchar *str;

		str = g_strdup_printf("0x%06x", BKPT_ADDR(s->addr));

		gtk_list_store_append(store, &iter);
			gtk_list_store_set(store, &iter, 
			COL_SYMBOL, str, 
			COL_TYPE, ti68k_bkpt_type_to_string(BK_TYPE_BIT),
			COL_STATUS, BKPT_IS_ENABLED(s->addr) ? _("enabled") : _("disabled"),
			COL_START, str,
			COL_END, "",
            COL_MODE, "",
			COL_DATA, l->data,
			-1);
/*
			if(options3.dbg_font_type)
				gtk_list_store_set(store, &iter, COL_FONT, options3.dbg_font_name, -1);
*/
		g_free(str);
	}

	// parse list to merge informations: ugly/heavy code !
	for(valid = gtk_tree_model_get_iter_first(model, &iter);
        valid; 
        valid = gtk_tree_model_iter_next(model, &iter))
    {
		gchar** row_text = g_malloc0((CLIST_NVCOLS + 1) * sizeof(gchar *));

		gtk_tree_model_get(model, &iter, 
				COL_SYMBOL, &row_text[COL_SYMBOL], 
				COL_TYPE, &row_text[COL_TYPE], 
				COL_START, &row_text[COL_START], 
				COL_END, &row_text[COL_END],
				COL_MODE, &row_text[COL_MODE],
				-1);
		mode = ti68k_string_to_bkpt_mode(row_text[COL_MODE]);

		memcpy(&iter2, &iter, sizeof(GtkTreeIter));
		for(valid2 = gtk_tree_model_iter_next(model, &iter2);
			valid2; 
			removed = FALSE)
		{
				gchar** row_text2 = g_malloc0((CLIST_NVCOLS + 1) * sizeof(gchar *));

				gtk_tree_model_get(model, &iter2, 
				COL_SYMBOL, &row_text2[COL_SYMBOL], 
				COL_TYPE, &row_text2[COL_TYPE], 
				COL_START, &row_text2[COL_START], 
				COL_END, &row_text2[COL_END],
				COL_MODE, &row_text2[COL_MODE],
				-1);
				mode2 = ti68k_string_to_bkpt_mode(row_text2[COL_MODE]);

				if(!strcmp(row_text2[COL_TYPE], _("access")) || !strcmp(row_text2[COL_TYPE], _("range")))
				{
					if(!strcmp(row_text[COL_START], row_text2[COL_START]) && !strcmp(row_text[COL_END], row_text2[COL_END]))
					{
						if( ((mode & BK_READ) && (mode2 & BK_WRITE)) || ((mode & BK_WRITE) && (mode2 & BK_READ)) )
						{
							const gchar *new_str;
							
							new_str = ti68k_bkpt_mode_to_string(0, mode | BK_RW);
							gtk_list_store_set(store, &iter, COL_MODE, new_str, -1);

							valid2 = gtk_list_store_remove(store, &iter2);
							removed = TRUE;
						}						
					}
				}

				g_strfreev(row_text2);

				if(!removed)
					valid2 = gtk_tree_model_iter_next(model, &iter2);
		}		

		g_strfreev(row_text);
	}

	// Prgm entry breakpoints
	for(l = bkpts.pgmentry; l != NULL; l = g_list_next(l))
	{
		uint32_t addr = GPOINTER_TO_INT(l->data);
		gchar *str;
		
		str = g_strdup_printf("#%04x", BKPT_ADDR(addr) >> 16);
		
		gtk_list_store_append(store, &iter);

		gtk_list_store_set(store, &iter, 
		COL_SYMBOL, str, 
		COL_TYPE, ti68k_bkpt_type_to_string(BK_TYPE_PGMENTRY),
		COL_STATUS, BKPT_IS_ENABLED(addr) ? _("enabled") : _("disabled"),
		COL_START, str,
		COL_END, "",		
        COL_MODE, BKPT_IS_TMP(addr) ? _("one-shot") : "",
		COL_DATA, l->data,
		-1);
/*
		if(options3.dbg_font_type)
			gtk_list_store_set(store, &iter, COL_FONT, options3.dbg_font_name, -1);
*/		
		g_free(str);
	}
}

static GtkListStore *store = NULL;

/*
	Display registers window
*/
GtkWidget* dbgbkpts_create_window(void)
{
	GtkWidget *dbox;
    GtkWidget *data;
	
	xml = glade_xml_new
		(tilp_paths_build_glade("dbg_bkpts-2.glade"), "dbgbkpts_window",
		 PACKAGE);
	if (!xml)
		g_error(_("%s: GUI loading failed!\n"), __FILE__);
	glade_xml_signal_autoconnect(xml);
	
	dbox = glade_xml_get_widget(xml, "dbgbkpts_window");
	if(options3.transient)
		gtk_window_set_transient_for(GTK_WINDOW(dbox), GTK_WINDOW(main_wnd));
	
	data = glade_xml_get_widget(xml, "treeview1");
    store = clist_create(data);
	clist_populate(store);

	gtk_tree_view_expand_all(GTK_TREE_VIEW(data));

	return wnd = dbox;
}

GtkWidget* dbgbkpts_display_window(void)
{
#ifdef WND_STATE
	if(!options3.bkpts.minimized)
	{
		gtk_window_resize(GTK_WINDOW(wnd), options3.bkpts.rect.w, options3.bkpts.rect.h);
		gtk_window_move(GTK_WINDOW(wnd), options3.bkpts.rect.x, options3.bkpts.rect.y);
	}
	else
		gtk_window_iconify(GTK_WINDOW(wnd));
#endif

	if(!GTK_WIDGET_VISIBLE(dbgw.bkpts) && !options3.bkpts.closed)
		gtk_widget_show(wnd);

	return wnd;
}

void dbgbkpts_refresh_window(void)
{
	WND_TMR_START();

	if(!options3.bkpts.closed)
	{
		gtk_list_store_clear(store);
		clist_populate(store);

		display_dbgcause_dbox2(glade_get("statusbar1"));
	}

	WND_TMR_STOP("Breakpoints Refresh Time");
}

void dbgbkpts_erase_context(void)
{
	display_dbgcause_dbox2(glade_get("statusbar1"));
}

static GtkWidget* display_dbgbkpts_popup_menu(void)
{
	GladeXML *xml;
	GtkWidget *data;
  
	xml = glade_xml_new
	    (tilp_paths_build_glade("dbg_bkpts-2.glade"), "dbgbkpts_popup",
	     PACKAGE);
	if (!xml)
		g_error(_("dbg_bkpts-2.glade: GUI loading failed !\n"));
	glade_xml_signal_autoconnect(xml);

	data = glade_xml_get_widget(xml, "dbgbkpts_popup");

	return data;
}

/* Add bkpt */
GLADE_CB void
dbgbkpts_button1_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *menu;
	guint butt = 0;
	guint32 time;

	time = gtk_get_current_event_time();
	menu = display_dbgbkpts_popup_menu();
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, butt, time);
	gtk_widget_show(menu);
}

/* Remove bkpt */
GLADE_CB void
dbgbkpts_button2_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *list = GTK_WIDGET(button);   // arg are swapped, why ?
	GtkTreeView *view = GTK_TREE_VIEW(list);
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GList *l;
	
	// get selection
	selection = gtk_tree_view_get_selection(view);
	for (l = gtk_tree_selection_get_selected_rows(selection, &model);
	     l != NULL; l = l->next) 
	{
		GtkTreeIter iter;
		GtkTreePath *path = l->data;
        gchar** row_text = g_malloc0((CLIST_NVCOLS + 1) * sizeof(gchar *));
        uint32_t n, type, min, max, mode;
			
		gtk_tree_model_get_iter(model, &iter, path);
		gtk_tree_model_get(model, &iter, 
            COL_SYMBOL, &row_text[COL_SYMBOL], 
            COL_TYPE, &row_text[COL_TYPE], 
            COL_START, &row_text[COL_START], 
            COL_END, &row_text[COL_END],
            COL_MODE, &row_text[COL_MODE],
            -1);
		
        type = ti68k_string_to_bkpt_type(row_text[COL_TYPE]);
        switch(type)
        {
        case BK_TYPE_CODE:
            sscanf(row_text[COL_START], "%x", &min);
            ti68k_bkpt_del_address(min);
			dbgcode_refresh_window();
            break;
        case BK_TYPE_EXCEPTION:
            sscanf(row_text[COL_SYMBOL], "#%i", &n);
            ti68k_bkpt_del_exception(n);
            break;
        case BK_TYPE_ACCESS:
            mode = ti68k_string_to_bkpt_mode(row_text[COL_MODE]);
            sscanf(row_text[COL_START], "%x", &min);
            ti68k_bkpt_del_access(min, mode);
            break;
        case BK_TYPE_RANGE:
            mode = ti68k_string_to_bkpt_mode(row_text[COL_MODE]);
            sscanf(row_text[COL_START], "%x", &min);
            sscanf(row_text[COL_END], "%x", &max);
            ti68k_bkpt_del_range(min, max, mode);
            break;
		case BK_TYPE_PGMENTRY:
			sscanf(row_text[COL_SYMBOL], "#%04x", &n);
			ti68k_bkpt_del_pgmentry((uint16_t)n);
			break;
		case BK_TYPE_BIT:
			mode = ti68k_string_to_bkpt_mode(row_text[COL_MODE]);
            sscanf(row_text[COL_START], "%x", &min);
            ti68k_bkpt_del_bits(min);
        }
        g_strfreev(row_text);
    }

	// free selection
	g_list_foreach (l, (GFunc)gtk_tree_path_free, NULL);
	g_list_free (l);

    dbgbkpts_refresh_window();
}


/* Disable bkpt */
GLADE_CB void
dbgbkpts_button3_clicked                     (GtkButton       *button,
                                              GtkWidget       *widget,
                                              gpointer         user_data)
{
	GtkWidget *list = GTK_WIDGET(button);   // arg are swapped, why ?
	GtkTreeView *view = GTK_TREE_VIEW(list);
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GList *l;
	
	// get selection
	selection = gtk_tree_view_get_selection(view);
	for (l = gtk_tree_selection_get_selected_rows(selection, &model);
	     l != NULL; l = l->next) 
	{
		GtkTreeIter iter;
		GtkTreePath *path = l->data;
        gchar** row_text = g_malloc0((CLIST_NVCOLS + 1) * sizeof(gchar *));
        uint32_t n, type, min, max, mode;
			
		gtk_tree_model_get_iter(model, &iter, path);
		gtk_tree_model_get(model, &iter, 
            COL_SYMBOL, &row_text[COL_SYMBOL], 
            COL_TYPE, &row_text[COL_TYPE], 
            COL_START, &row_text[COL_START], 
            COL_END, &row_text[COL_END],
            COL_MODE, &row_text[COL_MODE],
            -1);
		
        type = ti68k_string_to_bkpt_type(row_text[COL_TYPE]);
        switch(type)
        {
        case BK_TYPE_CODE:
            sscanf(row_text[COL_START], "%x", &min);
            {
              uint32_t addr = BKPT_ADDR(min);
              ti68k_bkpt_set_address(addr, BKPT_DISABLE(min));
            }
            break;
        case BK_TYPE_EXCEPTION:
            sscanf(row_text[COL_SYMBOL], "#%i", &n);
            {
              uint32_t addr = BKPT_ADDR(n);
              ti68k_bkpt_set_exception(addr, BKPT_DISABLE(n));
            }
            break;
        case BK_TYPE_ACCESS:
            mode = ti68k_string_to_bkpt_mode(row_text[COL_MODE]);
            sscanf(row_text[COL_START], "%x", &min);
            {
              uint32_t addr = BKPT_ADDR(min);
              ti68k_bkpt_set_access(addr, mode, BKPT_DISABLE(min));
            }
            break;
        case BK_TYPE_RANGE:
            mode = ti68k_string_to_bkpt_mode(row_text[COL_MODE]);
            sscanf(row_text[COL_START], "%x", &min);
            sscanf(row_text[COL_END], "%x", &max);
            {
              uint32_t addr1 = BKPT_ADDR(min);
              uint32_t addr2 = BKPT_ADDR(max);
              ti68k_bkpt_set_range(addr1, addr2, mode,
                                   BKPT_DISABLE(min), BKPT_DISABLE(max));
            }
            break;
        case BK_TYPE_PGMENTRY:
            //sscanf(row_text[COL_SYMBOL], "#%04x", &n);
            //{
            //  uint32_t addr = BKPT_ADDR(n);
            //  ti68k_bkpt_set_pgmentry(addr, BKPT_DISABLE(n));
            //}
            break;
		case BK_TYPE_BIT:
			mode = ti68k_string_to_bkpt_mode(row_text[COL_MODE]);
            sscanf(row_text[COL_START], "%x", &min);
            {
              uint32_t addr = BKPT_ADDR(min);
              ti68k_bkpt_set_bits(addr, BKPT_DISABLE(addr));
            }
			break;

        }
        g_strfreev(row_text);
    }

	// free selection
	g_list_foreach (l, (GFunc)gtk_tree_path_free, NULL);
	g_list_free (l);

    dbgbkpts_refresh_window();
}


/* Enable bkpt */
GLADE_CB void
dbgbkpts_button4_clicked                     (GtkButton       *button,
                                              gpointer         user_data)
{
	GtkWidget *list = GTK_WIDGET(button);   // arg are swapped, why ?
	GtkTreeView *view = GTK_TREE_VIEW(list);
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GList *l;
	
	// get selection
	selection = gtk_tree_view_get_selection(view);
	for (l = gtk_tree_selection_get_selected_rows(selection, &model);
	     l != NULL; l = l->next) 
	{
		GtkTreeIter iter;
		GtkTreePath *path = l->data;
        gchar** row_text = g_malloc0((CLIST_NVCOLS + 1) * sizeof(gchar *));
        uint32_t n, type, min, max, mode;
			
		gtk_tree_model_get_iter(model, &iter, path);
		gtk_tree_model_get(model, &iter, 
            COL_SYMBOL, &row_text[COL_SYMBOL], 
            COL_TYPE, &row_text[COL_TYPE], 
            COL_START, &row_text[COL_START], 
            COL_END, &row_text[COL_END],
            COL_MODE, &row_text[COL_MODE],
            -1);
		
        type = ti68k_string_to_bkpt_type(row_text[COL_TYPE]);
        switch(type)
        {
        case BK_TYPE_CODE:
            sscanf(row_text[COL_START], "%x", &min);
            {
              uint32_t addr = BKPT_ADDR(min);
              ti68k_bkpt_set_address(addr, BKPT_ENABLE(min));
            }
            break;
        case BK_TYPE_EXCEPTION:
            sscanf(row_text[COL_SYMBOL], "#%i", &n);
            {
              uint32_t addr = BKPT_ADDR(n);
              ti68k_bkpt_set_exception(addr, BKPT_ENABLE(n));
            }
            break;
        case BK_TYPE_ACCESS:
            mode = ti68k_string_to_bkpt_mode(row_text[COL_MODE]);
            sscanf(row_text[COL_START], "%x", &min);
            {
              uint32_t addr = BKPT_ADDR(min);
              ti68k_bkpt_set_access(addr, mode, BKPT_ENABLE(min));
            }
            break;
        case BK_TYPE_RANGE:
            mode = ti68k_string_to_bkpt_mode(row_text[COL_MODE]);
            sscanf(row_text[COL_START], "%x", &min);
            sscanf(row_text[COL_END], "%x", &max);
            {
              uint32_t addr1 = BKPT_ADDR(min);
              uint32_t addr2 = BKPT_ADDR(max);
              ti68k_bkpt_set_range(addr1, addr2, mode,
                                   BKPT_ENABLE(min), BKPT_ENABLE(max));
            }
            break;
        case BK_TYPE_PGMENTRY:
            //sscanf(row_text[COL_SYMBOL], "#%04x", &n);
            //{
            //  uint32_t addr = BKPT_ADDR(n);
            //  ti68k_bkpt_set_pgmentry(addr, BKPT_ENABLE(n));
            //}
            break;
		case BK_TYPE_BIT:
			mode = ti68k_string_to_bkpt_mode(row_text[COL_MODE]);
            sscanf(row_text[COL_START], "%x", &min);
            {
              uint32_t addr = BKPT_ADDR(min);
              ti68k_bkpt_set_bits(addr, BKPT_ENABLE(addr));
            }
			break;
        }
        g_strfreev(row_text);
    }

	// free selection
	g_list_foreach (l, (GFunc)gtk_tree_path_free, NULL);
	g_list_free (l);

    dbgbkpts_refresh_window();
}


/* Go to bkpt address */
GLADE_CB void
dbgbkpts_button5_clicked                     (GtkButton       *button,
                                              gpointer         user_data)
{
	GtkWidget *list = GTK_WIDGET(button);   // arg are swapped, why ?
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
        uint32_t type, min, n, addr;
			
		gtk_tree_model_get_iter(model, &iter, path);
		gtk_tree_model_get(model, &iter, 
            COL_SYMBOL, &row_text[COL_SYMBOL], 
            COL_TYPE, &row_text[COL_TYPE], 
            COL_START, &row_text[COL_START], 
            COL_END, &row_text[COL_END],
            COL_MODE, &row_text[COL_MODE],
            -1);
		
		type = ti68k_string_to_bkpt_type(row_text[COL_TYPE]);
		switch(type)
        {
        case BK_TYPE_CODE:
            sscanf(row_text[COL_START], "%x", &min);
			addr = BKPT_ADDR(min);
          	dbgcode_disasm_at(addr);
            break;
        case BK_TYPE_EXCEPTION:
            sscanf(row_text[COL_SYMBOL], "#%i", &n);
			dbgcode_disasm_at(mem_rd_long(4*n));
            break;
        case BK_TYPE_ACCESS:
		case BK_TYPE_RANGE:
		case BK_TYPE_BIT:
            sscanf(row_text[COL_START], "%x", &min);
            addr = BKPT_ADDR(min);
            dbgmem_add_tab(addr);
            break;
        case BK_TYPE_PGMENTRY:
			{
				gpointer data;
				uint16_t handle, offset;

				gtk_tree_model_get(model, &iter, COL_DATA, &data, -1);

				handle = GPOINTER_TO_INT(data) >> 16;
				offset = GPOINTER_TO_INT(data) & 0xffff;
				addr = heap_deref(handle) + offset;

				dbgcode_disasm_at(addr);
			}
            break;
        }
        g_strfreev(row_text);
    }

	// free selection
	g_list_foreach (l, (GFunc)gtk_tree_path_free, NULL);
	g_list_free (l);
}

GLADE_CB gboolean
on_treeview2_button_press_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
	GtkTreeView *view = GTK_TREE_VIEW(widget);
    GtkTreeModel *model = gtk_tree_view_get_model(view);
	GtkTreePath *path;
	GtkTreeViewColumn *column;
    GtkTreeIter iter;
    gboolean ret;

    if (event->type != GDK_2BUTTON_PRESS)	// double-click ?
		return FALSE;
	else
	{
		// retrieve selection
		gint tx = (gint) event->x;
	    gint ty = (gint) event->y;
	    gint cx, cy;
		gchar** row_text = g_malloc0((CLIST_NVCOLS + 1) * sizeof(gchar *));
		uint32_t type;
		gpointer data;
		
        ret = gtk_tree_view_get_path_at_pos(view, tx, ty, &path, &column, &cx, &cy);
        if(ret == FALSE)
            return FALSE;

		if (!gtk_tree_model_get_iter(model, &iter, path))
		    return FALSE;
        gtk_tree_path_free(path);

        gtk_tree_model_get(model, &iter, 
            COL_SYMBOL, &row_text[COL_SYMBOL], 
            COL_TYPE, &row_text[COL_TYPE], 
            COL_START, &row_text[COL_START], 
            COL_END, &row_text[COL_END],
            COL_MODE, &row_text[COL_MODE],
			COL_DATA, &data,
            -1);
		
        type = ti68k_string_to_bkpt_type(row_text[COL_TYPE]);
		if(type == BK_TYPE_CODE)
		{
			uint32_t old_addr, new_addr;

			sscanf(row_text[COL_START], "%x", &old_addr);
			new_addr = old_addr;

			if(display_dbgmem_address(&new_addr) == -1)
				return TRUE;

			ti68k_bkpt_set_address(old_addr, new_addr);			
			dbgbkpts_refresh_window();
			dbgcode_refresh_window();
		}
		if((type == BK_TYPE_ACCESS) || (type == BK_TYPE_RANGE))
		{
			uint32_t old_min, old_max;
			uint32_t new_min, new_max;
			uint32_t new_type;
			uint32_t old_mode, new_mode;

            old_mode = ti68k_string_to_bkpt_mode(row_text[COL_MODE]);
            sscanf(row_text[COL_START], "%x", &old_min);
            sscanf(row_text[COL_END], "%x", &old_max);

            new_type = type; new_mode = old_mode; new_min = old_min; new_max = old_max;
			if(dbgdata_display_dbox(&new_mode, &new_type, &new_min, &new_max) == -1)
				return TRUE;

			if(type != new_type)
				return TRUE;

			if(new_type == BK_TYPE_ACCESS)
			{
				ti68k_bkpt_del_access(old_min, old_mode);
				ti68k_bkpt_add_access(new_min, new_mode);
				//ti68k_bkpt_set_access(old_min, mode, new_min);
			}
			else if(new_type == BK_TYPE_RANGE)
			{
				ti68k_bkpt_del_range(old_min, old_max, old_mode);
				ti68k_bkpt_add_range(new_min, new_max, new_mode);
				//ti68k_bkpt_set_range(old_min, old_max, mode, new_min, new_max);
			}
			dbgbkpts_refresh_window();
		}
		if(type == BK_TYPE_BIT)
		{
			uint32_t old_addr;
			ADDR_BIT *s = (ADDR_BIT *)data;

			sscanf(row_text[COL_START], "%x", &old_addr);

			if(dbgbits_display_dbox(&s->addr, &s->checks, &s->states) == -1)
				return TRUE;

			dbgbkpts_refresh_window();
		}

		g_strfreev(row_text);
                
		return TRUE;
    }

    return FALSE;
}

GLADE_CB void
dbgbkpts_bit_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	uint32_t addr = 0;
	uint8_t checks = 0, states = 0xff;

	if(dbgbits_display_dbox(&addr, &checks, &states) != 0)
		return;

	ti68k_bkpt_add_bits(addr, checks, states);

	dbgbkpts_refresh_window();
}

GLADE_CB void
dbgbkpts_data_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	uint32_t mode, type=-1, min, max;

	// fill infos
	if(dbgdata_display_dbox(&mode, &type, &min, &max) != 0)
		return;

	// add breakpoint
	if(type == BK_TYPE_ACCESS)
		ti68k_bkpt_add_access(min, mode) ;
	else if(type == BK_TYPE_RANGE)
		ti68k_bkpt_add_range(min, max, mode);

	// and refresh
	dbgbkpts_refresh_window();
}


GLADE_CB void
dbgbkpts_vector_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	dbgvectors_display_dbox();
}

GLADE_CB void
dbgbkpts_prgmentry1_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	dbgentry_display_dbox();
}


