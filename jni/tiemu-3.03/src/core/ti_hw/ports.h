/* Hey EMACS -*- linux-c -*- */
/* $Id: main.c 245 2004-05-23 20:45:43Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Liévin
 *  Copyright (c) 2005, Romain Liévin, Kevin Kofler
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

#ifndef __TI68K_IO__
#define __TI68K_IO__

#include <stdint.h>
#include "bits.h"

int hw_io_init(void);
int hw_io_reset(void);
int hw_io_exit(void);

extern uint8_t  io_get_byte(uint32_t addr);
extern uint16_t io_get_word(uint32_t addr);
extern uint32_t io_get_long(uint32_t addr);

extern void io_put_long(uint32_t addr, uint32_t arg);
extern void io_put_word(uint32_t addr, uint16_t arg);
extern void io_put_byte(uint32_t addr, uint8_t  arg);

#define io_bit_get(a,b)		bit_get(tihw.io[a],b)
#define io_bit_set(a,b)		bit_set(tihw.io[a],b)
#define io_bit_clr(a,b)		bit_clr(tihw.io[a],b)

#define io_bit_tst(a,b)		bit_tst(tihw.io[a],b)
#define io_bit_chg(a,b,s)	bit_chg(tihw.io[a],b,s)	

// ---

extern uint8_t  io2_get_byte(uint32_t addr);
extern uint16_t io2_get_word(uint32_t addr);
extern uint32_t io2_get_long(uint32_t addr);

extern void io2_put_long(uint32_t addr, uint32_t arg);
extern void io2_put_word(uint32_t addr, uint16_t arg);
extern void io2_put_byte(uint32_t addr, uint8_t  arg);

#define io2_bit_get(a,b)		bit_get(tihw.io2[a],b)
#define io2_bit_set(a,b)		bit_set(tihw.io2[a],b)
#define io2_bit_clr(a,b)		bit_clr(tihw.io2[a],b)

#define io2_bit_tst(a,b)		bit_tst(tihw.io2[a],b)
#define io2_bit_chg(a,b,s)		bit_chg(tihw.io2[a],b,s)	

// ---

extern uint8_t  io3_get_byte(uint32_t addr);
extern uint16_t io3_get_word(uint32_t addr);
extern uint32_t io3_get_long(uint32_t addr);

extern void io3_put_long(uint32_t addr, uint32_t arg);
extern void io3_put_word(uint32_t addr, uint16_t arg);
extern void io3_put_byte(uint32_t addr, uint8_t  arg);

#define io3_bit_get(a,b)		bit_get(tihw.io3[a],b)
#define io3_bit_set(a,b)		bit_set(tihw.io3[a],b)
#define io3_bit_clr(a,b)		bit_clr(tihw.io3[a],b)

#define io3_bit_tst(a,b)		bit_tst(tihw.io3[a],b)
#define io3_bit_chg(a,b,s)		bit_chg(tihw.io3[a],b,s)	

#endif
