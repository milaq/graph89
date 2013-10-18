/* Hey EMACS -*- linux-c -*- */
/* $Id: iodefs.h 2268 2006-11-06 17:18:51Z roms $ */

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
  Breakpoint definitions
*/

#ifndef __IODEFS_H__
#define __IODEFS_H__

#include <stdio.h>
#include <stdint.h>
#include <glib.h>

/* Types */

typedef enum
{
	IO_RO = 1, IO_WO = 2, IO_RW = 3,
} IO_ACC;

typedef struct
{
	uint32_t	addr;		// $600000
	int			size;		// 1, 2, 4 bytes
	int			type;		// ro, wo, rw

	char*		bit_str;	// <..5...1.>
	int			bits[32];	// bit number like 1,5

	int			nbits;		// number of bits usable
	int			all_bits;	// set to 1 if all bits are used

	char*		bit_name[32];// name of each bit (like SLE)
	char*		name;		// "Constrast and battery status"
} IO_DEF;

/* Functions */

int iodefs_load(const char *path);
int iodefs_unload(void);

GNode* iodefs_tree(void);

#endif
