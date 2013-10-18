/* Hey EMACS -*- linux-c -*- */
/* $Id: handles.h 2268 2006-11-06 17:18:51Z roms $ */

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

#ifndef __HANDLES__
#define __HANDLES__

#include <stdint.h>

void heap_get_addr(uint32_t *base);

void heap_get_block_addr(int handle, uint32_t *addr);
void heap_get_block_size(int handle, uint16_t *size);
void heap_get_block_addr_and_size(int handle, uint32_t *addr, uint16_t *size);

void heap_search_for_address(uint32_t address, int *handle);

// aliases
uint32_t heap_deref(int handle);
uint16_t heap_size(int handle);

#define HEAP_MAX_SIZE 2000

#endif
