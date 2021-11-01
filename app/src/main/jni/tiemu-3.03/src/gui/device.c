/* Hey EMACS -*- linux-c -*- */
/* $Id: device.c 1364 2005-07-13 12:08:10Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *  Copyright (C) 1999-2005  Romain Lievin
 *  Copyright (C) 2007  Kevin Kofler
 *
 *  This program is free software you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <tiemuconfig.h>
#endif

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <string.h>

#include "intl.h"
#include "support.h"
#include "paths.h"
#include "device.h"
#include "refresh.h"
#include "tilibs.h"
#include "dboxes.h"
#include "tie_error.h"
#include "ti68k_int.h"

extern DeviceOptions	linkp;
static DeviceOptions	tmp;
static DeviceOptions	old;

static GtkWidget* lbl;
static GtkWidget* comm_cable;
static GtkWidget* comm_port;

gint display_device_dbox()
{
	GladeXML *xml;
	GtkWidget *dbox;
	GtkWidget *data;
	gint result;
	int err;

	xml = glade_xml_new(tilp_paths_build_glade("device-2.glade"), "device_dbox", PACKAGE);
	if (!xml)
		g_error(_("comm.c: GUI loading failed !\n"));
	glade_xml_signal_autoconnect(xml);

	dbox = glade_xml_get_widget(xml, "device_dbox");
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dbox), GTK_RESPONSE_OK,
	                                        GTK_RESPONSE_CANCEL,-1);

	lbl = glade_xml_get_widget(xml, "label7");

	// Cable  
	data = comm_cable = glade_xml_get_widget(xml, "combobox1");
	switch (linkp.cable_model) 
	{
	case CABLE_NUL:
		gtk_combo_box_set_active(GTK_COMBO_BOX(data), 0);
	break;

	case CABLE_GRY:
		gtk_combo_box_set_active(GTK_COMBO_BOX(data), 1);
	break;
		
	case CABLE_BLK:
		gtk_combo_box_set_active(GTK_COMBO_BOX(data), 2);
	break;

	case CABLE_PAR:
		gtk_combo_box_set_active(GTK_COMBO_BOX(data), 3);
	break;

	case CABLE_SLV:
		gtk_combo_box_set_active(GTK_COMBO_BOX(data), 4);
	break;	

	case CABLE_USB:
		gtk_combo_box_set_active(GTK_COMBO_BOX(data), 5);
	break;	

	case CABLE_VTI:
		gtk_combo_box_set_active(GTK_COMBO_BOX(data), 6);
	break;

    case CABLE_TIE:
		gtk_combo_box_set_active(GTK_COMBO_BOX(data), 7);
	break;
	
	case CABLE_ILP:
		gtk_combo_box_set_active(GTK_COMBO_BOX(data), 8);
	break;

	default:
		gtk_combo_box_set_active(GTK_COMBO_BOX(data), 0);
	break;
	}

	// Port
	data = comm_port = glade_xml_get_widget(xml, "combobox2");
	switch (linkp.cable_port) 
	{
	case PORT_0:
		gtk_combo_box_set_active(GTK_COMBO_BOX(data), 0);
	break;
	case PORT_1:
		gtk_combo_box_set_active(GTK_COMBO_BOX(data), 1);
	break;
	case PORT_2:
		gtk_combo_box_set_active(GTK_COMBO_BOX(data), 2);
	break;
	case PORT_3:
		gtk_combo_box_set_active(GTK_COMBO_BOX(data), 3);
	break;
	case PORT_4:
		gtk_combo_box_set_active(GTK_COMBO_BOX(data), 4);
	break;
	}

	// Timeout
	data = glade_xml_get_widget(xml, "spinbutton_comm_timeout");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(data), linkp.cable_timeout);
	
	// Delay
	data = glade_xml_get_widget(xml, "spinbutton_comm_delay");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(data), linkp.cable_delay);

	// Data exchange
	memcpy(&tmp, &linkp, sizeof(DeviceOptions));
	memcpy(&old, &linkp, sizeof(DeviceOptions));
	
	// Loop
	result = gtk_dialog_run(GTK_DIALOG(dbox));
	switch (result) 
	{
	case GTK_RESPONSE_CANCEL:
		break;
	case GTK_RESPONSE_OK:
		ti68k_linkport_unconfigure();

		data = glade_xml_get_widget(xml, "spinbutton_comm_delay");
		tmp.cable_delay = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(data));

		data = glade_xml_get_widget(xml, "spinbutton_comm_timeout");
		tmp.cable_timeout =	gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(data));

		memcpy(&linkp, &tmp, sizeof(DeviceOptions));
		tmp.calc_model = ti68k_calc_to_libti_calc();

        err = ti68k_linkport_reconfigure();
		if(err)
		{
			// if reconfiguring fails, fallback to null cable & port
			ti68k_linkport_unconfigure();

			memcpy(&linkp, &old, sizeof(DeviceOptions));
			tmp.calc_model = ti68k_calc_to_libti_calc();

			ti68k_linkport_reconfigure();
		}
		break;
	case GTK_RESPONSE_HELP:
		break;
	default:
		break;
	}
	gtk_widget_destroy(dbox);
	return 0;
}


GLADE_CB void
on_device_combobox1_changed            (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
	gint nitem = gtk_combo_box_get_active(combobox);
	
	switch(nitem)
	{
	case 0: tmp.cable_model = CABLE_NUL; break;
	case 1: tmp.cable_model = CABLE_GRY; break;
	case 2: tmp.cable_model = CABLE_BLK; break;
	case 3:	tmp.cable_model = CABLE_PAR; break;
	case 4: tmp.cable_model = CABLE_SLV; break;	
	case 5: tmp.cable_model = CABLE_USB; break;	
	case 6: tmp.cable_model = CABLE_VTI; break;
	case 7: tmp.cable_model = CABLE_TIE; break;
	case 8: tmp.cable_model = CABLE_ILP; break;
	}
}


GLADE_CB void
on_device_combobox2_changed            (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
	gint nitem = gtk_combo_box_get_active(combobox);
	
	switch(nitem)
	{
	case 0: tmp.cable_port = PORT_0; break;
	case 1: tmp.cable_port = PORT_1; break;
	case 2: tmp.cable_port = PORT_2; break;
	case 3: tmp.cable_port = PORT_3; break;
	case 4: tmp.cable_port = PORT_4; break;
	}
}


GLADE_CB void
comm_spinbutton_delay_changed          (GtkEditable     *editable,
                                        gpointer         user_data)
{
  	tmp.cable_delay =
    		gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(user_data));
}


GLADE_CB void
comm_spinbutton_timeout_changed        (GtkEditable     *editable,
                                        gpointer         user_data)
{
  	tmp.cable_timeout =
    		gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(user_data));
}
