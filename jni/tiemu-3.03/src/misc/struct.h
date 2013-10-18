/* Hey EMACS -*- linux-c -*- */
/* $Id: struct.h 2709 2007-12-13 17:37:31Z roms $ */

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

#ifndef STRUCT_H
#define STRUCT_H

#ifdef HAVE_CONFIG_H
# include <tiemuconfig.h>
#endif

#include <glib.h>

#include "paths.h"

/* Constants */

#define MAXCHARS 256

#define VIEW_NORMAL 1
#define VIEW_LARGE  2
#define VIEW_FULL   3
#define VIEW_CUSTOM	4

/* General options */
typedef struct
{
	gchar*  skin_file;
	gint	skin;
	gint    view;
	gfloat	scale;

    gchar*  keys_file;
	gint	kbd_dbg;

	gint	console;
	gint    fs_type;
	
	gchar*	qs_file;
	gint	qs_enabled;

	gchar*	kp_rec_file;
	gint	kp_rec_enabled;
	gchar*	kp_ply_file;
	gint	kp_ply_enabled;
} TieOptions;

/* Screen capture options */
typedef struct 
{
	int		format;
	int		type;
	int		size;

	char*	file;
	int		counter;
	char*	folder;

	int		shots;
	int		skips;

	int		clipboard;
} ScrOptions;

/* Debugger options for windows size and placement */
typedef struct 
{
	int x;
	int y;
	int w;
	int h;
} GdkRect;

typedef struct 
{
	GdkRect	rect;
	gint	minimized;
	gint	closed;
} WinState;

typedef struct 
{
	WinState	bkpts;
	WinState	code;
	WinState	mem;
	WinState	regs;
    WinState	pclog;
    WinState	stack;
	WinState	heap;
	WinState	iop;
	WinState	dock;

	gint		transient;

	WinState	calc;

	gint		dbg_font_type;
	gchar*		dbg_font_name;

	gint		dbg_dock;
} DbgOptions;

extern TieOptions options;
extern ScrOptions options2;
extern DbgOptions options3;

#endif
