/* Hey EMACS -*- linux-c -*- */
/* $Id: infos.c 2444 2007-04-15 08:29:34Z roms $ */

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
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details. *
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
#include "skinops.h"
#include "ti68k_int.h"

gint display_infos_dbox()
{
	GladeXML *xml;
	GtkWidget *dbox;
	GtkWidget *label;
	gint result;
	gchar *str;
	
	xml = glade_xml_new
		(tilp_paths_build_glade("infos-2.glade"), "infos_dbox",
		 PACKAGE);
	if (!xml)
		g_error(_("%s: GUI loading failed!\n"), __FILE__);
	glade_xml_signal_autoconnect(xml);
	
	dbox = glade_xml_get_widget(xml, "infos_dbox");

	label = glade_xml_get_widget(xml, "label20");
	switch(skin_infos.type)
	{
	case SKIN_TYPE_TIEMU:	str = g_strdup_printf("%s", "TiEmu v2.00"); break;
	case SKIN_TYPE_VTI:		str = g_strdup_printf("%s", "VTi 2.5"); break;
	case SKIN_TYPE_OLD_VTI:	str = g_strdup_printf("%s", "VTi 2.1"); break;
	default:				str = g_strdup_printf("%s", _("unknown")); break;
	}	
	gtk_label_set_text(GTK_LABEL(label), str);
	g_free(str);
	
	label = glade_xml_get_widget(xml, "label21");
	str = g_strdup_printf("%s", skin_infos.name);
	gtk_label_set_text(GTK_LABEL(label), str);
	g_free(str);
	
	label = glade_xml_get_widget(xml, "label22");
	if(skin_infos.author)
	    str = g_strdup_printf("%s", skin_infos.author);
	else
	    str = g_strdup("");
	gtk_label_set_text(GTK_LABEL(label), str);
	g_free(str);
	
	label = glade_xml_get_widget(xml, "label23");
	str = g_strdup_printf("%s", ti68k_calctype_to_string(tihw.calc_type));
	gtk_label_set_text(GTK_LABEL(label), str);
	g_free(str);
	
	label = glade_xml_get_widget(xml, "label24");
	str = g_strdup_printf("%s", tihw.rom_version);
	gtk_label_set_text(GTK_LABEL(label), str);
	g_free(str);
	
	label = glade_xml_get_widget(xml, "label25");
	str = g_strdup_printf("%i KB", tihw.ram_size >> 10);
	gtk_label_set_text(GTK_LABEL(label), str);
	g_free(str);
	
	label = glade_xml_get_widget(xml, "label26");
	str = g_strdup_printf("%i KB", tihw.rom_size >> 10);
	gtk_label_set_text(GTK_LABEL(label), str);
	g_free(str);
	
	label = glade_xml_get_widget(xml, "label27");
	str = g_strdup_printf("%s", ti68k_romtype_to_string(tihw.rom_flash));
	gtk_label_set_text(GTK_LABEL(label), str);
	g_free(str);

	label = glade_xml_get_widget(xml, "label28");
	str = g_strdup_printf("%s", ti68k_hwtype_to_string(tihw.hw_type));
	gtk_label_set_text(GTK_LABEL(label), str);
	g_free(str);
	
	result = gtk_dialog_run(GTK_DIALOG(dbox));
	switch (result) {
	case GTK_RESPONSE_OK:
		break;
	default:
		break;
	}

	gtk_widget_destroy(dbox);

	return 0;
}
