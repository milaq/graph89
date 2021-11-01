/* Hey EMACS -*- linux-c -*- */
/* $Id: debug.h 2603 2007-07-14 17:09:56Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Liévin
 *  Copyright (c) 2005, Romain Liévin
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

#ifndef __TI68K_DBG__
#define __TI68K_DBG__

int ti68k_debug_get_pc(void);
int ti68k_debug_get_old_pc(void);

uint32_t ti68k_debug_disassemble(uint32_t addr, char **line);

int ti68k_debug_break(void);
int ti68k_debug_trace(void);
int ti68k_debug_step(void);
int ti68k_debug_step_over(void);
int ti68k_debug_step_out(void);
int ti68k_debug_skip(uint32_t next_pc);
int ti68k_debug_do_instructions(int n);

int ti68k_debug_is_supervisor(void);

#endif
