/* Hey EMACS -*- linux-c -*- */
/* $Id: iodefs.c 2372 2007-02-25 21:43:23Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2007, Romain Li�vin
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
 *  Foundation, Inc., 51 Franklin Sarrayt - Fifth Floor, Boston, MA 02110-1301, USA.
 */

/*
    Memory Maps loader/parser.
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "ti68k_int.h"
#include "mem_map.h"
#include <androidlog.h>

MEM_MAP** array = NULL;
extern int img_changed;

static const char* memmap_calc2str(int calc_type)
{
	switch(calc_type)
	{
		case TI89:  return "ti89";
		case TI92:  return "ti92";
		case TI92p: return "ti92p";
		case V200:  return "v200";
        case TI89t: return "ti89t";
		default:    return "none";
	}
	return "";
}

static const char* memmap_get_filename()
{
	static char s[256] = "";

	sprintf(s, "memmap_%s_hw%i.txt", 
		memmap_calc2str(tihw.calc_type), tihw.hw_type);

	return s;
}

int memmap_unload(void)
{
  if(array != NULL) 
  {
	  MEM_MAP **ptr;
	  for(ptr = array; *ptr; ptr++)
		  g_free(*ptr);
	  g_free(array);
	  array = NULL;
  }

  return 0;
}

/*
	Load information on I/O ports.
	Return value:
	 0 if successful
	-1 if error
	-2 if no image
	-4 if already loaded

	File naming scheme : "memmap_model.txt" => memmap_89.txt
*/
int memmap_load(const char* path)
{
	FILE *f;
	gchar *filename;
	char line[1024];
	int n;

	static int calc_type = 0;
	static int hw_type = 0;

	if(!img_loaded) 
		return -2;
	if(calc_type != tihw.calc_type || hw_type != tihw.hw_type)
	{
		calc_type = tihw.calc_type;
		hw_type = tihw.hw_type;
	}
	else
		return -4;

	if(array)
		memmap_unload();
	
	filename = g_strconcat(path, memmap_get_filename(), NULL);
	f = fopen(filename, "rb");
	if(f == NULL)
	{
		g_free(filename);
		return -1;
	}

	for(n = 0; !feof(f);)
	{
		gchar **split;
		MEM_MAP *s;
		guint32 tmp;

		if (!fgets(line, sizeof(line), f))
			break;
		if (!*line)
			break;
		line[strlen(line) - 2] = '\0';

		if(line[0] == ';')
			continue;

		if(strlen(line) < 2)
			continue;

		if(feof(f))
			break;

		split = g_strsplit_set(line, "-:", 3);
		if(!split[0] || !split[1] || !split[2])
		{
			fprintf(stderr, "Error at line %i: malformed line !\n", n);
			return -1;
		}

		array = g_realloc(array, (n+2) * sizeof(MEM_MAP *));
		s = g_new0(MEM_MAP, 1);
		
		sscanf(split[0], "%06x", &s->addr);
		sscanf(split[1], "%06x", &tmp);
		s->size = tmp - s->addr + 1;
		s->name = strdup(split[2]);

		g_strfreev(split);
		array[n++] = s;
		array[n] = NULL;
	}

	g_free(filename);
	fclose(f);

	LOGI("loading memory map: %s", memmap_get_filename());
    return 0;
}

MEM_MAP** memmap_array(void)
{
	return array;
}
