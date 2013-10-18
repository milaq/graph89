/* Hey EMACS -*- linux-c -*- */
/* $Id: error.c 2378 2007-02-26 18:07:09Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Liévin
 *  Copyright (c) 2005-2006, Romain Liévin
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
    Transcoding of error codes into message strings
*/

#include <stdio.h>
#include <string.h>
#include <glib.h>

#include "intl.h"
#include "ti68k_err.h"

/* 
   This function put in err_msg the error message corresponding to the 
   error code.
   If the error code has been handled, the function returns 0 else it 
   propagates the error code by returning it.
*/
int ti68k_error_get(int err_num, char **error_msg)
{
	switch(err_num)
    {
    case ERR_NONE:
		*error_msg = g_strdup(_("No error."));
		break;

	case ERR_CANT_OPEN:
		*error_msg = g_strdup(_("Can not open file."));
		break;

	case ERR_CANT_OPEN_STATE:
		*error_msg = g_strdup(_("Can not open state image: file is corrupted or missing."));
		break;

	case ERR_REVISION_MATCH:
		*error_msg = g_strdup(_("Can not open state image: revision changed. You have to recreate the state image."));
		break;

	case ERR_HEADER_MATCH:
		*error_msg = g_strdup(_("Can not open state image: state image header does not match ROM image header: have you changed/updated your ROM image?"));
		break;

	case ERR_STATE_MATCH:
		*error_msg = g_strdup(_("Can not open state image: this state image is not targetted for your current emulator image (calculator model and/or OS version must match!). Choose another image before."));
		break;

	case ERR_INVALID_IMAGE:
		*error_msg = g_strdup(_("Invalid emulator image. File is corrupted or revision changed."));
		break;

	case ERR_INVALID_UPGRADE:
		*error_msg = g_strdup(_("Invalid FLASH upgrade."));
		break;

	case ERR_INVALID_ROM:
		*error_msg = g_strdup(_("Invalid ROM dump."));
		break;

	case ERR_NO_IMAGE:
		*error_msg = g_strdup(_("No image."));
		break;

	case ERR_INVALID_ROM_SIZE:
		*error_msg = g_strdup(_("ROM dump has a weird size."));
		break;

	case ERR_NOT_TI_FILE:
		*error_msg = g_strdup(_("This is not recognized as a TI file."));
		break;

	case ERR_CANT_OPEN_DIR:
		*error_msg = g_strdup(_("Can't parse folder."));
		break;

	case ERR_CANT_UPGRADE:
		*error_msg = g_strdup(_("Can't upgrade calculator."));
		break;		

    default:
      *error_msg = g_strdup(_("Error code not found in the list.\nThis is a bug. Please report it."));
      return err_num;
    }
  
  return 0;
}

