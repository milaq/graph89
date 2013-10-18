/* Hey EMACS -*- linux-c -*- */
/* $Id: calc.h 1977 2006-02-18 13:13:31Z roms $ */

/*  TiEmu - a TI emulator
 *  Copyright (c) 2007, Romain Liévin
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

#ifndef __KEYPRESS_H__
#define __KEYPRESS_H__

int  kp_recording_start(const char *filename);
int  kp_recording_stop(void);
int kp_recording_key(int key, int action);

int  kp_playing_start(const char *filename);
int  kp_playing_stop(void);
int kp_playing_key(int *key, int *action);

#endif
