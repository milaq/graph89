/* Hey EMACS -*- linux-c -*- */
/* $Id: hid.h 557 2004-07-14 08:17:39Z roms $ */

/*  TiEmu - a TI emulator
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

#ifndef __KBDMAPPER_H__
#define __KBDMAPPER_H__

#define KEYMAP_MAX  128

// types

typedef struct {
    int         value;
    const char  *name;
} KeyTuple;

typedef struct {
    int pc_key;     // PC key
    int ti_key;     // TI key
    int modifier;   // TI key such as TIKEY_ALPHA
} Pc2TiKey;

extern Pc2TiKey keymap[KEYMAP_MAX];

extern const KeyTuple tikeys[];
extern const KeyTuple pckeys[];

// functions

int keymap_string_to_value(const KeyTuple *array, const char *key_name);
const char* keymap_value_to_string(const KeyTuple *array, int key_value);

int keymap_read_header(const char *filename);
int keymap_load(const char *filename);

#endif
