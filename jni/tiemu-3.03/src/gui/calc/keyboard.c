/* Hey EMACS -*- linux-c -*- */
/* $Id: keyboard.c 2681 2007-11-20 18:01:02Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Liévin
 *  Copyright (c) 2005, Romain Liévin
 *  Copyright (c) 2005, Christian Walther (patches for Mac OS-X port)
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
#endif

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <gdk/gdkkeysyms.h>

#include "intl.h"
#include "paths.h"
#include "skinops.h"
#include "keydefs.h"
#include "support.h"
#include "ti68k_int.h"
#include "popup.h"
#include "screenshot.h"
#include "fs_misc.h"
#include "calc.h"
#include "keymap.h"
#include "dbg_all.h"
#include "keypress.h"
#include "dboxes.h"

Pc2TiKey*       kbd_keymap = keymap;
const char*     skn_keymap;

extern SKIN_INFOS skin_infos;

static int hwkey_to_tikey(guint16 hardware_keycode, int action)
{
    int i;		

    for(i = 0; i < KEYMAP_MAX; i++)
    {
        int ti_key, modifier;

        if(kbd_keymap[i].pc_key == hardware_keycode)
        {
            ti_key = kbd_keymap[i].ti_key;
            modifier = kbd_keymap[i].modifier;

			if(options.kbd_dbg)
			{
				gchar *str;

				str = g_strdup_printf("%s:%s,%s\n",
						keymap_value_to_string(pckeys, hardware_keycode),
						keymap_value_to_string(tikeys, ti_key),
						keymap_value_to_string(tikeys, modifier));
				msg_box1(_("Information"), str);
				g_free(str);
			}

			if(modifier != -1)
			{
                ti68k_kbd_set_key(modifier, action);
				kp_recording_key(modifier, action);
			}

            ti68k_kbd_set_key(ti_key, action);
			kp_recording_key(ti_key, action);

            return !0;
        }
    }

    return 0;
}

/* 
   Converts the mouse position into a TIKEY_xx code
   Checks if the mouse cursor is within a rectangle defined in 
   the key_pos array
*/
static int pos_to_key(int x, int y) 
{
  	int i;
  	RECT *kp = skin_infos.keys_pos;
    int nkeys = sizeof(skin_infos.keys_pos) / sizeof(RECT);
	extern float sf;
  
  	for(i = 0; i < nkeys; i++)
    {
      	if((x >= sf*kp[i].left) && (x < sf*kp[i].right) && 
	 	   (y >= sf*kp[i].top) && (y < sf*kp[i].bottom)) 
		{
			if(options.kbd_dbg)
				printf("tikey = %02x (%s)\n", skn_keymap[i], keymap_value_to_string(tikeys, skn_keymap[i]));		

			return skn_keymap[i];
		}
    }
  
  	return -1;
}

#ifdef __IPAQ__
/*
	Check if the mouse cursor is within the LCD rectangle.
*/
static int pos_to_mnu(int x, int y)
{
	extern LCD_RECT		lr;

	if((x >= lr.x) && (x <= lr.x + lr.w) && 
		(y >= lr.y) && (y <= lr.y + lr.h))
		return !0;

	return 0;
}
#endif /* __IPAQ__ */

// raise the main popup menu
static void
do_popup_menu (GtkWidget *my_widget, GdkEventButton *event)
{
	GtkWidget *menu;
	int event_button, event_time;

	menu = display_popup_menu();

	if (event)
    {
      event_button = event->button;
      event_time = event->time;
    }
	else
    {
      event_button = 0;
      event_time = gtk_get_current_event_time ();
    }

	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 
                  event_button, event_time);
	gtk_widget_show(menu);
}

GLADE_CB gboolean
on_calc_wnd_button_press_event     (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    if(event->type != GDK_BUTTON_PRESS)
        return FALSE;

	if (event->button == 3 && event->type == GDK_BUTTON_PRESS)
    {
		do_popup_menu(widget, event);
		return TRUE;
	}

    if(event->button == 1)
    {
        int key = pos_to_key((int)event->x, (int)event->y);
        if(key >= 0)
		{
			ti68k_kbd_set_key(key, 1);
			kp_recording_key(key, 1);
			return TRUE;
		}
    }

#ifdef __IPAQ__
	if(event->button == 1)
	{
		printf(".");
		if(pos_to_mnu((int)event->x, (int)event->y))
		{
			do_popup_menu(widget, event);
			return TRUE;
		}
	}
#endif

	return FALSE;
}

GLADE_CB gboolean
on_calc_wnd_button_release_event     (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    if(event->type != GDK_BUTTON_RELEASE)
        return FALSE;

    if(event->button == 1)
    {
        int key = pos_to_key((int)event->x, (int)event->y);
        if(key < 0)
            return FALSE;
	    
      	ti68k_kbd_set_key(key, 0);
		kp_recording_key(key, 0);
        return TRUE;
    }

    return FALSE;
}

GLADE_CB gboolean
on_calc_wnd_key_press_event        (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data)
{
#if 0
	printf("<hwkey=%04x (%c), key=%04x (%c), state=%04x>\n", 
		event->hardware_keycode, event->hardware_keycode, 
		event->keyval, event->keyval, event->state );
#endif
#if 0
	printf("KeyEvent:\n");
	printf(" type:		%i\n", event->type);
	printf(" window:	%p\n", event->window);
	printf(" send_event: %i\n", event->send_event);
	printf(" time:		%ul\n", event->time);
	printf(" state:		%u\n", event->state);
	printf(" keyval:	%08x\n", event->keyval);
	printf(" length:	%i\n", event->length);
	printf(" string:	%s\n", event->string);
	printf(" hw key:	%04x\n", event->hardware_keycode);
	printf(" group:		%u\n", event->group);
#endif

	if(event->keyval == GDK_Pause || (event->keyval == 0xffffff) && (event->hardware_keycode == 0x13))
	{
        on_now1_activate(NULL, NULL);
        return TRUE;
	}
        else if(event->keyval == GDK_Menu)
	{
		do_popup_menu(widget, NULL);
		return TRUE;
	}
	else  if(event->keyval == GDK_F10)
    {
        on_send_file_to_tiemu1_activate(NULL, NULL);
        return TRUE;
    }
    else if(event->keyval == GDK_F11)
    {
        on_enter_debugger1_activate(NULL, NULL);
        return TRUE;
    }
    else if(event->keyval == GDK_F12)
    {
        on_set_rom1_activate(NULL, NULL);
        return TRUE;
    }
    else
        return hwkey_to_tikey(event->hardware_keycode, !0) ? TRUE : FALSE;

    return FALSE;
}


GLADE_CB gboolean
on_calc_wnd_key_release_event      (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data)
{
    return hwkey_to_tikey(event->hardware_keycode, 0) ? TRUE : FALSE;
}

