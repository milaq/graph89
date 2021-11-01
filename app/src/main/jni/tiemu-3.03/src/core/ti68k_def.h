/* Hey EMACS -*- linux-c -*- */
/* $Id: ti68k_def.h 2819 2009-05-02 19:51:29Z roms $ */

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


#ifndef __TI68K_DEFS__
#define __TI68K_DEFS__

#ifdef HAVE_CONFIG_H
# include <tiemuconfig.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif

#include <glib.h>
#include <stdint.h>

#include "tilibs.h"
#include "mem_size.h"
#include "rtc_hw3.h"
#include "macros.h"

/* Equivalences */

#ifdef __WIN32__
# define strcasecmp _stricmp
#endif

/* Constants */

#define MAXCHARS	256

#define TI92 		(1 << 0)
#define TI89 		(1 << 1)
#define TI92p	 	(1 << 2)
#define V200		(1 << 3)
#define TI89t		(1 << 4)
#define CALC_MAX    TI89t
  
#define EXTERNAL	0
#define INTERNAL 	1

#define EPROM_ROM	0
#define FLASH_ROM 	2

#define KB			(1024)
#define MB			(1024*KB)

#define HW1			1
#define HW2			2
#define HW3         3
#define HW4         4

#define LCDMEM_W	240		// LCD _memory_ width
#define LCDMEM_H	128		// LCD _memory_ height

/* Structures */

typedef struct 
{
    char* rom_file;
    char* tib_file;
    char* sav_file;

    int restricted;     // CPU rate of a real calc
    int cpu_rate;       // OSC1
    int hw_rate;        // OSC2
    int lcd_rate;       // synched OSC2 (hw1) or OSC3 (hw2)
    int hw_protect;     // HW protection
    int recv_file;      // receive file enabled/disabled
} Ti68kParameters;

// If this structure is modified, the SAV_REVISION number (state.c)
// has to be incremented.
typedef struct
{
    // misc (non hardware pseudo-constants)
    int			calc_type;

    int			ram_size;	// RAM size
    int			rom_size;	// ROM size
	int			io_size;	// HWx io size
	int			io2_size;	// HW2 io size
	int			io3_size;	// HW3 io size

	uint32_t	rom_base;	// ROM base address
    int			rom_flash;	// ROM type
    char		rom_version[5];	// ROM/AMS version 

	int			hw_type;	// HW1/2/3/4

	int			ti92v1;		// ROM v1.x(y)
	int			ti92v2;		// ROM v2.x

	int			lcd_w;		// LCD physical width
	int			lcd_h;		// LCD physical height

    // keyboard
    int			on_key;

    // lcd
	uint32_t	lcd_adr;	// LCD address (as $4c00)
	char*		lcd_ptr;	// direct pointer to LCD in PC RAM
    int			contrast;
	int			log_w;		// LCD logical width
	int			log_h;		// LCD logical height
	int			on_off;
	unsigned long	lcd_tick;// used by grayscales

    // memory
    uint8_t*	rom;		// ROM
    uint8_t*	ram;		// RAM
    uint8_t*	io;			// HW1/2/3 i/o ports
    uint8_t*	io2;		// HW2/3   i/o ports
	uint8_t*	io3;		// HW3	   i/o ports
    uint8_t*	unused;		// unused

	uint32_t	initial_ssp;// SSP at vector #0
    uint32_t	initial_pc;	// PC  at vector #1

    // timer
    uint8_t     timer_value;// Current timer value
    uint8_t     timer_init;	// Value to reload

	// rtc (hw2)
	uint8_t		rtc_value;	// RTC value

	// rtc (hw3)
	TTIME		rtc3_ref;	// time reference
	TTIME		rtc3_beg;	// time value when
	TTIME		rtc3_load;	// clock is load

	// protection
	int			protect;		// hw protection state
	uint32_t	archive_limit;	// archive memory limit
	int			ram_exec[64];	// RAM page execution protection bitmask

} Ti68kHardware;

typedef struct
{
	// Memory
	GList *mem_rb;		// read byte
	GList *mem_rw;
	GList *mem_rl;

	GList *mem_wb;		// write byte	
	GList *mem_ww;	
	GList *mem_wl;

	GList *mem_rng_r;	// mem range
	GList *mem_rng_w;

	// Code
	GList *code;

	// Exceptions
	GList *exception;

	// Program entry
	GList *pgmentry;

	// Bits
	GList *bits;

	// Breakpoint cause
	int type;       // Ti68kBkptType
    int mode;       // Ti68kBkptMode
	int id;
} Ti68kBreakpoints;

typedef DeviceOptions	Ti68kLinkPort;

typedef struct
{
	// PC
	int         pclog_size;
    uint32_t*   pclog_buf;
    int         pclog_ptr;

	// Link
	int			link_size;	// buffer size
	uint16_t*	link_buf;	// buffer (LSB is data, MSB is S/R action)
	int			link_ptr;	// buffer index
	int			link_mask;	// actions (1: S, 2: R)

} Ti68kLogging;

/* Externs */

extern Ti68kParameters 	params;
extern Ti68kHardware 	tihw;
extern Ti68kLinkPort	linkp;
extern Ti68kBreakpoints	bkpts;
extern Ti68kLogging		logger;

/* Misc */

#ifndef TRY
#define TRY(x) { int aaa_; if((aaa_ = (x))) return aaa_; }
#endif

#ifdef __cplusplus
}
#endif

#endif
