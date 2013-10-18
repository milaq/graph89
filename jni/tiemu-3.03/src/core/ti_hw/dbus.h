/* Hey EMACS -*- linux-c -*- */
/* $Id: main.c 245 2004-05-23 20:45:43Z roms $ */

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

#ifndef __TI68K_DBUS__
#define __TI68K_DBUS__

#include "tilibs.h"

int hw_dbus_init(void);
int hw_dbus_reset(void);
int hw_dbus_exit(void);

extern void    (*hw_dbus_reinit)	(void);
extern void    (*hw_dbus_putbyte)	(uint8_t arg);
extern uint8_t (*hw_dbus_getbyte)	(void);
extern int     (*hw_dbus_checkread)	(void);

int send_ti_file(const char *filename);

int recfile(void);

/* Variables */

extern CableHandle* cable_handle;
extern CalcHandle*  calc_handle;

extern int recfile_flag;

#endif

