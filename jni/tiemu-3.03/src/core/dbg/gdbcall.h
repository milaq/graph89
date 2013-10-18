/* Hey EMACS -*- linux-c -*- */
/* $Id: gdbcall.h 2155 2006-07-20 19:53:06Z kevinkofler $ */

/*  TiEmu - a TI emulator
 *
 *  This file Copyright (c) 2005-2006, Kevin Kofler
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
    Gdbcall: GDB interfacing functions
*/

void gdbcall_run(void);
void gdbcall_continue(void);
void gdb_add_symbol_file(const char *filename, unsigned address);
void gdb_hbreak(const char *funcname);
