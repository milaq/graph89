/* Hey EMACS -*- linux-c -*- */
/* $Id: fs_misc.c 2840 2009-05-08 20:43:47Z kevinkofler $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Liévin
 *  Copyright (c) 2005-2009, Romain Liévin, Kevin Kofler
 *  Copyright (c) 2005, Christian Walther (patches for Mac OS-X port)
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

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "intl.h"
#include "filesel.h"
#include "skinops.h"
#include "refresh.h"
#include "paths.h"
#include "struct.h"
#include "ti68k_int.h"
#include "dboxes.h"
#include "calc.h"
#include "rcfile.h"
#include "pbars.h"
#include "tie_error.h"
#include "dbg_all.h"
#include "files.h"
#include "ti68k_err.h"
//"fs_misc.h"

gint display_skin_dbox(void)
{
	const gchar *filename;

	filename = (char *)create_fsel(inst_paths.skin_dir, NULL, "*.skn", FALSE);
	if (!filename)
	{
		return 0;
	}

    // Load new skin
    g_free(options.skin_file);
    options.skin_file = g_strdup(filename);
    
    hid_change_skin(options.skin_file);

	return 0;
}

int fs_load_state(const char *filename)
{
	int err;

	g_free(params.sav_file);
    params.sav_file = g_strdup(filename);
    
    err = ti68k_state_load(params.sav_file);
	if(err == ERR_STATE_MATCH)
	{
		gchar *rf, *tf;
		int ret = msg_box2(_("Warning"), 
			_("The state image you are attempting to load does not match the current running image. Press OK if you want TiEmu to automatically load the corresponding image or Cancel to abort."));
		
		if(ret == BUTTON2)	//cancel
			return 0;

		ti68k_state_parse(filename, &rf, &tf);
		
		if(!ti68k_is_a_img_file(rf))
			return 0;

        // Set tib file and image
        g_free(params.tib_file);
		params.tib_file = tf;

		g_free(params.rom_file);
		params.rom_file = rf;

		// Restart engine by exiting the GTK loop
		while(gtk_events_pending()) gtk_main_iteration();
		gtk_main_quit();	
	}
	else
		handle_error();

	return 0;
}

gint display_load_state_dbox(void)
{
	const gchar *filename;

    // get filename
	filename = create_fsel(inst_paths.img_dir, NULL, "*.sav", FALSE);
	if (!filename)
		return 0;

    fs_load_state(filename);

	return 0;
}

gint display_save_state_dbox(void)
{
    const gchar *filename;
	int err;
	gchar *basename;
	gchar *dot;
	gchar *pattern;

    // get filename
	basename = g_path_get_basename(params.rom_file);
	dot = strrchr(basename, '.');
	if(dot != NULL)
		*dot = '\0';
	pattern = g_strconcat(basename, ".sav", NULL);
	g_free(basename);

	filename = create_fsel(inst_paths.img_dir, pattern, "*.sav", TRUE);
	g_free(pattern);
	if (!filename)
		return 0;

    g_free(params.sav_file);
    params.sav_file = g_strdup(filename);
    err = ti68k_state_save(params.sav_file);
	handle_error();
    
    if(!rcfile_exist())
    {
        rcfile_write();
   
#ifdef __WIN32__
        msg_box1(_("Information"), 
	      _("You do not seem to have saved your settings. Configuration file saved (in tiemu.ini)."));
#else
		msg_box1(_("Information"), 
	      _("You do not seem to have saved your settings. Configuration file saved (in ~/.tiemu)."));
#endif
    }

	return 0;
}

void fs_send_file(const gchar *filename)
{
	int err;

	// set pbar title
	if(tifiles_file_is_flash(filename)) 
	{
		create_pbar_(FNCT_SEND_APP, _("Sending app(s)"));
	} 
	else if(tifiles_file_is_group(filename)) 
	{
		create_pbar_(FNCT_SEND_VAR, _("Sending var(s)"));
	} 
	else if(tifiles_file_is_backup(filename)) 
	{
		create_pbar_(FNCT_SEND_BACKUP, _("Restoring"));
	} 
	else if(tifiles_file_is_single(filename)) 
	{
		create_pbar_(FNCT_SEND_VAR, _("Sending var(s)"));
	}
	else if(tifiles_file_is_tigroup(filename))
	{
		create_pbar_type5(_("Restoring"));
	}

	// note that core is currently not bkpt-interruptible when
	// transferring file
	GTK_REFRESH();
	err = ti68k_linkport_send_file(filename);
	handle_error();
	destroy_pbar();	
}

int fs_send_files(gchar **filenames)
{
	gchar **ptr;
	int i, l;

	// check extension and send
	for(ptr = filenames, l = 0; *ptr; ptr++, l++);
	for(ptr = filenames, i = 0; *ptr; ptr++, i++)
	{
		if(!tifiles_file_is_ti(*ptr) || (!tifiles_calc_is_ti9x(tifiles_file_get_model(*ptr)) &&
			!tifiles_file_is_tigroup(*ptr))) 
		{
			msg_box1(_("Error"), _("This file is not a valid TI file."));
			g_strfreev(filenames);
			return -1;
		}

		fs_send_file(*ptr);
	}

	return 0;
}

gint display_send_files_dbox(void)
{
	const gchar *ext;
	gchar **filenames;
	static gchar *folder = NULL;
	int ret = 0;
	
	// Check for null cable
	if(linkp.cable_model != CABLE_ILP)
	{
		int ret, err;
		gchar *str;

		str = g_strdup_printf(_("The current link cable <%s> port <%s> does not allow direct file loading. Do you let me change link port settings to allow direct file loading?"),
			ticables_model_to_string(linkp.cable_model), ticables_port_to_string(linkp.cable_port));
		
		ret= msg_box2(_("Warning"), str);
		g_free(str);
		
		if(ret == BUTTON2)
			return -1;

		// reconfigure link port
		ti68k_linkport_unconfigure();

		linkp.cable_model = CABLE_ILP;
		linkp.cable_port = PORT_0;

        err = ti68k_linkport_reconfigure();
		handle_error();
	}

    // set mask
    switch(tihw.calc_type) 
	{
    case TI92:
        ext = "*.92?";
		break;
	default:
        ext = "*.89?;*.92?;*.9x?;*.9X?;*.v2?;*.V2?;*.tig";
        break;
    }

	// get filename
	if(folder == NULL)
		folder = g_strdup(inst_paths.base_dir);

	filenames = create_fsels(folder, NULL, (char *)ext);
	if(!filenames)
		return 0;

	// keep folder
	g_free(folder);
	folder = g_path_get_dirname(filenames[0]);

	ret = fs_send_files(filenames);
	g_strfreev(filenames);

	return ret;
}

int display_recv_files_dbox(const char *src, const char *dst)
{
	const gchar *fn;
	gchar *src_folder;
	gchar *dst_folder;
	gchar *basename;
	gchar *ext;

	// get file components
	src_folder = g_path_get_dirname(src);
	dst_folder = inst_paths.home_dir;
	basename = g_path_get_basename(dst);

	 // set mask
    switch(tihw.calc_type) 
	{
    case TI92:
        ext = "*.92?";
		break;
	default:
        ext = "*.89?;*.92?;*.9x?;*.9X?;*.v2?;*.V2?";
        break;
    }

	fn = create_fsel(dst_folder, basename, ext, TRUE);
	if (fn)
		tiemu_file_move_with_check(src, fn);
	else
		tiemu_file_delete(src);

	g_free(src_folder);
	g_free(basename);

	return 0;
}

#ifndef NO_GDB
void fs_send_file_and_debug_info(const gchar *filename)
{
    const gchar *ext;
    FileContent *metadata;

    fs_send_file(filename);

    ext = strrchr(filename, '.');
    if (ext)
    {
        gchar *temp;
        *(char *)ext = 0;
        temp = g_strconcat(filename, ".dbg", NULL);
#ifdef WIN32
        symfile = g_locale_from_utf8(temp,-1,NULL,NULL,NULL);
        g_free(temp);
#else
        symfile = temp;
#endif
        *(char *)ext = '.';
    }

    metadata = tifiles_content_create_regular(CALC_TI89);
    if (!tifiles_file_read_regular(filename, metadata))
    {
        if (metadata->num_entries > 0)
        {
            int handle = sym_find_handle (metadata->entries[0]->folder, metadata->entries[0]->name);
            if (handle)
                ti68k_bkpt_add_pgmentry (handle);
        }
    }
    tifiles_content_delete_regular(metadata);
}

gint display_debug_dbox(void)
{
	const gchar *filename;
	const gchar *ext;
	static gchar *folder = NULL;

    // set mask
    switch(tihw.calc_type) 
	{
    case TI92:
        ext = "*.92?";
		break;
	default:
        ext = "*.89?;*.92?;*.9x?;*.v2?";
        break;
    }

	// get filename
	if(folder == NULL)
		folder = g_strdup(inst_paths.base_dir);

	filename = (char *)create_fsel(folder, NULL, (char *)ext, FALSE);
	if (!filename)
    {
		return 0;
    }

	// keep folder
	g_free(folder);
	folder = g_path_get_dirname(filename);

    // check extension
    if(!tifiles_file_is_ti(filename) || 
        !tifiles_calc_is_ti9x(tifiles_file_get_model(filename))) 
	{
        msg_box1(_("Error"), _("This file is not a valid TI file."));
        return -1;
    }

    fs_send_file_and_debug_info(filename);

	return 0;
}
#else
void fs_send_file_and_debug_info(const gchar *filename) 
{
    fs_send_file(filename);
}
#endif

extern int dbgromcall_refresh_window(void);

gint display_set_tib_dbox(void)
{
    const gchar *filename;
	gchar *path, *name;
	int err;

    // get filename
	filename = create_fsel(inst_paths.base_dir, NULL, "*.89u;*.9xu;*.v2u;*.tib", FALSE);
	if (!filename)
		return 0;

	if(!ti68k_is_a_tib_file(filename))
	{
		msg_box1(_("Error"), _("Does not seem to be an upgrade."));
		return -1;
	}

	path = g_path_get_dirname(filename);
	name = g_path_get_basename(filename);

	// set tib file
	g_free(params.tib_file);
	params.tib_file = g_strconcat(path, G_DIR_SEPARATOR_S, name, NULL);
	g_free(path); g_free(name);

	err = ti68k_load_upgrade(params.tib_file);
	handle_error();
	if(err)
	{
		msg_box1(_("Error"), _("Cannot load the upgrade."));
		return -1;
	}

	// update ROM calls list
	dbgromcall_refresh_window();

    // simply reset, don't restart
    ti68k_reset();

    return 0;
}

int import_romversion(const char *filename)
{
	char *dstname;
	int err;

	if(ti68k_is_a_rom_file(filename))
	{
		err = ti68k_convert_rom_to_image(filename, inst_paths.img_dir, &dstname);
		handle_error();
		g_free(dstname);
	}
	else if(ti68k_is_a_tib_file(filename))
	{
		#ifdef _MSC_VER
		IMG_INFO infos = {0};
		#else
		IMG_INFO infos = {};
		#endif
		int err = ti68k_get_tib_infos(filename, &infos, 0);
		int hw_type = HW2;

		if(infos.calc_type == TI92p || infos.calc_type == TI89)
		{
			int ret = msg_box3(_("HW type"), 
				_("The FLASH upgrade can be imported as HW1 or HW2. Please choose..."), 
				"HW1", "HW2", "Default");
			if(ret == BUTTON1)
				hw_type = HW1;
			else if(ret == BUTTON2)
				hw_type = HW2;
		}
		else if(infos.calc_type == TI89t)
		{
			int ret = msg_box3(_("HW type"), 
				_("The FLASH upgrade can be imported as HW3 or HW4. Please choose..."), 
				"HW3", "HW4", "Default");

			hw_type = HW3; // default is HW3 for the Titanium, there's no Titanium HW2

			if(ret == BUTTON1)
				hw_type = HW3;
			else if(ret == BUTTON2)
				hw_type = HW4;
		}

		// fake rom
		err = ti68k_convert_tib_to_image(filename, inst_paths.img_dir, &dstname, hw_type);
		handle_error();
		g_free(dstname);
	}
	else
	{
		msg_box1(_("Error"), _("This is not a valid file"));
		return -1;
	}

	return 0;
}

gint display_import_romversion_dbox(void)
{
    const gchar *filename;
    
    // get filename
	filename = create_fsel(inst_paths.base_dir, NULL, "*.rom;*.89u;*.9xu;*.v2u;*.tib", FALSE);
	if (!filename)
		return 0;    

    return import_romversion(filename);;
}


