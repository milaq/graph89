/* Hey EMACS -*- linux-c -*- */
/* $Id: flash.c 2372 2007-02-25 21:43:23Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Li�vin
 *  Copyright (c) 2005, Romain Li�vin, Kevin Kofler
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
    FLASH algorithm management:
	- Sharp's LH28F160S3T: TI89/TI92+
	- Sharp's LH28F320BF: V200/TI89 Titanium
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h> //memset

#include "mem.h"
#include "ti68k_def.h"
#include "flash.h"
#include <androidlog.h>

FLASH_WSM   wsm;

int hw_flash_init(void)
{
	memset(&wsm, 0, sizeof(FLASH_WSM));
	wsm.nblocks = tihw.rom_size >> 16;
	wsm.changed = calloc(wsm.nblocks, sizeof(int));
	wsm.write_phase = 0x50;
	return 0;
}

int hw_flash_reset(void)
{
	return 0;
}

int hw_flash_exit(void)
{
	if(wsm.changed != NULL)
		free(wsm.changed);

	wsm.changed = NULL;
	return 0;
}

int hw_flash_nblocks(void)
{
	int i, n = 0;

	for(i = 0; i < wsm.nblocks; i++)
		if(wsm.changed[i])
			n++;

	return n;
}

/*
	Read a byte from a Sharp FLASH memory.
*/
uint8_t FlashReadByte(uint32_t addr)
{
	if (wsm.write_phase == 0x90)
	{
		switch(addr & 0xffff)
		{
		case 0:	return (tihw.calc_type == V200 || tihw.calc_type == TI89t) ? 0xb0 : 0x89; // manufacturer code
		case 1:	return 0x00;
		case 2: return 0xb5;	// device code
		case 3: return 0x00;
		default: return 0xff;
		}
	}
	
	return get_b(tihw.rom, addr, tihw.rom_size - 1) | wsm.ret_or;
}

uint16_t FlashReadWord(uint32_t addr)
{
	if (wsm.write_phase == 0x90)
	{
		switch(addr & 0xffff)
		{
		case 0:	return (tihw.calc_type == V200 || tihw.calc_type == TI89t) ? 0xb0 : 0x89; // manufacturer code
		case 2: return 0x00b5;	// device code
		default: return 0xffff;
		}
	}

	return get_w(tihw.rom, addr, tihw.rom_size - 1) | wsm.ret_or;
}

uint32_t FlashReadLong(uint32_t addr)
{
	return (FlashReadWord(addr) << 16) | FlashReadWord(addr+2);
}

/*
	Write a byte to a Sharp FLASH memory
*/
void FlashWriteByte(uint32_t addr, uint8_t v)
{
	//int i;
    uint8_t *rom = tihw.rom;
  
	if(tihw.calc_type == TI92)
        return;

    if(tihw.protect) 
        return;

	addr -= tihw.rom_base;
	addr &= tihw.rom_size - 1;

    // Write State Machine (WSM, Sharp's data sheet)
    if (wsm.write_ready)
    {
		if(rom[addr] != v)
			wsm.changed[addr>>16] = !0;
		rom[addr] &= v; /* can't set bits from 0 to 1 with a write! */
            
		wsm.write_ready--;
        wsm.ret_or = 0xffffffff;
    }
    else if (v == 0x50)
	{
		// clear status register
        wsm.write_phase = 0x50;
	}
    else if (v == 0x10)
    {
		//byte write setup/confirm
        if (wsm.write_phase == 0x50)
		{
	        wsm.write_phase = 0x51;
		}
        else if (wsm.write_phase == 0x51)
        {
	        wsm.write_ready = 2;
	        wsm.write_phase = 0x50;
        }
    }
    else if (v == 0x20)
    {
		// block erase setup/confirm
        if (wsm.write_phase == 0x50)
		{
	        wsm.write_phase = 0x20;
		}
    }
    else if (v == 0xd0)
    {
		// confirm and block erase
        if (wsm.write_phase == 0x20)
        {
	        wsm.write_phase = 0xd0;
	        wsm.ret_or = 0xffffffff;
	        wsm.erase = 0xffffffff;
	        wsm.erase_phase = 0;

			memset(&rom[addr & 0xff0000], 0xff, 64*KB);
			wsm.changed[addr>>16] = !0;
			//printf("%i erased\n", addr>>16);
        } 
    }
    else if (v == 0xff)
    {
		// read array/reset
        if (wsm.write_phase == 0x50)
        {
	        wsm.write_ready = 0;
	        wsm.ret_or = 0;
        }
    }
	else if (v == 0x90)
	{
		// read identifier codes
		wsm.write_phase = 0x90;
	}
}

void FlashWriteWord(uint32_t addr, uint16_t data)
{
	// roms: fix me...
	FlashWriteByte(addr+0,MSB(data));
	FlashWriteByte(addr+1,LSB(data));
}

void FlashWriteLong(uint32_t addr, uint32_t data)
{
	FlashWriteWord(addr+0,(uint16_t)((data>>16)&0xffff));
	FlashWriteWord(addr+2,(uint16_t)((data>> 0)&0xffff));
}

/*
	Search for a vector table and get SSP & PC values.
	This is needed by m68k_reset() for booting.
*/
void find_ssp_and_pc(uint32_t *ssp, uint32_t *pc)
{
    int vt = 0x000000; // vector table

    // find PC reset vector
    if(tihw.rom_flash)
    { 
        // FLASH (TI89, TI92+, V200, ...)
        for (vt = 0x12000; vt < tihw.rom_size; vt++)
	    {
			uint8_t *rom = tihw.rom + vt;

			if(rom[0] == 0xcc && rom[1] == 0xcc && rom[2] == 0xcc && rom[3] == 0xcc)
            {
	            vt += 4;
	            break;
	        }
        }

		*ssp = tihw.rom[vt+3] | (tihw.rom[vt+2]<<8) |
            (tihw.rom[vt+1]<<16) | (tihw.rom[vt]<<24);
    
        vt += 4; // skip SP
   
        *pc = tihw.rom[vt+3] | (tihw.rom[vt+2]<<8) |
            (tihw.rom[vt+1]<<16) | (tihw.rom[vt]<<24);
    }
	else
    { 
		// EPROM (TI92)
		vt = 0;

		*ssp = tihw.rom[vt+3] | (tihw.rom[vt+2]<<8) |
            (tihw.rom[vt+1]<<16) | (tihw.rom[vt]<<24);

		vt += 4; // skip SP
      
		*pc = tihw.rom[vt+3] | (tihw.rom[vt+2]<<8) |
			(tihw.rom[vt+1]<<16) | (tihw.rom[vt]<<24);
    }

	LOGI("found SSP=$%06x and PC=$%06x at offset 0x%x", *ssp, *pc, vt - 0x12000);
}

/*
#if 0
#define	printd	printf
#else
#define printd	
#endif

void FlashWriteWord(uint32_t addr, uint16_t data)
{
	uint8_t *rom = tihw.rom;
  
	if(tihw.calc_type == TI92)
        return;

    if(tihw.protect) 
        return;

	addr -= tihw.rom_base;
	addr &= tihw.rom_size - 1;

	if((addr & 0x1fff) == 0x1000)
		printf("WW: $%06x: %04x ($%06x)\n", addr+ti_rom_base[log_b2(tihw.calc_type)], data, m68k_getpc());

    // Write State Machine (WSM, Sharp's data sheet)
	switch(data & 0xff)
	{
	case FCD_SETUP_BYTE_WRITE:	//0x10: byte write setup/confirm
		printd("FCD_SETUP_BYTE_WRITE: $%06x\n", m68k_getpc());
		wsm.cmd = 0x10;
		wsm.write = 1;
		break;
	default:
		printd("FCD_BYTE_WRITE: %04x at $%06x\n", data, m68k_getpc());
		if(wsm.write)
		{
			wsm.cmd = 0xff;
			rom[addr+0] = MSB(data);
			rom[addr+1] = LSB(data);
			wsm.write = 0;
			wsm.ret_or = 0xffffffff;
		}
		break;
	case FCD_SETUP_BLCK_ERASE:	//0x20: block erase setup/confirm
		printd("FCD_SETUP_BLCK_ERASE: $%06x\n", m68k_getpc());
		wsm.cmd = 0x20;
		wsm.erase = 1;
		break;
	case FCD_CONFIRM_BLK_ERASE:	//0xd0: confirm and block erase
		if(wsm.cmd == 0x20)
		{
			printd("FCD_CONFIRM_BLK_ERASE: $%06x\n", m68k_getpc());
			wsm.cmd = 0xd0;
			memset(&rom[addr & 0xff0000], 0xff, 64*KB);
	        wsm.erase = 0;			
			wsm.ret_or = 0xffffffff;
		}
		break;
	case FCD_CLEAR_STATUS:		//0x50: clear status register
		printd("FCD_CLEAR_STATUS: $%06x\n", m68k_getpc());
		wsm.cmd = 0x50;
		wsm.write = 0;
		wsm.erase = 0;
		wsm.ret_or = 0;
		break;
	case FCD_READ_ID_CODES:		//0x90: read identifier codes
		printd("FCD_READ_ID_CODES: $%06x\n", m68k_getpc());
		wsm.cmd = 0x90;
		break;
	case FCD_READ_OR_RESET:		//0xff: read array/reset
		printd("FCD_READ_OR_RESET: $%06x\n", m68k_getpc());
		if(wsm.erase || wsm.write)
			break;

		wsm.cmd = 0xff;
		wsm.write = 0;
		wsm.erase = 0;
		wsm.ret_or = 0;
		break;
	}
}
  */
