/* Hey EMACS -*- linux-c -*- */
/* $Id: romcalls.h 864 2005-02-22 09:54:05Z roms $ */

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

#ifndef __TIMEM__
#define __TIMEM__

#include "mem.h"

/*
	Functions
*/

uint16_t rd_word(uint8_t *p);
uint32_t rd_long(uint8_t *p);

void wr_word(uint8_t *p, uint16_t d);
void wr_long(uint8_t *p, uint32_t d);

#define mem_rd_byte		mem_get_byte_ptr
#define mem_rd_word		mem_get_word_ptr
#define mem_rd_long		mem_get_long_ptr

#define mem_wr_byte		mem_put_byte_ptr
#define mem_wr_word		mem_put_word_ptr
#define mem_wr_long		mem_put_long_ptr

void mem_rd_block(uint32_t a, uint8_t *d, uint16_t len);
void mem_wr_block(uint32_t a, uint8_t *d, uint16_t len);

#endif
