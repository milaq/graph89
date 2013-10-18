/* Hey EMACS -*- linux-c -*- */
/* $Id: dbg_code.h 2268 2006-11-06 17:18:51Z roms $ */

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

#include <gtk/gtk.h>

typedef struct {
	GtkWidget *mem;
	GtkWidget *regs;
	GtkWidget *bkpts;
	GtkWidget *code;
	GtkWidget *pclog;
	GtkWidget *stack;
	GtkWidget *heap;
} DbgHandleBoxes;
extern DbgHandleBoxes dbghb;

GtkWidget* dbgdock_create_window(void);
GtkWidget* dbgdock_display_window(void);

void dbgdock_set_sensitivity(int state);

void dbgdock_show_all(int all);
void dbgdock_hide_all(int all);
