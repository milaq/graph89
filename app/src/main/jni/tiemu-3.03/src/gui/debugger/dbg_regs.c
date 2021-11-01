/* Hey EMACS -*- linux-c -*- */
/* $Id: dbg_regs.c 2825 2009-05-06 19:48:47Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004-2005, Romain Liévin
 *  Copyright (c) 2007, Romain Liévin
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
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <gdk/gdkkeysyms.h>

#include "intl.h"
#include "paths.h"
#include "support.h"
#include "ti68k_int.h"
#include "struct.h"
#include "dbg_wnds.h"

#define FLAG_T	15
#define FLAG_S	13
#define FLAG_X	4
#define FLAG_N	3
#define FLAG_Z	2
#define FLAG_V	1
#define FLAG_C	0

typedef struct
{
	GtkWidget	*d[8];
	GtkWidget	*a[8];
	GtkWidget	*pc;
	GtkWidget	*sr;
	GtkWidget	*usp, *ssp;

	GtkWidget	*t, *s, *i;
	GtkWidget	*x, *n, *z, *v, *c;
} WidgetRegs;

static WidgetRegs	wregs = { 0 };
static GdkColor red, black;

static int validate_value(const char *str, int ndigits)
{
	int i;
	
	if((int)strlen(str) > ndigits)
	 	return 0;
	
	for(i = 0; (i < ndigits) && (i < (int)strlen(str)); i++)
	{
		if(!isxdigit(str[i]))
			return 0;
	}
	
	return !0;
}

GLADE_CB void
on_dbgregs_checkbutton_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

static void change_widget_color(GtkWidget *widget, int changed)
{
#if 1
	gtk_widget_modify_text(widget, GTK_STATE_NORMAL, changed ? &red : &black);
#endif
}

static void labels_refresh(void)
{
	int i;
	int changed;
	uint32_t data, addr;
	gchar *str;

	// refresh Ax
	for(i = 0; i < 8; i++)
	{
		changed = ti68k_register_get_addr(i, &addr);
		str = g_strdup_printf("%08x", addr);
		gtk_entry_set_text(GTK_ENTRY(wregs.a[i]), str);
		change_widget_color(wregs.a[i], changed);
		g_free(str);
	}

	// refresh Dx
	for(i = 0; i < 8; i++)
	{
		changed = ti68k_register_get_data(i, &data);
		str = g_strdup_printf("%08x", data);
		gtk_entry_set_text(GTK_ENTRY(wregs.d[i]), str);
		change_widget_color(wregs.d[i], changed);
		g_free(str);
	}

	// refresh misc register
	changed = ti68k_register_get_pc(&data);
	str = g_strdup_printf("%06x", data);
	gtk_entry_set_text(GTK_ENTRY(wregs.pc), str);
	change_widget_color(wregs.pc, changed);
	g_free(str);

	changed = ti68k_register_get_usp(&data);
	str = g_strdup_printf("%06x", data);
	gtk_entry_set_text(GTK_ENTRY(wregs.usp), str);
	change_widget_color(wregs.usp, changed);
	g_free(str);

	changed = ti68k_register_get_ssp(&data);
	str = g_strdup_printf("%06x", data);
	gtk_entry_set_text(GTK_ENTRY(wregs.ssp), str);
	change_widget_color(wregs.ssp, changed);
	g_free(str);

	changed = ti68k_register_get_sr(&data);
	str = g_strdup_printf("%04x", data);
	gtk_entry_set_text(GTK_ENTRY(wregs.sr), str);
	change_widget_color(wregs.sr, changed);
	g_free(str);

	// refresh SR
	ti68k_register_get_sr(&data);

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(wregs.i), (data >> 8) & 7);

	g_signal_handlers_block_by_func(GTK_OBJECT(wregs.t), on_dbgregs_checkbutton_toggled, NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wregs.t), data & (1 << FLAG_T));
	g_signal_handlers_unblock_by_func(GTK_OBJECT(wregs.t), on_dbgregs_checkbutton_toggled, NULL);

	g_signal_handlers_block_by_func(GTK_OBJECT(wregs.s), on_dbgregs_checkbutton_toggled, NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wregs.s), data & (1 << FLAG_S));
	g_signal_handlers_unblock_by_func(GTK_OBJECT(wregs.s), on_dbgregs_checkbutton_toggled, NULL);

	g_signal_handlers_block_by_func(GTK_OBJECT(wregs.x), on_dbgregs_checkbutton_toggled, NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wregs.x), data & (1 << FLAG_X));
	g_signal_handlers_unblock_by_func(GTK_OBJECT(wregs.x), on_dbgregs_checkbutton_toggled, NULL);

	g_signal_handlers_block_by_func(GTK_OBJECT(wregs.n), on_dbgregs_checkbutton_toggled, NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wregs.n), data & (1 << FLAG_N));
	g_signal_handlers_unblock_by_func(GTK_OBJECT(wregs.n), on_dbgregs_checkbutton_toggled, NULL);

	g_signal_handlers_block_by_func(GTK_OBJECT(wregs.z), on_dbgregs_checkbutton_toggled, NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wregs.z), data & (1 << FLAG_Z));
	g_signal_handlers_unblock_by_func(GTK_OBJECT(wregs.z), on_dbgregs_checkbutton_toggled, NULL);

	g_signal_handlers_block_by_func(GTK_OBJECT(wregs.v), on_dbgregs_checkbutton_toggled, NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wregs.v), data & (1 << FLAG_V));
	g_signal_handlers_unblock_by_func(GTK_OBJECT(wregs.v), on_dbgregs_checkbutton_toggled, NULL);

	g_signal_handlers_block_by_func(GTK_OBJECT(wregs.c), on_dbgregs_checkbutton_toggled, NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wregs.c), data & (1 << FLAG_C));
	g_signal_handlers_unblock_by_func(GTK_OBJECT(wregs.c), on_dbgregs_checkbutton_toggled, NULL);
}

/*
	Display registers window
*/
GtkWidget* dbgregs_create_window(void)
{
	GladeXML *xml = NULL;
	GtkWidget *dbox;
	
	xml = glade_xml_new
		(tilp_paths_build_glade("dbg_regs-2.glade"), "dbgregs2_window",
		 PACKAGE);
	if (!xml)
		g_error(_("%s: GUI loading failed!\n"), __FILE__);
	glade_xml_signal_autoconnect(xml);
	
	dbox = glade_xml_get_widget(xml, "dbgregs2_window");
	if(options3.transient)
		gtk_window_set_transient_for(GTK_WINDOW(dbox), GTK_WINDOW(main_wnd));

	wregs.d[0] = glade_xml_get_widget(xml, "entry10");
	wregs.d[1] = glade_xml_get_widget(xml, "entry11");
	wregs.d[2] = glade_xml_get_widget(xml, "entry12");
	wregs.d[3] = glade_xml_get_widget(xml, "entry13");
	wregs.d[4] = glade_xml_get_widget(xml, "entry14");
	wregs.d[5] = glade_xml_get_widget(xml, "entry15");
	wregs.d[6] = glade_xml_get_widget(xml, "entry16");
	wregs.d[7] = glade_xml_get_widget(xml, "entry17");

	wregs.a[0] = glade_xml_get_widget(xml, "entry20");
	wregs.a[1] = glade_xml_get_widget(xml, "entry21");
	wregs.a[2] = glade_xml_get_widget(xml, "entry22");
	wregs.a[3] = glade_xml_get_widget(xml, "entry23");
	wregs.a[4] = glade_xml_get_widget(xml, "entry24");
	wregs.a[5] = glade_xml_get_widget(xml, "entry25");
	wregs.a[6] = glade_xml_get_widget(xml, "entry26");
	wregs.a[7] = glade_xml_get_widget(xml, "entry27");

	wregs.usp = glade_xml_get_widget(xml, "entry31");
	wregs.ssp = glade_xml_get_widget(xml, "entry32");
	wregs.pc = glade_xml_get_widget(xml, "entry33");
	wregs.sr = glade_xml_get_widget(xml, "entry34");

	wregs.t = glade_xml_get_widget(xml, "checkbutton41");
	wregs.s = glade_xml_get_widget(xml, "checkbutton42");
	wregs.i = glade_xml_get_widget(xml, "spinbutton44");
	wregs.x = glade_xml_get_widget(xml, "checkbutton51");
	wregs.n = glade_xml_get_widget(xml, "checkbutton52");
	wregs.z = glade_xml_get_widget(xml, "checkbutton53");
	wregs.v = glade_xml_get_widget(xml, "checkbutton54");
	wregs.c = glade_xml_get_widget(xml, "checkbutton55");

	// Change for a fixed size font
	if(!options3.dbg_font_type && options3.dbg_font_name)
	{
		PangoContext *context;
		PangoFontDescription *desc;
		const gchar *family;
		int size;
		int i;
		
		context = gtk_widget_get_pango_context(wregs.pc);
		desc = pango_context_get_font_description(context);
		family = pango_font_description_get_family(desc);
		size = pango_font_description_get_size(desc);

		pango_font_description_set_family(desc, "Courier");
		pango_font_description_set_size(desc, (int)(size*0.8));

		gtk_widget_modify_font(wregs.pc, desc);
		gtk_widget_modify_font(wregs.sr, desc);
		gtk_widget_modify_font(wregs.usp, desc);
		gtk_widget_modify_font(wregs.ssp, desc);
		for(i = 0; i < 8; i++)
		{
			gtk_widget_modify_font(wregs.d[i], desc);
			gtk_widget_modify_font(wregs.a[i], desc);
		}
	}
	else
	{
		PangoFontDescription *desc;
		int i;

		desc = pango_font_description_from_string(options3.dbg_font_name);

		gtk_widget_modify_font(wregs.pc, desc);
		gtk_widget_modify_font(wregs.sr, desc);
		gtk_widget_modify_font(wregs.usp, desc);
		gtk_widget_modify_font(wregs.ssp, desc);
		for(i = 0; i < 8; i++)
		{
			gtk_widget_modify_font(wregs.d[i], desc);
			gtk_widget_modify_font(wregs.a[i], desc);
		}
	}

	// Allocate colors
	{
		gboolean success;

		gdk_color_parse("Black", &black);
		gdk_colormap_alloc_colors(gdk_colormap_get_system(), &black, 1,
				  FALSE, FALSE, &success);
		gdk_color_parse("Red", &red);
		gdk_colormap_alloc_colors(gdk_colormap_get_system(), &red, 1,
				  FALSE, FALSE, &success);
	}

	return dbox;
}

GtkWidget* dbgregs_display_window(void)
{
#ifdef WND_STATE
	if(!options3.regs.minimized)
	{
		gtk_window_resize(GTK_WINDOW(dbgw.regs), options3.regs.rect.w, options3.regs.rect.h);
		gtk_window_move(GTK_WINDOW(dbgw.regs), options3.regs.rect.x, options3.regs.rect.y);
	}
	else
		gtk_window_iconify(GTK_WINDOW(dbgw.regs));
#endif

	if(!GTK_WIDGET_VISIBLE(dbgw.regs) && !options3.regs.closed)
		gtk_widget_show(dbgw.regs);

	return dbgw.regs;
}

void dbgregs_refresh_window(void)
{
	WND_TMR_START();

	if(!options3.regs.closed)
	{
		labels_refresh();
	}

	WND_TMR_STOP("Registers Refresh Time");
}

GLADE_CB gboolean
on_dbgregs_key_press_event             (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data)
{
	const gchar *label = gtk_label_get_text(GTK_LABEL(widget));
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(user_data));
	uint32_t value;
	int n = label[1] - '0';

	if(event->keyval != GDK_Return && event->keyval != GDK_KP_Enter)
		return FALSE;

	if(label[0] == 'A')
	{
		if(validate_value(text, 8))
		{
			sscanf(text, "%x", &value);			
			ti68k_register_set_addr(n, value);
		}

		if((n == 6) || (n == 7))
			dbgstack_refresh_window();
	} 
	else if(label[0] == 'D')
	{
		if(validate_value(text, 8))
		{
			sscanf(text, "%x", &value);			
			ti68k_register_set_data(n, value);
		}
	}
	else if(!strcmp(label, "PC="))
	{
		if(validate_value(text, 6))
		{
			sscanf(text, "%x", &value);			
			ti68k_register_set_pc(value);
		}
	}
	else if(!strcmp(label, "USP="))
	{
		if(validate_value(text, 6))
		{
			sscanf(text, "%x", &value);			
			ti68k_register_set_usp(value);
		}
	}
	else if(!strcmp(label, "SSP="))
	{
		if(validate_value(text, 6))
		{
			sscanf(text, "%x", &value);			
			ti68k_register_set_ssp(value);
		}
	}
	else if(!strcmp(label, "SR="))
	{
		if(validate_value(text, 4))
		{
			sscanf(text, "%x", &value);			
			ti68k_register_set_sr(value);

			dbgregs_refresh_window();
            dbgstack_refresh_window();
		}
	}

	labels_refresh();
	return TRUE;
}

GLADE_CB void
on_dbgregs_checkbutton_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	const gchar *label;
	gboolean state;
	uint32_t data;
	gchar *str;

	label = gtk_button_get_label(GTK_BUTTON(togglebutton));
	state = gtk_toggle_button_get_active(togglebutton);

	ti68k_register_get_sr(&data);

	switch(label[0])
	{
	case 'T':
		data &= ~(1 << FLAG_T);
		data |= (state << FLAG_T);
	break;
	case 'S':
		data &= ~(1 << FLAG_S);
		data |= (state << FLAG_S);
	break;
	case 'X':
		data &= ~(1 << FLAG_X);
		data |= (state << FLAG_X);
	break;
	case 'N':
		data &= ~(1 << FLAG_N);
		data |= (state << FLAG_N);
	break;
	case 'Z':
		data &= ~(1 << FLAG_Z);
		data |= (state << FLAG_Z);
	break;
	case 'V':
		data &= ~(1 << FLAG_V);
		data |= (state << FLAG_V);
	break;
	case 'C':
		data &= ~(1 << FLAG_C);
		data |= (state << FLAG_C);
	break;
	default:
	return;
	}

	ti68k_register_set_sr(data);

	str = g_strdup_printf("%04x", data);
	gtk_entry_set_text(GTK_ENTRY(wregs.sr), str);
	g_free(str);

	// update usp <=> ssp
    dbgregs_refresh_window();
    dbgstack_refresh_window();
}


GLADE_CB void
on_dbgregs_spinbutton_changed          (GtkEditable     *editable,
                                        gpointer         user_data)
{
	int i;
	uint32_t data;
	gchar *str;

	i = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(editable));

	ti68k_register_get_sr(&data);
	data &= ~0x700;
	data |= (i << 8);
	ti68k_register_set_sr(data);

	str = g_strdup_printf("%04x", data);
	gtk_entry_set_text(GTK_ENTRY(wregs.sr), str);
	g_free(str);
}

static GtkWidget* display_dbgregs_popup_menu(void);
static uint32_t value = 0;

GLADE_CB gboolean
on_dbgregs_button_press_event          (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
	if (event->type == GDK_2BUTTON_PRESS)
	{
		// select all text
		gtk_editable_select_region(GTK_EDITABLE(user_data), 0, -1);
		return TRUE;
	}
	else if (event->button == 3)
    {
        GdkEventButton *bevent;
        GtkWidget *menu;  
		const gchar *label = gtk_label_get_text(GTK_LABEL(widget));
		const gchar *text;
		int n = label[1] - '0';

		if(label[0] == 'A')
			text = gtk_entry_get_text(GTK_ENTRY(wregs.a[n]));
		else if(label[0] == 'D')
			text = gtk_entry_get_text(GTK_ENTRY(wregs.d[n]));
		else if(!strcmp(label, "PC="))
			text = gtk_entry_get_text(GTK_ENTRY(wregs.pc));
		else if(!strcmp(label, "USP="))
			text = gtk_entry_get_text(GTK_ENTRY(wregs.usp));
		else if(!strcmp(label, "SSP="))
			text = gtk_entry_get_text(GTK_ENTRY(wregs.ssp));

        sscanf(text, "%x", &value);

        // popup menu
       	bevent = (GdkEventButton *) (event);
        menu = display_dbgregs_popup_menu();

		gtk_menu_popup(GTK_MENU(menu),
				   NULL, NULL, NULL, NULL,
				   bevent->button, bevent->time);
	    gtk_widget_show(menu);

		return TRUE;
	}

	return FALSE;
}

/***** Popup menu *****/

/*
	Display popup menu (right click)
*/
static GtkWidget* display_dbgregs_popup_menu(void)
{
	GladeXML *xml;
	GtkWidget *menu;

	xml = glade_xml_new
	    (tilp_paths_build_glade("dbg_regs-2.glade"), "dbgregs_popup",
	     PACKAGE);
	if (!xml)
		g_error(_("%s: GUI loading failed!\n"), __FILE__);
	glade_xml_signal_autoconnect(xml);

	menu = glade_xml_get_widget(xml, "dbgregs_popup");
	return menu;
}

GLADE_CB void
on_go_to_address4_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    if(value == -1)
        return;
    
    dbgmem_add_tab(value);

    value = -1;
}
