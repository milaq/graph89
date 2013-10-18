/* Hey EMACS -*- linux-c -*- */
/* $Id: bkpts.h 2689 2007-11-30 03:58:24Z kevinkofler $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Liévin
 *  Copyright (c) 2005, Romain Liévin, Kevin Kofler
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

/*
  Breakpoint definitions
*/

#ifndef __TI68K_BKPTS__
#define __TI68K_BKPTS__

#include <stdint.h>

/* 
	Macros: addresses are 24-bits. We use the MSB to encode 
	bkpt state (enabled/disabled or temporary). This is more 
	efficient than complex structures.
*/

#define BKPT_MASK			0xc0000000

#define BKPT_ADDR(addr)		((addr) & ~BKPT_MASK)
#define BKPT_INFO(addr)		((addr) & BKPT_MASK)

#define BKPT_ENABLE_BIT		31
#define BKPT_TMP_BIT		30

#define BKPT_ENABLE_MASK		(1 << BKPT_ENABLE_BIT)
#define BKPT_ENABLE(addr)		((addr) &= ~BKPT_ENABLE_MASK)
#define BKPT_DISABLE(addr)		((addr) |= BKPT_ENABLE_MASK)
#define BKPT_IS_ENABLED(addr)	(!((addr) & BKPT_ENABLE_MASK))

#define BKPT_TMP_MASK		(1 << BKPT_TMP_BIT)
#define BKPT_NOTMP(addr)	((addr) &= ~BKPT_TMP_MASK)
#define BKPT_TMP(addr)		((addr) |= BKPT_TMPSK)
#define BKPT_IS_TMP(addr)	(((addr) & BKPT_TMP_MASK))

/* Types */

typedef struct
{
  uint32_t	 val1;
  uint32_t 	 val2;
} ADDR_RANGE;

typedef struct
{
	uint32_t	addr;
	uint8_t		checks;
	uint8_t		states;
} ADDR_BIT;

/* Constants */

// Breakpoints mode (ti68k_bkpt_set_[access|access_range])
#define BK_BYTE     1
#define BK_WORD     2
#define BK_LONG     4
#define BK_READ     16
#define BK_WRITE    32
#define BK_RW      (BK_READ | BK_WRITE)

#define BK_READ_BYTE	(BK_READ | BK_BYTE)
#define BK_READ_WORD 	(BK_READ | BK_WORD)
#define BK_READ_LONG 	(BK_READ | BK_LONG)

#define BK_WRITE_BYTE 	(BK_WRITE | BK_BYTE)
#define BK_WRITE_WORD 	(BK_WRITE | BK_WORD)
#define BK_WRITE_LONG 	(BK_WRITE | BK_LONG)

#define BK_RW_BYTE		(BK_READ_BYTE | BK_WRITE_BYTE)
#define BK_RW_WORD		(BK_READ_WORD | BK_WRITE_WORD)
#define BK_RW_LONG		(BK_READ_LONG | BK_WRITE_LONG)

// Breakpoints type 
typedef enum {
    BK_TYPE_ACCESS=1, BK_TYPE_RANGE, 
    BK_TYPE_CODE, BK_TYPE_EXCEPTION,
    BK_TYPE_PGMENTRY, BK_TYPE_PROTECT,
	BK_TYPE_BIT,
} Ti68kBkptType;

// Breakpoints cause (ti68k_bkpt_get_cause())
typedef enum {
    BK_CAUSE_ACCESS=1, BK_CAUSE_RANGE, BK_CAUSE_ADDRESS,
    BK_CAUSE_EXCEPTION, BK_CAUSE_PGMENTRY, BK_CAUSE_PROTECT,
	BK_CAUSE_BIT,
} Ti68kBkptCause;

#define DBG_BREAK   1	// user breakpoint
#define DBG_TRACE   2	// trace (T)
#define DBG_HWPV	3	// hardware protection violation

/* Functions */

int ti68k_bkpt_add_address(uint32_t address);
int ti68k_bkpt_add_access(uint32_t address, int mode);
int ti68k_bkpt_add_range(uint32_t min, uint32_t max, int mode);
int ti68k_bkpt_add_exception(uint32_t n);
int ti68k_bkpt_add_pgmentry(uint16_t handle);
int ti68k_bkpt_add_bits(uint32_t address, uint8_t checks, uint8_t states);

int ti68k_bkpt_del_address(uint32_t address);
int ti68k_bkpt_del_access(uint32_t address, int mode);
int ti68k_bkpt_del_range(uint32_t min, uint32_t max, int mode);
int ti68k_bkpt_del_exception(uint32_t n);
int ti68k_bkpt_del_pgmentry(uint16_t handle);
int ti68k_bkpt_del_bits(uint32_t address);

int ti68k_bkpt_set_address(uint32_t address, uint32_t new_address);
int ti68k_bkpt_set_access(uint32_t address, int mode, uint32_t new_address);
int ti68k_bkpt_set_range(uint32_t min, uint32_t max, int mode, uint32_t new_min, uint32_t new_max);
int ti68k_bkpt_set_exception(uint32_t n, uint32_t new_n);
int ti68k_bkpt_set_pgmentry(uint16_t handle, uint16_t new_handle);
int ti68k_bkpt_set_bits(uint32_t old_address, uint32_t address);
int ti68k_bkpt_set_bits_ex(uint32_t old_address, uint32_t address, uint8_t checks, uint8_t states);

int ti68k_bkpt_get_address(unsigned int idx, uint32_t *address);
int ti68k_bkpt_get_access(unsigned int idx, uint32_t *address, int mode);
int ti68k_bkpt_get_range(unsigned int idx, uint32_t *min, uint32_t *max, int mode);
int ti68k_bkpt_get_exception(unsigned int idx, uint32_t *n);
int ti68k_bkpt_get_pgmentry(unsigned int idx, uint16_t *handle);
int ti68k_bkpt_get_pgmentry_offset(unsigned int idx, uint16_t *handle, uint16_t *offset);
int ti68k_bkpt_get_bits(unsigned int idx, uint32_t *address, uint8_t *checks, uint8_t *states);

void ti68k_bkpt_clear_address(void);
void ti68k_bkpt_clear_access(void);
void ti68k_bkpt_clear_range(void);
void ti68k_bkpt_clear_exception(void);
void ti68k_bkpt_clear_pgmentry(void);
void ti68k_bkpt_clear_bits(void);

void ti68k_bkpt_set_cause(int type, int mode, int id);
void ti68k_bkpt_get_cause(int *type, int *id, int *mode);


#endif
