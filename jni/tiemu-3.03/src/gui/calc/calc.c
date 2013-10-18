/* Hey EMACS -*- linux-c -*- */
/* $Id: calc.c 2729 2007-12-16 15:29:16Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Liévin
 *  Copyright (c) 2005, Romain Liévin
 *  Copyright (c) 2005, Julien Blache
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

#ifdef __WIN32__
#include <io.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "intl.h"
#include "paths.h"
#include "skinops.h"
#include "support.h"
#include "ti68k_int.h"
#include "struct.h"
#include "tie_error.h"
#include "calc.h"
#include "dbg_all.h"
#include "screenshot.h"
#include "keymap.h"
#include "logging.h"
#include "gscales.h"

#define ALLOW_RESIZE_WIN32

GtkWidget *main_wnd = NULL;
gboolean explicit_destroy = 0;
GtkWidget *area = NULL;

SKIN_INFOS skin_infos = { 0 };

extern GdkPixbuf*	lcd_mem;
extern GdkPixbuf*	lcd;
extern GdkPixmap*	pixmap;

extern Pc2TiKey*    kbd_keymap;
extern const char*	skn_keymap;
extern const char	sknKey92[];
extern const char	sknKey89[];

extern uint32_t*	lcd_bytmap;

extern LCD_INFOS	li;
extern float		sf;	// scaling factor

extern LCD_RECT		ls;
extern LCD_RECT		lr;
extern SKN_RECT		sr;
extern WND_RECT		wr;

static guint tid = -1;

extern int			shot_cnt;
extern int			skip_cnt;

// part 1: set scale factor
static void set_scale(int view_mode)
{
	if(view_mode == VIEW_NORMAL)
	{
		options.scale = sf = 1.0;
	}
	else if(view_mode == VIEW_LARGE)
	{
		options.scale = sf = 2.0;
	}
	else if(view_mode == VIEW_FULL)
	{
		GdkScreen* screen = gdk_screen_get_default();
		gint sw = gdk_screen_get_width(screen);
		gint sh = gdk_screen_get_height(screen);

		sf = (float)sw / lr.w;
		sf = (float)sh / lr.h;
		//printf("%i %i %f\n", sw, lr.w, sf);
		//printf("%i %i %f\n", sh, lr.h, sf);

		options.scale = sf = (float)1.0;	// restricted to 3.0, too CPU intensive !
	}
}

// part 2: compute sizes
static void set_infos(void)	// set window & lcd sizes
{
	// LCD rectangle (source: skin)
	ls.x = (int)(sf * skin_infos.lcd_pos.left); 
	ls.y = (int)(sf * skin_infos.lcd_pos.top);
	ls.w = (int)(sf * tihw.lcd_w);
	ls.h = (int)(sf * tihw.lcd_h);

	// LCD rectangle (target: window)
	if(options.skin) 
	{
		lr.x = ls.x; 
		lr.y = ls.y;
	}
	else 
	{
		lr.x = 0;
		lr.y = 0;
	}  
	lr.w = (int)(sf * tihw.lcd_w);
	lr.h = (int)(sf * tihw.lcd_h);


	// SKN rectangle
	sr.x = sr.y = 0;
	sr.w = (int)(sf * skin_infos.width);
	sr.h = (int)(sf * skin_infos.height);

	// WND rectangle (= LCD or SKN depending on w/ or w/o skin)
	wr.x = wr.y = 0;
	if(options.skin)
	{
		wr.w = sr.w;
		wr.h = sr.h;
	}
	else
	{
		wr.w = lr.w;
		wr.h = lr.h;
	}

#if 0
	printf("LCD src: %3i %3i %3i %3i\n", ls.x, ls.y, ls.w, ls.h);
	printf("LCD dst: %3i %3i %3i %3i\n", lr.x, lr.y, lr.w, lr.h);
	printf("SKN    : %3i %3i %3i %3i\n", sr.x, sr.y, sr.w, sr.h);
	printf("WND    : %3i %3i %3i %3i\n", wr.x, wr.y, wr.w, wr.h);
#endif
}

// part 3: set changes on window
static void set_window(int full_redraw) 
{
	if(main_wnd->window == NULL)
		return;

	// resize window and drawing area
	if(full_redraw)
		gtk_window_resize(GTK_WINDOW(main_wnd), wr.w, wr.h);
	
#if defined(__WIN32__) && defined(ALLOW_RESIZE_WIN32)
	if(!full_redraw)
		gdk_window_resize(main_wnd->window, wr.w, wr.h);
#endif

	// reallocate backing pixmap
	if(pixmap != NULL)
	{
		// free current backing pixmap
		g_object_unref(pixmap);
		pixmap = NULL;

		// and allocate a new one
		pixmap = gdk_pixmap_new(main_wnd->window, wr.w, wr.h, -1);
		if(pixmap == NULL)
		{
			gchar *s = g_strdup_printf("unable to create backing pixmap.\n");
			tiemu_error(0, s);
			g_free(s);
			return;
		}
	}
}

static void set_constraints(void)
{
	// Allows resizing of window with a constant aspect ratio.
	// This is the right way as used under Linux. 
        // Does not work under Windows thus not enabled.

#if !defined(__WIN32__) || !defined(ALLOW_RESIZE_WIN32)
	if(1)
	{
		GdkGeometry geom = { -1 };
		GdkWindowHints mask = GDK_HINT_MIN_SIZE | GDK_HINT_ASPECT;
		double r = (float)wr.w / wr.h;

		geom.min_width = 100;
		geom.min_height = 100;

		geom.min_aspect = r;
		geom.max_aspect = r;
		gtk_window_set_geometry_hints(GTK_WINDOW(main_wnd), 
					      area, &geom, mask);
				
		//printf("set_constraints: %i %i %1.2f\n", wr.w, wr.h, r);		
	}
#endif
}

// Main wnd by loading glade xml file or by executing glade generated code
gint display_main_wnd(void)
{
	GladeXML *xml;
	gchar *title;

	xml = glade_xml_new
		(tilp_paths_build_glade("calc-2.glade"), "calc_wnd",
		 PACKAGE);
	if (!xml)
		g_error(_("%s: GUI loading failed!\n"), __FILE__);
	glade_xml_signal_autoconnect(xml);
	
	main_wnd = glade_xml_get_widget(xml, "calc_wnd");
	area = glade_xml_get_widget(xml, "drawingarea1");

	gtk_window_move(GTK_WINDOW(main_wnd), options3.calc.rect.x, options3.calc.rect.y);
	set_constraints();
	gtk_widget_realize(main_wnd);	// set drawing area valid

	// set window title (useful for TIGCC-IDE for instance)
	// Note: lpWindowName is "TiEmu (%s)" and lpClassName is "gdkWindowToplevel"
	title = g_strdup_printf("TiEmu (%s)", ti68k_calctype_to_string(tihw.calc_type));
	gtk_window_set_title(GTK_WINDOW(main_wnd), title);
	g_free(title);

	return 0;
}

extern void on_exit_without_saving_state1_activate(GtkMenuItem* item, gpointer data);

GLADE_CB void
on_calc_wnd_destroy                    (GtkObject       *object,
                                        gpointer         user_data)
{
	// Uninstall LCD refresh (to avoid earlier use of main_wnd by hid_lcd_update)
    g_source_remove(tid);

	// When GTK called this signal, the widget has already been destroy
	// thus set the pointer to a valid value, ie NULL .
	main_wnd = NULL;

	if(!explicit_destroy)
		on_exit_without_saving_state1_activate(NULL, NULL);
}

extern void redraw_skin(void);

GLADE_CB gboolean
on_drawingarea1_configure_event        (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data)
{
	float factor;
	
	// compute scaling factor
	if(options.skin)	
		factor = (float)event->width / (float)skin_infos.width;
	else
		factor = (float)event->width / (float)tihw.lcd_w;

#if 0
	printf("on_drawingarea1_configure_event:  x y w h = %i %i %i %i\n", 
		event->x, event->y, event->width, event->height);
	printf("on_drawingarea1_configure_event: f = %1.2f\n", factor);
#endif

	// if normal or large view then exits
	if((factor == 1.0) || (factor == 2.0))
		return FALSE;

	// set scaling factor
	options.scale = sf = factor;
	options.view = VIEW_CUSTOM;

	// compute sizes
	set_infos();

	// and set window size
	set_window(0);
	redraw_skin();

    return FALSE;
}

GLADE_CB gboolean
on_drawingarea1_expose_event           (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data)
{
    gdk_draw_pixmap(
        widget->window,
		widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		pixmap,
		event->area.x, event->area.y,
		event->area.x, event->area.y,
		event->area.width, event->area.height);

	return FALSE;
}

static int match_skin(int calc_type)
{
	SKIN_INFOS *sk = &skin_infos;
	int ok;
	gchar *skin_name, *s;

	s = g_strdup(ti68k_calctype_to_string(calc_type));
	skin_name = g_ascii_strdown(s, strlen(s));
	g_free(s);

	if(!strcmp(skin_name, "ti92+"))
		skin_name[4] = '\0';

#ifdef __IPAQ__
	s = g_strconcat("ipaq_", skin_name, NULL);
	g_free(skin_name);
	skin_name = s;
#endif

	// filename is "", load default skin
	if(!strcmp(g_basename(options.skin_file), ""))
	{
		g_free(options.skin_file);
		options.skin_file = g_strdup_printf("%s%s.skn", 
					    inst_paths.skin_dir, skin_name);
		g_free(skin_name);
		return -1;
	}

	// load skin header
	if(skin_read_header(sk, options.skin_file) == -1)
	{
		g_free(options.skin_file);
      	options.skin_file = g_strdup_printf("%s%s.skn", 
					    inst_paths.skin_dir, skin_name);
	    g_free(skin_name);
	    return -1;
	}

	// is skin compatible
	switch(tihw.calc_type)
	{
	    case TI92:
	    case TI92p:
            ok = !strcmp((const char *)sk->calc, SKIN_TI92) || !strcmp((const char *)sk->calc, SKIN_TI92P);
		break;
	    case TI89:
	    ok = !strcmp((const char *)sk->calc, SKIN_TI89);
		break;
	    case TI89t:
	    ok = !strcmp((const char *)sk->calc, SKIN_TI89T);
		break;
	    case V200:
	    ok = !strcmp((const char *)sk->calc, SKIN_V200);
		break;
	    default: 
	    ok = 0;
		break;
	}

	if(!ok)
	{
		g_free(options.skin_file);
      	options.skin_file = g_strdup_printf("%s%s.skn", 
			inst_paths.skin_dir, skin_name);

	    //tiemu_error(0, _("skin incompatible with the current calc model. Falling back to default skin."));
	    g_free(skin_name);
		return -1;
	}

    g_free(skin_name);
	return 0;
}

static int match_keymap(int calc_type)
{
	gchar *keys_name, *s;
    int ct, ok;

	s = g_strdup(ti68k_calctype_to_string(calc_type));
	keys_name = g_ascii_strdown(s, strlen(s));

	if(!strcmp(keys_name, "ti92+") || !strcmp(keys_name, "ti89t"))
		keys_name[4] = '\0';
	if(!strcmp(keys_name, "v200plt"))
		strcpy(keys_name, "ti92");

	// filename is "", load default keymap
	if(!strcmp(g_basename(options.keys_file), ""))
	{
		g_free(options.keys_file);
		options.keys_file = g_strdup_printf("%s%s.map", 
					    inst_paths.skin_dir, keys_name);
	}

	// load keymap header
    ct = keymap_read_header(options.keys_file);
	if(ct == -1)
	{
		g_free(options.keys_file);
      	options.keys_file = g_strdup_printf("%s%s.map", 
					    inst_paths.skin_dir, keys_name);
	    g_free(keys_name);
	    return -1;
	}

    // is keymap compatible
	switch(tihw.calc_type)
	{
	    case TI92:
		case TI92p:
        case V200:
            ok = (ct == TI92) || (ct == TI92p) || (ct == V200);
		break;
	    case TI89:
        case TI89t:
            ok = (ct == TI89) || (ct == TI89t);
		break;
	    default: 
            ok = 0;
		break;
	}

	if(!ok)
	{
		g_free(options.keys_file);
      	options.keys_file = g_strdup_printf("%s%s.map", 
			inst_paths.skin_dir, keys_name);

	    //tiemu_error(0, _("keymap incompatible with the current calc model. Falling back to default keymap."));
	    g_free(keys_name);
		return -1;
	}

    g_free(keys_name);
	return 0;
}

G_LOCK_EXTERN(lcd_flag);
extern volatile int lcd_flag;
extern volatile int debugger;

static gint hid_refresh (gpointer data)
{
    if(lcd_flag || (tihw.hw_type >= HW2))
    {
		// TI92+: jackycar, TI89: baballe
	    hid_update_lcd();
        G_LOCK(lcd_flag);
        lcd_flag = 0;
        G_UNLOCK(lcd_flag);

		if(tihw.hw_type >= HW2)
			lcd_hook_hw2(TRUE);
    }

    return TRUE;
}

void compute_convtable(void);
void compute_grayscale(void);

extern void dnd_init(void);
extern void dnd_exit(void);

int  hid_init(void)
{
    // Found a PC keyboard keymap
    match_keymap(tihw.calc_type);

    // Load kbd keymap
    if(keymap_load(options.keys_file) == -1)
    {
	    gchar *s = g_strdup_printf("unable to load this keymap: <%s>\n", options.keys_file);
	    tiemu_error(0, s);
	    g_free(s);
	    return -1;
    }

    // Found a skin
	match_skin(tihw.calc_type);

    // Load skin (2 parts)
    if(skin_load(&skin_infos, options.skin_file) == -1) 
    {
	    gchar *s = g_strdup_printf("unable to load this skin: <%s>\n", options.skin_file);
	    tiemu_error(0, s);
	    g_free(s);
	    return -1;
    }
  
	// Set skin keymap depending on calculator type
    switch(tihw.calc_type)
    {
    case TI92:
    case TI92p:
    case V200:
        skn_keymap = sknKey92;
        break;
    case TI89:
    case TI89t:
      	skn_keymap = sknKey89;
        break;
    default:
        {
	  	gchar *s = g_strdup_printf("no skin found for this calc\n");
	  	tiemu_error(0, s);
	  	g_free(s);
	  	return -1;
        }
	}

	// Set window/LCD sizes
	sf = options.scale;
	set_scale(options.view);
	set_infos();

    // Allocate the TI screen buffer
	lcd_bytmap = (uint32_t *)malloc(LCDMEM_W * LCDMEM_H);

    // Allocate the lcd pixbuf
    lcd_mem = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, LCDMEM_W, LCDMEM_H);
    if(lcd_mem == NULL)
    {
        gchar *s = g_strdup_printf("unable to create LCD pixbuf.\n");
	    tiemu_error(0, s);
	    g_free(s);
	    return -1;
    }

	// Used by TI89 (the LCD view is clipped from memory view)
	lcd = gdk_pixbuf_new_subpixbuf(lcd_mem, 0, 0, tihw.lcd_w, tihw.lcd_h);
    
	// Constants for LCD update (speed-up)
    li.n_channels = gdk_pixbuf_get_n_channels (lcd_mem);
	li.width = gdk_pixbuf_get_width (lcd_mem);
	li.height = gdk_pixbuf_get_height (lcd_mem);
	li.rowstride = gdk_pixbuf_get_rowstride (lcd_mem);
	li.pixels = gdk_pixbuf_get_pixels (lcd_mem);

	// Create main window
	display_main_wnd();

    // Allocate the backing pixmap (used for drawing and refresh)
    pixmap = gdk_pixmap_new(main_wnd->window, wr.w, wr.h, -1);
    if(pixmap == NULL)
    {
        gchar *s = g_strdup_printf("unable to create backing pixmap.\n");
	    tiemu_error(0, s);
	    g_free(s);
	    return -1;
    }
    
    // Draw the skin and compute grayscale palette
	set_window(1);
	redraw_skin();
  	compute_grayscale();

    // Init the planar/chunky conversion table for LCD
  	compute_convtable();

    // Install LCD refresh: 100 FPS (10 ms)
    tid = g_timeout_add((params.lcd_rate == -1) ? 50 : params.lcd_rate, 
		(GtkFunction)hid_refresh, NULL);

	explicit_destroy = 0;
	gtk_widget_show(main_wnd);	// show wnd here

	if(options.view == VIEW_FULL)
		gdk_window_fullscreen(main_wnd->window);
	
	lcd_planes[0] = tihw.lcd_adr;
	lcd_planebufs[0] = &tihw.ram[tihw.lcd_adr];
	ngc = 1;
	lcd_changed = 1;

	dnd_init();

    return 0;
}

int  hid_exit(void)
{

    // Uninstall LCD refresh
    g_source_remove(tid);

	// Release resources
    if(lcd_mem != NULL)
    {
        g_object_unref(lcd_mem);
        lcd_mem = NULL;
		g_object_unref(lcd);
		lcd = NULL;
    }

    if(pixmap != NULL)
    {
        g_object_unref(pixmap);
        pixmap = NULL;
    }

    // Destroy window
	if(main_wnd)
	{
		explicit_destroy = !0;
		gtk_widget_destroy(main_wnd);
	}		

    return 0;
}

void hid_lcd_rate_set(void)
{
	g_source_remove(tid);

	tid = g_timeout_add((params.lcd_rate == -1) ? 50 : params.lcd_rate, 
		(GtkFunction)hid_refresh, NULL);
}

int hid_switch_with_skin(void)
{
    options.skin = 1;
	set_infos();
	set_constraints();
	set_window(1);
	redraw_skin();

    return 0;
}

int hid_switch_without_skin(void)
{
    options.skin = 0;
	set_infos();
	set_constraints();
	set_window(1);
	redraw_skin();

    return 0;
}

int hid_change_skin(const char *filename)
{
    int ret1, ret2;
	
	ret1 = hid_exit();
	ret2 = hid_init();
	
	return ret1 | ret2;
}

int hid_switch_fullscreen(void)
{
	if(options.view != VIEW_FULL)
	{
		set_scale(options.view = VIEW_FULL);
		set_infos();
		set_window(1);
		redraw_skin();
		gdk_window_fullscreen(main_wnd->window);
	}

	return 0;
}

int hid_switch_normal_view(void)
{
	if(options.view != VIEW_NORMAL)
	{
		set_scale(options.view = VIEW_NORMAL);
		set_infos();
		set_window(1);
		redraw_skin();
		gdk_window_unfullscreen(main_wnd->window);
	}

    return 0;
}

int hid_switch_large_view(void)
{
	if(options.view != VIEW_LARGE)
	{
		set_scale(options.view = VIEW_LARGE);		
		set_infos();
		set_window(1);
		redraw_skin();
		gdk_window_unfullscreen(main_wnd->window);
	}

    return 0;
}

int  hid_screenshot_burst(void)
{
	shot_cnt = options2.shots;
	skip_cnt = options2.skips;

	return 0;
}

int  hid_screenshot_single(void)
{
	gchar *outfile;
	gchar *ext = "";
	gchar *type = "";

	GdkPixbuf *pixbuf = { 0 };
	gboolean result = FALSE;
	GError *error = NULL;

	switch(options2.format) 
	{
		case IMG_JPG: ext = "jpg"; type = "jpeg"; break;
		case IMG_PNG: ext = "png"; type = "png";  break;
		case IMG_ICO: ext = "ico"; type = "ico";  break;
 		case IMG_EPS: ext = "eps"; type = "eps";  break;
 		case IMG_PDF: ext = "pdf"; type = "pdf";  break;
		case IMG_BMP: ext = "bmp"; type = "bmp";  break;
		default: ext = "png"; type = "png";  break;
	}
  
	outfile = g_strdup_printf("%s%s%s%03i.%s", options2.folder, G_DIR_SEPARATOR_S,
		options2.file, options2.counter, ext);
	tiemu_info(_("screenshot to %s... "), outfile);

	if((options2.size == IMG_LCD) && (options2.type == IMG_BW)) 
	{
		// get pixbuf from TI memory (LCD buffer)
		pixbuf = hid_copy_lcd();
	} 
	else if((options2.size == IMG_LCD) && (options2.type == IMG_COL)) 
	{
        // get pixbuf from grayscale lcd
		pixbuf = gdk_pixbuf_copy(lcd);
	} 
	else if((options2.size == IMG_SKIN) && (options2.type == IMG_COL))
	{
		// get pixbuf from backing pixmap
		pixbuf = gdk_pixbuf_get_from_drawable(NULL, pixmap, NULL, 0, 0, 0, 0, wr.w, wr.h);
	}
	else
	{
		tiemu_warning(_("unsupported screenshot options combination, screenshot aborted."));
		return 0;
       }

	switch (options2.format)
	{
	case IMG_EPS:
		result = tiemu_screen_write_eps(outfile, pixbuf, &error);
		break;
	case IMG_PDF:
		result = tiemu_screen_write_pdf(outfile, pixbuf, &error);
		break;
	default:
		result = gdk_pixbuf_save(pixbuf, outfile, type, &error, NULL);
		break;
	}

	if(options2.clipboard)
	{
		GtkClipboard *clipboard;

		clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
		gtk_clipboard_set_image(clipboard, pixbuf);
	}

	if (result == FALSE) 
	{
		tiemu_warning(_("failed to save pixbuf file: %s: %s"), outfile, error->message);
		g_error_free(error);
	}
	g_object_unref(pixbuf);

	tiemu_info(_("done!"));
	options2.counter++;

	return 0;
}

GLADE_CB gboolean
on_calc_wnd_window_state_event         (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    return FALSE;
}
