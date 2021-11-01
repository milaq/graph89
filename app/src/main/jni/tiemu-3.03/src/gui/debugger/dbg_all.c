/* Hey EMACS -*- linux-c -*- */
/* $Id: dbg_all.c 2832 2009-05-08 10:56:40Z roms $ */

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

#include "ti68k_int.h"
#include "struct.h"
#include "dbg_all.h"
#include "dbg_wnds.h"
#include "dbg_dock.h"
#include "support.h"
#include "paths.h"
#include "engine.h"
#include "dboxes.h"
#ifndef NO_GDB
#include "gdbcall.h"

void gdbtk_hide_insight(void);
void gdbtk_show_insight(void);
void delete_command(void *, int);
void symbol_file_clear(int);
void gdbtk_clear_file(void);
void exec_build_section_table(void);

gchar *symfile;
#endif

int dbg_on = 0;
int dbg_load = 0;

/* Functions applicable to the whole debugger */

// create windows but don't show them yet
void gtk_debugger_preload(void)
{
	WND_TMR_START();

	dbgw.regs  = dbgregs_create_window();
	dbgw.mem   = dbgmem_create_window();
	dbgw.bkpts = dbgbkpts_create_window();
	dbgw.pclog = dbgpclog_create_window();
    dbgw.stack = dbgstack_create_window();
	dbgw.heap  = dbgheap_create_window();
	dbgw.iop   = dbgiop_create_window();
	dbgw.code  = dbgcode_create_window();
	if(options3.dbg_dock)	//must be launched as last
		dbgw.dock  = dbgdock_create_window();

	dbg_load = !0;

	WND_TMR_STOP("Debugger Preload Time");
}

void gtk_debugger_refresh(void)
{	
	WND_TMR_START();

	if(options3.dbg_dock || GTK_WIDGET_VISIBLE(dbgw.regs))
		dbgregs_refresh_window();
	if(options3.dbg_dock || GTK_WIDGET_VISIBLE(dbgw.mem))
		dbgmem_refresh_window();
	if(options3.dbg_dock || GTK_WIDGET_VISIBLE(dbgw.bkpts))
		dbgbkpts_refresh_window();
	if(options3.dbg_dock || GTK_WIDGET_VISIBLE(dbgw.pclog))
		dbgpclog_refresh_window();
	if(options3.dbg_dock || GTK_WIDGET_VISIBLE(dbgw.code))
		dbgcode_refresh_window();
    if(options3.dbg_dock || GTK_WIDGET_VISIBLE(dbgw.stack))
		dbgstack_refresh_window();
	if(options3.dbg_dock || GTK_WIDGET_VISIBLE(dbgw.heap))
		dbgheap_refresh_window();
	if(options3.dbg_dock || GTK_WIDGET_VISIBLE(dbgw.iop))
		dbgiop_refresh_window();

	WND_TMR_STOP("Debugger Refresh Time");
	printf("\n");
}

void gtk_debugger_display(void)
{
	WND_TMR_START();

	// display debugger windows (if not)
	if(options3.dbg_dock)
	{
		dbgdock_display_window();
		dbgiop_display_window();
		dbgpclog_display_window();
	}
	else
	{
		dbgregs_display_window();
		dbgmem_display_window();
		dbgbkpts_display_window();
		dbgpclog_display_window();
		dbgstack_display_window();
		dbgheap_display_window();
		dbgiop_display_window();
		dbgcode_display_window();	// the last has focus	
	}

	WND_TMR_STOP("Display Time");
}

// show previously created window
int gtk_debugger_enter(int context)
{
#ifndef NO_GDB
	gint type, id, mode;

	if (!dbg_on) gdbtk_show_insight();
#endif

	// debugger is open
	dbg_on = !0;

    // show breakpoint source (modal)
    switch(context)
    {
    case DBG_TRACE:
        break;
    case DBG_BREAK:
        break;
    }

    // display debugger windows (if not)
	gtk_debugger_display();
	gtk_debugger_refresh();

	// enable the debugger if GDB disabled it
	if (!options3.dbg_dock && !GTK_WIDGET_SENSITIVE(dbgw.regs))
		gtk_debugger_enable();

	// handle automatic debugging requests
#ifndef NO_GDB
	if (symfile)
	{
		// get context
		ti68k_bkpt_get_cause(&type, &mode, &id);

		if(type == BK_TYPE_PGMENTRY)
		{
			uint16_t handle, offset;
			uint32_t pc;

			ti68k_bkpt_get_pgmentry_offset(id, &handle, &offset);
			ti68k_bkpt_del_pgmentry(handle);
			if(options3.dbg_dock || GTK_WIDGET_VISIBLE(dbgw.bkpts))
				dbgbkpts_refresh_window();

			delete_command(NULL, 0);
			symbol_file_clear(0);
			gdbtk_clear_file ();
			ti68k_register_get_pc(&pc);
			pc -= offset-2;
			gdb_add_symbol_file(symfile, pc);
			g_free (symfile);
			symfile = NULL;
			exec_build_section_table();

			ti68k_unprotect_64KB_range(pc);

			gdb_hbreak("__main");
		}
	}
#endif

	return 0;
}

void gtk_debugger_close (void)
{
#ifndef NO_GDB
	// hide all windows
	gdbtk_hide_insight();
	dbg_on = 0;
	if(options3.dbg_dock)
		dbgdock_hide_all(!0);
	else
		dbgwnds_hide_all(!0);

    // and restarts the emulator
	ti68k_bkpt_set_cause(0, 0, 0);
	dbgbkpts_erase_context();

    if (engine_is_stopped()) gdbcall_continue();
#else
	// hide all windows
	dbg_on = 0;
	if(options3.dbg_dock)
		dbgdock_hide_all(!0);
	else
		dbgwnds_hide_all(!0);

    // and restarts the emulator
	ti68k_bkpt_set_cause(0, 0, 0);
	dbgbkpts_erase_context();

    engine_start();
#endif
}

static gint close_debugger_wrapper(gpointer data)
{
	gtk_debugger_close();
	return FALSE;
}

void gtk_debugger_close_async (void)
{
	g_idle_add(close_debugger_wrapper, NULL);
}

void gtk_debugger_disable(void)
{
	if(options3.dbg_dock)
		dbgdock_set_sensitivity(FALSE);
	else
		dbgwnds_set_sensitivity(FALSE);
}

void gtk_debugger_enable(void)
{
	if(options3.dbg_dock)
		dbgdock_set_sensitivity(TRUE);
	else
		dbgwnds_set_sensitivity(TRUE);
}
