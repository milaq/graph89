/* Hey EMACS -*- linux-c -*- */
/* $Id$ */

/*
 *   skinedit - a skin editor for the TiEmu emulator
 *   Copyright (C) 2002 Julien BLACHE <jb@tilp.info>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


/* 
From Romain Lievins(?) :
   Most of these definitions and code comes from the JB's SkinEdit
   which is based on TiEmu skin code. TiEmu skin code is also based on
   VTi's skin code.
   
contra-sh :
   This file is a (quasi ?) perfect copy of the tiemu skinops.h file ...
   Thank's to Romain Lievins and Julien Blache for this wonderful work.

*/



#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdint.h>


/***************/
/* Definitions */
/***************/

#define LCD_COLORTYPE_LOW    0
#define LCD_COLORTYPE_HIGH   1
#define LCD_COLORTYPE_CUSTOM 2

#define LCD_HI_WHITE 0xb0ccae
#define LCD_HI_BLACK 0x8a6f53

#define LCD_LOW_WHITE 0xcfe0cc
#define LCD_LOW_BLACK 0x222e31

#define MAX_COLORS (256 - 16)		// we need to keep 16 colors for grayscales
#define SKIN_KEYS  80

#define SKIN_TI73   "TI-73"
#define SKIN_TI82   "TI-82"
#define SKIN_TI83   "TI-83"
#define SKIN_TI83P  "TI-83+"
#define SKIN_TI85   "TI-85"
#define SKIN_TI86   "TI-86"
#define SKIN_TI89   "TI-89"
#define SKIN_TI92   "TI-92"
#define SKIN_TI92P  "TI-92+"
#define SKIN_V200   "V200PLT"
#define SKIN_TI89T  "TI-89TM"

#define SKIN_TYPE_TIEMU   10
#define SKIN_TYPE_VTI     2
#define SKIN_TYPE_OLD_VTI 1
#define SKIN_TYPE_NEW     0

#define ENDIANNESS_FLAG 0xfeedbabe
#define TIEMU_SKIN_ID   "TiEmu v2.00"



/*********/
/* Types */
/*********/


typedef struct
{
  uint32_t left;
  uint32_t top;
  uint32_t right;
  uint32_t bottom;
} RECT;


typedef struct
{
  int type;


  int width;
  int height;

  double sx, sy;		// scaling factor

  char calc[9];
  uint32_t colortype;

  uint32_t lcd_black;
  uint32_t lcd_white;

  char *name;
  char *author;

  RECT lcd_pos;
  RECT keys_pos[SKIN_KEYS];

  long	jpeg_offset;

} SKIN_INFOS;

/*************/
/* Functions */
/*************/

int skin_load(SKIN_INFOS *infos, const char *filename, GError **err);
int skin_unload(SKIN_INFOS *infos);

