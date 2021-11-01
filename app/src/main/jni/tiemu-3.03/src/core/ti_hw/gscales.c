/* Hey EMACS -*- linux-c -*- */
/* $Id: gscales.c 2268 2006-11-06 17:18:51Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2002, Romain Lievin, Julien Blache
 *  Copyright (c) 2003-2004, Romain Liévin
 *  Copyright (c) 2005-2006, Kevin Kofler
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
    Gray-scale detection and management.
	Rewritten by K. Kofler.
*/


#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <math.h>

#include "libuae.h"
#include "mem.h"
#include "ports.h"
#include "dbus.h"
#include "kbd.h"
#include "m68k.h"
#include "images.h"
#include "ti68k_def.h"

#define BUFSIZE			16	// store 16 plane addresses
#define UPDATE_PLANES	16	// must be a multiple of BUFSIZE

uint32_t lcd_planes[5];
uint8_t *lcd_planebufs[3];
int lcd_changed = 0;
int ngc = 1;

/*
	Grayscale management (common)
*/
static void process_address(uint32_t plane_addr)
{
	static uint32_t lcd_addrs[BUFSIZE];
	static uint8_t lcd_buffers[BUFSIZE*3][3840];
	static int lcd_cumulative_tickcounts[BUFSIZE];
    static int lcd_last_plane_count;
	static uint32_t lcd_last_planes[3];
	int lcd_exptime[3] = {0, 0, 0};
	int lcd_apparitions[3] = {0, 0, 0};
	int lcd_first_apparition[3] = {0, 0, 0};
	int cycle_start = 0, cycle_end; // cycle = [cycle_start, cycle_end[ (semi-open interval)
	static int cnt;
	static int already_reset;
	uint32_t tmp;

	// Detect plane switches and sync on them
	if (!already_reset)
	{
		switch (lcd_last_plane_count)
		{
			case 2:
				if (plane_addr == lcd_last_planes[1]) break;
				// We have a third plane we haven't had before. Try resetting.
				if (ngc <= 4)
				{
					cnt += BUFSIZE - (cnt % BUFSIZE);
					already_reset = 1;
					break;
				}
			case 1:
				if (plane_addr == lcd_last_planes[0]) break;
			case 0:
				lcd_last_planes[lcd_last_plane_count++] = plane_addr;
				break;
			default:
				if (plane_addr == lcd_last_planes[0] || plane_addr == lcd_last_planes[1] || plane_addr == lcd_last_planes[2]) break;
				// We have a 4th plane, reset the buffer.
				cnt += BUFSIZE - (cnt % BUFSIZE);
				already_reset = 1;
		}
	}

	lcd_addrs[cnt % BUFSIZE] = plane_addr;
	memcpy(lcd_buffers[cnt % (BUFSIZE * 3)], &tihw.ram[plane_addr], 3840);
	if (plane_addr == lcd_planes[0])
	{
		lcd_planebufs[0] = lcd_buffers[cnt % (BUFSIZE * 3)];
		lcd_changed = 1;
	}
	else if (ngc > 1 && plane_addr == lcd_planes[1])
	{
		lcd_planebufs[1] = lcd_buffers[cnt % (BUFSIZE * 3)];
		lcd_changed = 1;
	}
	else if (ngc > 4 && plane_addr == lcd_planes[2])
	{
		lcd_planebufs[2] = lcd_buffers[cnt % (BUFSIZE * 3)];
		lcd_changed = 1;
	}
	lcd_cumulative_tickcounts[cnt++ % BUFSIZE] = tihw.lcd_tick;

	if(!(cnt % UPDATE_PLANES))
	{
		int np, i, buffer_offset, ngp=1;
#if 0
		static int old_ngp=1;
#endif /* 0 */

		already_reset = 0;
		lcd_last_plane_count = 0;

		cycle_end = BUFSIZE - 1; // -1 because we'll need to read
		                         // up to cycle_end for the
		                         // exposition times

		// get address of plane #0
		np = 1;
		lcd_planes[0] = lcd_planes[1] = lcd_planes[2] = lcd_planes[3] = lcd_planes[4] = lcd_addrs[0];		

		// get address of plane #1
		for(i = 1; i < BUFSIZE; i++)
		{
			if(lcd_addrs[i] != lcd_planes[0])
			{
				lcd_planes[1] = lcd_addrs[i];
				np++;
				break;
			}
		}

		// get address of plane #2
		for(i = 1; i < BUFSIZE; i++)
		{
			if((lcd_addrs[i] != lcd_planes[0]) && (lcd_addrs[i] != lcd_planes[1]))
			{
				lcd_planes[2] = lcd_addrs[i];
				np++;
				break;
			}
		}

		// get address of plane #3 (plane-switching)
		for(i = 1; i < BUFSIZE; i++)
		{
			if((lcd_addrs[i] != lcd_planes[0]) && (lcd_addrs[i] != lcd_planes[1]) && (lcd_addrs[i] != lcd_planes[2]))
			{
				// plane-switching: set plane switch as last possible end of cycle
				cycle_end = i;
				lcd_planes[3] = lcd_addrs[i];
				np++;
				break;
			}
		}

		// get address of plane #4 (plane-switching)
		for(i = 1; i < BUFSIZE; i++)
		{
			if((lcd_addrs[i] != lcd_planes[0]) && (lcd_addrs[i] != lcd_planes[1]) && (lcd_addrs[i] != lcd_planes[2]) && (lcd_addrs[i] != lcd_planes[3]))
			{
				lcd_planes[4] = lcd_addrs[i];
				np++;
				break;
			}
		}

		// ignore plane #5 (can be produced by plane-switching, but we don't need it)

		if (np < 4)
			ngp = np;
		else if (np == 4)
		{
			ngp = 2;
			// plane-switching: set plane switch as last possible end of cycle
			for(i = 1; i < BUFSIZE; i++)
			{
				if(lcd_addrs[i] == lcd_planes[2])
				{
					cycle_end = i;
					break;
				}
			}
			// reset plane 2
			lcd_planes[2] = lcd_planes[0];
		}
		else
			ngp = 3;

		// search for a full cycle
		// Use the fact that if there are only 2 planes, plane 2 is the same as plane 0.
		if (ngp >= 2)
		{
			// skip the first plane (not a complete exposition)
			for (i = 0; lcd_addrs[i] == lcd_addrs[0]; i++);
			for (; i <= cycle_end; i++)
			{
				// only count an apparition if the plane has actually changed
				if (lcd_addrs[i] == lcd_planes[0]
				    && lcd_addrs[i-1] != lcd_planes[0])
					if (!(lcd_apparitions[0]++))
						lcd_first_apparition[0] = i;

				if (lcd_addrs[i] == lcd_planes[1]
				    && lcd_addrs[i-1] != lcd_planes[1])
					if (!(lcd_apparitions[1]++))
						lcd_first_apparition[1] = i;

				if (lcd_addrs[i] == lcd_planes[2]
				    && lcd_addrs[i-1] != lcd_planes[2])
					if (!(lcd_apparitions[2]++))
						lcd_first_apparition[2] = i;

				// stop (WITHOUT incrementing i) if all planes appeared at least twice
				if (lcd_apparitions[0] >= 2
				    && lcd_apparitions[1] >= 2
				    && lcd_apparitions[2] >= 2)
					break;
			}
			if (i > cycle_end)
			{
//				printf("Warning: no full grayscale cycle found (BUFSIZE too small?)\n");
				cycle_start = 0;
			}
			else
			{
				if (lcd_addrs[i] == lcd_planes[0])
					cycle_start = lcd_first_apparition[0];
				else if (lcd_addrs[i] == lcd_planes[1])
					cycle_start = lcd_first_apparition[1];
				else if (lcd_addrs[i] == lcd_planes[2])
					cycle_start = lcd_first_apparition[2];
				cycle_end = i;
			}


			// compute exposure time of plane #0 within the cycle
			for(i = cycle_start; i < cycle_end; i++)
			{
				if(lcd_addrs[i] == lcd_planes[0])
					lcd_exptime[0] += (lcd_cumulative_tickcounts[i+1]
					                   - lcd_cumulative_tickcounts[i]);
			}

			// compute exposure time of plane #1 within the cycle
			for(i = cycle_start; i < cycle_end; i++)
			{
				if(lcd_addrs[i] == lcd_planes[1])
					lcd_exptime[1] += (lcd_cumulative_tickcounts[i+1]
					                   - lcd_cumulative_tickcounts[i]);
			}

			// compute exposure time of plane #2 within the cycle
			for(i = cycle_start; i < cycle_end; i++)
			{
				if(lcd_addrs[i] == lcd_planes[2])
					lcd_exptime[2] += (lcd_cumulative_tickcounts[i+1]
					                   - lcd_cumulative_tickcounts[i]);
			}

			// sort plane addresses by exposition times
			if(lcd_exptime[0] < lcd_exptime[1])
			{
				tmp = lcd_planes[0];
				lcd_planes[0] = lcd_planes[1];
				lcd_planes[1] = tmp;
				tmp = lcd_exptime[0];
				lcd_exptime[0] = lcd_exptime[1];
				lcd_exptime[1] = tmp;
			}
			if((ngp >= 3) && (lcd_exptime[0] < lcd_exptime[2]))
			{
				tmp = lcd_planes[0];
				lcd_planes[0] = lcd_planes[2];
				lcd_planes[2] = tmp;
				tmp = lcd_exptime[0];
				lcd_exptime[0] = lcd_exptime[2];
				lcd_exptime[2] = tmp;
			}
			if((ngp >= 3) && (lcd_exptime[1] < lcd_exptime[2]))
			{
				tmp = lcd_planes[1];
				lcd_planes[1] = lcd_planes[2];
				lcd_planes[2] = tmp;
				tmp = lcd_exptime[1];
				lcd_exptime[1] = lcd_exptime[2];
				lcd_exptime[2] = tmp;
			}
		}

#if 0
		if(old_ngp != ngp)
			printf("Detected %i planes !\n", ngp);
		old_ngp = ngp;
#endif /* 0 */

		// now, determine number of grayscales (kevin)
		if(ngp == 1)
			ngc = 1;
		else if(ngp == 2)
			ngc = 3;
		else if(ngp == 3)
		{
			// check using fast integer computation and no possible divisions by 0
			// whether lcd_exptime[0] / lcd_exptime[2] > 3.5 = 7/2
			if (lcd_exptime[0] * 2 > lcd_exptime[2] * 7)
				ngc = 8;
			else
				ngc = 7;
		}

		// set the pointers to the plane buffers
		switch ((cnt / BUFSIZE) % 3)
		{
			case 1:
				buffer_offset = 0;
				break;
			case 2:
				buffer_offset = BUFSIZE;
				break;
			default:
				buffer_offset = 2 * BUFSIZE;
				break;
		}
		for (i = BUFSIZE - 1; i >= 0; i--)
		{
			if (lcd_addrs[i] == lcd_planes[0])
			{
				lcd_planebufs[0] = lcd_buffers[i + buffer_offset];
				break;
			}
		}
		if (ngp >= 2)
		{
			for (i = BUFSIZE - 1; i >= 0; i--)
			{
				if (lcd_addrs[i] == lcd_planes[1])
				{
					lcd_planebufs[1] = lcd_buffers[i + buffer_offset];
					break;
				}
			}
		}
		if (ngp >= 3)
		{
			for (i = BUFSIZE - 1; i >= 0; i--)
			{
				if (lcd_addrs[i] == lcd_planes[2])
				{
					lcd_planebufs[2] = lcd_buffers[i + buffer_offset];
					break;
				}
			}
		}

		lcd_changed = 1;

#if 0
		for (i=cycle_start; i<cycle_end; i++)
				printf("%05x ", lcd_addrs[i]);
		printf("\n");
		for (i=cycle_start; i<cycle_end; i++)
				printf("%05x ", lcd_cumulative_tickcounts[i]>>8);
		printf("\n");
		printf("%i (%i): %06x-%06x-%06x (%i-%i-%i)\n", ngp, ngc,
				lcd_planes[0], lcd_planes[1], lcd_planes[2],
				lcd_exptime[0]>>4, lcd_exptime[1]>>4, lcd_exptime[2]>>4
				);
#endif
	}
}

/*
	HW1 grayscale management
*/
void lcd_hook_hw1(void)
{
	process_address(tihw.lcd_adr);
}

/*
	HW2 grayscale management
*/
void lcd_hook_hw2(int refresh)
{
	static int dead_cnt = 0;
#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4305 )
#endif
	static const char d0d7a2a6_moveml_d0d7a2a6_a1[6] = {0x7c,0xff,0x48,0xd1,0x7c,0xff};
	static const char d1d7a2a6_moveml_d1d7a2a6_a1[6] = {0x7c,0xfe,0x48,0xd1,0x7c,0xfe};
	static const char d0d7a2a7_moveml_d0d7a2a7_a1[6] = {0xfc,0xff,0x48,0xd1,0xfc,0xff};
#ifdef _MSC_VER
#pragma warning( pop ) 
#endif

	// if refresh from GTK (calc.c), set 1 plane, read directly from LCD_MEM
	if(refresh)
	{
		if(++dead_cnt < 5)
			return;

		lcd_planes[0] = tihw.lcd_adr;
		lcd_planebufs[0] = &tihw.ram[tihw.lcd_adr];
		ngc = 1;
		lcd_changed = 1;
	}	

	// if refresh from CPU loop (m68k.c), search for opcode signature:
	else
	{
		unsigned char *pc_p = regs.pc_p;
		if (*(pc_p++) == 0x4c && *(pc_p++) == 0xd8) // search for the movem (%a0)+,... instruction
		{
			if(!memcmp(pc_p, d0d7a2a6_moveml_d0d7a2a6_a1, 6)
			   || !memcmp(pc_p, d0d7a2a7_moveml_d0d7a2a7_a1, 6))
			{
				uint32_t a0 = m68k_areg(regs,0)-0xa00;
				uint32_t a1 = m68k_areg(regs,1)-0xa00;

				if(a1 == 0x4c0c)
					a0 -= 12;
				else if(a1 != 0x4c00)
					return;

				process_address(a0);

				dead_cnt = 0;
			}
			else if (!memcmp(pc_p, d1d7a2a6_moveml_d1d7a2a6_a1, 6))
			{
				uint32_t a0 = m68k_areg(regs,0);
				uint32_t a1 = m68k_areg(regs,1);

				if(a1 != 0x4c00)
					return;

				process_address(a0);

				dead_cnt = 0;
			}
		}
	}
}

// opcode signature:
	// TIGCC grayscale lib and most others:
	//		movem.l  (%a0)+,%d0-%d7/%a2-%a6 ; (%a1)==0x4c00+0xa00 => plane:=-0xa00(%a0)
	// graphlib-titanik and graphlib-iceberg:
	//		movem.l  (%a0)+,%d0-%d7/%a2-%a6 ; (%a1)==0x4c00+0xa00+12 => plane:=-0xa00-12(%a0)
	// Patrick Davidson's gray.asm:
	//		movem.l  (%a0)+,%d1-%d7/%a2-%a6 ; (%a1)==0x4c00 => plane:=(%a0)
	// Grib:
	//		movem.l  (%a0)+,%d0-%d7/%a2-%a7 ; (%a1)==0x4c00+0xa00 => plane:=-0xa00(%a0)
