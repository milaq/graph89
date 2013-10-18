/* Hey EMACS -*- linux-c -*- */
/* $Id: registers.c 2603 2007-07-14 17:09:56Z roms $ */

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

/*
    Registers access/manipulation
*/

#ifdef HAVE_CONFIG_H
# include <tiemuconfig.h>
#endif

#include <string.h>

#include "libuae.h"
#include "ti68k_int.h"
#include "ti68k_def.h"
#include "bits.h"

/* Flushes GDB's register cache */
extern void registers_changed(void);
/* Flushes GDB's frame cache */
extern void reinit_frame_cache(void);
/* Refreshes Insight */
extern void gdbtk_update(void);

// SR bits set/get modifiers
#define SR_get_T(sr)        bit_get(sr, 15)
#define SR_get_S(sr)        bit_get(sr, 13)
#define SR_get_I(sr)        (((sr) >> 8) & 7)

#define SR_get_X(sr)        bit_get(sr, 4)
#define SR_get_N(sr)        bit_get(sr, 3)
#define SR_get_Z(sr)        bit_get(sr, 2)
#define SR_get_V(sr)        bit_get(sr, 1)
#define SR_get_C(sr)        bit_get(sr, 0)

#define SR_chg_T(sr, s)     bit_chg(sr, 15, s);
#define SR_chg_S(sr, s)     bit_chg(sr, 13, s)
#define SR_chg_I(sr, v)     { sr &= ~(7 << 8); sr |= (v << 8); }

#define SR_chg_X(sr, s)     bit_chg(sr, 4, s)
#define SR_chg_N(sr, s)     bit_chg(sr, 3, s)
#define SR_chg_Z(sr, s)     bit_chg(sr, 2, s)
#define SR_chg_V(sr, s)     bit_chg(sr, 1, s)
#define SR_chg_C(sr, s)     bit_chg(sr, 0, s)

// Previous state to detect change
static uint32_t old_d[8];
static uint32_t old_a[8];
static uint32_t old_sp, old_usp, old_ssp;
static uint32_t old_pc;
static uint16_t old_sr;
static char old_sf[32];
static char old_uf[32];

void ti68k_register_set_data(int n, uint32_t val)
{
    if (n>=0 && n<8) m68k_dreg(regs,n) = val;
#ifndef NO_GDB
    registers_changed ();
	gdbtk_update();
#endif
}

void ti68k_register_set_addr(int n, uint32_t val)
{
    if (n>=0 && n<8) m68k_areg(regs,n) = val;
#ifndef NO_GDB
    registers_changed ();
	gdbtk_update();
#endif
}

void ti68k_register_set_sp(uint32_t val)
{
    m68k_areg(regs,7) = val;
#ifndef NO_GDB
    registers_changed ();
    reinit_frame_cache ();
	gdbtk_update();
#endif
}

void ti68k_register_set_usp(uint32_t val)
{
    if(!regs.s)
        m68k_areg(regs,7) = val;
    else
        regs.usp = val;

#ifndef NO_GDB
    registers_changed ();
    reinit_frame_cache ();
	gdbtk_update();
#endif
}

void ti68k_register_set_ssp(uint32_t val)
{
    if(regs.s)
        m68k_areg(regs,7) = val;
    else
        regs.isp = val;

#ifndef NO_GDB
    registers_changed ();
    reinit_frame_cache ();
	gdbtk_update();
#endif
}

void ti68k_register_set_pc(uint32_t val)
{
    m68k_setpc(val);
    fill_prefetch_slow (); /* Force reloading the prefetch. */

#ifndef NO_GDB
    registers_changed ();
    reinit_frame_cache ();
	gdbtk_update();
#endif
}

void ti68k_register_set_sr(uint32_t val)
{
    regs.sr = (int)val;
    MakeFromSR();

#ifndef NO_GDB
    registers_changed ();
	gdbtk_update();
#endif
}

void ti68k_register_set_flag(uint8_t flag)
{
  	//TODO
  	/* T  0  S  0  0  I2 I1 I0 0  0  0  X  N  Z  V  C */	  
#ifndef NO_GDB
    registers_changed ();
	gdbtk_update();
#endif
}

int ti68k_register_set_flags(const char *sf, const char *uf)
{
	/* SR: T 0 S 0 0 I2 I1 I0 0 0 0 X N Z V C */
	int t, s, i, x, n, z, v, c;
	int nargs;
	
    MakeSR();

	if(sf != NULL)
	{
		nargs = sscanf(sf, "T=%d S=%d I=%d", &t, &s, &i);
		
		if(nargs < 3)
			return 0;
		if((i < 0) || (i > 7))
			return 0;
    
        SR_chg_T(regs.sr, 1);
        SR_chg_T(regs.sr, t);
        SR_chg_S(regs.sr, s);
        SR_chg_I(regs.sr, i);
	}

	if(uf != NULL)
	{
		nargs = sscanf(uf, "X=%d N=%d \nZ=%d V=%d C=%d", &x, &n, &z, &v, &c);

		if(nargs < 5)
			return 0;

        SR_chg_X(regs.sr, x);
        SR_chg_N(regs.sr, n);
        SR_chg_Z(regs.sr, z);
        SR_chg_V(regs.sr, v);
        SR_chg_C(regs.sr, c);
	}

    MakeFromSR();

#ifndef NO_GDB
    registers_changed ();
	gdbtk_update();
#endif

	return !0;
}

int ti68k_register_get_data(int n, uint32_t *val)
{
	int c = 0;
	
    if (n>=0 && n<8)
    	*val = m68k_dreg(regs,n);
    	
    if(m68k_dreg(regs,n) != old_d[n])
    	c = !0;
    	
    old_d[n] = m68k_dreg(regs,n);
    return c;
}

int ti68k_register_get_addr(int n, uint32_t *val)
{
	int c = 0;

    if (n>=0 && n<8) 
    	*val = m68k_areg(regs,n);
    
    if(m68k_areg(regs,n) != old_a[n])
    	c = !0;
    	
    old_a[n] = m68k_areg(regs,n);
    return c;
}
	
int ti68k_register_get_sp(uint32_t *val)
{
	int c = 0;
	
	*val = m68k_areg(regs,7);
	if(m68k_areg(regs,7) != old_sp)
		c = !0;

	old_sp = m68k_areg(regs,7);
    return c;
}

int ti68k_register_get_usp(uint32_t *val)
{
    int c = 0;
    uae_u32 *reg;

    if(!regs.s)
        reg = &m68k_areg(regs,7);
    else
        reg = &regs.usp;

    *val = *reg;
	if(*reg != old_usp)
		c = !0;

	old_usp = *reg;
    return c;
}

int ti68k_register_get_ssp(uint32_t *val)
{
    int c = 0;
    uae_u32 *reg;

    if(regs.s)
        reg = &m68k_areg(regs,7);
    else
        reg = &regs.isp;

    *val = *reg;
	if(*reg != old_ssp)
		c = !0;

	old_ssp = *reg;
    return c;
}

int ti68k_register_get_pc(uint32_t *val)
{
	int c = 0;

    *val = m68k_getpc();
	if(*val != old_pc)
		c = !0;

	old_pc = regs.pc;
	return c;
}

int ti68k_register_get_sr(uint32_t *val)
{
	int c =0;

    MakeSR();
    *val = regs.sr;
	if(regs.sr != old_sr)
		c = !0;

	old_sr = regs.sr;
	return c;
}

const char *ti68k_register_get_flag(void)
{
    static char str[64];

	/* T  0  S  0  0  I2 I1 I0 0  0  0  X  N  Z  V  C */
    MakeSR();
    printf("T=%d S=%d I=%d | X=%d N=%d\nZ=%d V=%d C=%d\n",
        SR_get_T(regs.sr), SR_get_S(regs.sr),
        SR_get_I(regs.sr), SR_get_X(regs.sr),
        SR_get_N(regs.sr), SR_get_Z(regs.sr),
        SR_get_V(regs.sr), SR_get_C(regs.sr)
        );

    return str;
}

int ti68k_register_get_flags(char *sf, char *uf)
{
	int c =0;

	/* SR: T 0 S 0 0 I2 I1 I0 0 0 0 X N Z V C */
    MakeSR();
    sprintf(sf, "T=%d S=%d I=%d", SR_get_T(regs.sr), SR_get_S(regs.sr), SR_get_I(regs.sr));
	sprintf(uf, "X=%d N=%d \nZ=%d V=%d C=%d",   /* %dSPC\n: SPC is important ! */
        SR_get_X(regs.sr), SR_get_N(regs.sr), SR_get_Z(regs.sr), 
        SR_get_V(regs.sr), SR_get_C(regs.sr));	 

	if(strcmp(sf, old_sf) || strcmp(uf, old_uf))
		c = !0;

	strcpy(old_sf, sf);
	strcpy(old_uf, uf);
	return c;
}

