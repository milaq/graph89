/* Hey EMACS -*- linux-c -*- */
/* $Id: files.c 2385 2007-03-12 21:04:20Z roms $ */

/*  TiLP - Ti Linking Program
 *  Copyright (C) 1999-2005  Romain Lievin
 *  Copyright (c) 2005, Romain Liévin (tweaked for TiEmu)
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
	This file contains utility functions about files management.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#ifdef __WIN32__
#include <windows.h>
#endif

#include <glib.h>
#include <glib/gstdio.h>

#ifdef _MSC_VER
# include "../../build/msvc/dirent.h"	// for S_ISDIR use
#endif

#include "dboxes.h"
#define _(x)	(x)

/* File operations */

#ifndef __WIN32__
int tiemu_file_copy(const char *src, const char *dst)
{
	FILE *in, *out;
	int c;

	if ((in = fopen(src, "rb")) == NULL) 
	{
		return -1;
	}

	if ((out = fopen(dst, "wb")) == NULL) 
	{
		return -2;
	}

	while (!feof(in)) 
	{
		c = fgetc(in);
		if (feof(in))
			break;
		fputc(c, out);
	}

	fclose(in);
	fclose(out);

	return 0;
}

#else				

int tiemu_file_copy(const char *src, const char *dst)
{
	if (!CopyFile(src, dst, FALSE))
		return -1;

	return 0;
}
#endif				

int tiemu_file_delete(const char *f)
{
	if(g_unlink(f) < 0)
	{
		msg_box1(_("Information"), _("Unable to remove the file!"));
		return -1;
	}

	return 0;
}

int tiemu_file_move(const char *src, const char *dst)
{
	if(tiemu_file_copy(src, dst) < 0)
	//if(g_rename(src, dst) < 0)
	{
		msg_box1(_("Information"), _("Unable to move file.\n\n"));
		return -1;
	}
	tiemu_file_delete(src);

	return 0;
}

int tiemu_file_mkdir(const char *pathname)
{
#ifdef __WIN32__
	if(g_mkdir(pathname, S_IRWXU) < 0)
#else
	if(g_mkdir(pathname, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) < 0)
#endif
	{
		msg_box1(_("Information"), _("Unable to create the directory.\n\n"));
		return -1;
	}

	return 0;
}

int tiemu_file_exist(const char* filename)
{
	return !access(filename, F_OK);
}

/* 
   Check for file existence. If file already exists, ask for an
   action (skip, overwrite or rename).
   Return 0 if skipped. 
*/
int tiemu_file_check(const char *src, char **dst)
{
	int ret;
	char buffer[256];
	char *dirname;
	*dst = NULL;

	if (1) 
	{
		if (access(src, F_OK) == 0) 
		{
			sprintf(buffer, _("The file %s already exists.\nOverwrite?"), src);
			ret =
			    msg_box3(_("Warning"), buffer,
					_("Overwrite "), _("Rename "), _("Skip "));

			switch (ret) 
			{
			case BUTTON1:
				*dst = g_strdup(src);
				return !0;
				break;
			case BUTTON2:
				dirname =
				    msg_entry(_("Rename the file"),
						   _("New name: "), src);
					if (dirname == NULL)
						return 0;
					*dst = g_strdup(dirname);
					g_free(dirname);
				return !0;
				break;
			case BUTTON3:
				return 0;
				break;
			default:
				return 0;
				break;
			}
		} 
		else 
		{
			*dst = g_strdup(src);
			return !0;
		}
	} 
	else 
	{
		*dst = g_strdup(src);
		return !0;
	}

	return !0;
}


/*
  Try and move a file. If file already exists, ask for an action
  (skip, overwrite or rename)
  Return 0 if skipped. 
*/
int tiemu_file_move_with_check(const char *src, const char *dst)
{
	char *dst2;

	if (tiemu_file_check(dst, &dst2)) 
	{
		if (tiemu_file_move(src, dst2)) 
		{
			msg_box1(_("Error"), _("Unable to move the temporary file.\n"));
			g_free(dst2);
			return 0;
		}
	} 
	else 
	{
		g_free(dst2);
		return 0;
	}
	g_free(dst2);

	return !0;
}
