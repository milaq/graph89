/* Hey EMACS -*- linux-c -*- */
/* $Id: release.c 2649 2007-09-17 15:35:08Z roms $ */

/*  TiEmu - a TI emulator
 *  Copyright (C) 1999-2005  Romain Lievin
 *  Copyright (C) 2005 Kevin Kofler
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
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "intl.h"
#include "paths.h"
#include "engine.h"

gint display_release_dbox()
{
	GladeXML *xml;
	GtkWidget *dbox;
	GtkTextBuffer *txtbuf;
	GtkWidget *text;
	FILE *fd;
	gchar *filename;
	gchar buffer[65536];
	gint len = 0;
	struct stat stbuf;
	gint result;
	PangoFontDescription *font_desc;

#ifdef _MSC_VER /* MSVC builds. MinGW builds use Linux file structures. */
	filename = g_strconcat(inst_paths.base_dir, "Release.txt", NULL);

#else				/*  */
	filename = g_strconcat(inst_paths.base_dir, "RELEASE", NULL);
#endif				/*  */

	if (access(filename, F_OK) == 0) 
	{
		if (stat(filename, &stbuf) != -1) 
		{
			len = stbuf.st_size;
		}
		if ((fd = fopen(filename, "r")) != NULL) 
		{
			memset(buffer, 0, sizeof(buffer));
			len = fread(buffer, 1, len, fd);
			fclose(fd);
		}
	}

	xml = glade_xml_new(tilp_paths_build_glade("release-2.glade"), "release_dbox",
	     PACKAGE);
	if (!xml)
		g_error(_("%s: GUI loading failed!\n"), __FILE__);
	glade_xml_signal_autoconnect(xml);

	dbox = glade_xml_get_widget(xml, "release_dbox");
	text = glade_xml_get_widget(xml, "textview1");

	// Change font
	font_desc = pango_font_description_from_string ("Courier");
	gtk_widget_modify_font (text, font_desc);
	pango_font_description_free (font_desc);

	// Set text
	txtbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));
	gtk_text_buffer_set_text(txtbuf, buffer, len);

	result = gtk_dialog_run(GTK_DIALOG(dbox));
	switch (result) 
	{
	case GTK_RESPONSE_OK:
		break;
	default:
		break;
	}

	gtk_widget_destroy(dbox);
	return 0;
}
