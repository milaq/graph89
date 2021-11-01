/* Hey EMACS -*- linux-c -*- */
/* $Id: dbg_code.c 2680 2007-11-19 20:53:09Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Liévin
 *  Copyright (c) 2005-2007, Romain Liévin, Kevin Kofler
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

/*
	This unit needs some explanations due to some tricks...

	Up to TiEmu v3.02, it was designed with a multi-windows system.
	Starting at v3.03, a dock has been added 


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
#include "dbg_dock.h"

DbgHandleBoxes dbghb = { 0 };

static GtkWidget* gtk_widget_get_child_(GtkWidget* widget)
{
	GtkWidget *child = gtk_bin_get_child(GTK_BIN(widget));
	return child;
}

static void gtk_widget_reparent_(GtkWidget* dst, GtkWidget* src)
{
	GtkWidget *child = gtk_widget_get_child_(src);

#if 1
	gtk_widget_reparent(child, dst);
#else
	gtk_widget_ref(child);
    gtk_container_remove(GTK_CONTAINER(src), child);
    gtk_container_add(GTK_CONTAINER(dst), child);
    gtk_widget_unref(child);
#endif
}

GtkWidget* dbgdock_create_window(void)
{
	GladeXML  *xml;
	GtkWidget *dbox;
	
	xml = glade_xml_new
		(tilp_paths_build_glade("dbg_dock-2.glade"), "dbgdock_window",
		 PACKAGE);
	if (!xml)
		g_error(_("%s: GUI loading failed!\n"), __FILE__);
	glade_xml_signal_autoconnect(xml);

	dbox = glade_xml_get_widget(xml, "dbgdock_window");
	if(options3.transient)
		gtk_window_set_transient_for(GTK_WINDOW(dbox), GTK_WINDOW(main_wnd));

	dbghb.stack = glade_xml_get_widget(xml, "frame1");
	dbghb.regs  = glade_xml_get_widget(xml, "frame2");
	dbghb.bkpts = glade_xml_get_widget(xml, "frame3");
	dbghb.code  = glade_xml_get_widget(xml, "frame4");
	dbghb.mem   = glade_xml_get_widget(xml, "frame5");
	dbghb.heap  = glade_xml_get_widget(xml, "frame6");

	// re-attach window childs to dock and destroy parent window
	gtk_widget_reparent_(dbghb.stack, dbgw.stack);
	gtk_widget_reparent_(dbghb.regs,  dbgw.regs);
	gtk_widget_reparent_(dbghb.bkpts, dbgw.bkpts);
	gtk_widget_reparent_(dbghb.code,  dbgw.code);
	gtk_widget_reparent_(dbghb.mem,   dbgw.mem);
	gtk_widget_reparent_(dbghb.heap,  dbgw.heap);

	// set them as 'open'
	options3.stack.closed = 0;
	options3.regs.closed = 0;
	options3.bkpts.closed = 0;
	options3.code.closed = 0;
	options3.mem.closed = 0;
	options3.heap.closed = 0;

	gtk_widget_destroy(dbgw.stack);
	dbgw.stack = NULL;
	gtk_widget_destroy(dbgw.regs);
	dbgw.regs = NULL;
	gtk_widget_destroy(dbgw.bkpts);
	dbgw.bkpts = NULL;
	gtk_widget_destroy(dbgw.mem);
	dbgw.mem = NULL;
	gtk_widget_destroy(dbgw.heap);
	dbgw.heap = NULL;
#if 0	// has to be fixed...
	gtk_widget_destroy(dbgw.code);
	dbgw.code = NULL;
#endif

	return dbox;
}

GtkWidget* dbgdock_display_window(void)
{
	gtk_widget_show(dbgw.dock);
    return dbgw.dock;
}

void dbgdock_refresh_window(void)
{
	return;
}

void dbgdock_set_sensitivity(int state)
{
    gtk_widget_set_sensitive(dbghb.regs, state);
    gtk_widget_set_sensitive(dbghb.bkpts, state);
    gtk_widget_set_sensitive(dbghb.mem, state);
    gtk_widget_set_sensitive(dbghb.stack, state);
    gtk_widget_set_sensitive(dbghb.heap, state);
}

void dbgdock_show_all(int all)
{
	if(!GTK_WIDGET_VISIBLE(dbgw.dock) && all)
        gtk_widget_show(dbgw.dock);

	if(GTK_WIDGET_VISIBLE(dbgw.iop))
        gtk_window_iconify(GTK_WINDOW(dbgw.iop));
	if(GTK_WIDGET_VISIBLE(dbgw.pclog))
        gtk_window_iconify(GTK_WINDOW(dbgw.pclog));
}

void dbgdock_hide_all(int all)
{
	if(GTK_WIDGET_VISIBLE(dbgw.dock) && all)
        gtk_widget_hide(dbgw.dock);

    if(GTK_WIDGET_VISIBLE(dbgw.pclog))
        gtk_widget_hide(dbgw.pclog);
	if(GTK_WIDGET_VISIBLE(dbgw.iop))
        gtk_widget_hide(dbgw.iop);
}
