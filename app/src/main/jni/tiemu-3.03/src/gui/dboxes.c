/* Hey EMACS -*- linux-c -*- */
/* $Id: dboxes.c 2753 2007-12-30 23:14:15Z kevinkofler $ */

/*  TiEmu - Tiemu Is an EMUlator
 *  Copyright (C) 1999-2004  Romain Lievin
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
 *  Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <tiemuconfig.h>
#endif				/*  */

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <string.h>

#include "intl.h"
#include "dboxes.h"
#include "support.h"
#include "paths.h"

static gint ret_val = 0;

static gint str2msg(const char *title)
{
	gint msg_type = GTK_MESSAGE_INFO;

	if (!strcmp(title, _("Information")))
		msg_type = GTK_MESSAGE_INFO;
	else if (!strcmp(title, _("Warning")))
		msg_type = GTK_MESSAGE_WARNING;
	else if (!strcmp(title, _("Question")))
		msg_type = GTK_MESSAGE_QUESTION;
	else if (!strcmp(title, _("Error")))
		msg_type = GTK_MESSAGE_ERROR;
	else
		msg_type = GTK_MESSAGE_INFO;

	return msg_type;
}

int msg_box1(const char *title, const char *message)
{
	GtkWidget *dialog, *label;
	gint result;
	gint msg_type;

	msg_type = str2msg(title);
	if (msg_type != -1) 
	{
		dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
					   msg_type, GTK_BUTTONS_CLOSE,
					   message);
		gtk_dialog_run(GTK_DIALOG(dialog));
	} 
	else 
	{
		dialog = gtk_dialog_new_with_buttons(title, GTK_WINDOW(NULL),
						(GtkDialogFlags)
						(GTK_DIALOG_MODAL),
						GTK_STOCK_OK,
						GTK_RESPONSE_OK, NULL);
		gtk_dialog_set_default_response(GTK_DIALOG(dialog),
						GTK_RESPONSE_OK);
		label = gtk_label_new(message);
		gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), label);
		gtk_widget_show(label);

		result = gtk_dialog_run(GTK_DIALOG(dialog));
		switch (result) 
		{
		case GTK_RESPONSE_OK:
			break;
		default:
			break;
		}
	}
	gtk_widget_destroy(dialog);

	return 0;
}

gint msg_box2(const char *title, const char *message)
{
	GtkWidget *dialog;
	gint result;
	gint msg_type;

	msg_type = str2msg(title);
	dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, msg_type,
				   GTK_BUTTONS_OK_CANCEL, message);
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK,
	                                        GTK_RESPONSE_CANCEL,-1);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog),
					GTK_RESPONSE_CANCEL);

	result = gtk_dialog_run(GTK_DIALOG(dialog));
	switch (result) 
	{
	case GTK_RESPONSE_OK:
		gtk_widget_destroy(dialog);
		return BUTTON1;
		break;
	default:
		gtk_widget_destroy(dialog);
		return BUTTON2;
		break;
	}

	return 0;
}

gint msg_box3(const char *title, const char *message, const char *button1,
	      const char *button2, const char *button3)
{
	GtkWidget *dialog;
	gint result;
	gint msg_type;

	msg_type = str2msg(title);
	dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, msg_type,
				   GTK_BUTTONS_NONE, message);
	gtk_dialog_add_button(GTK_DIALOG(dialog), button1, GTK_RESPONSE_YES);
	gtk_dialog_add_button(GTK_DIALOG(dialog), button2, GTK_RESPONSE_NO);
	gtk_dialog_add_button(GTK_DIALOG(dialog), button3, GTK_RESPONSE_CANCEL);
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_YES,
	                                        GTK_RESPONSE_NO, GTK_RESPONSE_CANCEL, -1);

	gtk_dialog_set_default_response(GTK_DIALOG(dialog),
					GTK_RESPONSE_CANCEL);

	result = gtk_dialog_run(GTK_DIALOG(dialog));
	switch (result) 
	{
	case GTK_RESPONSE_YES:
		gtk_widget_destroy(dialog);
		return BUTTON1;
		break;
	case GTK_RESPONSE_NO:
		gtk_widget_destroy(dialog);
		return BUTTON2;
		break;
	default:
		gtk_widget_destroy(dialog);
		return BUTTON3;
		break;
	}
	
	return 0;
}

gint msg_box4(const char *title, const char *message)
{
	GtkWidget *dialog;
	gint result;
	gint msg_type;

	msg_type = str2msg(title);
	dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, msg_type,
				   GTK_BUTTONS_NONE, message);
	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_GO_FORWARD, GTK_RESPONSE_OK);
	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK,
	                                        GTK_RESPONSE_CANCEL,-1);

	gtk_dialog_set_default_response(GTK_DIALOG(dialog),
					GTK_RESPONSE_CANCEL);

	result = gtk_dialog_run(GTK_DIALOG(dialog));
	switch (result) 
	{
	case GTK_RESPONSE_OK:
		gtk_widget_destroy(dialog);
		return BUTTON1;
		break;
	default:
		gtk_widget_destroy(dialog);
		return BUTTON2;
		break;
	}

	return 0;
}


/* Create the dialog box entry and wait */
char *msg_entry(const char *title, const char *message, const char *content)
{
	GladeXML *xml;
	GtkWidget *data;
	GtkWidget *dbox;
	GtkWidget *entry = NULL;
	gchar *ret = NULL;
	gint result;
	ret_val = 0;

	xml = glade_xml_new
	    (tilp_paths_build_glade("user_boxes-2.glade"), "entry_dbox",
	     PACKAGE);
	if (!xml)
		g_error("dboxes.c: GUI loading failed !\n");
	glade_xml_signal_autoconnect(xml);

	dbox = data = glade_xml_get_widget(xml, "entry_dbox");
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dbox), GTK_RESPONSE_OK,
	                                        GTK_RESPONSE_CANCEL,-1);
	gtk_window_set_title(GTK_WINDOW(data), title);

	data = glade_xml_get_widget(xml, "frame1");
	gtk_frame_set_label(GTK_FRAME(data), message);

	data = glade_xml_get_widget(xml, "entry1");
	entry = GTK_WIDGET(data);
	gtk_entry_set_text(GTK_ENTRY(data), content);

	gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1);

	result = gtk_dialog_run(GTK_DIALOG(dbox));
	switch (result) 
	{
	case GTK_RESPONSE_OK:
		ret = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
		break;
	default:
		break;
	}
	gtk_widget_destroy(dbox);

	return ret;
}
