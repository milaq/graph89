/* Hey EMACS -*- linux-c -*- */
/* $Id: mem.c 2601 2007-07-14 08:49:30Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Li�vin
 *  Copyright (c) 2005, Romain Li�vin, Kevin Kofler
 *  Copyright (c) 2007, Romain Li�vin
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
    Memory management: RAM, PROM/FLASH, I/O ports and bkpts
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "libuae.h"
#include "ports.h"
#include "hw.h"
#include "mem.h"
#include "images.h"
#include "bkpts.h"
#include "m68k.h"
#include "ti68k_def.h"
#include "ti68k_int.h"
#include "mem89.h"
#include "mem92.h"
#include "mem92p.h"
#include "mem89tm.h"
#include "memv2.h"
#include "hwprot.h"

static IMG_INFO *img = &img_infos;

// 000000-0fffff : RAM (128 or 256 KB)
// 100000-1fffff : 
// 200000-2fffff : internal ROM (TI92, TI89, V200) or unused
// 300000-3fffff : idem
// 400000-4fffff : external ROM (TI92, TI92-II, TI92+) or unused
// 500000-5fffff : idem
// 600000-6fffff : memory mapped I/O (all HW)
// 700000-7fffff : memory mapped I/O (HW2, HW3)
// 800000-8fffff : ROM (TI89 Titanium) or unused
// 900000-9fffff : idem
// a00000-afffff : idem
// b00000-bfffff : idem
// c00000-cfffff : unused
// d00000-dfffff :	 ...
// e00000-efffff :   ...
// d00000-ffffff : unused

static GETBYTE_FUNC	get_byte_ptr;	// set on memXX.c or hwprot.c
static GETWORD_FUNC	get_word_ptr;
static GETLONG_FUNC	get_long_ptr;

static PUTBYTE_FUNC	put_byte_ptr;
static PUTWORD_FUNC	put_word_ptr;
static PUTLONG_FUNC	put_long_ptr;

GETBYTE_FUNC	mem_get_byte_ptr;	// set by memXX.c:tiXX_mem_init
GETWORD_FUNC	mem_get_word_ptr;
GETLONG_FUNC	mem_get_long_ptr;

PUTBYTE_FUNC	mem_put_byte_ptr;
PUTWORD_FUNC	mem_put_word_ptr;
PUTLONG_FUNC	mem_put_long_ptr;

REALADR_FUNC	mem_get_real_addr_ptr;

/* Mem init/exit */

int hw_mem_init(void)
{
    // get memory sizes
	if(tihw.ti92v2)
	{
		// TI92 II is same as TI92+ in memory size
		tihw.rom_size = ti68k_get_rom_size(TI92p);
		tihw.ram_size = ti68k_get_ram_size(TI92p);
		tihw.io_size  = ti68k_get_io_size(TI92p);
	}
	else
	{
		tihw.rom_size = ti68k_get_rom_size(tihw.calc_type);
		tihw.ram_size = ti68k_get_ram_size(tihw.calc_type);
		tihw.io_size  = ti68k_get_io_size(tihw.calc_type);
		tihw.io2_size = ti68k_get_io2_size(tihw.calc_type);
		tihw.io3_size = ti68k_get_io3_size(tihw.calc_type);
	}

    // clear breakpoints
	ti68k_bkpt_clear_access();
	ti68k_bkpt_clear_range();

    // allocate mem
    tihw.ram = malloc(tihw.ram_size);
    tihw.rom = malloc(tihw.rom_size);
    tihw.io  = malloc(tihw.io_size);
    tihw.io2 = malloc(tihw.io2_size);
	tihw.io3 = malloc(tihw.io3_size);
    tihw.unused = malloc(16);

    // clear RAM/ROM/IO
    memset(tihw.ram, 0x00, tihw.ram_size);
    memset(tihw.io , 0x00, tihw.io_size);  
	memset(tihw.io2, 0x00, tihw.io2_size);
	memset(tihw.io2, 0x00, tihw.io3_size);
    memset(tihw.rom, 0xff, tihw.rom_size);
    memset(tihw.unused, 0x14, 16);

    // set banks and mappers on per calc basis
    switch(tihw.calc_type)
    {
    case TI92:  ti92_mem_init();  break;
    case TI92p: ti92p_mem_init(); break;
    case TI89:  ti89_mem_init();  break;
    case V200:  v200_mem_init();  break;
    case TI89t: ti89t_mem_init(); break;
    default: break;
    }
  
    // blit ROM
    memcpy(tihw.rom, img->data, img->size);
    free(img->data);

    if(!tihw.ram || !tihw.rom || !tihw.io || !tihw.io2)
        return -1;

	// set memory mappers for hw protection
	if(params.hw_protect && (tihw.calc_type != TI92))
	{
		get_byte_ptr = hwp_get_byte;
		get_word_ptr = hwp_get_word;
		get_long_ptr = hwp_get_long;
		put_byte_ptr = hwp_put_byte;
		put_word_ptr = hwp_put_word;
		put_long_ptr = hwp_put_long;
	}
	else
	{
		get_byte_ptr = mem_get_byte_ptr;
		get_word_ptr = mem_get_word_ptr;
		get_long_ptr = mem_get_long_ptr;
		put_byte_ptr = mem_put_byte_ptr;
		put_word_ptr = mem_put_word_ptr;
		put_long_ptr = mem_put_long_ptr;
	}

    return 0;
}

int hw_mem_reset(void)
{
    return 0;
}

int hw_mem_exit(void)
{
	// free memory
    if(tihw.ram)
        free(tihw.ram); 
    tihw.ram=NULL;
  
    if(tihw.rom)
	    free(tihw.rom);
    tihw.rom = NULL;
 
    if(tihw.io)  
        free(tihw.io);  
    tihw.io = NULL;

    if(tihw.io2)
        free(tihw.io2);
    tihw.io2 = NULL;

	if(tihw.io3)
        free(tihw.io3);
    tihw.io3 = NULL;

	// clear breakpoints
	ti68k_bkpt_clear_access();
	ti68k_bkpt_clear_range();

    return 0;
}

uint8_t* hw_get_real_address(uint32_t adr)
{
	return mem_get_real_addr_ptr(adr);
}

uint32_t hw_get_long(uint32_t adr) 
{
  //  GList* l;
  
    adr &= 0xFFFFFF;

 /*   if (bkpts.mem_rl && !(regs.spcflags & SPCFLAG_BRK))
    {
		for(l = bkpts.mem_rl, bkpts.id = 0; l; l = l->next, bkpts.id++)
	    {
	        if ((uint32_t)GPOINTER_TO_INT(l->data) == adr) 
	        {
				bkpts.type = BK_TYPE_ACCESS;
	            bkpts.mode = BK_READ_LONG; 
	            regs.spcflags |= SPCFLAG_BRK;	            
	            break;
	        }
	    }
    }
  
    if (bkpts.mem_rng_r && !(regs.spcflags & SPCFLAG_BRK)) 
    {
		for(l = bkpts.mem_rng_r, bkpts.id = 0; l; l = l->next, bkpts.id++)
		{
            ADDR_RANGE *r = l->data;

			if (((adr+3) >= r->val1) && (adr <= r->val2))
	        {
				bkpts.type = BK_TYPE_RANGE;
	            bkpts.mode = BK_READ_LONG; 
	            regs.spcflags |= SPCFLAG_BRK;	            
	            break;
	        }
	    }
    }
    */
  
    // Odd address: exception !
    if(adr & 1) 
    {
        regs.spcflags |= SPCFLAG_ADRERR;
        return 0;
    }

	return get_long_ptr(adr);
}

uint16_t hw_get_word(uint32_t adr) 
{
//    GList* l;
	
    adr &= 0xFFFFFF;

 /*   if (bkpts.mem_rw && !(regs.spcflags & SPCFLAG_BRK))
    {
		for(l = bkpts.mem_rw, bkpts.id = 0; l; l = l->next, bkpts.id++)
	    {
	        if ((uint32_t)GPOINTER_TO_INT(l->data) == adr) 
	        {
				bkpts.type = BK_TYPE_ACCESS;
	            bkpts.mode = BK_READ_WORD;
	            regs.spcflags |= SPCFLAG_BRK;
	            break;
	        }
	    }
    }
  
    if (bkpts.mem_rng_r && !(regs.spcflags & SPCFLAG_BRK))
    {
		for(l = bkpts.mem_rng_r, bkpts.id = 0; l; l = l->next, bkpts.id++)
	    {
            ADDR_RANGE *r = l->data;

			if (((adr+1) >= r->val1) && (adr <= r->val2))
	        {
				bkpts.type = BK_TYPE_RANGE;
	            bkpts.mode = BK_READ_WORD; 
	            regs.spcflags |= SPCFLAG_BRK;	            
	            break;
	        }
	    }
    }
  */
    // Odd address: exception !
    if(adr & 1) 
    {
        regs.spcflags |= SPCFLAG_ADRERR;
        return 0;
    }

	return get_word_ptr(adr);
}

uint8_t hw_get_byte(uint32_t adr) 
{
 //   GList* l;
  
    adr &= 0xFFFFFF;

 /*   if (bkpts.mem_rb && !(regs.spcflags & SPCFLAG_BRK))
    {
		for(l = bkpts.mem_rb, bkpts.id = 0; l; l = l->next, bkpts.id++)
		{
	        if ((uint32_t)GPOINTER_TO_INT(l->data) == adr) 
	        {
				bkpts.type = BK_TYPE_ACCESS;
	            bkpts.mode = BK_READ_BYTE;
	            regs.spcflags |= SPCFLAG_BRK;	            
	            break;
	        }
		}
    }

    if (bkpts.mem_rng_r && !(regs.spcflags & SPCFLAG_BRK))
    {
		for(l = bkpts.mem_rng_r, bkpts.id = 0; l; l = l->next, bkpts.id++)
	    {
            ADDR_RANGE *r = l->data;

			if ((adr >= r->val1) && (adr <= r->val2))
	        {
				bkpts.type = BK_TYPE_RANGE;
	            bkpts.mode = BK_READ_BYTE; 
	            regs.spcflags |= SPCFLAG_BRK;	            
	            break;
	        }	  
 	    }
    }
  */
	return get_byte_ptr(adr);
}

uint8_t hw_get_byte_noexcept(uint32_t adr) 
{
    adr &= 0xFFFFFF;
	return get_byte_ptr(adr);
}

void hw_put_long(uint32_t adr, uint32_t arg) 
{
//    GList* l;

    adr &= 0xFFFFFF;

 /*   if (bkpts.mem_wl && !(regs.spcflags & SPCFLAG_BRK))
    {
		for(l = bkpts.mem_wl, bkpts.id = 0; l; l = l->next, bkpts.id++)
	    {
	        if ((uint32_t)GPOINTER_TO_INT(l->data) == adr) 
	        {
				bkpts.type = BK_TYPE_ACCESS;
	            bkpts.mode = BK_WRITE_LONG;
	            regs.spcflags |= SPCFLAG_BRK;	            
	            break;
	        }
	    }
    }
  
    if (bkpts.mem_rng_w && !(regs.spcflags & SPCFLAG_BRK)) 
    {
		for(l = bkpts.mem_rng_w, bkpts.id = 0; l; l = l->next, bkpts.id++)
	    {
	        ADDR_RANGE *r = l->data;

			if (((adr+3) >= r->val1) && (adr <= r->val2))
	        {
				bkpts.type = BK_TYPE_RANGE;
	            bkpts.mode = BK_WRITE_LONG; 
	            regs.spcflags |= SPCFLAG_BRK;	            
	            break;
	        }
	    }
    }
*/
    // Odd address: exception !
    if(adr & 1)
    {
        regs.spcflags |= SPCFLAG_ADRERR;
        return;
    }
    
    // Protected memory violation. Triggered when memory below [$000120] is
	// written while bit 2 of [$600001] is set
	if((adr < 0x120) && io_bit_tst(0x01,2))
		hw_m68k_irq(7);
	else
		put_long_ptr(adr, arg);
}

void hw_put_word(uint32_t adr, uint16_t arg) 
{
//    GList* l;
	
    adr &= 0xFFFFFF;

 /*   if (bkpts.mem_ww && !(regs.spcflags & SPCFLAG_BRK))
    {
		for(l = bkpts.mem_ww, bkpts.id = 0; l; l = l->next, bkpts.id++)
	    {
	        if ((uint32_t)GPOINTER_TO_INT(l->data) == adr) 
	        {
				bkpts.type = BK_TYPE_ACCESS;
	            bkpts.mode = BK_WRITE_WORD;
	            regs.spcflags |= SPCFLAG_BRK;	            
	            break;
	        }
	    }
    }
  
    if (bkpts.mem_rng_w && !(regs.spcflags & SPCFLAG_BRK))
    {
		for(l = bkpts.mem_rng_w, bkpts.id = 0; l; l = l->next, bkpts.id++)
	    {
            ADDR_RANGE *r = l->data;

			if (((adr+1) >= r->val1) && (adr <= r->val2))
	        {
				bkpts.type = BK_TYPE_RANGE;
	            bkpts.mode = BK_WRITE_WORD; 
	            regs.spcflags |= SPCFLAG_BRK;	            
	            break;
	        }
	    }
    }
    */
  
    // Odd address: exception !
    if(adr & 1)
	{
        regs.spcflags |= SPCFLAG_ADRERR;
		return;
	}

    // Protected memory violation. Triggered when memory below [$000120] is
	// written while bit 2 of [$600001] is set
    if((adr < 0x120) && io_bit_tst(0x01,2))
		hw_m68k_irq(7);
	else
		put_word_ptr(adr, arg);
}

void hw_put_byte(uint32_t adr, uint8_t arg) 
{
//    GList* l;
	
    adr &= 0xFFFFFF;
  
/*    if (bkpts.mem_wb && !(regs.spcflags & SPCFLAG_BRK))
    {
		for(l = bkpts.mem_wb, bkpts.id = 0; l; l = l->next, bkpts.id++)
	    {
	        if ((uint32_t)GPOINTER_TO_INT(l->data) == adr) 
	        {
				bkpts.type = BK_TYPE_ACCESS;
	            bkpts.mode = BK_WRITE_BYTE;
	            regs.spcflags |= SPCFLAG_BRK;	            
	            break;
	        }
	    }
    }

    if (bkpts.mem_rng_w && !(regs.spcflags & SPCFLAG_BRK)) 
    {
		for(l = bkpts.mem_rng_w, bkpts.id = 0; l; l = l->next, bkpts.id++)
	    {
            ADDR_RANGE *r = l->data;

			if ((adr >= r->val1) && (adr <= r->val2))
	        {
				bkpts.type = BK_TYPE_RANGE;
	            bkpts.mode = BK_WRITE_BYTE; 
	            regs.spcflags |= SPCFLAG_BRK;	            
	            break;
	        }
		}
    }

	if (bkpts.bits && !(regs.spcflags & SPCFLAG_BRK))
	{
		for(l = bkpts.bits, bkpts.id = 0; l != NULL; l = l->next, bkpts.id++)
		{
			ADDR_BIT *s = l->data;

			if((adr == s->addr) & ((arg & s->checks) == (s->states & s->checks)))
	        {
				bkpts.type = BK_TYPE_BIT;
	            bkpts.mode = BK_WRITE_BYTE; 
	            regs.spcflags |= SPCFLAG_BRK;	            
	            break;
	        }
		}
	}
	*/

    // Protected memory violation. Triggered when memory below [$000120] is
	// written while bit 2 of [$600001] is set
    if((adr < 0x120) && io_bit_tst(0x01,2))
		hw_m68k_irq(7);
	else
		put_byte_ptr(adr, arg);
}

void hw_put_byte_noexcept(uint32_t adr, uint8_t arg) 
{
    adr &= 0xFFFFFF;  
    put_byte_ptr(adr, arg);
}
