/* Hey EMACS -*- linux-c -*- */
/* $Id: pbars.c 2385 2007-03-12 21:04:20Z roms $ */

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

#include <string.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

#include "intl.h"
#include "support.h"
#include "pbars.h"
#include "struct.h"
#include "tilibs.h"

struct progress_window p_win = { 0 };
static GtkWidget *pbar_window = NULL;
extern CalcUpdate calc_update;

static void reset_counters(void)
{
	calc_update.cnt1 = calc_update.max1 = 0;
	calc_update.cnt2 = calc_update.max2 = 0;
	calc_update.cnt3 = calc_update.max3 = 1;
	calc_update.cancel = 0;
}

/* Create a window with 1 progress bar */
void create_pbar_type1(const gchar *title)
{
	GladeXML *xml;

	reset_counters();

	xml = glade_xml_new
	    (tilp_paths_build_glade("pbars-2.glade"), "pbar1_dbox",
	     PACKAGE);
	if (!xml)
		g_error(_("%s: GUI loading failed!\n"), __FILE__);
	glade_xml_signal_autoconnect(xml);

	pbar_window = glade_xml_get_widget(xml, "pbar1_dbox");
	gtk_window_set_title(GTK_WINDOW(pbar_window), title);

	p_win.pbar1 = glade_xml_get_widget(xml, "progressbar10");
	p_win.label_rate = glade_xml_get_widget(xml, "label11");

	gtk_widget_show_all(pbar_window);
}


/* Create a window with 1 label */
void create_pbar_type2(const gchar *title)
{
	GladeXML *xml;

	reset_counters();

	xml = glade_xml_new
	    (tilp_paths_build_glade("pbars-2.glade"), "pbar2_dbox",
	     PACKAGE);
	if (!xml)
		g_error("GUI loading failed !\n");
	glade_xml_signal_autoconnect(xml);

	pbar_window = glade_xml_get_widget(xml, "pbar2_dbox");
	gtk_window_set_title(GTK_WINDOW(pbar_window), title);

	p_win.label = glade_xml_get_widget(xml, "label20");

	gtk_widget_show_all(pbar_window);
}


/* Create a window with 2 progress bars */
void create_pbar_type3(const gchar *title)
{
	GladeXML *xml;

	reset_counters();

	xml = glade_xml_new
	    (tilp_paths_build_glade("pbars-2.glade"), "pbar3_dbox",
	     PACKAGE);
	if (!xml)
		g_error("GUI loading failed !\n");
	glade_xml_signal_autoconnect(xml);

	pbar_window = glade_xml_get_widget(xml, "pbar3_dbox");
	gtk_window_set_title(GTK_WINDOW(pbar_window), title);

	p_win.pbar2 = glade_xml_get_widget(xml, "progressbar30");
	p_win.pbar1 = glade_xml_get_widget(xml, "progressbar31");
	p_win.label_rate = glade_xml_get_widget(xml, "label32");

	gtk_widget_show_all(pbar_window);
}


/* Create a window with a 1 progress bar and 1 label */
void create_pbar_type4(const gchar *title)
{
	GladeXML *xml;

	reset_counters();

	xml = glade_xml_new
	    (tilp_paths_build_glade("pbars-2.glade"), "pbar4_dbox",
	     PACKAGE);
	if (!xml)
		g_error("GUI loading failed !\n");
	glade_xml_signal_autoconnect(xml);

	pbar_window = glade_xml_get_widget(xml, "pbar4_dbox");
	gtk_window_set_title(GTK_WINDOW(pbar_window), title);

	p_win.label_part = glade_xml_get_widget(xml, "label41");
	p_win.label = glade_xml_get_widget(xml, "label42");
	p_win.pbar1 = glade_xml_get_widget(xml, "progressbar40");
	p_win.label_rate = glade_xml_get_widget(xml, "label43");

	gtk_widget_show_all(pbar_window);
}


/* Create a window with 2 progress bars and 1 label */
void create_pbar_type5(const gchar *title)
{
	GladeXML *xml;

	reset_counters();

	xml = glade_xml_new
	    (tilp_paths_build_glade("pbars-2.glade"), "pbar5_dbox",
	     PACKAGE);
	if (!xml)
		g_error("GUI loading failed !\n");
	glade_xml_signal_autoconnect(xml);

	pbar_window = glade_xml_get_widget(xml, "pbar5_dbox");
	gtk_window_set_title(GTK_WINDOW(pbar_window), title);

	p_win.label_part = glade_xml_get_widget(xml, "label52");
	p_win.label = glade_xml_get_widget(xml, "label53");
	p_win.pbar2 = glade_xml_get_widget(xml, "progressbar50");
	p_win.pbar1 = glade_xml_get_widget(xml, "progressbar51");	
	p_win.label_rate = glade_xml_get_widget(xml, "label54");

	gtk_widget_show_all(pbar_window);
}

/* Create a pbar window */
void create_pbar(int type, const gchar *title)
{
	switch(type)
	{
	case 1: create_pbar_type1(title); break;
	case 2: create_pbar_type2(title); break;
	case 3: create_pbar_type3(title); break;
	case 4: create_pbar_type4(title); break;
	case 5: create_pbar_type5(title); break;
	default: break;
	}
		
}

/* 
   Destroy a pbar window
*/
void destroy_pbar(void)
{
	p_win.pbar1 = NULL;
	p_win.pbar2 = NULL;
	p_win.label = NULL;
	p_win.label_part = NULL;
	p_win.label_rate = NULL;

	if (pbar_window)
		gtk_widget_destroy(pbar_window);
	pbar_window = NULL;
}


GLADE_CB void on_pbar_okbutton1_pressed(GtkButton * button,
					gpointer user_data)
{
	calc_update.cancel = 1;
}

/*
	Get list of counters to refresh
 */
int tilp_pbar_type(int op)
{
	extern CalcHandle*  calc_handle;
	const char **array = calc_handle->calc->counters;
	const char *str = array[op];

	if(!strcmp(str, "1P"))
		return 1;
	else if(!strcmp(str, "1L"))
		return 2;
	else if(!strcmp(str, "2P"))
		return 3;
	else if(!strcmp(str, "1P1L"))
		return 4;
	else if(!strcmp(str, "2P1L"))
		return 5;

	return 0;
}
