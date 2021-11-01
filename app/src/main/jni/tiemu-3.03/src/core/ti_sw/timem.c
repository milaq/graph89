/* Hey EMACS -*- linux-c -*- */
/* $Id: timem.c 2268 2006-11-06 17:18:51Z roms $ */

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

/*
    Memory access (take care of little/big endian issues)
*/

#include <stdint.h>
#include <glib.h>

#include "timem.h"
#include "mem.h"

/* General functions (not related to TI memory) */

uint16_t rd_word(uint8_t *p)
{
	uint16_t *p16 = (uint16_t *)p;
	return GUINT16_FROM_BE(*p16);
}

uint32_t rd_long(uint8_t *p)
{
	uint32_t *p32 = (uint32_t *)p;
	return GUINT32_FROM_BE(*p32);
}

void wr_word(uint8_t *p, uint16_t d)
{
	uint16_t *p16 = (uint16_t *)p;
	*p16 = GUINT16_TO_BE(d);
}

void wr_long(uint8_t *p, uint32_t d)
{
	uint32_t *p32 = (uint32_t *)p;
	*p32 = GUINT32_TO_BE(d);
}

/* Memory access functions (TI memory) */

void mem_rd_block(uint32_t a, uint8_t *d, uint16_t len)
{
	int i;

	for(i = 0; i < len; i++)
		d[i] = mem_rd_byte(a+i);
}

void mem_wr_block(uint32_t a, uint8_t *d, uint16_t len)
{
	int i;

	for(i = 0; i < len; i++)
		mem_wr_byte(a+i, d[i]);
}
