/* Hey EMACS -*- linux-c -*- */
/* $Id: dbg_all.c 2707 2007-12-13 13:18:45Z roms $ */

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

#include <glib.h>
#include <gtk/gtk.h>
#ifdef __WIN32__
#include <gdk/gdk.h>
#include <gdk/gdkwin32.h>
#endif
#ifdef __MINGW32__
#include <windows.h>
#endif

#include "intl.h"
#include "ti68k_int.h"
#include "struct.h"
#include "dbg_wnds.h"
#include "dbg_all.h"
#include "dbg_wnds.h"
#include "support.h"
#include "paths.h"
#include "engine.h"
#include "dboxes.h"
#include "rcfile.h"

DbgOptions options3;
DbgWidgets dbgw = { 0 };

// make windows (un-)modifiable
void dbgwnds_set_sensitivity(int state)
{
	if(options3.dbg_dock)
		return;

    gtk_widget_set_sensitive(dbgw.regs, state);
    gtk_widget_set_sensitive(dbgw.bkpts, state);
    gtk_widget_set_sensitive(dbgw.mem, state);
    gtk_widget_set_sensitive(dbgw.pclog, state);
    gtk_widget_set_sensitive(dbgw.stack, state);
    gtk_widget_set_sensitive(dbgw.heap, state);
	gtk_widget_set_sensitive(dbgw.iop, state);
}

// minimize all windows
void dbgwnds_minimize_all(int all)
{
	if(options3.dbg_dock)
		return;

    if(GTK_WIDGET_VISIBLE(dbgw.regs))
        gtk_window_iconify(GTK_WINDOW(dbgw.regs));
    if(GTK_WIDGET_VISIBLE(dbgw.bkpts))
        gtk_window_iconify(GTK_WINDOW(dbgw.bkpts));
    if(GTK_WIDGET_VISIBLE(dbgw.mem))
        gtk_window_iconify(GTK_WINDOW(dbgw.mem));
    if(GTK_WIDGET_VISIBLE(dbgw.pclog))
        gtk_window_iconify(GTK_WINDOW(dbgw.pclog));
    if(GTK_WIDGET_VISIBLE(dbgw.code) & all)
        gtk_window_iconify(GTK_WINDOW(dbgw.code));
    if(GTK_WIDGET_VISIBLE(dbgw.stack))
        gtk_window_iconify(GTK_WINDOW(dbgw.stack));
	if(GTK_WIDGET_VISIBLE(dbgw.heap))
        gtk_window_iconify(GTK_WINDOW(dbgw.heap));
	if(GTK_WIDGET_VISIBLE(dbgw.iop))
        gtk_window_iconify(GTK_WINDOW(dbgw.iop));
}

// unminimize all windows
void dbgwnds_unminimize_all(int all)
{
	if(options3.dbg_dock)
		return;

    if(GTK_WIDGET_VISIBLE(dbgw.regs))
        gtk_window_deiconify(GTK_WINDOW(dbgw.regs));
    if(GTK_WIDGET_VISIBLE(dbgw.bkpts))
        gtk_window_deiconify(GTK_WINDOW(dbgw.bkpts));
    if(GTK_WIDGET_VISIBLE(dbgw.mem))
        gtk_window_deiconify(GTK_WINDOW(dbgw.mem));
    if(GTK_WIDGET_VISIBLE(dbgw.pclog))
        gtk_window_deiconify(GTK_WINDOW(dbgw.pclog));
    if(GTK_WIDGET_VISIBLE(dbgw.code) & all)
        gtk_window_deiconify(GTK_WINDOW(dbgw.code));
    if(GTK_WIDGET_VISIBLE(dbgw.stack))
        gtk_window_deiconify(GTK_WINDOW(dbgw.stack));
	if(GTK_WIDGET_VISIBLE(dbgw.heap))
        gtk_window_deiconify(GTK_WINDOW(dbgw.heap));
	if(GTK_WIDGET_VISIBLE(dbgw.iop))
        gtk_window_deiconify(GTK_WINDOW(dbgw.iop));
}

// show all windows
void dbgwnds_show_all(int all)
{
    if(options3.dbg_dock)
		return;

    if(!GTK_WIDGET_VISIBLE(dbgw.regs))
        gtk_widget_show(dbgw.regs);
    if(!GTK_WIDGET_VISIBLE(dbgw.bkpts))
        gtk_widget_show(dbgw.bkpts);
    if(!GTK_WIDGET_VISIBLE(dbgw.mem))
        gtk_widget_show(dbgw.mem);
    if(!GTK_WIDGET_VISIBLE(dbgw.pclog))
        gtk_widget_show(dbgw.pclog);
    if(!GTK_WIDGET_VISIBLE(dbgw.code) && all)
        gtk_widget_show(dbgw.code);
    if(!GTK_WIDGET_VISIBLE(dbgw.stack))
        gtk_widget_show(dbgw.stack);
	if(!GTK_WIDGET_VISIBLE(dbgw.heap))
        gtk_widget_show(dbgw.heap);
	if(!GTK_WIDGET_VISIBLE(dbgw.iop))
        gtk_widget_show(dbgw.iop);
}

// or hide them
void dbgwnds_hide_all(int all)
{
    if(options3.dbg_dock)
		return;

    if(GTK_WIDGET_VISIBLE(dbgw.regs))
        gtk_widget_hide(dbgw.regs);
    if(GTK_WIDGET_VISIBLE(dbgw.bkpts))
        gtk_widget_hide(dbgw.bkpts);
    if(GTK_WIDGET_VISIBLE(dbgw.mem))
        gtk_widget_hide(dbgw.mem);
    if(GTK_WIDGET_VISIBLE(dbgw.pclog))
        gtk_widget_hide(dbgw.pclog);
    if(GTK_WIDGET_VISIBLE(dbgw.code) && all)
        gtk_widget_hide(dbgw.code);
    if(GTK_WIDGET_VISIBLE(dbgw.stack))
        gtk_widget_hide(dbgw.stack);
	if(GTK_WIDGET_VISIBLE(dbgw.heap))
        gtk_widget_hide(dbgw.heap);
	if(GTK_WIDGET_VISIBLE(dbgw.iop))
        gtk_widget_hide(dbgw.iop);
}

/* Callbacks */

// callbacks from dbg_code.c (window menu)
// used to show/hide or minimize/un-minimize windows

GLADE_CB void
on_registers1_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    if(GTK_CHECK_MENU_ITEM(menuitem)->active != TRUE) 
        gtk_widget_hide(dbgw.regs);
  	else
		gtk_widget_show(dbgw.regs);
}


GLADE_CB void
on_breakpoints1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    if(GTK_CHECK_MENU_ITEM(menuitem)->active != TRUE) 
        gtk_widget_hide(dbgw.bkpts);
  	else
        gtk_widget_show(dbgw.bkpts);
}


GLADE_CB void
on_memory1_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    if(GTK_CHECK_MENU_ITEM(menuitem)->active != TRUE) 
        gtk_widget_hide(dbgw.mem);
  	else
        gtk_widget_show(dbgw.mem);
}

GLADE_CB void
on_pc_log1_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    if(GTK_CHECK_MENU_ITEM(menuitem)->active != TRUE) 
        gtk_widget_hide(dbgw.pclog);
  	else
        gtk_widget_show(dbgw.pclog);
}

GLADE_CB void
on_stack_frame1_activate                    (GtkMenuItem     *menuitem,
                                             gpointer         user_data)
{
    if(GTK_CHECK_MENU_ITEM(menuitem)->active != TRUE) 
        gtk_widget_hide(dbgw.stack);
  	else
        gtk_widget_show(dbgw.stack);
}

GLADE_CB void
on_heap_frame1_activate                    (GtkMenuItem     *menuitem,
                                             gpointer         user_data)
{
    if(GTK_CHECK_MENU_ITEM(menuitem)->active != TRUE) 
        gtk_widget_hide(dbgw.heap);
  	else
        gtk_widget_show(dbgw.heap);
}

GLADE_CB void
on_ioports_frame1_activate                    (GtkMenuItem     *menuitem,
                                             gpointer         user_data)
{
    if(GTK_CHECK_MENU_ITEM(menuitem)->active != TRUE) 
        gtk_widget_hide(dbgw.iop);
  	else
        gtk_widget_show(dbgw.iop);
}

GLADE_CB void
on_transient1_activate                 (GtkMenuItem     *menu_item,
                                        gpointer         user_data)
{
	// This make dbg wnd's as children of the main window.
	// Thus, the taskbar is not filled-up with a lot of windows.
	options3.transient = GTK_CHECK_MENU_ITEM(menu_item)->active;
 
	msg_box1(_("Warning"), _("You will have to save configuration and restart TiEmu for changes to take effect!"));
}

GLADE_CB void
on_dockmode1_activate                  (GtkMenuItem     *menu_item,
                                        gpointer         user_data)
{
	msg_box1(_("Warning"), _("TiEmu is about to restart..."));

#ifndef NO_GDB
	// In GDB mode, we have to restart the engine here, otherwise
	// gtk_debugger_close will call gdbcall_continue to do so and never
	// return.
	engine_start();
#endif
	gtk_debugger_close();
	// Stop the engine before calling gtk_main_quit.
	// Otherwise, it will keep running even when it is supposed to have
	// been stopped by the debugger.
	engine_stop();
	if(options3.dbg_dock)
		gtk_widget_destroy(dbgw.dock);

	options3.dbg_dock = GTK_CHECK_MENU_ITEM(menu_item)->active;	

	gtk_main_quit();
}

GLADE_CB void
on_quit1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	gtk_debugger_close();
}

GLADE_CB void
on_show_all1_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	dbgwnds_show_all(0);
}


GLADE_CB void
on_hide_all1_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	dbgwnds_hide_all(0);
}

GLADE_CB void
on_minimize_all1_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    dbgwnds_minimize_all(0);
}


GLADE_CB void
on_maximize_all1_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    dbgwnds_unminimize_all(0);
}

extern void options3_set_default(void);

GLADE_CB void
on_restore_all1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	// restore windows with default settings
	options3_set_default();
}

// reflects window state in menu
void update_submenu(GtkWidget *widget, gpointer user_data)
{
    GtkMenuShell *shell = GTK_MENU_SHELL(widget);
    GList *list = shell->children;
    GList *elt;
    GtkCheckMenuItem *item;

    // regs
    elt = g_list_nth(list, 0);
    item = GTK_CHECK_MENU_ITEM(elt->data);
	if(!options3.dbg_dock)
	{
		g_signal_handlers_block_by_func(GTK_OBJECT(item), on_registers1_activate, NULL);
		gtk_check_menu_item_set_active(item, GTK_WIDGET_VISIBLE(dbgw.regs));
		g_signal_handlers_unblock_by_func(GTK_OBJECT(item), on_registers1_activate, NULL);
	}
	else
		gtk_widget_set_sensitive(GTK_WIDGET(item), FALSE);

    // bkpts
    elt = g_list_nth(list, 1);
    item = GTK_CHECK_MENU_ITEM(elt->data);
	if(!options3.dbg_dock)
	{
		g_signal_handlers_block_by_func(GTK_OBJECT(item), on_breakpoints1_activate, NULL);
		gtk_check_menu_item_set_active(item, GTK_WIDGET_VISIBLE(dbgw.bkpts));
		g_signal_handlers_unblock_by_func(GTK_OBJECT(item), on_breakpoints1_activate, NULL);
	}
	else
		gtk_widget_set_sensitive(GTK_WIDGET(item), FALSE);

    // mem
    elt = g_list_nth(list, 2);
    item = GTK_CHECK_MENU_ITEM(elt->data);
	if(!options3.dbg_dock)
	{
		g_signal_handlers_block_by_func(GTK_OBJECT(item), on_memory1_activate, NULL);
		gtk_check_menu_item_set_active(item, GTK_WIDGET_VISIBLE(dbgw.mem));
		g_signal_handlers_unblock_by_func(GTK_OBJECT(item), on_memory1_activate, NULL);
	}
	else
		gtk_widget_set_sensitive(GTK_WIDGET(item), FALSE);

    // pclog
    elt = g_list_nth(list, 3);
    item = GTK_CHECK_MENU_ITEM(elt->data);
    g_signal_handlers_block_by_func(GTK_OBJECT(item), on_pc_log1_activate, NULL);
    gtk_check_menu_item_set_active(item, GTK_WIDGET_VISIBLE(dbgw.pclog));
    g_signal_handlers_unblock_by_func(GTK_OBJECT(item), on_pc_log1_activate, NULL);

    // stack
    elt = g_list_nth(list, 4);
    item = GTK_CHECK_MENU_ITEM(elt->data);
	if(!options3.dbg_dock)
	{
		g_signal_handlers_block_by_func(GTK_OBJECT(item), on_stack_frame1_activate, NULL);
		gtk_check_menu_item_set_active(item, GTK_WIDGET_VISIBLE(dbgw.stack));
		g_signal_handlers_unblock_by_func(GTK_OBJECT(item), on_stack_frame1_activate, NULL);
	}
	else
		gtk_widget_set_sensitive(GTK_WIDGET(item), FALSE);
	
	// heap
    elt = g_list_nth(list, 5);
    item = GTK_CHECK_MENU_ITEM(elt->data);
	if(!options3.dbg_dock)
	{
		g_signal_handlers_block_by_func(GTK_OBJECT(item), on_heap_frame1_activate, NULL);
		gtk_check_menu_item_set_active(item, GTK_WIDGET_VISIBLE(dbgw.heap));
		g_signal_handlers_unblock_by_func(GTK_OBJECT(item), on_heap_frame1_activate, NULL);
	}
	else
		gtk_widget_set_sensitive(GTK_WIDGET(item), FALSE);

	// ioports
	elt = g_list_nth(list, 6);
    item = GTK_CHECK_MENU_ITEM(elt->data);
    g_signal_handlers_block_by_func(GTK_OBJECT(item), on_ioports_frame1_activate, NULL);
    gtk_check_menu_item_set_active(item, GTK_WIDGET_VISIBLE(dbgw.iop));
    g_signal_handlers_unblock_by_func(GTK_OBJECT(item), on_ioports_frame1_activate, NULL);

	// dock/multi mode
	elt = g_list_nth(list, 8);
	item = GTK_CHECK_MENU_ITEM(elt->data);
    g_signal_handlers_block_by_func(GTK_OBJECT(item), on_dockmode1_activate, NULL);
    gtk_check_menu_item_set_active(item, options3.dbg_dock);
    g_signal_handlers_unblock_by_func(GTK_OBJECT(item), on_dockmode1_activate, NULL);

	// transient mode
	elt = g_list_nth(list, 9);
    item = GTK_CHECK_MENU_ITEM(elt->data);
    g_signal_handlers_block_by_func(GTK_OBJECT(item), on_transient1_activate, NULL);
    gtk_check_menu_item_set_active(item, options3.transient);
    g_signal_handlers_unblock_by_func(GTK_OBJECT(item), on_transient1_activate, NULL);

	if(options3.dbg_dock)
	{
		int i;

		for(i = 11; i <= 15; i++)
		{
			elt = g_list_nth(list, i);
			gtk_widget_set_sensitive(GTK_WIDGET(elt->data), FALSE);
		}
	}
}

// callbacks from dbg_regs.c

/* 
	This function exists because GDK retrieves client coordinates, not window ones.
	(Kevin: GDK uses GetClientRect and ClientToScreen).
	We need that to save and restore windows position.
*/
void window_get_rect(GtkWidget *widget, GdkRect *rect)
{
	gtk_window_get_size(GTK_WINDOW(widget), &rect->w, &rect->h);

#ifdef __WIN32__
	{
		BOOL bResult;
		HWND hWnd = GDK_WINDOW_HWND(widget->window);
		RECT lpRect;
		GdkRectangle gdkRect;

		bResult = GetWindowRect(hWnd, &lpRect);

		rect->x = lpRect.left;
		rect->y = lpRect.top;

		// Now obtain and add the offset between GDK and Win32 coordinates
		// (in the multi-screen case).
		gdk_screen_get_monitor_geometry(gdk_screen_get_default(), 0, &gdkRect);
		rect->x += gdkRect.x;
		rect->y += gdkRect.y;
	}
#else
	gdk_window_get_position(widget->window, &rect->x, &rect->y);
#endif
}

GLADE_CB gboolean
on_dbgregs_window_delete_event         (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    gtk_widget_hide(widget);
	return TRUE;
}

GLADE_CB void
on_dbgregs_window_state_event		   (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    GdkEventWindowState *wstate = (GdkEventWindowState *)event;
    GdkWindowState state = wstate->new_window_state;
    GdkWindowState mask = wstate->changed_mask;

	if(mask & GDK_WINDOW_STATE_WITHDRAWN && dbg_on)
		options3.regs.closed = (state & GDK_WINDOW_STATE_WITHDRAWN);

	window_get_rect(widget, &options3.regs.rect);

	if(mask & GDK_WINDOW_STATE_ICONIFIED)
		options3.regs.minimized = state & GDK_WINDOW_STATE_ICONIFIED;
}

// callbacks from dbg_pclog.c
GLADE_CB gboolean
on_dbgpclog_window_delete_event        (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    gtk_widget_hide(widget);    
    return TRUE;
}

GLADE_CB void
on_dbgpclog_window_state_event		   (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    GdkEventWindowState *wstate = (GdkEventWindowState *)event;
    GdkWindowState state = wstate->new_window_state;
    GdkWindowState mask = wstate->changed_mask;

	if(mask & GDK_WINDOW_STATE_WITHDRAWN && dbg_on)
		options3.pclog.closed = (state & GDK_WINDOW_STATE_WITHDRAWN);

	window_get_rect(widget, &options3.pclog.rect);

	if(mask & GDK_WINDOW_STATE_ICONIFIED)
		options3.pclog.minimized = state & GDK_WINDOW_STATE_ICONIFIED;
}

// callbacks from dbg_mem.c

GLADE_CB gboolean
on_dbgmem_window_delete_event       (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    gtk_widget_hide(widget);    
    return TRUE;
}

GLADE_CB void
on_dbgmem_window_state_event		   (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    GdkEventWindowState *wstate = (GdkEventWindowState *)event;
    GdkWindowState state = wstate->new_window_state;
    GdkWindowState mask = wstate->changed_mask;

	if(mask & GDK_WINDOW_STATE_WITHDRAWN && dbg_on)
		options3.mem.closed = (state & GDK_WINDOW_STATE_WITHDRAWN);

	window_get_rect(widget, &options3.mem.rect);

	if(mask & GDK_WINDOW_STATE_ICONIFIED)
		options3.mem.minimized = state & GDK_WINDOW_STATE_ICONIFIED;
}

// callbacks from dbg_code.c

GLADE_CB gboolean
on_dbgcode_window_delete_event       (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    if (dbgcode_quit_enabled())
    {
        gtk_widget_hide(widget);
        on_quit1_activate(NULL, NULL);    
    }
    return TRUE;
}

GLADE_CB void
on_dbgcode_window_state_event		   (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    GdkEventWindowState *wstate = (GdkEventWindowState *)event;
    GdkWindowState state = wstate->new_window_state;
    GdkWindowState mask = wstate->changed_mask;

	if(mask & GDK_WINDOW_STATE_WITHDRAWN && dbg_on)
		options3.code.closed = (state & GDK_WINDOW_STATE_WITHDRAWN);

	window_get_rect(widget, &options3.code.rect);

	if(mask & GDK_WINDOW_STATE_ICONIFIED)
		options3.code.minimized = state & GDK_WINDOW_STATE_ICONIFIED;
}

// callbacks from dbg_bkpts.c

GLADE_CB gboolean
on_dbgbkpts_window_delete_event       (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    gtk_widget_hide(widget);
    return TRUE;
}

GLADE_CB void
on_dbgbkpts_window_state_event		   (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    GdkEventWindowState *wstate = (GdkEventWindowState *)event;
    GdkWindowState state = wstate->new_window_state;
    GdkWindowState mask = wstate->changed_mask;

	if(mask & GDK_WINDOW_STATE_WITHDRAWN && dbg_on)
		options3.bkpts.closed = (state & GDK_WINDOW_STATE_WITHDRAWN);

	window_get_rect(widget, &options3.bkpts.rect);

	if(mask & GDK_WINDOW_STATE_ICONIFIED)
		options3.bkpts.minimized = state & GDK_WINDOW_STATE_ICONIFIED;
}

// callbacks from dbg_stack.c
GLADE_CB gboolean
on_dbgstack_window_delete_event       (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    gtk_widget_hide(widget);    
    return TRUE;
}

GLADE_CB void
on_dbgstack_window_state_event		   (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    GdkEventWindowState *wstate = (GdkEventWindowState *)event;
    GdkWindowState state = wstate->new_window_state;
    GdkWindowState mask = wstate->changed_mask;

	if(mask & GDK_WINDOW_STATE_WITHDRAWN && dbg_on)
		options3.stack.closed = (state & GDK_WINDOW_STATE_WITHDRAWN);

	window_get_rect(widget, &options3.stack.rect);

	if(mask & GDK_WINDOW_STATE_ICONIFIED)
		options3.stack.minimized = state & GDK_WINDOW_STATE_ICONIFIED;
}

// callbacks from dbg_heap.c
GLADE_CB gboolean
on_dbgheap_window_delete_event       (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    gtk_widget_hide(widget);    
    return TRUE;
}

GLADE_CB void
on_dbgheap_window_state_event		   (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    GdkEventWindowState *wstate = (GdkEventWindowState *)event;
    GdkWindowState state = wstate->new_window_state;
    GdkWindowState mask = wstate->changed_mask;

	if(mask & GDK_WINDOW_STATE_WITHDRAWN && dbg_on)
		options3.heap.closed = (state & GDK_WINDOW_STATE_WITHDRAWN);

	window_get_rect(widget, &options3.heap.rect);

	if(mask & GDK_WINDOW_STATE_ICONIFIED)
		options3.heap.minimized = state & GDK_WINDOW_STATE_ICONIFIED;
}

// callbacks from dbg_iop.c
GLADE_CB gboolean
on_dbgioports_window_delete_event       (GtkWidget       *widget,
                                        GdkEvent         *event,
                                        gpointer          user_data)
{
    gtk_widget_hide(widget);    
    return TRUE;
}

GLADE_CB void
on_dbgioports_window_state_event	   (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    GdkEventWindowState *wstate = (GdkEventWindowState *)event;
    GdkWindowState state = wstate->new_window_state;
    GdkWindowState mask = wstate->changed_mask;

	if(mask & GDK_WINDOW_STATE_WITHDRAWN && dbg_on)
		options3.iop.closed = (state & GDK_WINDOW_STATE_WITHDRAWN);

	window_get_rect(widget, &options3.iop.rect);

	if(mask & GDK_WINDOW_STATE_ICONIFIED)
		options3.iop.minimized = state & GDK_WINDOW_STATE_ICONIFIED;
}

// misc

// Common remplacement for gtk_window_(de)iconify)
void gtk_window_minimize(GtkWindow *window, gboolean action)
{
	if(action)
		gtk_window_iconify(window);
	else
		gtk_window_deiconify(window);
}


/*
tiTIME profile;
void profile_start(void)
{
	TO_START(profile);
}
void profile_stop(const char *str)
{
	printf("Duration (%s): %i ms\n", str, TO_CURRENT(profile));
}
*/
