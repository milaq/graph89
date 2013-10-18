/* Hey EMACS -*- linux-c -*- */
/* $Id: extern.h 2268 2006-11-06 17:18:51Z roms $ */

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

#ifndef EXTERN_H
#define EXTERN_H

#include <gtk/gtk.h>

#include "struct.h"

/* Some linked-list for breakpoints */
extern GList *bkpt_address_list;
extern GList *bkpt_access_list;
extern GList *bkpt_vector_list;
extern GList *bkpt_trap_list;

/* Used widget */
extern GtkWidget *debugger_dbox;
extern GtkWidget *code_clist;
extern GtkWidget *reg_text;
extern GtkWidget *data_bkpt_dbox;

extern gint selected_row;
extern gint data_bkpt_selected_row;
extern gint bkpt_encountered;

#endif
