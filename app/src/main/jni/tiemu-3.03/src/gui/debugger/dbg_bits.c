/* Hey EMACS -*- linux-c -*- */
/* $Id: dbg_data.c 2385 2007-03-12 21:04:20Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2007, Romain Liévin, Kevin Kofler
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

gint dbgbits_display_dbox(uint32_t *address, uint8_t *checks, uint8_t *states)
{
	GtkWidget *dbox;
	GtkWidget *data;
	gint result;
	gchar *str;
	gint i;
	
	xml = glade_xml_new
		(tilp_paths_build_glade("dbg_bits-2.glade"), "dbgbits_dbox",
		 PACKAGE);
	if (!xml)
		g_error(_("%s: GUI loading failed!\n"), __FILE__);
	glade_xml_signal_autoconnect(xml);
	
	dbox = glade_xml_get_widget(xml, "dbgbits_dbox");
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dbox), GTK_RESPONSE_OK,
	                                        GTK_RESPONSE_CANCEL,-1);

	data = glade_xml_get_widget(xml, "entry4");
	str = g_strdup_printf("0x%06x", *address);
	gtk_entry_set_text(GTK_ENTRY(data), str);
	g_free(str);

	for(i = 0; i < 8; i++)
	{
		str = g_strdup_printf("checkbutton1%i", i);
		data = glade_xml_get_widget(xml, str);
		g_free(str);

		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data), *checks & (1 << i));
	}

	for(i = 0; i < 8; i++)
	{
		str = g_strdup_printf("checkbutton2%i", i);
		data = glade_xml_get_widget(xml, str);
		g_free(str);

		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data), *states & (1 << i));
	}

loop:
	result = gtk_dialog_run(GTK_DIALOG(dbox));
	switch (result) 
	{
	case GTK_RESPONSE_OK:
		data = glade_xml_get_widget(xml, "entry4");
		str = (gchar *)gtk_entry_get_text(GTK_ENTRY(data));
		result = sscanf(str, "%x", address);
		if(result < 1)
			goto loop;

		for(*checks = i = 0; i < 8; i++)
		{
			str = g_strdup_printf("checkbutton1%i", i);
			data = glade_xml_get_widget(xml, str);
			g_free(str);

			*checks |= (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data)) << i);
		}

		for(*states = i = 0; i < 8; i++)
		{
			str = g_strdup_printf("checkbutton2%i", i);
			data = glade_xml_get_widget(xml, str);
			g_free(str);

			*states |= (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data)) << i);
		}

		gtk_widget_destroy(dbox);
		return 0;
	default:
		gtk_widget_destroy(dbox);
		return -1;
	}	

	return 0;
}
