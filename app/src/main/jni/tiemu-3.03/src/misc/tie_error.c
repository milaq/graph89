/* Hey EMACS -*- linux-c -*- */
/* $Id: tie_error.c 2372 2007-02-25 21:43:23Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Li�vin
 *  Copyright (c) 2005, Romain Li�vin
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


#include <stdio.h>
#include <string.h>

#include "tilibs.h"

#include "intl.h"
#include "logging.h"
#include "ti68k_int.h"
#include <androidlog.h>
/*
  This function can take 2 parameters:
  - an error to translate or 0
  - a pure message or NULL
 */
int tiemu_err(int err_code, char *err_str)
{
	int err = err_code;
	char *s = NULL;

	if(!err_code && !err_str)
		return 0;

	if(err_code) 
	{
		/* Retrieve the error message */
		err = ticables_error_get(err, &s);
		if (err) 
		{
			//free(s);
			err = tifiles_error_get(err, &s);
			if (err) 
			{
				//free(s);
				err = ticalcs_error_get(err, &s);
				if (err) 
				{
					// next level: error for TiEmu
					//free(s);
					err = ti68k_error_get(err, &s);
				}
			}
		}
	} 
	else if(err_str) 
	{
		s = strdup(err_str);
	}
	
	LOGW("%s", s);

	return err_code;
}
