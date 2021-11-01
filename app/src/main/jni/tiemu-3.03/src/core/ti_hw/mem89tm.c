/* Hey EMACS -*- linux-c -*- */
/* $Id: mem89tm.c 2428 2007-04-04 17:05:38Z roms $ */

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
    Memory management: TI89 Titanium without any HW protection
	Some values may be hard-coded for performance reasons !
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "libuae.h"
#include "ports.h"
#include "hw.h"
#include "mem.h"
#include "mem89tm.h"
#include "images.h"
#include "bkpts.h"
#include "m68k.h"
#include "ti68k_def.h"
#include "ti68k_int.h"
#include "flash.h"

// 000000-03ffff : RAM (256 KB)
// 100000-1fffff : unused
// 200000-2fffff : mirror of 0x000000
// 300000-3fffff : unused
// 400000-4fffff : mirror of 0x000000
// 500000-5fffff : unused
// 600000-6fffff : memory mapped I/O (all HW)
// 700000-7fffff : memory mapped I/O (HW2, HW3), non ghost'ed
// 800000-8fffff : FLASH (4 MB)
// 900000-9fffff :
// a00000-afffff :
// b00000-bfffff :
// c00000-cfffff : unused
// d00000-dfffff :	 ...
// e00000-efffff :   ...
// d00000-ffffff : unused

int ti89t_mem_init(void)
{
	// set mappers
	mem_get_byte_ptr = ti89t_get_byte;
	mem_get_word_ptr = ti89t_get_word;
	mem_get_long_ptr = ti89t_get_long;
	mem_put_byte_ptr = ti89t_put_byte;
	mem_put_word_ptr = ti89t_put_word;
	mem_put_long_ptr = ti89t_put_long;

	mem_get_real_addr_ptr = ti89t_get_real_addr;

    return 0;
}

uint8_t* ti89t_get_real_addr(uint32_t adr)
{
	// RAM access
	if(IN_BOUNDS(0x000000, adr, 0x03ffff) ||
	   IN_BOUNDS(0x200000, adr, 0x23ffff) ||
	   IN_BOUNDS(0x400000, adr, 0x43ffff))
	{
		return get_p(tihw.ram, adr, 0x03ffff);
	}

	// FLASH access
    else if(IN_BOUNDS(0x800000, adr, 0xbfffff))			
	{
		return get_p(tihw.rom, adr, ROM_SIZE_TI89T - 1);
	}

	// memory-mapped I/O
    else if(IN_BOUNDS(0x600000, adr, 0x6fffff))
	{
		return get_p(tihw.io, adr, IO1_SIZE_TI89T - 1);
	}

	// memory-mapped I/O (hw2)
	else if(IN_RANGE(adr, 0x700000, IO2_SIZE_TI89T))
	{
		return get_p(tihw.io2, adr, IO2_SIZE_TI89T - 1);
	}

	// memory-mapped I/O (hw3)
	else if(IN_RANGE(adr, 0x710000, IO3_SIZE_TI89T))
	{
		return get_p(tihw.io3, adr, IO3_SIZE_TI89T - 1);
	}

	return tihw.unused;
}

uint32_t ti89t_get_long(uint32_t adr) 
{
	// RAM access
	if(IN_BOUNDS(0x000000, adr, 0x03ffff) ||
	   IN_BOUNDS(0x200000, adr, 0x23ffff) ||
	   IN_BOUNDS(0x400000, adr, 0x43ffff))
	{
		return get_l(tihw.ram, adr, 0x03ffff);
	}

	// FLASH access
    else if(IN_BOUNDS(0x800000, adr, 0xbfffff))			
	{
		// use FlashReadLong due to Epson Device ID
		return FlashReadLong(adr);
	}

	// memory-mapped I/O
    else if(IN_BOUNDS(0x600000, adr, 0x6fffff))
	{
       return io_get_long(adr);
	}

	// memory-mapped I/O (hw2)
	else if(IN_RANGE(adr, 0x700000, IO2_SIZE_TI89T))
	{
		return io2_get_long(adr);
	}

	// memory-mapped I/O (hw3)
	else if(IN_RANGE(adr, 0x710000, IO3_SIZE_TI89T))
	{
		return io3_get_long(adr);
	}

    return 0x14141414;
}

uint16_t ti89t_get_word(uint32_t adr) 
{
	// RAM access
	if(IN_BOUNDS(0x000000, adr, 0x03ffff) ||
	   IN_BOUNDS(0x200000, adr, 0x23ffff) ||
	   IN_BOUNDS(0x400000, adr, 0x43ffff))
	{
		return get_w(tihw.ram, adr, 0x03ffff);
	}

	// FLASH access
    else if(IN_BOUNDS(0x800000, adr, 0xbfffff))			
	{
		return FlashReadWord(adr);
	}

	// memory-mapped I/O
    else if(IN_BOUNDS(0x600000, adr, 0x6fffff))
	{
       return io_get_word(adr);
	}

	// memory-mapped I/O (hw2)
	else if(IN_RANGE(adr, 0x700000, IO2_SIZE_TI89T))
	{
		return io2_get_word(adr);
	}

	// memory-mapped I/O (hw3)
	else if(IN_RANGE(adr, 0x710000, IO3_SIZE_TI89T))
	{
		return io3_get_word(adr);
	}

    return 0x1414;
}

uint8_t ti89t_get_byte(uint32_t adr) 
{
	// RAM access
	if(IN_BOUNDS(0x000000, adr, 0x03ffff) ||
	   IN_BOUNDS(0x200000, adr, 0x23ffff) ||
	   IN_BOUNDS(0x400000, adr, 0x43ffff))
	{
		return get_b(tihw.ram, adr, 0x03ffff);
	}

	// FLASH access
    else if(IN_BOUNDS(0x800000, adr, 0xbfffff))			
	{
		return FlashReadByte(adr);
	}

	// memory-mapped I/O
    else if(IN_BOUNDS(0x600000, adr, 0x6fffff))
	{
       return io_get_byte(adr);
	}

	// memory-mapped I/O (hw2)
	else if(IN_RANGE(adr, 0x700000, IO2_SIZE_TI89T))
	{
		return io2_get_byte(adr);
	}

	// memory-mapped I/O (hw3)
	else if(IN_RANGE(adr, 0x710000, IO3_SIZE_TI89T))
	{
		return io3_get_byte(adr);
	}

    return 0x14;
}

void ti89t_put_long(uint32_t adr, uint32_t arg) 
{
	// RAM access
	if(IN_BOUNDS(0x000000, adr, 0x03ffff) ||
	   IN_BOUNDS(0x200000, adr, 0x23ffff) ||
	   IN_BOUNDS(0x400000, adr, 0x43ffff))
	{
		put_l(tihw.ram, adr, 0x03ffff, arg);
	}

	// FLASH access
    else if(IN_BOUNDS(0x800000, adr, 0xbfffff))
	{
		FlashWriteLong(adr, arg);
	}

	// memory-mapped I/O
    else if(IN_BOUNDS(0x600000, adr, 0x6fffff))			
	{
		io_put_long(adr, arg);
	}

	// memory-mapped I/O (hw2)
	else if(IN_RANGE(adr, 0x700000, IO2_SIZE_TI89T))
	{
		io2_put_long(adr, arg);
	}

	// memory-mapped I/O (hw3)
	else if(IN_RANGE(adr, 0x710000, IO3_SIZE_TI89T))
	{
		io3_put_long(adr, arg);
	}

    return;
}

void ti89t_put_word(uint32_t adr, uint16_t arg) 
{
	// RAM access
	if(IN_BOUNDS(0x000000, adr, 0x03ffff) ||
	   IN_BOUNDS(0x200000, adr, 0x23ffff) ||
	   IN_BOUNDS(0x400000, adr, 0x43ffff))
	{
		put_w(tihw.ram, adr, 0x03ffff, arg);
	}

	// FLASH access
    else if(IN_BOUNDS(0x800000, adr, 0xbfffff))
	{
		FlashWriteWord(adr, arg);
	}

	// memory-mapped I/O
    else if(IN_BOUNDS(0x600000, adr, 0x6fffff))			
	{
		io_put_word(adr, arg);
	}

	// memory-mapped I/O (hw2)
	else if(IN_RANGE(adr, 0x700000, IO2_SIZE_TI89T))
	{
		io2_put_word(adr, arg);
	}

	// memory-mapped I/O (hw3)
	else if(IN_RANGE(adr, 0x710000, IO3_SIZE_TI89T))
	{
		io3_put_word(adr, arg);
	}

    return;
}

void ti89t_put_byte(uint32_t adr, uint8_t arg) 
{
	// RAM access
	if(IN_BOUNDS(0x000000, adr, 0x03ffff) ||
	   IN_BOUNDS(0x200000, adr, 0x23ffff) ||
	   IN_BOUNDS(0x400000, adr, 0x43ffff))
	{
		put_b(tihw.ram, adr, 0x03ffff, arg);
	}

	// FLASH access
    else if(IN_BOUNDS(0x800000, adr, 0xbfffff))
	{
		FlashWriteByte(adr, arg);
	}

	// memory-mapped I/O
    else if(IN_BOUNDS(0x600000, adr, 0x6fffff))			
	{
		io_put_byte(adr, arg);
	}

	// memory-mapped I/O (hw2)
	else if(IN_RANGE(adr, 0x700000, IO2_SIZE_TI89T))
	{
		io2_put_byte(adr, arg);
	}

	// memory-mapped I/O (hw3)
	else if(IN_RANGE(adr, 0x710000, IO3_SIZE_TI89T))
	{
		io3_put_byte(adr, arg);
	}

    return;
}
