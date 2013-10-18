/* Hey EMACS -*- linux-c -*- */
/* $Id: dbg_data.c 2753 2007-12-30 23:14:15Z kevinkofler $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Liévin
 *  Copyright (c) 2005, Romain Liévin
 *  Copyright (c) 2007, Kevin Kofler
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

#include "intl.h"
#include "paths.h"
#include "ti68k_int.h"
#include "support.h"
#include "dbg_bkpts.h"

static GladeXML *xml;

gint dbgdata_display_dbox(gint *mode, gint *type, uint32_t *start, uint32_t *stop)
{
	GtkWidget *dbox;
	GtkWidget *data;
	gint result;

	G_CONST_RETURN gchar *sc_start, *sc_stop;
	gchar *s_start, *s_stop;
	
	xml = glade_xml_new
		(tilp_paths_build_glade("dbg_data-2.glade"), "dbgdata_dbox",
		 PACKAGE);
	if (!xml)
		g_error(_("%s: GUI loading failed!\n"), __FILE__);
	glade_xml_signal_autoconnect(xml);
	
	dbox = glade_xml_get_widget(xml, "dbgdata_dbox");
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dbox), GTK_RESPONSE_OK,
	                                        GTK_RESPONSE_CANCEL,-1);

	data = glade_xml_get_widget(xml, "radiobutton20");
	g_signal_emit_by_name(G_OBJECT(data), "toggled");

	// set type
	if(*type == -1)
	{
		// skip box preset step
	}
	else 
	{
		data = glade_xml_get_widget(xml, "radiobutton10");
		if(*mode & BK_READ)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data), TRUE);

		data = glade_xml_get_widget(xml, "radiobutton11");
		if(*mode & BK_WRITE)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data), TRUE);

		data = glade_xml_get_widget(xml, "radiobutton12");
		if((*mode & BK_READ) && (*mode & BK_WRITE))
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data), TRUE);

		if(*type == BK_TYPE_ACCESS)
		{
			data = glade_xml_get_widget(xml, "radiobutton20");
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data), TRUE);

			data = glade_xml_get_widget(xml, "comboboxentry1");
			if(*mode & BK_BYTE)
				gtk_combo_box_set_active(GTK_COMBO_BOX(data), 0);
			else if(*mode & BK_WORD)
				gtk_combo_box_set_active(GTK_COMBO_BOX(data), 1);
			else if(*mode & BK_LONG)
				gtk_combo_box_set_active(GTK_COMBO_BOX(data), 2);

			data = glade_xml_get_widget(xml, "entry3");
			s_start = g_strdup_printf("0x%06x", *start);
			gtk_entry_set_text(GTK_ENTRY(data), s_start);
			g_free(s_start);
		}
		else if(*type == BK_TYPE_RANGE)
		{
			data = glade_xml_get_widget(xml, "radiobutton21");
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data), TRUE);

			data = glade_xml_get_widget(xml, "entry1");
			s_start = g_strdup_printf("0x%06x", *start);
			gtk_entry_set_text(GTK_ENTRY(data), s_start);
			g_free(s_start);

			data = glade_xml_get_widget(xml, "entry2");
			s_stop = g_strdup_printf("0x%06x", *stop);
			gtk_entry_set_text(GTK_ENTRY(data), s_stop);
			g_free(s_stop);
		}
	}

loop:
	result = gtk_dialog_run(GTK_DIALOG(dbox));
	switch (result) {
	case GTK_RESPONSE_OK:
		sc_start = sc_stop = "";

		// Retrieve settings from fields
		data = glade_xml_get_widget(xml, "radiobutton10");
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data)))
			*mode = BK_READ;
		data = glade_xml_get_widget(xml, "radiobutton11");
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data)))
			*mode = BK_WRITE;
		data = glade_xml_get_widget(xml, "radiobutton12");
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data)))
			*mode = BK_READ | BK_WRITE;

		data = glade_xml_get_widget(xml, "radiobutton20");
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data)))
			*type = BK_TYPE_ACCESS;

		data = glade_xml_get_widget(xml, "radiobutton21");
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data)))
			*type = BK_TYPE_RANGE;
	
		if(*type == BK_TYPE_ACCESS)
		{
			data = glade_xml_get_widget(xml, "comboboxentry1");
			switch(gtk_combo_box_get_active(GTK_COMBO_BOX(data)))
			{
			case 0: *mode |= BK_BYTE; break;
			case 1: *mode |= BK_WORD; break;
			case 2: *mode |= BK_LONG; break;
			}

			data = glade_xml_get_widget(xml, "entry3");
			sc_start = sc_stop = gtk_entry_get_text(GTK_ENTRY(data));
		} 
		else if(*type == BK_TYPE_RANGE)
		{
			data = glade_xml_get_widget(xml, "entry1");
			sc_start = gtk_entry_get_text(GTK_ENTRY(data));

			data = glade_xml_get_widget(xml, "entry2");
			sc_stop = gtk_entry_get_text(GTK_ENTRY(data));			
		}

		// Convert values and check
		result = sscanf(sc_start, "%x", start);
		if(result < 1)
			goto loop;

		result = sscanf(sc_stop, "%x", stop);
		if((result < 1) && (*type == 2))
			goto loop;

		if((*start > *stop) && (*type == 2))
			goto loop;

		gtk_widget_destroy(dbox);
		return 0;
	default:
		gtk_widget_destroy(dbox);
		return -1;
	}	

	return 0;
}


GLADE_CB void
on_radiobutton20_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	GtkWidget *data;

	data = glade_xml_get_widget(xml, "optionmenu1");
	gtk_widget_set_sensitive(data, TRUE);
	data = glade_xml_get_widget(xml, "entry3");
	gtk_widget_set_sensitive(data, TRUE);

	data = glade_xml_get_widget(xml, "entry1");
	gtk_widget_set_sensitive(data, FALSE);
	data = glade_xml_get_widget(xml, "entry2");
	gtk_widget_set_sensitive(data, FALSE);
}

GLADE_CB void
on_radiobutton21_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	GtkWidget *data;

	data = glade_xml_get_widget(xml, "optionmenu1");
	gtk_widget_set_sensitive(data, FALSE);
	data = glade_xml_get_widget(xml, "entry3");
	gtk_widget_set_sensitive(data, FALSE);

	data = glade_xml_get_widget(xml, "entry1");
	gtk_widget_set_sensitive(data, TRUE);
	data = glade_xml_get_widget(xml, "entry2");
	gtk_widget_set_sensitive(data, TRUE);
}



