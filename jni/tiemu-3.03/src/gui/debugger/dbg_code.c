/* Hey EMACS -*- linux-c -*- */
/* $Id: dbg_code.c 2825 2009-05-06 19:48:47Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Liévin
 *  Copyright (c) 2005-2008, Romain Liévin, Kevin Kofler
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
#include <string.h>

#include "intl.h"
#include "paths.h"
#include "support.h"
#include "ti68k_int.h"
#include "struct.h"
#include "dbg_wnds.h"
#include "dbg_all.h"
#include "dbg_romcall.h"
#include "engine.h"
#include "logger/log_link.h"
#include "popup.h"

gint reset_disabled = FALSE;

//#define FIXED_SIZE
//#define RUN_DBG_OPEN

enum { 
	    COL_ICON, COL_ADDR, COL_OPCODE, COL_OPERAND,
        COL_HEXADDR, COL_FONT,
};
#define CLIST_NVCOLS	(4)		// 4 visible columns
#define CLIST_NCOLS		(6)		// 7 real columns

#ifdef FIXED_SIZE
#define NLINES      10
#else
static gint NLINES = 10;
#endif

GladeXML *xml = NULL;

static GtkListStore* clist_create(GtkWidget *widget)
{
	GtkTreeView *view = GTK_TREE_VIEW(widget);
	GtkListStore *store;
	GtkTreeModel *model;
	GtkCellRenderer *renderer;
	GtkTreeSelection *selection;
    const gchar *text[CLIST_NVCOLS] = { "  ", _("Address"), _("Opcode"), _("Operand") };
    gint i;
	
	store = gtk_list_store_new(CLIST_NCOLS,
				GDK_TYPE_PIXBUF, 
                G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
                G_TYPE_INT, G_TYPE_STRING,
				-1
            );
    model = GTK_TREE_MODEL(store);
	
    gtk_tree_view_set_model(view, model); 
    gtk_tree_view_set_headers_visible(view, TRUE);
	gtk_tree_view_set_rules_hint(view, FALSE);
  
  	renderer = gtk_cell_renderer_pixbuf_new();
	set_renderer_pad(renderer);
	gtk_tree_view_insert_column_with_attributes(view, -1, 
            text[COL_ICON], renderer, 
            "pixbuf", COL_ICON,
			NULL);
  
	for(i = COL_ADDR; i <= COL_OPERAND; i++)
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
#if 0
		gtk_tree_view_column_set_resizable(col, FALSE);
		gtk_tree_view_column_set_sizing (col, GTK_TREE_VIEW_COLUMN_FIXED);
		gtk_tree_view_column_set_fixed_width(col, 100);
		gtk_tree_view_set_fixed_height_mode(view, TRUE);		
#endif
	}
	
	selection = gtk_tree_view_get_selection(view);
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);

	return store;
}

static void clist_populate(GtkListStore *store, uint32_t addr)
{
    GtkTreeIter iter;
    GdkPixbuf *pix;
    gint i;
    uint32_t pc = ti68k_debug_get_pc();

	/* We can't start disassembling stuff before GDB is even loaded, and in the
	   cases where we have GDB running, but not the debugger, it's just a waste
	   of time. */
	if (!dbg_on) return;

    for(i = 0; i < NLINES; i++)
    {
        gchar *output;
        int offset;
		gchar** split;
        gchar** row_text = g_malloc0((CLIST_NVCOLS + 1) * sizeof(gchar *));
        uint32_t value;
        
        // disassemble at 'addr' address into 'output' and returns offset to the
        // next instruction address
        offset = ti68k_debug_disassemble(addr, &output);

        split = g_strsplit(output, " ", 3);
		g_free(output);

		row_text[0] = g_strdup(split[0]);
		sscanf(row_text[0], "%x", &value);
        if(split[1] == NULL)
            row_text[1] = g_strdup("");
		else
			row_text[1] = g_strdup(split[1]);
		if(split[2] == NULL)
            row_text[2] = g_strdup("");
		else
			row_text[2] = g_strdup(split[2]);

		if((g_list_find(bkpts.code, GINT_TO_POINTER(addr)) != NULL) && (addr != pc) ||
			(g_list_find(bkpts.code, GINT_TO_POINTER(addr | BKPT_TMP_MASK)) != NULL) && (addr != pc))
            pix = create_pixbuf("bkpt.xpm");
		else if((g_list_find(bkpts.code, GINT_TO_POINTER(addr)) != NULL) && (addr == pc) ||
			(g_list_find(bkpts.code, GINT_TO_POINTER(addr | BKPT_TMP_MASK)) != NULL) && (addr == pc))
			pix = create_pixbuf("run_2.xpm");
		else if(addr == pc)
			pix = create_pixbuf("run_1.xpm");
        else
            pix = create_pixbuf("void.xpm");

        gtk_list_store_append(store, &iter);
	    gtk_list_store_set(store, &iter, 
        COL_ICON, pix,
	    COL_ADDR, row_text[0], 
		COL_OPCODE, row_text[1],
        COL_OPERAND, row_text[2],
        COL_HEXADDR, value,
		-1);

		if(options3.dbg_font_type)
			gtk_list_store_set(store, &iter, COL_FONT, options3.dbg_font_name, -1);

        addr += offset;
        g_strfreev(split);
		g_strfreev(row_text);
		g_object_unref(pix);
    }
}

static void cyccnt_refresh(GtkWidget *l1, GtkWidget *l2)
{
	unsigned int count, diff;
	gchar *str1, *str2;

	count = ti68k_get_cycle_count(0, &diff);
	str1 = g_strdup_printf("%u", count);
	str2 = g_strdup_printf("%2u", diff);

	gtk_label_set_text(GTK_LABEL(l1), str1);
	gtk_label_set_text(GTK_LABEL(l2), str2);

	g_free(str1);
	g_free(str2);
}

static void clist_refresh(GtkListStore *store, gboolean reload)
{
    GtkTreeModel *model = GTK_TREE_MODEL(store);
    gboolean valid;
    GtkTreeIter iter;

    uint32_t addr;
    uint32_t pc, old_pc;
    int found = 0;

    const int offset = 3;   // 3 instructions are still visible
    uint32_t addr3;
    gint i;

	// Data/Bit bkpt encounter after instruction execution so take care of this
	addr3 = pc = ti68k_debug_get_pc();
	old_pc = ti68k_debug_get_old_pc();

    // check for refresh (search for pc)
    for(valid = gtk_tree_model_get_iter_first(model, &iter), i = 0;
        valid; 
        valid = gtk_tree_model_iter_next(model, &iter), i++)
    {
        gchar *str;

        gtk_tree_model_get(model, &iter, COL_ADDR, &str, -1);
        sscanf(str, "%x", &addr);

        if(i == offset)
            addr3 = addr;

        if(addr == pc)
            found = i+1;

        g_free(str);
    }

	// pc not found, erase and populate
    if(!found && reload)
    {
        gtk_list_store_clear(store);
		//printf("%06x %06x %i\n", old_pc, pc, abs(old_pc - pc));
        clist_populate(store, abs((old_pc - pc)) > 8 ? pc : old_pc);
    }

    // repopulate so that 3 instructions are still visible at the bottom of the list
    if(found && reload)
    {
        if(NLINES - found < offset)
        {
            gtk_list_store_clear(store);
            clist_populate(store, addr3);
        }
    }

    // look for pc and matching bkpt
    for(valid = gtk_tree_model_get_iter_first(model, &iter);
        valid; 
        valid = gtk_tree_model_iter_next(model, &iter))
    {
        GdkPixbuf *pix;
        gchar *str;

        gtk_tree_model_get(model, &iter, COL_ADDR, &str, -1);
        sscanf(str, "%x", &addr);

        if(((g_list_find(bkpts.code, GINT_TO_POINTER(addr)) != NULL) && (addr != pc) ||
			(g_list_find(bkpts.code, GINT_TO_POINTER(addr | BKPT_TMP_MASK)) != NULL) && (addr != pc)))
            pix = create_pixbuf("bkpt.xpm");
		else if(((g_list_find(bkpts.code, GINT_TO_POINTER(addr)) != NULL) && (addr == pc) ||
			(g_list_find(bkpts.code, GINT_TO_POINTER(addr | BKPT_TMP_MASK)) != NULL) && (addr == pc)))
            pix = create_pixbuf("run_2.xpm");
		else if(addr == pc)
			pix = create_pixbuf("run_1.xpm");
        else
            pix = create_pixbuf("void.xpm");

        gtk_list_store_set(store, &iter, COL_ICON, pix, -1);
        g_free(str);
		g_object_unref(pix);
    }

	// update cycle counter
	cyccnt_refresh(glade_get("label3"), glade_get("label4"));
}

static GtkWidget *list;
static GtkListStore *store;

typedef struct {
	GtkWidget *b1;
	GtkWidget *b2;
	GtkWidget *b3;
	GtkWidget *b4;
	GtkWidget *b5;
	GtkWidget *b6;
	GtkWidget *b7;
} TB;
static TB tb;

typedef struct {
    GtkWidget *m1;
    GtkWidget *m2;
    GtkWidget *m3;
    GtkWidget *m4;
    GtkWidget *m5;
	GtkWidget *m6;
	GtkWidget *m8;
} MI;
static MI mi;

static void tb_set_states(int s1, int s2, int s3, int s4, int s5, int s6, int s7, int s8)
{
	gtk_widget_set_sensitive(tb.b1, s1);
	gtk_widget_set_sensitive(tb.b2, s2);
	gtk_widget_set_sensitive(tb.b3, s3);
	gtk_widget_set_sensitive(tb.b4, s4);
	gtk_widget_set_sensitive(tb.b5, s5);
	gtk_widget_set_sensitive(tb.b6, s6);
	gtk_widget_set_sensitive(tb.b7, s7);

    gtk_widget_set_sensitive(mi.m1, s1);
	gtk_widget_set_sensitive(mi.m2, s2);
	gtk_widget_set_sensitive(mi.m3, s3);
	gtk_widget_set_sensitive(mi.m4, s4);
	gtk_widget_set_sensitive(mi.m5, s5);
	gtk_widget_set_sensitive(mi.m6, s6);
	gtk_widget_set_sensitive(mi.m8, s8);
}

/*
	Display source code window
*/

extern int update_submenu(GtkWidget*, gpointer);	//dbg_wnds.c

GtkWidget* dbgcode_create_window(void)
{
	GtkWidget *dbox;
    GtkWidget *data;	
	
	xml = glade_xml_new
		(tilp_paths_build_glade("dbg_code-2.glade"), "dbgcode_window",
		 PACKAGE);
	if (!xml)
		g_error(_("%s: GUI loading failed!\n"), __FILE__);
	glade_xml_signal_autoconnect(xml);

	dbox = glade_xml_get_widget(xml, "dbgcode_window");
	if(options3.transient)
		gtk_window_set_transient_for(GTK_WINDOW(dbox), GTK_WINDOW(main_wnd));

	g_object_set(G_OBJECT(dbox), "allow-shrink", TRUE, NULL);

	/*
	{
		GtkStyle *copy;

		GtkStyle *style = gtk_rc_get_style(dbox);
		printf("=>> %i\n", style->ythickness);

		copy =  gtk_style_copy(style);
		gtk_style_detach(style);

		style->ythickness = 0;

		gtk_widget_realize(dbox);
		gtk_style_attach(copy, dbox);		
	}
	*/

    data = glade_xml_get_widget(xml, "windows1_menu");
    g_signal_connect(G_OBJECT(data), "map", G_CALLBACK(update_submenu), NULL);

	tb.b1 = glade_xml_get_widget(xml, "button1");
	tb.b2 = glade_xml_get_widget(xml, "button2");
	tb.b3 = glade_xml_get_widget(xml, "button3");
	tb.b4 = glade_xml_get_widget(xml, "button4");
	tb.b5 = glade_xml_get_widget(xml, "button5");
	tb.b6 = glade_xml_get_widget(xml, "button6");
	tb.b7 = glade_xml_get_widget(xml, "button7");

    mi.m1 = glade_xml_get_widget(xml, "run1");
    mi.m2 = glade_xml_get_widget(xml, "step1");
    mi.m3 = glade_xml_get_widget(xml, "step_over1");
	mi.m4 = glade_xml_get_widget(xml, "step_out1");
    mi.m5 = glade_xml_get_widget(xml, "run_to_cursor1");
    mi.m6 = glade_xml_get_widget(xml, "break1");
    mi.m8 = glade_xml_get_widget(xml, "quit1");

	list = glade_xml_get_widget(xml, "treeview1");
    store = clist_create(list);
	clist_populate(store, ti68k_debug_get_pc());

	gtk_tree_view_expand_all(GTK_TREE_VIEW(list));
	gtk_widget_show(list);

	data = glade_xml_get_widget(xml, "progressbar1");
	gtk_widget_hide(data);

	dbgromcall_create_window(xml);
	dbgromcall_refresh_window();

	return dbox;
}

GtkWidget* dbgcode_display_window(void)
{
#ifdef WND_STATE
	if(!options3.code.minimized)
	{
		gtk_window_resize(GTK_WINDOW(dbgw.code), options3.code.rect.w, options3.code.rect.h);
		gtk_window_move(GTK_WINDOW(dbgw.code), options3.code.rect.x, options3.code.rect.y);
	}
	else
		gtk_window_iconify(GTK_WINDOW(dbgw.code));
#endif

	gtk_widget_set_sensitive(list, TRUE);	
	tb_set_states(1, 1, 1, 1, 1, 0, 1, 1);
	reset_disabled = FALSE;
	gtk_debugger_enable();

	gtk_widget_show(dbgw.code);

    return dbgw.code;
}

void dbgcode_refresh_window(void)
{
	WND_TMR_START();

	if(!options3.code.closed)
	{
		gtk_list_store_clear(store);
		clist_refresh(store, TRUE);
	}

	WND_TMR_STOP("Code Refresh Time");
}

void dbgcode_disasm_at(uint32_t addr)
{
	GtkTreeView *view = GTK_TREE_VIEW(list);
	GtkTreePath *path;
	//GtkTreeViewColumn *column;

    gtk_list_store_clear(store);
    clist_populate(store, addr);

	//set selection
	path = gtk_tree_path_new_from_string("0");
    gtk_tree_view_set_cursor(view, path, NULL, FALSE);
	//gtk_tree_view_row_activated(view, path, column);        // show selection
    gtk_tree_path_free(path);
}


GLADE_CB void
on_run1_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
#ifdef RUN_DBG_OPEN
	tb_set_states(0, 0, 0, 0, 0, 1, 0, 0);
	reset_disabled = TRUE;
    gtk_widget_set_sensitive(list, FALSE);
    gtk_debugger_disable();

	ti68k_debug_step();	// skip possible current bkpt
    engine_start();
#else
	gtk_debugger_close();
#endif
}


GLADE_CB void
on_step1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	ti68k_debug_step();

	clist_refresh(store, TRUE);
    dbgregs_refresh_window();
	dbgpclog_refresh_window();
    dbgmem_refresh_window();
	dbgstack_refresh_window();
	dbgheap_refresh_window();
}


GLADE_CB void
on_step_over1_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	tb_set_states(0, 0, 0, 0, 0, 1, 0, 0);
	reset_disabled = TRUE;
    ti68k_debug_step_over();
	tb_set_states(1, 1, 1, 1, 1, 0, 1, 1);
	reset_disabled = FALSE;

	clist_refresh(store, TRUE);
    dbgregs_refresh_window();
	dbgpclog_refresh_window();
    dbgmem_refresh_window();
	dbgstack_refresh_window();
	dbgheap_refresh_window();
}

GLADE_CB void
on_step_out1_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    ti68k_debug_step_out();

	clist_refresh(store, TRUE);
    dbgregs_refresh_window();
	dbgpclog_refresh_window();
    dbgmem_refresh_window();
	dbgstack_refresh_window();
	dbgheap_refresh_window();
}


GLADE_CB void
on_run_to_cursor1_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GtkTreeView *view = GTK_TREE_VIEW(list);
	GtkTreeModel *model = gtk_tree_view_get_model(view);
	GtkListStore *store = GTK_LIST_STORE(model);
    GtkTreeSelection *selection;
    GtkTreeIter iter;
    gboolean valid;
    gchar *str;
    uint32_t addr;

    selection = gtk_tree_view_get_selection(view);
    valid = gtk_tree_selection_get_selected(selection, NULL, &iter);
	if(!valid) return;

    // Get address to go
    gtk_tree_model_get(model, &iter, COL_ADDR, &str, -1);
    sscanf(str, "%x", &addr);

	tb_set_states(0, 0, 0, 0, 0, 1, 0, 0);
	reset_disabled = TRUE;
    gtk_debugger_disable();

    ti68k_debug_skip(addr);
    gtk_tree_selection_unselect_iter(selection, &iter);

	tb_set_states(1, 1, 1, 1, 1, 0, 1, 1);
	reset_disabled = FALSE;
    gtk_debugger_enable();
    
	clist_refresh(store, FALSE);
    dbgregs_refresh_window();
	dbgpclog_refresh_window();
    dbgmem_refresh_window();
	dbgstack_refresh_window();
	dbgheap_refresh_window();
}


GLADE_CB void
on_break1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    // Mode 1 is fastest
#ifdef RUN_DBG_OPEN
	GtkTreeView *view = GTK_TREE_VIEW(list);
	GtkTreeModel *model = gtk_tree_view_get_model(view);
	GtkListStore *store = GTK_LIST_STORE(model);

    engine_stop();
    gtk_widget_set_sensitive(list, TRUE);
	tb_set_states(1, 1, 1, 1, 1, 0, 1, 1);
	reset_disabled = FALSE;
    gtk_debugger_enable();
    clist_refresh(store, TRUE);
#else
    ti68k_debug_break();
#endif
}


// Toggle breakpoint
GLADE_CB void
dbgcode_button6_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkTreeView *view = GTK_TREE_VIEW(list);
	GtkTreeModel *model = gtk_tree_view_get_model(view);
	GtkListStore *store = GTK_LIST_STORE(model);
    GtkTreeSelection *selection;
    GtkTreeIter iter;
    gboolean valid;
    gchar *str;
    uint32_t addr;

    selection = gtk_tree_view_get_selection(view);
    valid = gtk_tree_selection_get_selected(selection, NULL, &iter);
	if(!valid) return;

    gtk_tree_model_get(model, &iter, COL_ADDR, &str, -1);
    sscanf(str, "%x", &addr);

    if(g_list_find(bkpts.code, GINT_TO_POINTER(addr)) == NULL)
        ti68k_bkpt_add_address(addr);
    else
        ti68k_bkpt_del_address(addr);

    clist_refresh(store, FALSE);
    dbgbkpts_refresh_window();
}

// Toggle tmp breakpoint
GLADE_CB void
dbgcode_button7_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkTreeView *view = GTK_TREE_VIEW(list);
	GtkTreeModel *model = gtk_tree_view_get_model(view);
	GtkListStore *store = GTK_LIST_STORE(model);
    GtkTreeSelection *selection;
    GtkTreeIter iter;
    gboolean valid;
    gchar *str;
    uint32_t addr;

    selection = gtk_tree_view_get_selection(view);
    valid = gtk_tree_selection_get_selected(selection, NULL, &iter);
	if(!valid) return;

    gtk_tree_model_get(model, &iter, COL_ADDR, &str, -1);
    sscanf(str, "%x", &addr);

    if(g_list_find(bkpts.code, GINT_TO_POINTER(addr | BKPT_TMP_MASK)) == NULL)
        ti68k_bkpt_add_address(addr | BKPT_TMP_MASK);
    else
        ti68k_bkpt_del_address(addr | BKPT_TMP_MASK);

    clist_refresh(store, FALSE);
    dbgbkpts_refresh_window();
}

// Reset cycle counter
GLADE_CB void
dbgcode_button8_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
	ti68k_get_cycle_count(!0, NULL);
	gtk_label_set_text(GTK_LABEL(glade_get("label3")), "0");
}

GLADE_CB void
on_save1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	on_quick_save_state_image1_activate(NULL, NULL);
}


GLADE_CB void
on_revert1_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	ti68k_state_load(params.sav_file);
	gtk_debugger_refresh();
}

/***** Popup menu *****/

/*
	Display popup menu (right click)
*/
static GtkWidget* display_dbgcode_popup_menu(void)
{
	GladeXML *xml;
	GtkWidget *menu;

	xml = glade_xml_new
	    (tilp_paths_build_glade("dbg_code-2.glade"), "dbgcode_popup",
	     PACKAGE);
	if (!xml)
		g_error(_("%s: GUI loading failed!\n"), __FILE__);
	glade_xml_signal_autoconnect(xml);

	menu = glade_xml_get_widget(xml, "dbgcode_popup");
	return menu;
}

GLADE_CB gboolean
on_treeview1_button_press_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    switch (event->type) 
    {
    case GDK_BUTTON_PRESS:	// third button clicked
	    if (event->button == 3) 
        {
            GdkEventButton *bevent;
            GtkWidget *menu;

		    bevent = (GdkEventButton *) (event);
            menu = display_dbgcode_popup_menu();

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

static int export_disasm_to_file(GtkWidget *widget)
{
	GtkTreeView *view = GTK_TREE_VIEW(widget);
	GtkTreeModel *model = gtk_tree_view_get_model(view);
    GtkTreeIter iter;
    gchar *str;
    uint32_t addr, start;
    gchar *output;
    int offset;
	int i, j;

	FILE *f;
	gchar **split;

    // starting address
    gtk_tree_model_get_iter_first(model, &iter);
    gtk_tree_model_get(model, &iter, COL_ADDR, &str, -1);
    sscanf(str, "%x", &start);

#ifdef __WIN32__
	f = fopen("C:\\disasm.txt", "a+t");
#else
	f = fopen("/tmp/disasm.txt", "a+t");
#endif
	if(f == NULL)
		return -1;

	addr = start;
	for(i = 0; i < NLINES; i++)
	{
		offset = ti68k_debug_disassemble(addr, &output);
		addr += offset;

		split = g_strsplit(output, " ", 3);
		g_free(output);

		fprintf(f, "%s\t%s", split[0], split[1]);
		for(j=10-strlen(split[1]); j >= 0; j--)
			fprintf(f, " ");
		fprintf(f, "\t%s", split[2]);
		for(j=16-strlen(split[2]); j >= 0; j--)
			fprintf(f, " ");
		fprintf(f, "\t;\n");
		g_strfreev(split);
	}

	fclose(f);

	return 0;
}

// used to implement accelerator keys
GLADE_CB void
on_go_to_address1_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);
GLADE_CB void
on_set_tmp_bkpt1_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);
GLADE_CB void
on_set_breakpoint1_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

GLADE_CB void
on_set_pc_to_selection1_activate       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

GLADE_CB gboolean
on_treeview1_key_press_event           (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data)
{
    GtkTreeView *view = GTK_TREE_VIEW(widget);
	GtkTreeModel *model = gtk_tree_view_get_model(view);
	GtkListStore *store = GTK_LIST_STORE(model);
    GtkTreeSelection *selection;
    GtkTreeIter iter;
    GtkTreePath *path;
    gboolean valid;
    gchar *str;
    gchar *row;
    gint row_idx, row_max;
    uint32_t addr, start;
    gchar *output;
    int offset;

    // starting address
    gtk_tree_model_get_iter_first(model, &iter);
    gtk_tree_model_get(model, &iter, COL_ADDR, &str, -1);
    sscanf(str, "%x", &start);

    // current address
    selection = gtk_tree_view_get_selection(view);
    valid = gtk_tree_selection_get_selected(selection, NULL, &iter);
    if(valid)
    {
        gtk_tree_model_get(model, &iter, COL_ADDR, &str, -1);
        sscanf(str, "%x", &addr);

        row = gtk_tree_model_get_string_from_iter(model, &iter);
        sscanf(row, "%i", &row_idx);
        row_max = gtk_tree_model_iter_n_children(model, NULL) - 1;
    }
    else
        row_idx = row_max = -1;

    // bind our key
	switch(event->keyval) 
	{
	case GDK_F1:
		str = g_strdup_printf("%i", row_idx+1);
        path = gtk_tree_path_new_from_string(str);	// restore selection
		g_free(str);
        gtk_tree_selection_select_path(selection, path);
		on_set_pc_to_selection1_activate(NULL, user_data);
		return FALSE;

	case GDK_F2:
		on_set_breakpoint1_activate(NULL, NULL);
		return FALSE;

	case GDK_F3:
		on_set_tmp_bkpt1_activate(NULL, NULL);
		return FALSE;
	
	case GDK_F4:
		on_run_to_cursor1_activate(NULL, NULL);
		return FALSE;

	case GDK_F5:
		on_run1_activate(NULL, NULL);
		return FALSE;

	case GDK_F6:
		on_set_pc_to_selection1_activate(NULL, user_data);
		return FALSE;

	case GDK_F7:
		on_step1_activate(NULL, NULL);
		return FALSE;

	case GDK_F8:
		on_step_over1_activate(NULL, NULL);
		return FALSE;

	case GDK_F9:
		on_step_out1_activate(NULL, NULL);
		return FALSE;

	case GDK_d:
		export_disasm_to_file(widget);
		return FALSE;

	case GDK_G:
	case GDK_g:
		on_go_to_address1_activate(NULL, user_data);
		return TRUE;

    case GDK_Up:
        if(row_max == -1)
            break;

        if(row_idx > 0)
            break;

        gtk_list_store_clear(store);
        clist_populate(store, addr - 2);
        return FALSE;

    case GDK_Down:
        if(row_max == -1)
            break;

        if(row_idx < row_max)
            break;

        offset = ti68k_debug_disassemble(start, &output);
		g_free(output);

        gtk_list_store_clear(store);
        clist_populate(store, start + offset);
    
        str = g_strdup_printf("%i", row_max);
        path = gtk_tree_path_new_from_string(str);	// restore selection
		g_free(str);
        gtk_tree_selection_select_path(selection, path);

        return TRUE;

    case GDK_Page_Up:
        if(row_max == -1)
            break;

        if(row_idx > 0)
            break;

        gtk_list_store_clear(store);
        clist_populate(store, addr - 0x10);

		path = gtk_tree_path_new_from_string("0");
        gtk_tree_view_set_cursor(view, path, NULL, FALSE);

        return TRUE;

    case GDK_Page_Down:
        if(row_max == -1)
            break;

        if(row_idx < row_max)
            break;

        gtk_list_store_clear(store);
        clist_populate(store, addr/* + 0x10*/);

		str = g_strdup_printf("%i", row_max);
        path = gtk_tree_path_new_from_string(str);	// restore selection
		g_free(str);
        gtk_tree_selection_select_path(selection, path);

        return TRUE;

	default:
		return FALSE;
	}

	return FALSE;
}

// note: user_data data passing has been manually added to Glade file
GLADE_CB void
on_go_to_address1_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    uint32_t addr = 0;

    if(display_dbgmem_address(&addr) == -1)
		return;

    gtk_list_store_clear(store);
    clist_populate(store, addr);
}


GLADE_CB void
on_go_to_pc1_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    clist_refresh(store, TRUE);
}


GLADE_CB void
on_set_breakpoint1_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    dbgcode_button6_clicked(NULL, NULL);
}


GLADE_CB void
on_set_tmp_bkpt1_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    dbgcode_button7_clicked(NULL, NULL);
}


GLADE_CB void
on_set_pc_to_selection1_activate       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    //GtkWidget *list = GTK_WIDGET(user_data);
	GtkTreeView *view = GTK_TREE_VIEW(list);
	GtkTreeModel *model = gtk_tree_view_get_model(view);
    GtkTreeSelection *selection;
    GtkTreeIter iter;
    gboolean valid;
    gchar *str;
    uint32_t addr;

    selection = gtk_tree_view_get_selection(view);
    valid = gtk_tree_selection_get_selected(selection, NULL, &iter);
	if(!valid) return;
    gtk_tree_model_get(model, &iter, COL_ADDR, &str, -1);
    sscanf(str, "%x", &addr);

    ti68k_register_set_pc(addr);
    dbgcode_refresh_window();
    dbgregs_refresh_window();
}

GLADE_CB void
on_view_memory1_activate       (GtkMenuItem     *menuitem,
                                gpointer         user_data)
{
    //GtkWidget *list = GTK_WIDGET(user_data);
	GtkTreeView *view = GTK_TREE_VIEW(list);
	GtkTreeModel *model = gtk_tree_view_get_model(view);
    GtkTreeSelection *selection;
    GtkTreeIter iter;
    gboolean valid;
    gchar *str;
    uint32_t addr;

    selection = gtk_tree_view_get_selection(view);
    valid = gtk_tree_selection_get_selected(selection, NULL, &iter);
	if(!valid) return;
    gtk_tree_model_get(model, &iter, COL_ADDR, &str, -1);
    sscanf(str, "%x", &addr);
    dbgmem_add_tab(addr);
}

GLADE_CB void
on_treeview1_size_allocate             (GtkWidget       *widget,
                                        GdkRectangle    *allocation,
                                        gpointer         user_data)
{
#ifndef FIXED_SIZE
	GtkTreeView *view = GTK_TREE_VIEW(list);
	GtkTreePath *path;
	GdkRectangle rect;
	static int old = 0;

	path = gtk_tree_path_new_from_string("0");
	gtk_tree_view_get_background_area(view, path, NULL, &rect);
	gtk_tree_path_free(path);

	//printf("allocation: %i %i / rect: %i %i\n", allocation->width, allocation->height, rect.width, rect.height);

	if(rect.height == 0)
		NLINES = 1;	// at least 1 line in the window
	else
		NLINES = allocation->height / rect.height - 2;
	//printf("#lines: %i (%i %i)\n", NLINES, allocation->height, rect.height);

	if(old != NLINES)
	{	
		gtk_list_store_clear(store);
		clist_refresh(store, TRUE);
	}

	old = NLINES;
#endif
}

void gdbcallback_disable_debugger(void)
{
	if (dbg_on)
	{
		tb_set_states(0, 0, 0, 0, 0, 1, 0, 0);
		gtk_widget_set_sensitive(list, FALSE);
		gtk_debugger_disable();
		reset_disabled = TRUE;
	}
}

void gdbcallback_enable_debugger(void)
{
	if (dbg_on)
	{
		gtk_widget_set_sensitive(list, TRUE);
		tb_set_states(1, 1, 1, 1, 1, 0, 1, 1);
		gtk_debugger_enable();
		reset_disabled = FALSE;
	}
}

void gdbcallback_refresh_debugger(void)
{
	if (dbg_on)
	{
		dbgcode_refresh_window();
		dbgregs_refresh_window();
		dbgpclog_refresh_window();
		dbgmem_refresh_window();
		dbgstack_refresh_window();
		dbgheap_refresh_window();

		// force refresh !
		while(gtk_events_pending()) gtk_main_iteration_do(FALSE);
	}
}

int dbgcode_quit_enabled(void)
{
	return GTK_WIDGET_SENSITIVE(mi.m8);
}

static int close_debugger_wrapper(gpointer data)
{
	gtk_debugger_close();
	return FALSE;
}

int gdbcallback_close_debugger(void *clientdata, void *interp, int argc, const char **argv)
{
	if (dbg_on && dbgcode_quit_enabled()) g_idle_add(close_debugger_wrapper, NULL);
	return 0;
}

GLADE_CB void
on_linkport1_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	display_loglink_dbox();
}

// Font change

static GtkWidget *font;
static gint tmp_type;
static gchar *tmp_name;

GLADE_CB void
on_font_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GladeXML *xml;
	GtkWidget *dbox;
	gpointer data;
	gint result;
	
	xml = glade_xml_new
		(tilp_paths_build_glade("dbg_code-2.glade"), "dbgcode_font", PACKAGE);
	if (!xml)
		g_error(_("%s: GUI loading failed!\n"), __FILE__);
	glade_xml_signal_autoconnect(xml);
	
	dbox = glade_xml_get_widget(xml, "dbgcode_font");
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dbox), GTK_RESPONSE_OK,
	                                        GTK_RESPONSE_CANCEL,-1);
	font = glade_xml_get_widget(xml, "label5");

	tmp_type = options3.dbg_font_type;
	tmp_name = g_strdup(options3.dbg_font_name);

	if(!tmp_type)
	{
		data = glade_xml_get_widget(xml, "radiobutton4");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data), TRUE);
		data = glade_xml_get_widget(xml, "button9");
		gtk_widget_set_sensitive(GTK_WIDGET(data), FALSE);
	}
	else
	{
		data = glade_xml_get_widget(xml, "radiobutton6");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data), TRUE);
		if(options3.dbg_font_name)
			gtk_label_set_text(GTK_LABEL(font), tmp_name);
		data = glade_xml_get_widget(xml, "button9");
		gtk_widget_set_sensitive(GTK_WIDGET(data), TRUE);
	}

	result = gtk_dialog_run(GTK_DIALOG(dbox));
	switch (result) 
	{
	case GTK_RESPONSE_OK:
		options3.dbg_font_type = tmp_type;
		if(tmp_name)
		{
			g_free(options3.dbg_font_name);
			options3.dbg_font_name = g_strdup(tmp_name);
		}

		gtk_debugger_refresh();

		break;
	default:
		break;
	}

	gtk_widget_destroy(dbox);
	return;
}

GLADE_CB void
on_radiobutton4_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
	PangoContext *context;
	PangoFontDescription *desc;

	context = gtk_widget_get_pango_context(GTK_WIDGET(button));
	desc = pango_context_get_font_description(context);
	gtk_label_set_text(GTK_LABEL(font), pango_font_description_to_string(desc));

	gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);
	tmp_type = 0;
}

GLADE_CB void
on_radiobutton6_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
	if(tmp_name)
		gtk_label_set_text(GTK_LABEL(font), tmp_name);

	gtk_widget_set_sensitive(GTK_WIDGET(button), TRUE);
	tmp_type = 1;
}

GLADE_CB void
on_dbgcode_button9_clicked             (GtkButton       *button,
                                        gpointer         user_data)
{
	gint result;
	GtkWidget *dlg;
	gchar *str;
	
	dlg = gtk_font_selection_dialog_new(_("Choose font..."));
	if(tmp_name)
		gtk_font_selection_dialog_set_font_name(GTK_FONT_SELECTION_DIALOG(dlg), tmp_name);
	result = gtk_dialog_run(GTK_DIALOG(dlg));	

	switch (result) 
	{ 
	case GTK_RESPONSE_OK:
		str = gtk_font_selection_dialog_get_font_name(GTK_FONT_SELECTION_DIALOG(dlg));
		g_free(tmp_name);
		tmp_name = g_strdup(str);
		gtk_label_set_text(GTK_LABEL(font), str);
		g_free(str);
		break;
	default:
		break;
	}

	gtk_widget_destroy(dlg);
}
