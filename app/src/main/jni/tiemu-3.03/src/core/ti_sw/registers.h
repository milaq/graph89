/* Hey EMACS -*- linux-c -*- */
/* $Id: registers.h 2268 2006-11-06 17:18:51Z roms $ */

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

#ifndef __TI68K_REGS__
#define __TI68K_REGS__

typedef enum {
    REG_D0, REG_D1, REG_D2, REG_D3,
    REG_D4, REG_D5, REG_D6, REG_D7,
} Ti68kDataRegister;

typedef enum {
    REG_A0, REG_A1, REG_A2, REG_A3,
    REG_A4, REG_A5, REG_A6, REG_A7,
} Ti68kAddrRegister;

void ti68k_register_set_data(int n, uint32_t val);
void ti68k_register_set_addr(int n, uint32_t val);
void ti68k_register_set_sp(uint32_t val);
void ti68k_register_set_usp(uint32_t val);
void ti68k_register_set_ssp(uint32_t val);
void ti68k_register_set_pc(uint32_t val);
void ti68k_register_set_sr(uint32_t val);
void ti68k_register_set_flag(uint8_t flag);
int  ti68k_register_set_flags(const char *sf, const char *uf);

int ti68k_register_get_data(int n, uint32_t *val);
int ti68k_register_get_addr(int n, uint32_t *val);
int ti68k_register_get_sp(uint32_t *val);
int ti68k_register_get_usp(uint32_t *val);
int ti68k_register_get_ssp(uint32_t *val);
int ti68k_register_get_pc(uint32_t *val);
int ti68k_register_get_sr(uint32_t *val);
const char *ti68k_register_get_flag(void);
int ti68k_register_get_flags(char *sf, char *uf);

#endif
