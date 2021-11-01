/* Hey EMACS -*- linux-c -*- */
/* $Id: type2str.h 2268 2006-11-06 17:18:51Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2002, Romain Lievin, Julien Blache
 *  Copyright (c) 2003-2004, Romain Liévin
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

#ifndef __TI68K_TYPE2STR__
#define __TI68K_TYPE2STR__

const char *ti68k_calctype_to_string(int type);
int         ti68k_string_to_calctype(const char *str);

const char *ti68k_romtype_to_string(int type);
int         ti68k_string_to_romtype(const char *str);

const char *ti68k_hwtype_to_string(int type);
int         ti68k_string_to_hwtype(const char *str);

const char *ti68k_exception_to_string(int number);

const char *ti68k_bkpt_type_to_string(int type);
int         ti68k_string_to_bkpt_type(const char *str);

const char *ti68k_bkpt_mode_to_string(int type, int mode);
int         ti68k_string_to_bkpt_mode(const char * str);

#endif
