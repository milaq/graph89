/*
 * libtilemdb - Utilities for debugging Z80 assembly programs
 *
 * Copyright (C) 2012 Benjamin Moody
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include "tilemdb.h"

/* Allow up to 16 bytes per line for macros */
#define REWIND_WINDOW_SIZE 16

/* Search a maximum of 256 bytes for convergent disassemblies */
#define REWIND_LIMIT 256

dword tilem_disasm_guess_prev_address(const TilemDisasm* dasm,
                                      TilemCalc* calc, int phys, dword addr)
{
	dword destbuf[REWIND_LIMIT];
	unsigned int n = 0;
	unsigned int i;
	dword a1, a2;

#define DEST(x) destbuf[addr - 1 - (x)]

	while (n < REWIND_LIMIT) {
		n++;
		a1 = addr - n;

		/* determine address in disassembly following a1 */

		if (n > addr)
			a2 = (dword) -1;
		else
			tilem_disasm_disassemble(dasm, calc, phys, a1,
			                         &a2, NULL, 0);

		/*
		  set DEST(a1) to:

		   -1, if this disassembly misses addr

		   the address preceding addr in disassembly, if this
		   disassembly hits addr
		*/

		if (a2 == addr)
			DEST(a1) = a1;
		else if (a2 < addr)
			DEST(a1) = DEST(a2);
		else
			DEST(a1) = (dword) -1;

		if (n < REWIND_WINDOW_SIZE)
			continue;

		/* if all of the previous REWIND_WINDOW_SIZE values of
		   DEST are -1, then (because we are assuming no
		   instruction is longer than REWIND_WINDOW_SIZE)
		   there is no disassembly that leads to addr. */

		for (i = 0; i < REWIND_WINDOW_SIZE; i++)
			if (DEST(a1 + i) != (dword) -1)
				break;

		if (i == REWIND_WINDOW_SIZE)
			break;

		/* if, among the previous REWIND_WINDOW_SIZE values of
		   DEST, the only ones that are not -1 are all equal,
		   then all disassemblies that hit addr must also pass
		   through a. */

		a2 = DEST(a1 + i);
		for (; i < REWIND_WINDOW_SIZE; i++)
			if (DEST(a1 + i) != (dword) -1 && DEST(a1 + i) != a2)
				break;

		if (i == REWIND_WINDOW_SIZE)
			return a2;
	}

	if (addr != 0)
		return addr - 1;
	else if (phys)
		return (calc->hw.romsize + calc->hw.ramsize - 1);
	else
		return 0xffff;
}
