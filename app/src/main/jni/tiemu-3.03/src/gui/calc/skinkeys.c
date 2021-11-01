/* Hey EMACS -*- linux-c -*- */
/* $Id: skinkeys.c 2268 2006-11-06 17:18:51Z roms $ */

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

#ifdef HAVE_CONFIG_H
#include <tiemuconfig.h>
#endif

#include "keydefs.h" // in core/ti_hw

/*
  Key mapping for skins
*/

const char sknKey92[] = 
{
    TIKEY_HAND,
    TIKEY_F1, TIKEY_F2, TIKEY_F3, TIKEY_F4, TIKEY_F5, TIKEY_F6, TIKEY_F7,
    TIKEY_F8,
    TIKEY_Q, TIKEY_W, TIKEY_E, TIKEY_R, TIKEY_T, TIKEY_Y, TIKEY_U, TIKEY_I,
    TIKEY_O,
    TIKEY_P,
    TIKEY_A, TIKEY_S, TIKEY_D, TIKEY_F, TIKEY_G, TIKEY_H, TIKEY_J, TIKEY_K,
    TIKEY_L,
    TIKEY_Z, TIKEY_X, TIKEY_C, TIKEY_V, TIKEY_B, TIKEY_N, TIKEY_M,
    TIKEY_THETA,
    TIKEY_SHIFT, TIKEY_ON, TIKEY_DIAMOND, TIKEY_2ND, TIKEY_STORE,
    TIKEY_SPACE,
    TIKEY_EQUALS,
    TIKEY_BACKSPACE, TIKEY_ENTER1, TIKEY_2ND, TIKEY_ESCAPE, TIKEY_MODE,
    TIKEY_CLEAR,
    TIKEY_LN,
    TIKEY_APPS, TIKEY_ENTER2, TIKEY_LEFT, TIKEY_RIGHT, TIKEY_UP,
    TIKEY_DOWN,
    TIKEY_SIN, TIKEY_COS, TIKEY_TAN, TIKEY_POWER, TIKEY_PALEFT,
    TIKEY_PARIGHT,
    TIKEY_COMMA,
    TIKEY_DIVIDE, TIKEY_7, TIKEY_8, TIKEY_9, TIKEY_MULTIPLY, TIKEY_4,
    TIKEY_5, TIKEY_6,
    TIKEY_MINUS,
    TIKEY_1, TIKEY_2, TIKEY_3, TIKEY_PLUS, TIKEY_0, TIKEY_PERIOD, TIKEY_NEGATE,
    TIKEY_ENTER1
};

const char sknKey89[] = 
{
    TIKEY_F1, TIKEY_F2, TIKEY_F3, TIKEY_F4, TIKEY_F5,
    TIKEY_2ND, TIKEY_SHIFT, TIKEY_ESCAPE, TIKEY_LEFT, TIKEY_RIGHT,
    TIKEY_UP, TIKEY_DOWN, TIKEY_DIAMOND, TIKEY_ALPHA, TIKEY_APPS,
    TIKEY_HOME, TIKEY_MODE, TIKEY_CATALOG, TIKEY_BACKSPACE, TIKEY_CLEAR,
    TIKEY_X, TIKEY_Y, TIKEY_Z, TIKEY_T, TIKEY_POWER,
    TIKEY_EQUALS, TIKEY_PALEFT, TIKEY_PARIGHT, TIKEY_COMMA, TIKEY_DIVIDE,
    TIKEY_PIPE, TIKEY_7, TIKEY_8, TIKEY_9, TIKEY_MULTIPLY,
    TIKEY_EE, TIKEY_4, TIKEY_5, TIKEY_6, TIKEY_MINUS,
    TIKEY_STORE, TIKEY_1, TIKEY_2, TIKEY_3, TIKEY_PLUS,
    TIKEY_ON, TIKEY_0, TIKEY_PERIOD, TIKEY_NEGATE, TIKEY_ENTER1
};
