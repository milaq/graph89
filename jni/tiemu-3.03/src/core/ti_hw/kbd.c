/* Hey EMACS -*- linux-c -*- */
/* $Id: kbd.c 2268 2006-11-06 17:18:51Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Li�vin
 *  Copyright (c) 2005, Romain Li�vin
 *  Copyright (c) 2006, Kevin Kofler
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
    Keyboard management
*/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "libuae.h"
#include "keydefs.h"
#include "ti68k_def.h"
#include "m68k.h"
#include "bits.h"
#include "ports.h"
#include "tichars.h"

static TiKey key_states[NB_MAX_KEYS];
static int  *key_row;
static int   key_change;
static int	 on_change;
static int *key_buffer;
static int *key_buffer_ptr;
static int key_buffer_state;

int cyclecount = 0;
int KeyBufferRear;
int KeyBufferFront;
int KeyBufferCount;
int KeyBuffer[32];

int keyRow92[10][8] =
{
  {TIKEY_DOWN, TIKEY_RIGHT, TIKEY_UP, TIKEY_LEFT, TIKEY_HAND, TIKEY_SHIFT, TIKEY_DIAMOND, TIKEY_2ND},
  {TIKEY_3, TIKEY_2, TIKEY_1, TIKEY_F8, TIKEY_W, TIKEY_S, TIKEY_Z, TIKEY_VOID},
  {TIKEY_6, TIKEY_5, TIKEY_4, TIKEY_F3, TIKEY_E, TIKEY_D, TIKEY_X, TIKEY_VOID},
  {TIKEY_9, TIKEY_8, TIKEY_7, TIKEY_F7, TIKEY_R, TIKEY_F, TIKEY_C, TIKEY_STORE},
  {TIKEY_COMMA, TIKEY_PARIGHT, TIKEY_PALEFT, TIKEY_F2, TIKEY_T, TIKEY_G, TIKEY_V, TIKEY_SPACE},
  {TIKEY_TAN, TIKEY_COS, TIKEY_SIN, TIKEY_F6, TIKEY_Y, TIKEY_H, TIKEY_B, TIKEY_DIVIDE},
  {TIKEY_P, TIKEY_ENTER2, TIKEY_LN, TIKEY_F1, TIKEY_U, TIKEY_J, TIKEY_N, TIKEY_POWER},
  {TIKEY_MULTIPLY, TIKEY_APPS, TIKEY_CLEAR, TIKEY_F5, TIKEY_I, TIKEY_K, TIKEY_M, TIKEY_EQUALS},
  {TIKEY_NU, TIKEY_ESCAPE, TIKEY_MODE, TIKEY_PLUS, TIKEY_O, TIKEY_L, TIKEY_THETA, TIKEY_BACKSPACE},
  {TIKEY_NEGATE, TIKEY_PERIOD, TIKEY_0, TIKEY_F4, TIKEY_Q, TIKEY_A, TIKEY_ENTER1, TIKEY_MINUS}
};

int keyRow89[10][8] =
{
  {TIKEY_ALPHA, TIKEY_DIAMOND, TIKEY_SHIFT, TIKEY_2ND, TIKEY_RIGHT, TIKEY_DOWN, TIKEY_LEFT, TIKEY_UP},
  {TIKEY_F5, TIKEY_CLEAR, TIKEY_POWER, TIKEY_DIVIDE, TIKEY_MULTIPLY, TIKEY_MINUS, TIKEY_PLUS, TIKEY_ENTER1},
  {TIKEY_F4, TIKEY_BACKSPACE, TIKEY_T, TIKEY_COMMA, TIKEY_9, TIKEY_6, TIKEY_3, TIKEY_NEGATE},
  {TIKEY_F3, TIKEY_CATALOG, TIKEY_Z, TIKEY_PARIGHT, TIKEY_8, TIKEY_5, TIKEY_2, TIKEY_PERIOD},
  {TIKEY_F2, TIKEY_MODE, TIKEY_Y, TIKEY_PALEFT, TIKEY_7, TIKEY_4, TIKEY_1, TIKEY_0},
  {TIKEY_F1, TIKEY_HOME, TIKEY_X, TIKEY_EQUALS, TIKEY_PIPE, TIKEY_EE, TIKEY_STORE, TIKEY_APPS},
  {TIKEY_VOID, TIKEY_VOID, TIKEY_VOID, TIKEY_VOID, TIKEY_VOID, TIKEY_VOID, TIKEY_VOID, TIKEY_ESCAPE},
  {TIKEY_VOID, TIKEY_VOID, TIKEY_VOID, TIKEY_VOID, TIKEY_VOID, TIKEY_VOID, TIKEY_VOID, TIKEY_VOID},
  {TIKEY_VOID, TIKEY_VOID, TIKEY_VOID, TIKEY_VOID, TIKEY_VOID, TIKEY_VOID, TIKEY_VOID, TIKEY_VOID},
  {TIKEY_VOID, TIKEY_VOID, TIKEY_VOID, TIKEY_VOID, TIKEY_VOID, TIKEY_VOID, TIKEY_VOID, TIKEY_VOID}
};

int keyRowV200[10][8] =
{
  {TIKEY_DOWN, TIKEY_RIGHT, TIKEY_UP, TIKEY_LEFT, TIKEY_HAND, TIKEY_SHIFT, TIKEY_DIAMOND, TIKEY_2ND},
  {TIKEY_3, TIKEY_2, TIKEY_1, TIKEY_F8, TIKEY_W, TIKEY_S, TIKEY_Z, TIKEY_NU},
  {TIKEY_6, TIKEY_5, TIKEY_4, TIKEY_F3, TIKEY_E, TIKEY_D, TIKEY_X, TIKEY_NU},
  {TIKEY_9, TIKEY_8, TIKEY_7, TIKEY_F7, TIKEY_R, TIKEY_F, TIKEY_C, TIKEY_STORE},
  {TIKEY_COMMA, TIKEY_PARIGHT, TIKEY_PALEFT, TIKEY_F2, TIKEY_T, TIKEY_G, TIKEY_V, TIKEY_SPACE},
  {TIKEY_TAN, TIKEY_COS, TIKEY_SIN, TIKEY_F6, TIKEY_Y, TIKEY_H, TIKEY_B, TIKEY_DIVIDE},  
  {TIKEY_P, TIKEY_ENTER2, TIKEY_LN, TIKEY_F1, TIKEY_U, TIKEY_J, TIKEY_N, TIKEY_POWER},
  {TIKEY_MULTIPLY, TIKEY_APPS, TIKEY_CLEAR, TIKEY_F5, TIKEY_I, TIKEY_K, TIKEY_M, TIKEY_EQUALS},
  {TIKEY_NU, TIKEY_ESCAPE, TIKEY_MODE, TIKEY_PLUS, TIKEY_O, TIKEY_L, TIKEY_THETA, TIKEY_BACKSPACE},
  {TIKEY_NEGATE, TIKEY_PERIOD, TIKEY_0, TIKEY_F4, TIKEY_Q, TIKEY_A, TIKEY_ENTER1, TIKEY_MINUS}
};

int hw_kbd_init(void)
{
	int i;

	key_change = 0;
	on_change = 0;
    tihw.on_key = 0;

    switch(tihw.calc_type)
    {
    case TI89:
    case TI89t:
        key_row = (int*)keyRow89;
        break;
    case TI92:
    case TI92p:
        key_row = (int*)keyRow92;
        break;
    case V200:
        key_row = (int*)keyRowV200;
    default:
        break;
    }

	for(i = 0; i < MAX_TIKEYS; i++)
		key_states[i] = 0;


 	for(i = 0; i < 32; ++i)KeyBuffer[i]=0;
	
	KeyBufferFront = 0;
	KeyBufferRear = -1;
	KeyBufferCount = 0;

    return 0;
}

int hw_kbd_reset(void)
{
    return 0;
}

int hw_kbd_exit(void)
{
    key_row = NULL;

    return 0;
}


void KeyBufferPush(int item)
{
	if(KeyBufferCount<32)
	{
		++KeyBufferCount;
		KeyBufferRear = (KeyBufferRear + 1) % 32;
		KeyBuffer[KeyBufferRear]=item;
	}
}

int KeyBufferPop()
{
	int item  = -1;
	if(KeyBufferCount > 0)
	{
		--KeyBufferCount;
		item = KeyBuffer[KeyBufferFront];
		KeyBufferFront = (KeyBufferFront+1) % 32;
	}

	return item;
}

void ti68k_kbd_set_key(int key, int active)
{
	if(key == TIKEY_ALPHA)
	{
		if(active)
		{
			key_states[key]++;
			key_change = !0;
		}
		else
			key_states[key]--;
	}
	else if(key == TIKEY_ON)
	{
		tihw.on_key = active;
		if(active)
			on_change = !0;
	}
	else
	{
		key_states[key] = active;
		if(active)
			key_change = !0;	
	}
}

int ti68k_kbd_is_key_pressed(int key)
{
    return key_states[key];
}

int hw_kbd_update(void)		// ~600Hz
{
	// Push the keys we have been asked to push by ti68k_kbd_push_chars.
	if (key_buffer)
	{
		int key = *key_buffer_ptr;
		if (key == -1)
		{
			free(key_buffer);
			key_buffer = NULL;
			key_buffer_ptr = NULL;
			key_buffer_state = 0;
		}
		else if (key == TIKEY_VOID) // give the calculator some time to react
		{
			if (++key_buffer_state == 30)
			{
				key_buffer_state = 0;
				key_buffer_ptr++;
			}
		}
		else
		{
			switch (key_buffer_state++)
			{
			  case 0:
				ti68k_kbd_set_key(key,TRUE);
				break;
			  case 7:
				ti68k_kbd_set_key(key,FALSE);
				break;
			  case 16:
				key_buffer_state = 0;
				key_buffer_ptr++;
				break;
			  default:
				break;
			}
		}
	}
	else if(KeyBufferCount > 0)
	{
		int key = KeyBufferPop();
		if (key & 0x80)
		{
			ti68k_kbd_set_key(key&0x7F, FALSE);
		}
		else
		{
			ti68k_kbd_set_key(key, TRUE);
		}
	}
	
	if(key_change)
	{
		// Auto-Int 2 is triggered when the first unmasked key is pressed. Keeping the key
		// pressed, or pressing another one without releasing the first key, will not generate
		// additional interrupts.
		if((tihw.hw_type == HW1) || !(io2_bit_tst(0x1f, 2) && !io2_bit_tst(0x1f, 1)))
			hw_m68k_irq(2);
		key_change = 0;
	}
	
	if((on_change == 1) && tihw.on_key) 
	{
		// Auto-Int 6 triggered when [ON] is pressed.
		hw_m68k_irq(6);
		on_change = 0;
	}
	
	return 0;
}

static uint8_t get_rowmask(uint8_t r) 
{
    uint8_t rc = 0;
    int i;
    int *row = key_row + (r << 3);
  
    for(i=0; i<8; i++)
    {
        rc |= (key_states[row[i]] & 1) << (7-i);
    }

    return rc;
}

uint8_t hw_kbd_read_cols(void)
{
    static uint8_t i;
    static uint8_t arg;
    static uint16_t mask;

    mask = (((uint16_t)tihw.io[0x18]) << 8) | tihw.io[0x19];
    for(i = 0, arg = 0; i < 10; i++)
    {
        if(!(mask & (1<<i)))
            arg |= get_rowmask(i);
    }

    return ~arg;
}

// "chars" is expected to be in the TI calculator charset.
// Only works (and returns TRUE) if the internal keyboard buffer was empty,
// otherwise returns FALSE.
int ti68k_kbd_push_chars(const char *chars)
{
    if (key_buffer)
        return FALSE;

    key_buffer = chars_to_keys(chars,
                   tihw.calc_type==TI89 || tihw.calc_type==TI89t);
    key_buffer_ptr = key_buffer;
    return TRUE;
}
