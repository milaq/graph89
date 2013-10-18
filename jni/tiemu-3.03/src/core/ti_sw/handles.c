/* Hey EMACS -*- linux-c -*- */
/* $Id: handles.c 2268 2006-11-06 17:18:51Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Liévin
 *  Copyright (c) 2005, Romain Liévin, Kevin Kofler
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
    Handles/Heap access:
	$5D42 is a pointer on the Handles[] array. An handle is an index in this array
	which poins on a memory allocatedblock :
	Handles[0] => block 0
	Handles[1] => block 1 ....

	The first word just before the beginning of the block is the block size.

	- HeapAlloc: | size | block |
				  		  |
	HeapDeref           --+

	- HeapAllocPtr:		--+
						  |
		| size | handle | block |
				 |
	HeapDeref  --+

	- PedRom: | size.l | handle | block |
								  |
	HeapDeref					--+


*/

#include <stdio.h>

#include "handles.h"
#include "romcalls.h"
#include "ti68k_def.h"
#include "ti68k_int.h"

static int pedrom = 0;

/*
	Retrieve address of heap (pointed by $5D42 on TI92).
*/
void heap_get_addr(uint32_t *base)
{
	pedrom = (mem_rd_word(0x32) == (('R'<<8)+'O'));

	if(pedrom && mem_rd_word(0x30)<=0x0080) // PedroM <=0.80
	{
		uint32_t ptr = 0x5d58;				// fixed by PPhD for AMS1 compat
		*base = mem_rd_long(ptr);
	}
	else if(tihw.ti92v2) // TI-92 II
	{
		uint32_t ptr = 0x4720 + 0x1902;		//tios::main_lcd equ tios::globals+$0000
		*base = mem_rd_long(ptr);
	}
	else if(tihw.ti92v1) // TI-92 I
	{
		uint32_t ptr = 0x4440 + 0x1902;		//and tios::heap equ tios::globals+$1902
		*base = mem_rd_long(ptr);
	}
	else
	{
		uint32_t b, size, addr, ptr;

		romcalls_get_table_infos(&b, &size);
		if(size < 0x441 && !pedrom) // AMS 1
		{
			romcalls_get_symbol_address(0x96, &addr);	// tios::HeapDeref (#0x096)
			ptr = mem_rd_word(addr + 8);				// MOVEA.W $7592,A0
			*base = mem_rd_long(ptr);
		} else // AMS 2, PedroM >=0.81
		{
			romcalls_get_symbol_address(0x441, &addr);	// tios::HeapTable	(#0x441)
			*base  = addr;
		}
	}
}

/*
	Get address of an allocated block (like HeapDeref)
*/

uint32_t heap_deref(int handle)
{
	uint32_t base;

	heap_get_addr(&base);
	return mem_rd_long(base + 4*handle);
}

void heap_get_block_addr(int handle, uint32_t *addr)
{
	*addr = heap_deref(handle);
}

/*
	Get size of an allocated block (like HeapSize)
*/
uint16_t heap_size(int handle)
{
	uint32_t base;
	uint32_t addr;
	uint16_t size;

	heap_get_addr(&base);

	addr = mem_rd_long(base + 4*handle);
	if(!pedrom)
	{
		size = mem_rd_word(addr - 2);
		size &= ~(1 << 16);	// remove lock
		size <<= 1;			// size is twice
		size -= 2;
	}
	else if(addr >= tihw.rom_base) // archived file on PedroM - use file size
	{
		size = mem_rd_word(addr);
		size += 2;
	}
	else
	{
		size = (uint16_t)mem_rd_long(addr - 6);
		size -= 6;
	}

	return size;
}

void heap_get_block_size(int handle, uint16_t *size)
{
	*size = heap_size(handle);
}

/*
	Given an handle, retrieve block size and block address
*/
void heap_get_block_addr_and_size(int handle, uint32_t *addr, uint16_t *size)
{
	*addr = heap_deref(handle);
	*size = heap_size(handle);
}

/*
	Walk in the heap to search for a block address.
*/
void heap_search_for_address(uint32_t address, int *handle)
{
	uint32_t base;
	int i;

	heap_get_addr(&base);

	for(i = 1; i < HEAP_MAX_SIZE; i++)
	{
		uint32_t addr = mem_rd_long(base + 4*i);
		uint16_t size = mem_rd_word(addr - 2);
		
		if (addr && (address >= addr) && (address < addr+size))
			*handle = i;
	}

	*handle = -1;
}

//#define HeapDeref(handle) HeapTable[handle]
//#define HeapSize(handle) ((short*)HeapDeref(handle))[-1]<<1;
