/* Hey EMACS -*- linux-c -*- */
/* $Id: keyboard.c 2268 2006-11-06 17:18:51Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2007, Romain Liévin
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
#  include <tiemuconfig.h>
#endif

#include <stdio.h>

#include "intl.h"
#include "struct.h"
#include "tie_error.h"
#include "keymap.h"

static FILE *f_rec = NULL;
static FILE *f_ply = NULL;

static GTimer* timer = NULL;

int kp_recording_stop(void)
{
	fclose(f_rec);
	f_rec = NULL;

	g_timer_destroy(timer);
	timer = NULL;

	return 0;
}

int kp_recording_start(const char *filename)
{
	if(f_rec)
		kp_recording_stop();

	f_rec = fopen(filename, "wt");
	if(f_rec == NULL)
	{
		tiemu_err(0, _("Can't open file."));
		return -1;
	}

	timer = g_timer_new();
	g_timer_start(timer);

	return 0;
}

int kp_playing_stop(void)
{
	fclose(f_ply);
	f_ply = NULL;

	return 0;
}

int kp_playing_start(const char *filename)
{
	if(f_ply)
		kp_playing_stop();

	f_ply = fopen(filename, "rt");
	if(f_ply == NULL)
	{
		tiemu_err(0, _("Can't open file."));
		return -1;
	}

	return 0;
}

int kp_recording_key(int key, int action)
{
	if(options.kp_rec_enabled && f_rec)
	{
		fprintf(f_rec, "%f:%s=%i\n", 
			g_timer_elapsed(timer, NULL), 
			keymap_value_to_string(tikeys, key), 
			action);
	}

	return 0;
}

int kp_playing_key(int *key, int *action)
{
	double time;
	char line[256];
	char **split;

	if(f_ply == NULL) return -1;
	if(feof(f_ply)) return -1;

	fgets(line, sizeof(line), f_ply);
	split = g_strsplit_set(line, ":=", 3);

	if(!split[0] || !split[1] || !split[2])
		return -1;

	sscanf(split[0], "%f", &time);
	*key = keymap_string_to_value(tikeys, split[1]);
	*action = (split[2][0] == '1') ? 1 : 0;	

	return 0;
}
