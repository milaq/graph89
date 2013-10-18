/* Hey EMACS -*- linux-c -*- */
/* $Id: about.c 2444 2007-04-15 08:29:34Z roms $ */

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
#endif				/* HAVE_CONFIG_H */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

#include "intl.h"
#include "about.h"
#include "paths.h"
#include "version.h"
#include "tilibs.h"
#include "engine.h"
#include "support.h"

static const char* authors[] =  
{
	"Romain Lievin (Linux/Win32) <roms@tilp.info>", 
	"Kevin Kofler  (gdb & Linux/Win32) <kevin@tigcc.ticalc.org>", 
	"Christian Walther (Mac OS-X) <cwalther@gmx.ch>",
	NULL 
};

static const char* documenters[] = 
{
	"Romain Lievin (Linux/Win32) <roms@tilp.info>", 
	NULL
};

static const char* artists[] = 
{
	"Jaime Fernando Meza Meza (icon & skin design)",
	"hibou/hiboo (logo)",
	NULL
};

gint display_about_dbox(void)
{
	GtkWidget* widget;
	GtkAboutDialog* dlg;
	GdkPixbuf *pix;

	struct stat stbuf;
	FILE *fd;
	gchar *filename;
	int len = 0;
	gchar buffer[32768];
	gchar *version;

#ifdef _MSC_VER /* MSVC builds. MinGW builds use Linux file structures. */
	filename = g_strconcat(inst_paths.base_dir, "License.txt", NULL);
#else				/*  */
	filename = g_strconcat(inst_paths.base_dir, "COPYING", NULL);
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

	version = g_strdup_printf(_("Framework version (cables=%s, files=%s, calcs=%s, conv=%s)"),
	     ticables_version_get(), tifiles_version_get(), ticalcs_version_get(), ticonv_version_get());

	//---

	widget = gtk_about_dialog_new();
	dlg = GTK_ABOUT_DIALOG(widget);
	pix = create_pixbuf("logo.xpm");

	gtk_about_dialog_set_name(dlg, "TiEmu - Ti Emulator - ");
	gtk_about_dialog_set_version(dlg, TIEMU_VERSION);
	gtk_about_dialog_set_comments(dlg, version);
	gtk_about_dialog_set_copyright(dlg, "Copyright (c) 1999-2007 The TiEmu Team");
	gtk_about_dialog_set_license(dlg, buffer);
	gtk_about_dialog_set_website(dlg, "http://lpg.ticalc.org/prj_tiemu");
	gtk_about_dialog_set_authors(dlg, authors);
	gtk_about_dialog_set_documenters(dlg, documenters);
	gtk_about_dialog_set_artists(dlg, artists);
	gtk_about_dialog_set_logo(dlg, pix);

	g_signal_connect_swapped(dlg, "response",
		G_CALLBACK(gtk_widget_destroy), dlg);

	//gtk_show_about_dialog(NULL, "");
	gtk_widget_show_all(widget);

	return 0;
}


