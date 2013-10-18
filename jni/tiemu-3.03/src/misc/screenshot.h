/* Hey EMACS -*- linux-c -*- */
/* $Id: screenshot.h 2190 2006-08-03 15:29:55Z roms $ */

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

#ifndef __SCREENSHOT__
#define __SCREENSHOT__

#include <gdk-pixbuf/gdk-pixbuf.h>

#define IMG_XPM 1 // unsupported by PixBuf
#define IMG_PCX 2 // unsupported
#define IMG_JPG 3 // supported by PixBuf
#define IMG_PNG 4 // supported
#define IMG_BMP 5 // supported
#define IMG_ICO 6 // supported
#define IMG_EPS 7 // unsupported, custom code
#define IMG_PDF 8 // unsupported, custom code

#define IMG_TYPE 64
#define IMG_COL  (IMG_TYPE+1)
#define IMG_BW   (IMG_TYPE+2)

#define IMG_SIZE 128
#define IMG_LCD  (IMG_SIZE+1)
#define IMG_SKIN (IMG_SIZE+2)

gboolean tiemu_screen_write_eps(const gchar *filename, GdkPixbuf *pixbuf, GError **error);
gboolean tiemu_screen_write_pdf(const gchar *filename, GdkPixbuf *pixbuf, GError **error);

#endif





