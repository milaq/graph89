/* Hey EMACS -*- linux-c -*- */
/* $Id: rtc_hw3.c 2559 2007-06-24 17:33:54Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2002, Romain Lievin, Julien Blache
 *  Copyright (c) 2003-2004, Romain Li�vin
 *  Copyright (c) 2005-2007, Romain Li�vin, Kevin Kofler
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

/*
    TI's HW3 RTC helpers.
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>

#include "libuae.h"
#include "mem.h"
#include "ports.h"
#include "images.h"
#include "ti68k_def.h"
#include "rtc_hw3.h"
#include "gettimeofday.h"

int rtc3_init(void)
{
	struct tm ref;
	time_t now;

	if(tihw.hw_type < HW3)
		return 0;

	// Computes pseudo-constant (difference between PC ref and TI ref) because:
	// - TI counts seconds since January 1st, 1997 00:00:00
	// - PC counts seconds since January 1st, 1970 00:00:00
	time(&now);	
	memcpy(&ref, localtime(&now), sizeof(struct tm)); // get tm_isdst field (DST status)
	ref.tm_year  = 1997 - 1900;
	ref.tm_mon   = 0;
	ref.tm_yday  = 0;
	ref.tm_mday  = 1;
	ref.tm_wday  = 3;
	ref.tm_hour  = 0;
	ref.tm_min   = 0;
	ref.tm_sec   = 0;
	tihw.rtc3_beg.s  = tihw.rtc3_ref.s  = mktime(&ref);
	tihw.rtc3_beg.ms = tihw.rtc3_ref.ms = 0;
	//printf("<<%s>>\n", asctime(&ref));

	tihw.io3[0x5f] = 0x80;

	return 0;
}

int rtc3_reset(void)
{
	return 0;
}

int rtc3_exit(void)
{
	return 0;
}

// return seconds and milli-seconds
// FIXME: kill this stupid wrapper which loses precision and use gettimeofday
//        and struct timeval directly instead
// Problem: We'll have to bump the savefile revision if we do that. :-(
void rtc3_get_time(TTIME* tt)
{
	struct timeval tp;
	gettimeofday(&tp, NULL);
	tt->s = tp.tv_sec;
	tt->ms = tp.tv_usec/1000;
}

// tt = t2 - t1 and take care of reporting milli-seconds
void rtc3_diff_time(TTIME* t2, TTIME* t1, TTIME* tt)
{
	tt->s = (time_t)difftime(t2->s, t1->s);
	tt->ms = t2->ms - t1->ms;

	if(tt->ms < 0)
	{
		tt->ms += 1000;
		tt->s--;
	}
}

// tt = t2 + t1 and take care of reporting milli-seconds
void rtc3_add_time(TTIME* t2, TTIME* t1, TTIME* tt)
{
	tt->ms = t1->ms + t2->ms;
	tt->s = t1->s + t2->s;

	if(tt->ms > 1000)
	{
		tt->ms -= 1000;
		tt->s++;
	}
}

// Call it before ti68k_state_save to update registers with current clock
// so that clock is correcly saved
int rtc3_state_save(void)
{
	TTIME rtc3_cur;
	TTIME d, a;

	if(tihw.hw_type < HW3)
		return 0;

	// get time and computes time elapsed since reload (cur - beg + load)
	rtc3_get_time(&rtc3_cur);
	rtc3_diff_time(&rtc3_cur, &tihw.rtc3_beg, &d);
	rtc3_add_time(&d, &tihw.rtc3_load, &a);

	// 1/16th of seconds
	tihw.io3[0x45] = (a.ms+a.ms) / 125;

	// seconds since January 1st, 1997 00:00:00
	tihw.io3[0x46] = MSB(MSW(a.s));
	tihw.io3[0x47] = LSB(MSW(a.s)); 
	tihw.io3[0x48] = MSB(LSW(a.s)); 
	tihw.io3[0x49] = LSB(LSW(a.s)); 

	//printf("%i.%i\n", tihw.io3[0x49], tihw.io3[0x45]);

	/*
	rtc3_diff_time(&rtc3_cur, &tihw.rtc3_beg, &r);
	printf("%i.%i - %i.%i = %i.%i\n", 
		rtc3_cur.s, rtc3_cur.ms, tihw.rtc3_beg.s, tihw.rtc3_beg.ms, r.s, r.ms);
	*/
	/*
	rtc3_add_time(&rtc3_cur, &tihw.rtc3_beg, &r);
	printf("%i.%i - %i.%i = %i.%i\n", 
		rtc3_cur.s, rtc3_cur.ms, tihw.rtc3_beg.s, tihw.rtc3_beg.ms, r.s, r.ms);
	*/

	return 0;
}

// Call it after ti68k_state_load to update current clock so that the calc always
// display the right time even if calc has been shudown.
int rtc3_state_load(void)
{
	TTIME rtc3_cur;

	if(tihw.hw_type < HW3)
		return 0;

	// clock disabled ?
	if(!io3_bit_tst(0x5f,0))
		return 0;

	// update current time
	rtc3_get_time(&rtc3_cur);

	return 0;
}

// When compiling without GDB, build the Win32 implementation of gettimeofday
// here to avoid the makefile hackery. GDB-enabled builds build it as part of
// libiberty.
#if defined(_WIN32) && defined(NO_GDB) && !defined(_MSC_VER)
#include "../../misc/gettimeofday_win32_impl.h"
#endif
