/* Hey EMACS -*- linux-c -*- */
/* $Id: paths.c 2268 2006-11-06 17:18:51Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Liévin
 *  Copyright (c) 2005, Romain Liévin, Kevin Kofler
 *  Copyright (c) 2005, Christian Walther (patches for Mac OS-X port)
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

/*
	Platform independant paths
*/

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef __WIN32__
#include <windows.h>
#include <direct.h> // _mkdir
#endif
#include <unistd.h>

#include "struct.h"
#include "paths.h"


TiemuInstPaths inst_paths;      // installation paths


/*
	Called by TiEmu at startup for initializing platform dependant paths.
*/
#if defined(__LINUX__) || defined(__BSD__) || defined(__MACOSX__)
static void init_linux_paths(void)
{
	gchar *tmp;

	// set base dir
	inst_paths.base_dir =
	    g_strconcat(SHARE_DIR, G_DIR_SEPARATOR_S, NULL);

	// set others
	inst_paths.pixmap_dir =
	    g_strconcat(inst_paths.base_dir, "pixmaps/", NULL);
	inst_paths.help_dir =
	    g_strconcat(inst_paths.base_dir, "help/", NULL);
	inst_paths.manpage_dir = 
        g_strconcat(inst_paths.base_dir, "", NULL);
	inst_paths.skin_dir =
	    g_strconcat(inst_paths.base_dir, "skins/", NULL);
	inst_paths.glade_dir =
	    g_strconcat(inst_paths.base_dir, "glade/", NULL);
	inst_paths.home_dir =
		g_strconcat(g_get_home_dir(), "/", NULL);
	inst_paths.img_dir = 
        g_strconcat(inst_paths.home_dir, CONF_DIR, "images/", NULL);
	inst_paths.rom_dir =
		g_strconcat(inst_paths.base_dir, "pedrom/", NULL);
	inst_paths.misc_dir =
		g_strconcat(inst_paths.base_dir, "misc/", NULL);
	inst_paths.screen_dir =
		g_strconcat(inst_paths.home_dir, CONF_DIR, "screenshots/", NULL);

	// create conf folder (~/.tiemu/)
	tmp = g_strconcat(inst_paths.home_dir, CONF_DIR, NULL);
	mkdir(tmp, 0777);
	g_free(tmp);

	// create image repository
	mkdir(inst_paths.img_dir, 0777);

	// create screen repository
	mkdir(inst_paths.screen_dir, 0777);

	/* bintextdomain(PACKAGE, "/usr/share/locale"); ->
	   '/usr/share/locale/  fr/LC_MESSAGES/tilp.mo' */
#ifdef ENABLE_NLS
	inst_paths.locale_dir = g_strconcat(LOCALEDIR, "/", NULL);
#endif				/*  */

#if 0 /* This breaks Insight's path detection when running TiEmu from a relative
         path. */
	// on LINUX systems, HOME directory by default for security reasons
	chdir(inst_paths.home_dir);
#endif /* 0 */
}
#endif				/*  */

#define MINGW_REL	"share\\tiemu"

/*
	Same for Win32
*/
#if defined(__WIN32__)
static void init_win32_paths(void)
{
	HMODULE hModule;
	DWORD dWord;
	char *sBuffer;
	gchar *dirname;
	gchar *basename;

	// Init the path for the Windows version by getting the 
	// executable location.
	hModule = GetModuleHandle("tiemu.exe");
	sBuffer = (char *) malloc(4096 * sizeof(char));
	dWord = GetModuleFileName(hModule, sBuffer, 4096);
	dirname = g_path_get_dirname(sBuffer);
	basename = g_path_get_basename(dirname);

	// modify exec path like '/target/bin' into '/target/share/tiemu' if we 
	// are running an Msys path (MinGW). Otherwise, do nothing if we are running from a
	// local path (MSVC & InnoSetup)
	if((strlen(basename) == 3) && !g_strcasecmp(basename, "bin"))
	{
		gchar *token;

		dirname = g_realloc(dirname, strlen(dirname) + strlen(MINGW_REL) + 1);
		token = dirname + strlen(dirname) - 3;
		strcpy(token, MINGW_REL);
	}	

	// set base dir	
	inst_paths.base_dir = g_strconcat(dirname, "\\", NULL);
	free(sBuffer);  // malloc -> free
	g_free(dirname);
	g_free(basename);

	// set others
	inst_paths.pixmap_dir =
	    g_strconcat(inst_paths.base_dir, "pixmaps\\", NULL);
	inst_paths.help_dir =
	    g_strconcat(inst_paths.base_dir, "help\\", NULL);
	inst_paths.manpage_dir =
	    g_strconcat(inst_paths.base_dir, "", NULL);
	inst_paths.skin_dir =
	    g_strconcat(inst_paths.base_dir, "skins\\", NULL);
	inst_paths.glade_dir =
	    g_strconcat(inst_paths.base_dir, "glade\\", NULL);
	inst_paths.home_dir = 
	    g_strconcat(inst_paths.base_dir, "", NULL);
	inst_paths.img_dir = 
        g_strconcat(inst_paths.base_dir, "images\\", NULL);
	inst_paths.rom_dir =
		g_strconcat(inst_paths.base_dir, "pedrom\\", NULL);
	inst_paths.misc_dir =
		g_strconcat(inst_paths.base_dir, "misc\\", NULL);
	inst_paths.screen_dir =
		g_strconcat(inst_paths.base_dir, "screenshots\\", NULL);

	// create image repository
	_mkdir(inst_paths.img_dir);

#ifdef ENABLE_NLS
	inst_paths.locale_dir =
	    g_strconcat(inst_paths.base_dir, "locale\\", NULL);
#endif				/*  */
#if 0 /* This breaks Insight's path detection when running TiEmu from a relative
         path, at least it does on *nix. */
	// on WIN32 systems, local by default
	_chdir(inst_paths.home_dir);
#endif /* 0 */
}
#endif				/*  */
int initialize_paths(void)
{
#ifdef __WIN32__
	init_win32_paths();
#else
	init_linux_paths();
#endif
	return 0;
}

const char *tilp_paths_build_glade(const char *name)
{
	static char *path = NULL;
	
	g_free(path);
	path = g_strconcat(inst_paths.glade_dir, name, NULL);
	
	return path;
}
