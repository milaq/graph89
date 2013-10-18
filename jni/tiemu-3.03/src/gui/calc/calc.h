/* Hey EMACS -*- linux-c -*- */
/* $Id: calc.h 1977 2006-02-18 13:13:31Z roms $ */

/*  TiEmu - a TI emulator
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

#ifndef __CALC_H__
#define __CALC_H__

#include <stdint.h>
#include "struct.h"

/* Variables */

extern GtkWidget *main_wnd;

/* Functions */

int hid_init(void);
int hid_exit(void);

void hid_lcd_rate_set(void);

int hid_update_keys(void);
int hid_update_lcd(void);

int hid_switch_with_skin(void);
int hid_switch_without_skin(void);
int hid_change_skin(const char *filename);

int hid_switch_fullscreen(void);
int hid_switch_unfullscreen(void);

int hid_switch_normal_view(void);
int hid_switch_large_view(void);

void hid_set_callbacks(void);

int hid_screenshot_single(void);
int hid_screenshot_burst(void);

int hid_popup_menu(void);

GdkPixbuf* hid_copy_lcd(void);

/* Private Types */

typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} RGB;

typedef struct
{
	int 	width;
	int 	height;
	int 	rowstride;
	int 	n_channels;
	guchar*	pixels;
	gulong*	pixels2;
} LCD_INFOS;

typedef GdkRect LCD_RECT;
typedef GdkRect SKN_RECT;
typedef GdkRect WND_RECT;

#endif
