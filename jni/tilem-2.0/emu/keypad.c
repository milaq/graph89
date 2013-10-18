/*
 * libtilemcore - Graphing calculator emulation library
 *
 * Copyright (C) 2009 Benjamin Moody
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include "tilem.h"
#include "scancodes.h"


int cyclecount = 0;
int keybuffer_rear;
int keybuffer_front;
int keybuffer_count;
int keybuffer[32];

void tilem_keybufferPush(int item);
int tilem_keybufferPop();

int cntr = 0;

void tilem_keypad_reset(TilemCalc* calc)
{
	int i;

	calc->keypad.group = 0xff;
	calc->keypad.onkeydown = 0;
	calc->keypad.onkeyint = 0;
	for (i = 0; i < 8; i++)
		calc->keypad.keysdown[i] = 0;

	for(i = 0; i < 32; ++i)keybuffer[i]=0;
	keybuffer_front = 0;
	keybuffer_rear = -1;
	keybuffer_count = 0;
}

void tilem_keypad_set_group(TilemCalc* calc, byte group)
{
	calc->keypad.group = group;
}

byte tilem_keypad_read_keys(TilemCalc* calc)
{
	int i;
	byte keys, old;
	int keypress = -1;

	++cntr;

	if (cntr % 10 == 0)
	{
		keypress = tilem_keybufferPop();

		if (keypress >= 0)
		{
			if (keypress & 0x80)
			{
				tilem_keypad_release_key(calc, keypress&0x7F);
			}
			else
			{
				tilem_keypad_press_key(calc, keypress);
			}
		}
	}

	keys = 0;
	for (i = 0; i < 8; i++) {
		if (!(calc->keypad.group & (1 << i)))
			keys |= calc->keypad.keysdown[i];
	}

	do {
		old = keys;
		for (i = 0; i < 8; i++) {
			if (keys & calc->keypad.keysdown[i])
				keys |= calc->keypad.keysdown[i];
		}
	} while (keys != old);

	return ~keys;
}

void tilem_keypad_press_key(TilemCalc* calc, int scancode)
{
	if (scancode == TILEM_KEY_ON) {
		if (!calc->keypad.onkeydown && calc->keypad.onkeyint)
			calc->z80.interrupts |= TILEM_INTERRUPT_ON_KEY;
		calc->keypad.onkeydown = 1;
	}
	else if (scancode > 0 && scancode < 65) {
		scancode--;
		calc->keypad.keysdown[scancode / 8] |= (1 << (scancode % 8));
	}
}

void tilem_keypad_release_key(TilemCalc* calc, int scancode)
{
	if (scancode == TILEM_KEY_ON) {
		if (calc->keypad.onkeydown && calc->keypad.onkeyint)
			calc->z80.interrupts |= TILEM_INTERRUPT_ON_KEY;
		calc->keypad.onkeydown = 0;
	}
	else if (scancode > 0 && scancode < 65) {
		scancode--;
		calc->keypad.keysdown[scancode / 8] &= ~(1 << (scancode % 8));
	}
}

void tilem_keybufferPush(int item)
{
	if(keybuffer_count<32)
	{
		++keybuffer_count;
		keybuffer_rear = (keybuffer_rear + 1) % 32;
		keybuffer[keybuffer_rear]=item;
	}
}

int tilem_keybufferPop()
{
	int item  = -1;
	if(keybuffer_count > 0)
	{
		--keybuffer_count;
		item = keybuffer[keybuffer_front];
		keybuffer_front = (keybuffer_front+1) % 32;
	}

	return item;
}
