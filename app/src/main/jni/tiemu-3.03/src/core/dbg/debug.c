/* Hey EMACS -*- linux-c -*- */
/* $Id: debug.c 2792 2008-05-26 16:48:30Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Li�vin
 *  Copyright (c) 2005, Romain Li�vin, Kevin Kofler
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
    Debug: debugging functions
*/

#ifdef HAVE_CONFIG_H
# include <tiemuconfig.h>
#endif

//#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>

#include "libuae.h"
#include "m68k.h"
#include "romcalls.h"
#include "debug.h"
#include "ti68k_def.h"
#include "ti68k_err.h"

/* Flushes GDB's register cache */
extern void registers_changed(void);
/* Flushes GDB's frame cache */
extern void reinit_frame_cache(void);
/* Refreshes Insight */
extern void gdbtk_update(void);

int ti68k_debug_get_pc(void)
{
	return m68k_getpc();
}

int ti68k_debug_get_old_pc(void)
{
	return logger.pclog_buf[(logger.pclog_ptr + logger.pclog_size-1) % logger.pclog_size];
}

int ti68k_debug_break(void)
{
    regs.spcflags |= SPCFLAG_BRK;
    return 0;
}

int ti68k_debug_trace(void)
{
    // Set up an internal trap (DBTRACE) which will 
    // launch/refresh the debugger when encountered
    regs.spcflags |= SPCFLAG_DBTRACE;

    return 0;
}

int ti68k_debug_step(void)
{
    regs.spcflags |= SPCFLAG_DBSKIP;
	return ti68k_debug_do_instructions(1);
}

static const uint16_t rets[] = { 
	
	0x4e77,		// RTR
	0x4e75,		// RTS
	0x4e74,		// RTD
	0x4e73,		// RTE
	0x4e72,		// STOP
};

static inline int is_ret_inst(uint16_t inst)
{
	int i;
	for(i = 0; i < sizeof(rets) / sizeof(uint16_t); i++)
		if(curriword() == rets[i])
			return !0;
	return 0;
}

static inline int is_bsr_inst(uint16_t ci)
{
	int t1, t2, t3, t4, t5;
	
	t1 = ((ci >> 6) == (0x4e80 >> 6));						/* jsr */
    t2 = ((ci >> 8) == (0x61ff >> 8));						/* bsr */
    t3 = (ci >= 0xf800 && ci <= 0xffee);						/* fline */
	t4 = (ci == 0xfff0) || (ci == 0xfff2) ||
		((ci & 0xf000) == 0x5000) && ((ci & 0x00f8) == 0x00c8);	/* dbcc */
	t5 = ((ci >> 4) == (0x4e40 >> 4));						/* trap */

	//printf("<%i %i %i %i %i>\n", ret1, ret2, ret3, ret4, ret5);

	return t1 || t2 || t3 || t4 || t5;
}

int ti68k_step_over_noflush(void)
{
    uint32_t curr_pc, next_pc;
	gchar *output;

	// get current PC and next PC
	curr_pc = m68k_getpc();	
	next_pc = curr_pc + ti68k_debug_disassemble(curr_pc, &output);
	//printf("$%06x => $%06x %04x <%s>\n", curr_pc, next_pc, curriword(), output);
	g_free(output);	

	// check current instruction
	if(!is_bsr_inst((uint16_t)curriword()))
	{
		ti68k_debug_step();
		return 0;
	}

	// run emulation until address after instruction is reached
	hw_m68k_run(1);
	// FIXME: While _bcd_math also exists on the TI-92, we can't easily find it.
	// romcalls_get_symbol_address definitely won't work on the TI-92.
	if(tihw.calc_type != TI92)
	{
		uint32_t bcd_math_pc;
		// _bcd_math returns to pc + 2 rather than pc
		// This works only for jsr calls. F-Line calls are handled in the disassembler.
		romcalls_get_symbol_address(0xb5, &bcd_math_pc);
		if (m68k_getpc() == bcd_math_pc)
			next_pc += 2;
	}
	while ((next_pc != m68k_getpc()) && !(regs.spcflags & SPCFLAG_BRK))
	{
		hw_m68k_run(1);

		// force GUI refresh in order to be able to cancel operation
		//while(gtk_events_pending()) gtk_main_iteration_do(FALSE);
	}

	if(regs.spcflags & SPCFLAG_BRK)
		regs.spcflags &= ~SPCFLAG_BRK;

    return 0;
}

int ti68k_debug_step_over(void)
{
	int result = ti68k_step_over_noflush();
#ifndef NO_GDB
    registers_changed();
	reinit_frame_cache();
	gdbtk_update();
#endif
    return result;
}

int ti68k_debug_step_out(void)
{
	uint32_t curr_pc, next_pc;
	gchar *output;
	//int i;

	// get current PC and next PC
	curr_pc = m68k_getpc();	
	next_pc = curr_pc + ti68k_debug_disassemble(curr_pc, &output);
	//printf("$%06x => $%06x <%s>\n", curr_pc, next_pc, output);
	g_free(output);	

	// run emulation until address is reached
	do
	{
		hw_m68k_run(1);

		//printf("$%06x: %06x\n", m68k_getpc(), curriword());
		if(is_ret_inst((uint16_t)curriword()))
		{
			hw_m68k_run(1);
#ifndef NO_GDB
			registers_changed();
			reinit_frame_cache();
			gdbtk_update();
#endif
			return 0;
		}	
	}
	while(1);

    return 0;
}

int ti68k_debug_skip(uint32_t next_pc)
{
	// run emulation until address is reached
    do 
    {
		ti68k_debug_step();
/*
		// too far: stop
		if(m68k_getpc() > next_pc) 
			break;

		// jump back: stop
		if(next_pc - m68k_getpc() > 0x80) 
			break;
*/
		// force GUI refresh in order to be able to cancel operation
		//while(gtk_events_pending()) gtk_main_iteration_do(FALSE);
    } 
    while ((next_pc != m68k_getpc()) && !(regs.spcflags & SPCFLAG_BRK));

	if(regs.spcflags & SPCFLAG_BRK)
		regs.spcflags &= ~SPCFLAG_BRK;

    return 0;
}

int ti68k_debug_do_instructions(int n)
{
	int result = hw_m68k_run(n);
#ifndef NO_GDB
    registers_changed();
	reinit_frame_cache();
	gdbtk_update();
#endif
    return result;
}

// Used to read/modify/write memory directly from debugger
uint8_t* ti68k_get_real_address(uint32_t addr)
{
	return hw_get_real_address(addr);
}

int ti68k_debug_is_supervisor(void)
{
    return regs.s;
}
