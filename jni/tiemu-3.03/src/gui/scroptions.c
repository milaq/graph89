/* Hey EMACS -*- linux-c -*- */
/* $Id: scroptions.c 2753 2007-12-30 23:14:15Z kevinkofler $ */

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
#include <string.h>

#include "intl.h"
#include "support.h"
#include "paths.h"
#include "struct.h"
//#include "skinops.h"
#include "ti68k_int.h"
#include "screenshot.h"

static ScrOptions tmp_options;

static GtkWidget *frame = NULL;
static GtkWidget *label = NULL;
void refresh_label(void);

gint display_scroptions_dbox()
{
	GladeXML *xml;
	GtkWidget *dbox;
	GtkWidget *data;
	gint result;
	
	xml = glade_xml_new
		(tilp_paths_build_glade("scroptions-2.glade"), 
		 "scroptions_dbox",
		 PACKAGE);
	if (!xml)
		g_error(_("%s: GUI loading failed!\n"), __FILE__);
	glade_xml_signal_autoconnect(xml);
	
	dbox = glade_xml_get_widget(xml, "scroptions_dbox");
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dbox), GTK_RESPONSE_OK,
	                                        GTK_RESPONSE_CANCEL,-1);
	memcpy(&tmp_options, &options2, sizeof(ScrOptions));
    tmp_options.file = g_strdup(options2.file);
	tmp_options.folder = g_strdup(options2.folder);

	frame = glade_xml_get_widget(xml, "frame5");
	gtk_widget_set_sensitive(frame, tmp_options.size == IMG_LCD);

	switch (tmp_options.format)
	{
	case IMG_JPG: 
		data = glade_xml_get_widget(xml, "radiobutton30");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data), TRUE);
		break;
	case IMG_PNG: 
		data = glade_xml_get_widget(xml, "radiobutton31");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data), TRUE);
		break;
	case IMG_ICO: 
		data = glade_xml_get_widget(xml, "radiobutton32");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data), TRUE);
		break; 
	case IMG_EPS:
		data = glade_xml_get_widget(xml, "radiobutton33");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data), TRUE);
		break;
	case IMG_PDF:
		data = glade_xml_get_widget(xml, "radiobutton34");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data), TRUE);
		break;
	case IMG_BMP:
		data = glade_xml_get_widget(xml, "radiobutton35");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data), TRUE);
		break;
	}

	switch (tmp_options.type)
	{
	case IMG_BW:
		data = glade_xml_get_widget(xml, "radiobutton10");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data), TRUE);
		break;
	case IMG_COL:
		data = glade_xml_get_widget(xml, "radiobutton11");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data), TRUE);
		break;
	}

	switch (tmp_options.size)
	{
	case IMG_LCD:
		data = glade_xml_get_widget(xml, "radiobutton20");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data), TRUE);
		break;
	case IMG_SKIN:
		data = glade_xml_get_widget(xml, "radiobutton21");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data), TRUE);
		break;
	}

	data = glade_xml_get_widget(xml, "spinbutton1");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(data), tmp_options.shots);

	data = glade_xml_get_widget(xml, "spinbutton2");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(data), tmp_options.skips);

	label = glade_xml_get_widget(xml, "label10");
	refresh_label();
		
	data = glade_xml_get_widget(xml, "entry10");
	gtk_entry_set_text(GTK_ENTRY(data), tmp_options.file);

	data = glade_xml_get_widget(xml, "filechooserbutton1");
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (data), tmp_options.folder);
	
	result = gtk_dialog_run(GTK_DIALOG(dbox));
	switch (result) {
	case GTK_RESPONSE_OK:
        g_free(options2.file);
		g_free(options2.folder);

		memcpy(&options2, &tmp_options, sizeof(ScrOptions));

		options2.file = g_strdup(tmp_options.file);
        g_free(tmp_options.file);

		options2.folder = g_strdup(tmp_options.folder);
		g_free(tmp_options.folder);
		break;
	default:
		break;
	}

	frame = label = NULL;
	gtk_widget_destroy(dbox);

	return 0;
}

void refresh_label(void)
{
    gchar *ext = "???";
    gchar *str;
	
    if(label == NULL)
        return;

	switch(tmp_options.format)
	{
	    case IMG_JPG: ext = "jpg"; break;
	    case IMG_PNG: ext = "png"; break;
	    case IMG_ICO: ext = "ico"; break;
	    case IMG_EPS: ext = "eps"; break;
	    case IMG_PDF: ext = "pdf"; break;
		case IMG_BMP: ext = "bmp"; break;
	    default: break;
	}
	
    str = g_strdup_printf("%03i.%s", tmp_options.counter, ext);
	gtk_label_set_text(GTK_LABEL(label), str);
    g_free(str);
}


GLADE_CB void
on_scopt_radiobutton10_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  if(gtk_toggle_button_get_active(togglebutton))
    tmp_options.type = IMG_BW;
}


GLADE_CB void
on_scopt_radiobutton11_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    if(gtk_toggle_button_get_active(togglebutton))
        tmp_options.type = IMG_COL;
}


GLADE_CB void
on_scopt_radiobutton20_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    if(gtk_toggle_button_get_active(togglebutton))
        tmp_options.size = IMG_LCD;
	gtk_widget_set_sensitive(frame, tmp_options.size == IMG_LCD);
}


GLADE_CB void
on_scopt_radiobutton21_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    if(gtk_toggle_button_get_active(togglebutton))
        tmp_options.size = IMG_SKIN;
	gtk_widget_set_sensitive(frame, tmp_options.size == IMG_LCD);
}


GLADE_CB void
on_scopt_radiobutton30_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    if(gtk_toggle_button_get_active(togglebutton))
        tmp_options.format = IMG_JPG;
    refresh_label();
}


GLADE_CB void
on_scopt_radiobutton31_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    if(gtk_toggle_button_get_active(togglebutton))
        tmp_options.format = IMG_PNG;
    refresh_label();
}


GLADE_CB void
on_scopt_radiobutton32_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    if(gtk_toggle_button_get_active(togglebutton))
        tmp_options.format = IMG_ICO;
    refresh_label();
}


GLADE_CB void
on_scopt_radiobutton33_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    if(gtk_toggle_button_get_active(togglebutton))
        tmp_options.format = IMG_EPS;
    refresh_label();
}


GLADE_CB void
on_scopt_radiobutton34_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    if(gtk_toggle_button_get_active(togglebutton))
        tmp_options.format = IMG_PDF;
    refresh_label();
}

GLADE_CB void
on_scopt_radiobutton35_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    if(gtk_toggle_button_get_active(togglebutton))
        tmp_options.format = IMG_BMP;
    refresh_label();
}


GLADE_CB void
on_entry1_changed                      (GtkEditable     *editable,
                                        gpointer         user_data)
{
    gchar *str;

    g_free(tmp_options.file);
    str = gtk_editable_get_chars(editable, 0, -1);
    tmp_options.file = g_strdup(str);
    g_free(str);
    refresh_label();
}

GLADE_CB void
on_filechooserbutton1_current_folder_changed
                                        (GtkFileChooser  *filechooser,
                                        gpointer         user_data)
{
	gchar *fname = gtk_file_chooser_get_filename (filechooser);

	g_free(tmp_options.folder);
	tmp_options.folder = fname;
}


GLADE_CB void
on_spinbutton1_changed                 (GtkEditable     *editable,
                                        gpointer         user_data)
{
	tmp_options.shots = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(editable));
}


GLADE_CB void
on_spinbutton2_changed                 (GtkEditable     *editable,
                                        gpointer         user_data)
{
	tmp_options.skips = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(editable));
}

/* */
