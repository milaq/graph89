/* Hey EMACS -*- linux-c -*- */
/* $Id: hw.c 2749 2007-12-30 17:55:13Z roms $ */

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
 * Modified to run in Android OS. Dritan Hashorva 2012
 */



/*
    TI's ASIC management: glue logic for screen, keyboard, linkport, timers.
*/


#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "libuae.h"
#include "mem.h"
#include "ports.h"
#include "dbus.h"
#include "kbd.h"
#include "hwprot.h"
#include "flash.h"
#include "m68k.h"
#include "images.h"
#include "ti68k_def.h"
#include "gettimeofday.h"
#include "gscales.h"

// This is the ratio OSC1/(OSC2/2^5). We express everything else in fractions of OSC2/2^5.
// On HW1, AI3 is triggered every ~10/7 of a second.
#define HW1_RATE    427    // 10MHz / (2^19*(10/7)/2^5)
#define HW2_RATE    732    // 12MHz / (2^19/2^5)
#define HW4_RATE    977    // 16MHz / (2^19/2^5)

// Timer masks at 2^5, 2^9, 2^12, 2^18 (port $600015)
const unsigned int timer_masks[4] = {0, 15, 127, 8191};
unsigned int timer_mask = 15;

unsigned int cycle_instr = HW1_RATE;
unsigned int cycle_count = 0;

void set_prescaler(int i)
{
	timer_mask = timer_masks[i];
}

/*
	Init hardware...
	A ROM image must have been loaded before calling this function.
*/
int hw_init(void)
{
	cycle_instr = HW1_RATE;
	cycle_count = 0;
	timer_mask = 15;

    IMG_INFO *img = &img_infos;

    // Get infos from image
	tihw.calc_type = img_infos.calc_type;
	tihw.rom_base = img->rom_base << 16;
	tihw.rom_flash = img->flash;
	strcpy(tihw.rom_version, img->version);
	tihw.hw_type = img->hw_type;
	
	tihw.ti92v1 = (tihw.calc_type == TI92) && (strcmp(tihw.rom_version, "2.0") < 0);
	tihw.ti92v2 = (tihw.calc_type == TI92) && (strcmp(tihw.rom_version, "2.0") >= 0);

    switch(tihw.calc_type)
    {
    case TI89:
    case TI89t:
		tihw.log_w = tihw.lcd_w = 160;
		tihw.log_h = tihw.lcd_h = 100;	
        break;
    case TI92:
    case TI92p:
    case V200:
		tihw.log_w = tihw.lcd_w = 240;
		tihw.log_h = tihw.lcd_h = 128;
        break;
    default:
        return -1;
        break;
    }

    // Do sub-initializations.
	TRY(hw_mem_init());
	TRY(hw_flash_init());
	TRY(hw_io_init());
	TRY(hw_hwp_init());
	TRY(hw_dbus_init());
	TRY(hw_kbd_init());
	TRY(hw_m68k_init());

    // Set hardware update rate (dependant from io[0x15])
    if(params.hw_rate != -1)
        cycle_instr = params.hw_rate;
    else if(tihw.hw_type == HW1)
        cycle_instr = HW1_RATE;
    else if(tihw.hw_type <= HW3)
        cycle_instr = HW2_RATE;
    else
        cycle_instr = HW4_RATE;

    return 0;
}

int hw_reset(void)
{
	TRY(hw_mem_reset());
	TRY(hw_flash_reset());
	TRY(hw_io_reset());
	TRY(hw_hwp_reset());
	TRY(hw_kbd_reset());
	TRY(hw_dbus_reset());
	TRY(hw_m68k_reset());

    return 0;
}

int hw_exit(void)
{
	TRY(hw_m68k_exit());
	TRY(hw_dbus_exit());
	TRY(hw_kbd_exit());
	TRY(hw_io_exit());
	TRY(hw_flash_exit());
	TRY(hw_mem_exit());
	TRY(hw_hwp_exit());

    return 0;
}

//G_LOCK_DEFINE(lcd_flag);
volatile int lcd_flag = !0;

/*
    This function is called by do_cycles to regularly updates the hardware.
    Rate is the same as the timer tick rate.
*/
void hw_update(void)
{
	static unsigned int timer;
	//time_t curr_clock;

	// OSC2 enable (bit clear means oscillator stopped!)
	int osc2_enabled = io_bit_tst(0x15,1);

	timer++;
	if (osc2_enabled)
	{
		// Increment timer
		if(!(timer & timer_mask) && io_bit_tst(0x15,3))
		{
			if (tihw.timer_value == 0x00) 
				tihw.timer_value = tihw.io[0x17];
			else 
				tihw.timer_value++;
		}
	}

	// Increment HW2 RTC timer every 8192 seconds
	if ((tihw.hw_type >= HW2) && io2_bit_tst(0x1f, 2) && io2_bit_tst(0x1f, 1))
	{
		static struct timeval ref = {0, 0};
		struct timeval tmp = {0, 0};
		gettimeofday(&tmp, NULL);

		// Check if 8192 seconds elapsed, avoiding 32-bit overflow
		if((unsigned)(tmp.tv_sec-ref.tv_sec)*500000u
		   + ((unsigned)(tmp.tv_usec-ref.tv_usec)>>1u) >= 4096000000u)
		{
			gettimeofday(&ref, NULL);
			tihw.rtc_value++;
		}
	}

	// Toggles every FS (every time the LCD restarts at line 0) -> 90 Hz ~ timer/192
	// Don't use the actual LCD count (and use 192 rather than 182) to keep exposure
	// times consistent
	if(!(timer % 192) && tihw.hw_type >= HW2) 
		tihw.io2[0x1d] ^= 0x80;

	/* Auto-int management */

	if (osc2_enabled)
	{
		// Auto-int 1: 1/2^6 of timer rate
		// Triggered at a fixed rate: OSC2/2^11 = (OSC2/2^5)/2^6
		if(!(timer & 63)) 
		{
			if(!io_bit_tst(0x15,7))
				if((tihw.hw_type == HW1) || !(io2_bit_tst(0x1f, 2) && !io2_bit_tst(0x1f, 1)))
					hw_m68k_irq(1);
		}

		// Auto-int 2: keyboard scan
		// see keyboard.c
	}

	if(osc2_enabled || tihw.hw_type == HW2)
	{
		// Auto-int 3: disabled by default by AMS
		// When enabled, it is triggered at a fixed rate: OSC2/2^19 = 1/16384 of timer rate = 1Hz
		if(!(timer & 16383))
		{
			if(!io_bit_tst(0x15,7) && io_bit_tst(0x15,2))
				if((tihw.hw_type == HW1) || !(io2_bit_tst(0x1f, 2) && !io2_bit_tst(0x1f, 1)))
					hw_m68k_irq(3);
		}
	}

	// DBus: External link activity ?
	/*
	if(!ticables_cable_get_d0(cable_handle) || !ticables_cable_get_d1(cable_handle))
	{
		io_bit_set(0x0d,3);	//SA
		io_bit_set(0x0d,2);	//EA
	}
	*/

	// DBUS enabled ?
	if(!io_bit_tst(0x0c,6))
	{
		// Check for data arrival (link cable)
		hw_dbus_checkread();

		// Auto-int 4: triggered by linkport (error, link act, txbuf empty or rxbuf full)
		if((io_bit_tst(0x0c,3) && io_bit_tst(0x0d,7))  ||
			(io_bit_tst(0x0c,2) && io_bit_tst(0x0d,3)) ||
			(io_bit_tst(0x0c,1) && io_bit_tst(0x0d,6)) ||
			(io_bit_tst(0x0c,0) && io_bit_tst(0x0d,5)))
		{
			hw_m68k_irq(4);
		}
	}

	if (osc2_enabled)
	{
		// Auto-int 5: triggered by the programmable timer.
		// The default rate is OSC2/(K*2^9), where K=79 for HW1 and K=53 for HW2
		// Make sure AI5 is triggered only if the timer was actually incremented.
		if(!(timer & timer_mask) && io_bit_tst(0x15,3) && tihw.timer_value == 0)
		{
			if(!io_bit_tst(0x15,7))	
				if((tihw.hw_type == HW1) || !(io2_bit_tst(0x1f, 2) && !io2_bit_tst(0x1f, 1)))
					hw_m68k_irq(5);
		}
	}

	// Auto-int 6: triggered when [ON] is pressed.
	// see keyboard.c

	// Auto-int 7: "vector table write protection" & "stack overflow"
	// see memory.c

	/* Hardware refresh */
  
	// Update keyboard (~600Hz). Not related to timer but as a convenience
	if(!(timer & 127))	// 31 and 63 don't work, 127 and 255 are ok
		hw_kbd_update();

	// Update LCD (HW1: every 256th timer tick, HW2: unrelated)
	if((tihw.hw_type == HW1) && !(timer & 255))
	{
		//G_LOCK(lcd_flag);
		lcd_flag = !0;
		//G_UNLOCK(lcd_flag);
		lcd_hook_hw1();
	}
}


/*
    This function is used to regularly update the hardware from CPU loop. 
    Note that CPU is running against OSC1 (HW1 @10Mhz, HW2 @12MHz)
    while hardware is synched against OSC2 (HW1 @700kHz,  HW2 @~520 kHz).
    OSC2 is the timing base for the timers, the link I/O hardware and 
    (HW1 only) the LCD controller.
    These 2 oscillators are independants.

    See hw.h for inline definition.
*/
/*
static void INLINE do_cycles(void);
*/

