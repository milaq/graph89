/* Hey EMACS -*- linux-c -*- */
/* $Id: wizard.c 2753 2007-12-30 23:14:15Z kevinkofler $ */

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
#endif

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <stdlib.h>

#include "intl.h"
#include "support.h"
#include "struct.h"
#include "dboxes.h"
#include "wizard.h"
//#include "engine.h"
#include "filesel.h"
#include "ti68k_int.h"
#include "tie_error.h"

#ifdef __WIN32__
# define strcasecmp _stricmp
#endif

gchar *wizard_rom = NULL;
gint wizard_ok = FALSE;

static gint action = 0;

static gint display_step1_dbox(void);
gint display_wz_rom_dbox(void);
gint display_wz_tib_dbox(void);

gint display_wizard_dbox(void)
{
    return display_step1_dbox();
}

static gint display_msg_dbox(void)
{
    GladeXML *xml;
	GtkWidget *dbox;
	gint result;

    xml = glade_xml_new
	    (tilp_paths_build_glade("wizard-2.glade"), "msg_dbox", PACKAGE);
	if (!xml)
		g_error(_("%s: GUI loading failed!\n"), __FILE__);
	glade_xml_signal_autoconnect(xml);

	dbox = glade_xml_get_widget(xml, "msg_dbox");

    result = gtk_dialog_run(GTK_DIALOG(dbox));
	
    gtk_widget_destroy(dbox);
    exit(0);

	return 0;
}

static gint display_step1_dbox(void)
{
    GladeXML *xml;
	GtkWidget *dbox;
	GtkWidget *data;
	gint result = 0;
	GtkWidget *dialog;

    xml = glade_xml_new
	    (tilp_paths_build_glade("wizard-2.glade"), "step1_dbox", PACKAGE);
	if (!xml)
		g_error(_("comm.c: GUI loading failed !\n"));
	glade_xml_signal_autoconnect(xml);

	dbox = glade_xml_get_widget(xml, "step1_dbox");
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dbox), GTK_RESPONSE_OK,
	                                        GTK_RESPONSE_APPLY, GTK_RESPONSE_CANCEL,-1);

    data = glade_xml_get_widget(xml, "applybutton1");
    gtk_widget_hide(data);

#ifdef __IPAQ__
    // make menu smaller
    data = glade_xml_get_widget(xml, "label19");
    gtk_widget_hide(data);
    data = glade_xml_get_widget(xml, "radiobutton1");
    gtk_button_set_label(GTK_BUTTON(data), _("Use PedRom"));
    data = glade_xml_get_widget(xml, "radiobutton2");
    gtk_button_set_label(GTK_BUTTON(data), _("Use FLASH OS"));
    data = glade_xml_get_widget(xml, "radiobutton3");
    gtk_button_set_label(GTK_BUTTON(data), _("Use ROM dump"));
    data = glade_xml_get_widget(xml, "radiobutton4");
    gtk_widget_hide(data);
    data = glade_xml_get_widget(xml, "label20");
    gtk_widget_hide(data);
#endif

	action = 1;	// default button
    result = gtk_dialog_run(GTK_DIALOG(dbox));

    if(result == GTK_RESPONSE_OK)
    {
        switch(action)
        {
            case 1: 
				dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
						GTK_MESSAGE_INFO,
						GTK_BUTTONS_CLOSE, _("Importing TIBs. Please wait..."));
				g_signal_connect_swapped(GTK_OBJECT(dialog), "response",
						G_CALLBACK(gtk_widget_destroy),
						GTK_OBJECT(dialog));
				gtk_widget_show_all(GTK_WIDGET(dialog));
				while(gtk_events_pending()) gtk_main_iteration();
				
				ti68k_scan_files(inst_paths.rom_dir, inst_paths.img_dir, 0);

				gtk_widget_destroy(dialog);
				gtk_widget_destroy(dbox);

				wizard_ok = 2;
			break;
            case 2: 
                gtk_widget_destroy(dbox);
                display_wz_tib_dbox();
            break;
            case 3: 
                gtk_widget_destroy(dbox);
                display_wz_rom_dbox();
            break;
			case 4: 
                gtk_widget_destroy(dbox);
                display_msg_dbox();
            break;
            default:
			break;
        }
	}
    else
    {
        gtk_widget_destroy(dbox);
        exit(0);
    }

	return 0;
}

static gint display_step3_dbox(void)
{
    GladeXML *xml;
	GtkWidget *dbox;
	GtkWidget *data;
	gint result;

    xml = glade_xml_new
	    (tilp_paths_build_glade("wizard-2.glade"), "step3_dbox", PACKAGE);
	if (!xml)
		g_error(_("comm.c: GUI loading failed !\n"));
	glade_xml_signal_autoconnect(xml);

	dbox = glade_xml_get_widget(xml, "step3_dbox");
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dbox), GTK_RESPONSE_OK,
	                                        GTK_RESPONSE_APPLY, GTK_RESPONSE_CANCEL,-1);

    //data = glade_xml_get_widget(xml, "cancelbutton2");
    //gtk_button_set_label(data, "<= Back");
    data = glade_xml_get_widget(xml, "applybutton2");
    gtk_widget_hide(data);
    //data = glade_xml_get_widget(xml, "okbutton2");
    //gtk_button_set_label(data, "Next =>");

    result = gtk_dialog_run(GTK_DIALOG(dbox));
    if(result == GTK_RESPONSE_OK)
    {
        gtk_widget_destroy(dbox);
        wizard_ok = 1;
    }
    else
    {
        gtk_widget_destroy(dbox);
        exit(0);
    }

	return 0;
}

gint display_wait_dbox(void)
{
    GladeXML *xml;
	GtkWidget *dbox;
	//GtkWidget *data;
	gint result;

    xml = glade_xml_new
	    (tilp_paths_build_glade("wizard-2.glade"), "wait_dbox", PACKAGE);
	if (!xml)
		g_error(_("comm.c: GUI loading failed !\n"));
	glade_xml_signal_autoconnect(xml);

	dbox = glade_xml_get_widget(xml, "wait_dbox");

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

gint display_wz_rom_dbox(void)
{
    const gchar *filename;
	gchar *dstname;
	int err;

    // get filename
	filename = (char *)create_fsel(inst_paths.rom_dir, NULL, "*.rom", FALSE);
	if (filename == NULL)
	{
		display_step1_dbox();
		return -1;
	}

	if(!ti68k_is_a_rom_file(filename))
    {
        msg_box1(_("Error"), _("Invalid ROM dump."));
        display_step1_dbox();     
        return -1;
    }
  
	err = ti68k_convert_rom_to_image(filename, inst_paths.img_dir, &dstname);
	handle_error();
	if (err)
	{
		display_step1_dbox();
		return -1;
	}
    wizard_rom = g_strdup(dstname);
	g_free(dstname);
    
    display_step3_dbox();

	return 0;
}

gint display_wz_tib_dbox(void)
{
    const gchar *filename;
    gchar *dstname;
	int err;

    // get filename
	filename = (char *)create_fsel(inst_paths.rom_dir, NULL, "*.89u;*.9xu;*.v2u;*.tib", FALSE);
	if (filename == NULL)
	{
		display_step1_dbox();
		return -1;
	}

    if(!ti68k_is_a_tib_file(filename))
    {
        msg_box1(_("Error"), _("Invalid FLASH upgrade."));
        display_step1_dbox();
        return -1;
    }

    err = ti68k_convert_tib_to_image(filename, inst_paths.img_dir, &dstname, -1);
	handle_error();
	if (err)
	{
		display_step1_dbox();
		return -1;
	}
    wizard_rom = g_strdup(dstname);
	g_free(dstname);
    
    display_step3_dbox();  

	return 0;
}


GLADE_CB void
step1_on_radiobutton1_toggled          (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  action = 1;
}

GLADE_CB void
step1_on_radiobutton2_toggled          (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  action = 2;
}

GLADE_CB void
step1_on_radiobutton3_toggled          (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  action = 3;
}

GLADE_CB void
step1_on_radiobutton4_toggled          (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  action = 4;
}
