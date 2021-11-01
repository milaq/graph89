/* Hey EMACS -*- linux-c -*- */
/* $Id: engine.h 2556 2007-06-24 05:05:05Z kevinkofler $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Liévin
 *  Copyright (c) 2005, Romain Liévin
 *  Copyright (c) 2006-2007 Kevin Kofler
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

#ifndef __ENGINE_H__
#define __ENGINE_H__

#define ENGINE_TIME_LIMIT 30 // 30 ms

#ifdef __cplusplus
extern "C" {
#endif
int engine_num_cycles_per_loop(void);

void engine_calibrate(void);

void engine_start(void); 
void engine_stop(void);

int engine_is_stopped(void);
int engine_is_running(void);
#ifdef __cplusplus
}
#endif

#endif
