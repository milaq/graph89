/* Hey EMACS -*- linux-c -*- */
/* $Id: dbg_all.h 2707 2007-12-13 13:18:45Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Liévin
 *  Copyright (c) 2005-2006, Romain Liévin, Kevin Kofler
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

#ifndef __DBG_WINDOWS_H__
#define __DBG_WINDOWS_H__

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS
#include "dbg_bkpts.h"
#include "dbg_bits.h"
#include "dbg_code.h"
#include "dbg_cause.h"
#include "dbg_data.h"
#include "dbg_mem.h"
#include "dbg_regs.h"
#include "dbg_vectors.h"
#include "dbg_pclog.h"
#include "dbg_stack.h"
#include "dbg_heap.h"
#include "dbg_entry.h"
#include "dbg_iop.h"

// calc.c: calculator window
extern GtkWidget *main_wnd;

// Save window state
#define WND_STATE
#define LINE_PAD	0

/* Definitions */

typedef struct {
    GtkWidget *mem;
    GtkWidget *regs;
    GtkWidget *bkpts;
    GtkWidget *code;
    GtkWidget *pclog;
    GtkWidget *stack;
	GtkWidget *heap;
	GtkWidget *iop;
	GtkWidget *dock;
} DbgWidgets;
extern DbgWidgets dbgw;

/* Functions */

void dbgwnds_set_sensitivity(int state);

void dbgwnds_minimize_all(int all);
void dbgwnds_unminimize_all(int all);

void dbgwnds_show_all(int all);
void dbgwnds_hide_all(int all);

#define glade_get(s)		glade_xml_get_widget(xml, (s))

/* Macros */

#define set_renderer_pad(renderer)	\
{ \
	g_object_set(G_OBJECT(renderer), "xpad", LINE_PAD, NULL); \
	g_object_set(G_OBJECT(renderer), "ypad", LINE_PAD, NULL); \
}

//#define MEASURE_WND_TIME

#ifdef MEASURE_WND_TIME
 #define WND_TMR_START()		GTimer *tmr = g_timer_new();
 #define WND_TMR_STOP(s)	{	\
	g_timer_stop(tmr);	\
	printf("%s: %f\n", s, g_timer_elapsed(tmr, NULL));	\
	g_timer_destroy(tmr);	\
}
#else
 #define WND_TMR_START()
 #define WND_TMR_STOP(s)
#endif

G_END_DECLS

#endif
