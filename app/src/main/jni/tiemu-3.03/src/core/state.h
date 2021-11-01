/* Hey EMACS -*- linux-c -*- */
/* $Id: state.h 2601 2007-07-14 08:49:30Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2002, Romain Lievin, Julien Blache
 *  Copyright (c) 2003-2004, Romain Liévin
 *  Copyright (c) 2005-2006, Romain Liévin, Kevin Kofler
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

#ifndef __TI68K_STATE__
#define __TI68K_STATE__

// Please update the docs/TiEmu_img_format.txt documentation when making changes
// on the structure below

// If this structure is modified, the SAV_REVISION number (state.c)
// has to be incremented.
typedef struct
{
	long	revision;		// structure revision
	long	size;			// and size (backwards compatibility)
	long    regs_offset;    // offset to M68K area
    long    io_offset;      // offset to IO area
    long    ram_offset;     // offset to RAM area
	long	misc_offset;	// offset to extra informations
    long    bkpts_offset;   // offset to bkpts area
	long	rom_offset;		// offset to FLASH changes
	long	str_offset;		// offset to image location (string)
} SAV_INFO;

int ti68k_state_load(const char *filename);
int ti68k_state_save(const char *filename);

int ti68k_state_parse(const char *filename, char **rom_file, char **tib_file);

int ti68k_is_a_sav_file(const char *filename);

#endif
