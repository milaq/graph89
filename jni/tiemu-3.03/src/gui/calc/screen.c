/* Hey EMACS -*- linux-c -*- */
/* $Id: screen.c 2268 2006-11-06 17:18:51Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Liévin
 *  Copyright (c) 2005, Romain Liévin
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

#ifdef HAVE_CONFIG_H
#  include <tiemuconfig.h>
#endif				/*  */

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <string.h>

#include "intl.h"
#include "paths.h"
#include "skinops.h"
#include "support.h"
#include "ti68k_int.h"
#include "struct.h"
#include "tie_error.h"
#include "calc.h"
#include "gscales.h"

/* Types */

#define BPP 8               // 8 bits per pixel
#define NGS 16              // number of gray scales (contrast level)

#define SCREEN_ON   (!0)
#define SCREEN_OFF	0

/* Variables */

extern GtkWidget *main_wnd;
extern GtkWidget *area;

extern SKIN_INFOS skin_infos;

GdkPixbuf *lcd_mem = NULL;
GdkPixbuf *lcd = NULL;
GdkPixmap *pixmap = NULL;

uint32_t*	lcd_bytmap;				// LCD buffer (color-mapped as grayscale)

LCD_INFOS	li;
float		sf;	// scaling factor

LCD_RECT	ls;	// LCD rectangle in skin
LCD_RECT	lr;	// LCD rectangle in window
SKN_RECT	sr;	// skin rectangle
union {
  WND_RECT wr;
  GdkRectangle gr;
}	wr;	// window rectangle

static uint32_t convtab[512];      	// planar to chunky conversion table
static RGB      grayscales[16];		// gray scales rgb values (colormap)
static int		lcd_state = -1;     // screen state

static int contrast = NGS;          // current contrast level
static int old_contrast = 0;        // previous contrast level
static int new_contrast = NGS;		// new contrast level

static int max_plane = 0;         	// number of grayscales to emulate

int shot_cnt = 0;					// number of captures
int	skip_cnt = 0;					// number of frames to skip

/* Compute conversion table */
void compute_convtable(void) 
{
  	int i, j;
  	uint8_t k;
  	uint8_t *tab = (uint8_t *)convtab;

  	for(i=0, j=0; i<256; i++) 
    {
      	for(k = 1<<7; k; k>>=1)
		{
			tab[j++] = (i & k) ? 1 : 0;
		}
    }
}

/* The value v is between l and h and can not be outside */
#define filter(v, l, h) (v<l ? l : (v>h ? h : v))

/* Computes the 16 grays level colors */
void compute_grayscale(void)
{
  	int i;
  	int sr, sg, sb;
  	int er, eg, eb;
  	int r, g ,b;
    uint32_t white = skin_infos.lcd_white;  // 0xcfe0ce
    uint32_t black = skin_infos.lcd_black;  // 0x222e31

	//printf("# planes: %i | contrast: %i\n", max_plane, contrast);

	// Compute RBG bsaic values
  	sr = (white & 0xff0000) >> 8;
  	sg = (white & 0x00ff00);
  	sb = (white & 0x0000ff) << 8;
  	
  	er = (black & 0xff0000) >> 8;
  	eg = (black & 0x00ff00);
  	eb = (black & 0x0000ff) << 8;

	// Compute RGB values tuned with contrast
  	if(contrast < NGS) 
    {
      	sr = sr - (sr-er) * (NGS - contrast)/NGS;
      	sg = sg - (sg-eg) * (NGS - contrast)/NGS;
      	sb = sb - (sb-eb) * (NGS - contrast)/NGS;
    }
  	else 
    {
      	er = er - (er-sr)*(contrast - NGS)/NGS;
      	eg = eg - (eg-sg)*(contrast - NGS)/NGS;
      	eb = eb - (eb-sb)*(contrast - NGS)/NGS;
    }

	//printf("a: %02x%02x%02x | %02x%02x%02x\n", sr>>8, sg>>8, sb>>8, er>>8, eg>>8, eb>>8);
  
  	r = sr;
  	g = sg;
  	b = sb;

  	if(lcd_state) 
    {
      	for(i = 0; i <= (max_plane+1); i++) 
		{
	  		grayscales[i].r = filter(r, 0x0000, 0xfff0) >> 8;
	  		grayscales[i].g = filter(g, 0x0000, 0xff00) >> 8;
	  		grayscales[i].b = filter(b, 0x0000, 0xff00) >> 8;

	  		r -= ((sr-er) / (max_plane+1));
	  		g -= ((sg-eg) / (max_plane+1));
	  		b -= ((sb-eb) / (max_plane+1));
		}
    }

	// Compute grayscale palette
	if(!max_plane)
		return;
	
	for(i = 0; i <= max_plane; i++)
	{
		grayscales[i].r = ((sr-er)/max_plane * (max_plane-i) + er) >> 8;
		grayscales[i].g = ((sg-eg)/max_plane * (max_plane-i) + eg) >> 8;
		grayscales[i].b = ((sb-eb)/max_plane * (max_plane-i) + eb) >> 8;
	}
}

/* Redraw the skin into window but don't reload skin file */
void redraw_skin(void) 
{
	// no skin ?
  	if(!options.skin) 
    	return;

	// scale image
	g_object_unref(skin_infos.image);
	skin_infos.image = gdk_pixbuf_scale_simple(skin_infos.raw, wr.wr.w, wr.wr.h, GDK_INTERP_NEAREST);

	// and draw image into pixmap (next, into window on expose event)
    gdk_draw_pixbuf(pixmap, main_wnd->style->fg_gc[GTK_WIDGET_STATE(main_wnd)],
		  skin_infos.image, 0, 0, 0, 0, -1, -1, GDK_RGB_DITHER_NONE, 0, 0);
	gdk_window_invalidate_rect(main_wnd->window, &wr.gr, FALSE);
}

/* Redraw the lcd only */
void redraw_lcd(void) 
{
	if(!pixmap)
		return;

	// scale image
	g_object_unref(skin_infos.image);
	skin_infos.image = 
		gdk_pixbuf_scale_simple(skin_infos.raw, sr.w, sr.h, GDK_INTERP_NEAREST);

	// and draw
	gdk_draw_pixbuf(pixmap, main_wnd->style->fg_gc[GTK_WIDGET_STATE(main_wnd)],
		  skin_infos.image, ls.x, ls.y, lr.x, lr.y, lr.w, lr.h, GDK_RGB_DITHER_NONE, 0, 0);
	gtk_widget_queue_draw_area(area, lr.x, lr.y, lr.w, lr.h);
}

// gray plane sequences in relation with gscales.c
const int gp_seq[9][8] = {			
	{ -1 },						// unused
	{ 0, -1 },					// b&w		(1 plane)
	{ -1 },						// unused
	{ 0, 0, 1, -1 },			// 4 colors (2 planes)
	{ -1 },						// unused
	{ -1 },						// unused
	{ -1 },						// unused
	{ 2, 0, 1, 0, 1, 0, -1 },	// 7 colors (3 planes)
	{ 1, 0, 2, 0, 0, 1, 0, -1 },// 8 colors (3 planes)
};

/* Update LCD screen part */
int hid_update_lcd(void)
{
	int i, j, k, l;
	uint8_t *lcd_bitmap = &tihw.ram[tihw.lcd_adr];	
	uint8_t *lcd_buf = (uint8_t *)lcd_bytmap;	
	GdkRect src;
	guchar *p;

	tihw.lcd_ptr = (char *) &tihw.ram[tihw.lcd_adr];	

    if(!pixmap || !lcd_mem || !tihw.lcd_ptr)
        return 0;

	// Check for LCD state change (from TI HW)
	if(lcd_state != tihw.on_off)
	{
		lcd_state = tihw.on_off;
		lcd_changed = 1;

		if(!lcd_state)
			redraw_lcd();	// to clear LCD
	}

	// Check for contrast change (from TI HW)
	if(contrast != tihw.contrast)
	{
		gint c = contrast = tihw.contrast;

		new_contrast = (c + old_contrast) / 2;
  		old_contrast = c;

		compute_grayscale();

		lcd_changed = 1;
	}

	// Check for gray plane change (menu/hw)
    if(max_plane != ngc) 
    {
		max_plane = ngc;
		compute_grayscale();
    }

	// LCD off or unchanged: don't refresh !
	if(!lcd_state || !lcd_changed)
		return 0;

	// Reset LCD changed flag.
	lcd_changed = 0;

	// Convert the bitmap screen to a bytemap screen and grayscalize
	memset(lcd_bytmap, 0, LCDMEM_H*LCDMEM_W);	
	for(l = 0; l < 8; l++)
	{
		int pp = gp_seq[ngc][l];
		if(pp == -1) break;

		lcd_bitmap = lcd_planebufs[pp];

		for(j = 0, k = 0; k < LCDMEM_H; k++)
		{
			for(i = 0; i < LCDMEM_W/8; i++, lcd_bitmap++) 
			{
				lcd_bytmap[j++] += convtab[(*lcd_bitmap << 1)  ];
				lcd_bytmap[j++] += convtab[(*lcd_bitmap << 1)+1];
			}
		}
    }

    if(1)
	{
		// Copy LCD from buffer to pixbuf
		for(j = 0; j < LCDMEM_H; j++)
		{
			for (i = 0; i < LCDMEM_W; i++) 
			{
				p = li.pixels + j * li.rowstride + i * li.n_channels;
				
				p[0] = grayscales[*lcd_buf].r;
				p[1] = grayscales[*lcd_buf].g;
				p[2] = grayscales[*lcd_buf].b;
				lcd_buf++;
			}
		}

        // Set region to update
        src.x = src.y = 0;
		src.w = (tihw.log_w > tihw.lcd_w) ? tihw.lcd_w : tihw.log_w;
		src.h = (tihw.log_h > tihw.lcd_h) ? tihw.lcd_h : tihw.log_h;

		// Copy surface into window
		if(sf)
		{
			src.w = (int)(sf * src.w);
			src.h = (int)(sf * src.h);

			// scale image
			g_object_unref(skin_infos.image);
			skin_infos.image = gdk_pixbuf_scale_simple(lcd, lr.w, lr.h, GDK_INTERP_NEAREST);

			// and draw image into pixmap (next, into window on expose event)
			gdk_draw_pixbuf(pixmap, main_wnd->style->fg_gc[GTK_WIDGET_STATE(main_wnd)],
			 skin_infos.image, src.x, src.y, lr.x, lr.y, src.w, src.h,
			  GDK_RGB_DITHER_NONE, 0, 0);
			gtk_widget_queue_draw_area(area, lr.x, lr.y, src.w, src.h);
		}
		else
		{
			// and draw image into pixmap (next, into window on expose event)
			gdk_draw_pixbuf(pixmap, main_wnd->style->fg_gc[GTK_WIDGET_STATE(main_wnd)],
			  lcd_mem, src.x, src.y, lr.x, lr.y, src.w, src.h,
			  GDK_RGB_DITHER_NONE, 0, 0);
			gtk_widget_queue_draw_area(area, lr.x, lr.y, src.w, src.h);
		}
    }

	// Screen shot
	if(!--skip_cnt && shot_cnt > 0)
	{
		skip_cnt = options2.skips;

		hid_screenshot_single();
		shot_cnt--;
	}

    return -1;
}

// Copy LCD pixuf into B&W pixbuf
GdkPixbuf* hid_copy_lcd(void)
{
	int i, j, k;
	uint8_t *lcd_bitmap = &tihw.ram[tihw.lcd_adr];	
	uint8_t *lcd_buf = (uint8_t *)lcd_bytmap;
	guchar *p;

	tihw.lcd_ptr = (char *) &tihw.ram[tihw.lcd_adr];	

	// convert the bitmap screen to a bytemap screen and grayscalize
	memset(lcd_bytmap, 0, LCDMEM_H*LCDMEM_W);	
	for(j = 0, k = 0; k < LCDMEM_H; k++)
	{
		for(i = 0; i < LCDMEM_W/8; i++, lcd_bitmap++) 
		{
			lcd_bytmap[j++] = convtab[(*lcd_bitmap << 1)  ];
			lcd_bytmap[j++] = convtab[(*lcd_bitmap << 1)+1];
		}
	}

	// and copy
	for(j = 0; j < LCDMEM_H; j++)
	{
		for (i = 0; i < LCDMEM_W; i++) 
		{
			p = li.pixels + j * li.rowstride + i * li.n_channels;
			
			p[0] = *lcd_buf ? 0x00: 0xff;
			p[1] = *lcd_buf ? 0x00: 0xff;
			p[2] = *lcd_buf ? 0x00: 0xff;
			lcd_buf++;
		}
	}

	return gdk_pixbuf_new_subpixbuf(lcd_mem, 0, 0, tihw.lcd_w, tihw.lcd_h);
}
