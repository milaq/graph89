/* Hey EMACS -*- linux-c -*- */
/* $Id: main.c 245 2004-05-23 20:45:43Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Liévin
 *  Copyright (c) 2005, Romain Liévin
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

#ifndef __TI68K_FLASH__
#define __TI68K_FLASH__

#include "stdint.h"

/* Types */

typedef struct
{
	int	cmd;			// FLASH command (CUI)
    int ret_or;			// status flag
    int write;			// write in progress
    int erase;			// erase in progress
	int	*changed;		// 64KB blocks changed
	int	nblocks;		// number of blocks

    int write_ready;	// will be removed later
    int write_phase;
    int erase_phase;
} FLASH_WSM;

extern FLASH_WSM   wsm;

/* Functions */

int hw_flash_init(void);
int hw_flash_reset(void);
int hw_flash_exit(void);

int hw_flash_nblocks(void);

uint8_t  FlashReadByte(uint32_t addr);
uint16_t FlashReadWord(uint32_t addr);
uint32_t FlashReadLong(uint32_t addr);

void FlashWriteByte(uint32_t addr, uint8_t  v);
void FlashWriteWord(uint32_t addr, uint16_t v);
void FlashWriteLong(uint32_t addr, uint32_t v);

void find_ssp_and_pc(uint32_t *ssp, uint32_t *pc);

/* Command definitions */
#define FCD_SETUP_BYTE_WRITE	0x10
#define FCD_SETUP_BLCK_ERASE	0x20
#define FCD_CONFIRM_BLK_ERASE	0xd0
#define FCD_CLEAR_STATUS		0x50
#define FCD_READ_ID_CODES		0x90
#define FCD_READ_OR_RESET		0xff

#endif
