/* Hey EMACS -*- linux-c -*- */
/* $Id: files.h 1789 2005-09-20 20:06:46Z roms $ */

/*  TiLP - Ti Linking Program
 *  Copyright (C) 1999-2005  Romain Lievin
 *  Copyright (c) 2005, Romain Liévin (tweaked for TiEmu)
 *
 *  This program is free software you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __TILP_FILES__
#define __TILP_FILES__

int tiemu_file_copy(const char *src, const char *dst);
int tiemu_file_move(const char *src, const char *dst);
int tiemu_file_delete(const char *f);
int tiemu_file_mkdir(const char *pathname);
int tiemu_file_exist(const char* filename);
int tiemu_file_check(const char *src, char **dst);
int tiemu_file_move_with_check(const char *src, const char *dst);
int tiemu_file_chdir(const char *path);
#define tiemu_chdir	tiemu_file_chdir

#endif
