/* Hey EMACS -*- linux-c -*- */
/* $Id: version.h 2820 2009-05-03 14:07:15Z roms $ */

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

#ifndef VERSION_H
#define VERSION_H

#ifdef HAVE_CONFIG_H
# include <tiemuconfig.h>
#endif

/*
  This file contains version number
  and library requirements.
*/

#ifdef __WIN32__
# define TIEMU_VERSION "3.03"		// For Win32
#else
# define TIEMU_VERSION VERSION
#endif
#define TIEMU_REQUIRES_LIBCABLES_VERSION	"1.3.0"
#define TIEMU_REQUIRES_LIBCALCS_VERSION 	"1.0.7"
#define TIEMU_REQUIRES_LIBFILES_VERSION		"1.0.7"
#define TIEMU_REQUIRES_LIBCONV_VERSION		"1.0.4"

#endif


