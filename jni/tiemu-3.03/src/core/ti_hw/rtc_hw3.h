/* Hey EMACS -*- linux-c -*- */
/* $Id: hw.h 1455 2005-05-31 18:38:03Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Liévin
 *  Copyright (c) 2005, Romain Liévin
 *  Copyright (c) 2005, Christian Walther (patches for Mac OS-X port)
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

#ifndef __TI68K_RTC3__
#define __TI68K_RTC3__

#include <time.h>

typedef struct
{
	time_t	s;
	int		ms;
} TTIME;

int rtc3_init(void);
int rtc3_reset(void);
int rtc3_exit(void);

int rtc3_state_save(void);
int rtc3_state_load(void);

void rtc3_get_time(TTIME* tt);

#endif
