/* Hey EMACS -*- linux-c -*- */
/* $Id: engine.c 2686 2007-11-30 01:24:44Z kevinkofler $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Li�vin
 *  Copyright (c) 2005-2007, Romain Li�vin, Kevin Kofler
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
 * Modified to run in Android OS. Dritan Hashorva 2012
 */


/*
    Run engine from GTK main loop at regular interval.
*/

#ifdef HAVE_CONFIG_H
#include <tiemuconfig.h>
#endif

#include <glib.h>
#include <signal.h>

#include "intl.h"
#include "ti68k_def.h"
#include "ti68k_int.h"
#include "m68k.h"
#include "engine.h"
#include "logging.h"
#include "tsource.h"

#define dbg_on 0

void sim_exception(int which);
#ifndef SIGTRAP
/* WARNING: This MUST match the definitions in GDB and sim. */
#define SIGTRAP 5
#endif

/* 
   The TI92/89 should approximately execute NUM_CYCLES_PER_LOOP_HW[12] in 
   ENGINE_TIME_LIMIT milliseconds (10.000.000 or 12.000.000 cycles/s).
   If you think this values are a bit too big, you can slow down 
   the emulator by changing them 
*/
#define NUM_CYCLES_PER_LOOP_HW1 300000	// 300000 cycles in 30ms
#define NUM_CYCLES_PER_LOOP_HW2 360000	// 360000 cycles in 30ms
#define NUM_CYCLES_PER_LOOP_HW4 480000	// 480000 cycles in 30ms
#define MIN_INSTRUCTIONS_PER_CYCLE   4	// instructions take at least 4 cycles

static int cpu_cycles = NUM_CYCLES_PER_LOOP_HW2;

static guint tid = 0;

// returns the instruction rate (default or custom value)
int engine_num_cycles_per_loop(void)
{
	if(params.cpu_rate != -1)
		return params.cpu_rate;
	else if (tihw.hw_type == HW1)
		return NUM_CYCLES_PER_LOOP_HW1;
	else if (tihw.hw_type <= HW3)
		return NUM_CYCLES_PER_LOOP_HW2;
	else
		return NUM_CYCLES_PER_LOOP_HW4;
}

// function called by g_timeout_add_full/g_idle_add_full
static gboolean engine_func(gint *data)
{
	gint    res;
	
	// set instruction rate
	cpu_cycles = engine_num_cycles_per_loop();

	// run emulation core
	res = hw_m68k_run(cpu_cycles / MIN_INSTRUCTIONS_PER_CYCLE);

	// a bkpt has been encountered ? If yes, stop engine
	if(res)
	{
//		if (!dbg_on)
//			gtk_debugger_enter(GPOINTER_TO_INT(res));
//#ifndef NO_GDB
//		sim_exception(bkpts.type ?
//		              ((bkpts.type == BK_CAUSE_EXCEPTION || bkpts.type == BK_CAUSE_PROTECT) ? SIGSEGV
//		                                                                                    : SIGTRAP)
//		              : SIGINT);
//#endif
		tid = 0;	// reset source id, we're stopping the engine
		return FALSE;	// stop engine !
	}

	return TRUE;
}

// start emulation engine
void engine_start(void) 
{
/*	if(params.restricted)
		tid = g_timeout2_add_full(G_PRIORITY_DEFAULT_IDLE, ENGINE_TIME_LIMIT, 
				(GSourceFunc)engine_func, NULL, NULL);
	else
		tid = g_idle_add_full(G_PRIORITY_DEFAULT_IDLE, 
				(GSourceFunc)engine_func, NULL, NULL);*/
}

// stop it
void engine_stop(void) 
{
/*	if(tid)
	{
		g_source_remove(tid);
		// and reset source id
		tid = 0;
	}
	*/
}


