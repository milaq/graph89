/* Hey EMACS -*- linux-c -*- */
/* $Id: dbg_all.h 2832 2009-05-08 10:56:40Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Liévin
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

#ifndef __DBG_ALL_H__
#define __DBG_ALL_H__

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

/* Variables */

extern int dbg_on;
extern int dbg_load;
extern gchar *symfile;

/* Functions */

void gtk_debugger_preload(void);
void gtk_debugger_display(void);
void gtk_debugger_refresh(void);

void gtk_debugger_disable(void);
void gtk_debugger_enable(void);

int  gtk_debugger_enter(int context);
void gtk_debugger_close(void);
void gtk_debugger_close_async(void);

G_END_DECLS

#endif
