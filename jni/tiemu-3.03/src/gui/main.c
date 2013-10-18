/* Hey EMACS -*- linux-c -*- */
/* $Id: main.c 2630 2007-08-23 14:13:14Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Liévin
 *  Copyright (c) 2005-2007, Romain Liévin, Kevin Kofler
 *  Copyright (c) 2007, Peter Fernandes
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
#include <tiemuconfig.h>
#endif

#include <glib.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <locale.h>
#ifdef __WIN32__
#undef setjmp
extern int asm_setjmp(jmp_buf b);
#define setjmp asm_setjmp
#include "oleaut_c.h"
#endif

#if WITH_KDE
#include "kde.h"
#endif

#if WITH_DBUS
#include "dbus_ipc.h"
#endif

#include "intl.h"
#include "tilibs.h"
#include "struct.h"
#include "calc.h"
#include "version.h"
#include "cmdline.h"
#include "files.h"
#include "rcfile.h"
#include "ti68k_int.h"
#include "logging.h"

#include "engine.h"
#include "refresh.h"
#include "calc.h"

#include "wizard.h"
#include "popup.h"
#include "splash.h"
#include "tie_error.h"
#include "dboxes.h"
#include "dbg_all.h"
#include "romversion.h"
#include "fs_misc.h"

#ifndef NO_SOUND
#include "audio.h"
#endif

#ifndef NO_GDB
#include "gdbcall.h"
#include "../gdb/gdb/main.h"
#include "../gdb/gdb/gdb_string.h"
#ifndef PARAMS
#define PARAMS(x) x
#endif
#define READLINE_LIBRARY
#include "../gdb/readline/readline.h"
void gdbtk_hide_insight(void);
void gdbtk_delete_interp(void);

static void start_insight_timer(void);
static void stop_insight_timer(void);
static gint gdbtk_hide_insight_and_run_wrapper(gpointer data);
#endif

ScrOptions options2;
TieOptions options;		// general tiemu options
jmp_buf quit_gdb;       // longjmp target used when quitting GDB

/* Special */

static gint exit_loop = 0;

void exit_main_loop(void)
{
	exit_loop = !0;
}

static void my_log_handler                  (const gchar *log_domain,
                                             GLogLevelFlags log_level,
                                             const gchar *message,
                                             gpointer user_data) {}

/* Main function */

extern char *file_to_send;

int main(int argc, char **argv) 
{
	int err;
    

	/*
		Do primary initializations 
	*/

	/* Display program version */
	tiemu_version();

	/* Initialize platform independant paths */
	initialize_paths();

	/* Init i18n support */
#ifdef ENABLE_NLS
	tiemu_info("setlocale: <%s>", setlocale(LC_ALL, ""));
  	tiemu_info("bindtextdomain: <%s>", bindtextdomain(PACKAGE, inst_paths.locale_dir));
  	bind_textdomain_codeset(PACKAGE, "UTF-8"/*"ISO-8859-15"*/);
  	tiemu_info("textdomain: <%s>", textdomain(PACKAGE));
#endif

	/* Initialize/reload config */
	rcfile_default();   // (step 2)
	rcfile_read();

	/* Scan and modify command line */
	scan_cmdline(argc, argv);

#ifndef NO_SOUND
	//init audio lib
	if(audio_init()) {
		tiemu_warning(_("Unable to initialize audio, sound will not play\n"));
		audioerr=1;
	}
#endif

    /* 
		Init GTK+ toolkit
	*/
	gtk_init(&argc, &argv);
    add_pixmap_directory(inst_paths.pixmap_dir);

	/*
		Get rid of glib, gdk, gtk warnings when compiled in Release mode
	*/
#if !defined(_DEBUG)
	g_log_set_handler ("GLib", 
		G_LOG_LEVEL_WARNING | G_LOG_LEVEL_MESSAGE | G_LOG_LEVEL_INFO | G_LOG_LEVEL_DEBUG,
		my_log_handler, NULL);
	g_log_set_handler ("Gdk", 
		G_LOG_LEVEL_WARNING | G_LOG_LEVEL_MESSAGE | G_LOG_LEVEL_INFO | G_LOG_LEVEL_DEBUG,
		my_log_handler, NULL);
	g_log_set_handler ("Gtk", 
		G_LOG_LEVEL_WARNING | G_LOG_LEVEL_MESSAGE | G_LOG_LEVEL_INFO | G_LOG_LEVEL_DEBUG,
		my_log_handler, NULL);

	g_log_set_handler ("GLib", G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION, 
		my_log_handler, NULL);
#endif

    /*
        Set splash screen
    */
#ifndef __IPAQ__
    splash_screen_start();
    splash_screen_set_label(_("Initializing GTK+..."));
#endif

#if WITH_KDE
    splash_screen_set_label(_("Initializing KDE..."));
    sp_kde_init(argc, argv, "tiemu", _("TiEmu"), VERSION, _("TI calculator emulator"), "Copyright (c) 2000, Thomas Corvazier, Romain Lievin\nCopyright (c) 2001-2002, Romain Lievin, Julien Blache\nCopyright (c) 2003-2004, Romain Lievin\nCopyright (c) 2005-2007, Romain Lievin, Kevin Kofler\nCopyright (c) 2007, Peter Fernandes", "http://lpg.ticalc.org/prj_tiemu/", "gtktiemu-users@lists.sf.net");
    atexit(sp_kde_finish);
    g_timeout_add(26, sp_kde_process_qt_events, NULL);
#endif

#if WITH_DBUS
    splash_screen_set_label(_("Initializing D-Bus..."));
    dbus_init();
    atexit(dbus_finish);
#endif

#ifdef __WIN32__
    splash_screen_set_label(_("Initializing OLE..."));
    oleaut_init();
    atexit(oleaut_finish);
#endif

	/* 
	   Check the version of libraries 
	 */
    splash_screen_set_label(_("Initializing TiLP framework..."));

	if (strcmp(ticonv_version_get(), TIEMU_REQUIRES_LIBCONV_VERSION) < 0) 
	{
		tiemu_warning(_("libticonv library version <%s> mini required (<%s> found)."),
			TIEMU_REQUIRES_LIBCONV_VERSION, ticonv_version_get());
		msg_box1(_("Error"), _("libticonv: version mismatches."));
		exit(-1);
	}

	if (strcmp(tifiles_version_get(), TIEMU_REQUIRES_LIBFILES_VERSION) < 0) 
	{
		tiemu_warning(_("libtifiles library version <%s> mini required (<%s> found)."),
			TIEMU_REQUIRES_LIBFILES_VERSION, tifiles_version_get());
		msg_box1(_("Error"), _("libtifiles: version mismatches."));
		exit(-1);
	}
	
	if (strcmp(ticables_version_get(), TIEMU_REQUIRES_LIBCABLES_VERSION) < 0) 
	{
		tiemu_warning(_("libticables library version <%s> mini required (<%s> found)."),
			TIEMU_REQUIRES_LIBCABLES_VERSION, ticables_version_get());
		msg_box1(_("Error"), _("libticables: version mismatches."));
		exit(-1);
	}
	
	if (strcmp(ticalcs_version_get(), TIEMU_REQUIRES_LIBCALCS_VERSION) < 0) 
	{
		tiemu_warning(_("libticalcs library version <%s> mini required (<%s> found)."),
			TIEMU_REQUIRES_LIBCALCS_VERSION, ticalcs_version_get());
		msg_box1(_("Error"), _("libticalcs: version mismatches."));
		exit(-1);
	}

    /*
        Search for dumps or upgrades to convert (in the image directory)
    */
    splash_screen_set_label(_("Searching for ROM dumps..."));
    err = ti68k_scan_files(inst_paths.img_dir, inst_paths.img_dir, !0);
	handle_error();

	/*
		Attempt to load an image (step 3)
	*/
	while(!exit_loop)
	{

		/* Windows follows the locale settings even for basic stdio I/O functions.
		   This is an annoyance for floating-point numbers in GDB, so we override
		   it here. Unfortunately, this disease seems to have spread to glibc as
		   well recently. */
		setlocale(LC_NUMERIC, "C");

		err = ti68k_load_image(params.rom_file);
		if(err) 
		{
	rescan:
			if(ti68k_find_image(inst_paths.img_dir, NULL))
			{
				// search for an available image to use
				if(display_romversion_dbox(TRUE) == -1)
					goto wizard;
			}
			else
			{
				// launch wizard
	wizard:
				display_wizard_dbox();
				while(!wizard_ok)
					GTK_REFRESH();

				if(wizard_ok == 2)
					goto rescan;
			
				g_free(params.rom_file);
				params.rom_file = g_strdup(wizard_rom);
				g_free(wizard_rom);
			}

			splash_screen_set_label(_("Loading image..."));
			err = ti68k_load_image(params.rom_file);
			handle_error();
		}

		/* 
			Initialize emulation engine (step 4)
		*/
		splash_screen_set_label(_("Initializing m68k emulation engine..."));
		err = ti68k_init();
		handle_error();
		if(err)	return -1;

		err = hid_init();
		handle_error();
		if(err)	return -1;

		/*
			Load FLASH upgrade (if any)
		*/
		if(params.tib_file != NULL) 
		{
			err = ti68k_load_upgrade(params.tib_file);
			handle_error();
		}

		/* 
			Override refresh functions of the ticalcs2 library 
			(must be done after init of ti68k engine)
		*/
		tiemu_update_set_gtk();

		/* 
			Reset emulation engine (step 5)
		*/
		ti68k_reset();

		/* 
			Load calculator state image 
		*/
		splash_screen_set_label(_("Loading previously saved state..."));
		if(params.sav_file != NULL) 
		{
			err = ti68k_state_load(params.sav_file);
			handle_error();
		}

		/*
			Cache debugger windows to speed-up display and install custom event
		*/
#ifndef __IPAQ__
		splash_screen_set_label(_("Pre-loading debugger..."));
		gtk_debugger_preload();
#endif  

		/*
			Check for a file to send
		*/
		if(file_to_send)
		{
			if(!img_loaded || !ti68k_linkport_ready())
				break;

			fs_send_file(file_to_send);
			g_free(file_to_send);
		}

		/* 
			Start emulation engine and run main loop 
		*/		
		splash_screen_stop();
#ifdef NO_GDB
		engine_start();
		gtk_main();
		engine_stop();
#else

		/*
			Run Insight GDB
		*/
		start_insight_timer();
		if (setjmp(quit_gdb) == 0)
		{
			struct captured_main_args args;
			memset (&args, 0, sizeof args);
			args.argc = argc;
			args.argv = argv;
			args.use_windows = 1;
			args.interpreter_p = "insight";
			g_idle_add(gdbtk_hide_insight_and_run_wrapper, NULL);
			gdb_main (&args);
		}
		stop_insight_timer();

		/*
			Clean up in case we interrupted GDB during command-line
			parsing
		*/
		rl_callback_handler_remove();
#endif

		err = hid_exit();
		handle_error();

		err = ti68k_exit();
		handle_error();


		ti68k_unload_image_or_upgrade();
	}

#ifndef NO_GDB
	gdbtk_delete_interp();
#endif

	return 0;
}

/*
   These functions are used by Insight to enable/disable its UI hook.
*/
#ifndef NO_GDB
int x_event(int);
static guint gdbtk_timer_id = 0;

static gint tiemu_x_event_wrapper(gpointer data)
{
  x_event(0);
  return TRUE;
}

static void start_insight_timer(void)
{
  if (!gdbtk_timer_id)
    gdbtk_timer_id = g_timeout_add(25, tiemu_x_event_wrapper, NULL); /* 25 ms */
}

static void stop_insight_timer(void)
{
  if (gdbtk_timer_id)
    {
      g_source_remove(gdbtk_timer_id);
      gdbtk_timer_id = 0;
    }
}

static gint gdbtk_hide_insight_and_run_wrapper(gpointer data)
{
  gdbtk_hide_insight();
  gdbcall_run();
  return FALSE;
}
#endif

/* 
   If GtkTiEmu is compiled in console mode (_CONSOLE), 
   then we use the 'main' entry point.
   If GtkTiEmu is compiled as a windowed application (_WINDOWS), 
   then we use the 'WinMain' entry point.
*/
#if defined(__WIN32__) && defined(_WINDOWS)// && !defined(_CONSOLE)

#ifdef __MINGW32__
#include <windows.h>
#endif

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	return main(__argc, __argv);
}
#endif
