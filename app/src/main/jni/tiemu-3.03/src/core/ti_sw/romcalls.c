/* Hey EMACS -*- linux-c -*- */
/* $Id: romcalls.c 2268 2006-11-06 17:18:51Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Liévin
 *  Copyright (c) 2005, Romain Liévin
 *  Copyright (c) 2006, Kevin Kofler
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
    Symbols (ROM calls address and names)
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "romcalls.h"
#include "images.h"
#include "ti68k_def.h"
#include "timem.h"

extern int img_changed;		// set if image modified
static int loaded = 0;		// loaded

static ROM_CALL table[NMAX_ROMCALLS];	// list by id
static GList	*list = NULL;			// sorted list (by id, addr or name)


/* =========== */

/*
	Retrieve base address of ROM calls table and size.
*/
void romcalls_get_table_infos(uint32_t *base, uint32_t *size)
{
	*base = *size = 0;

	if(tihw.calc_type == TI92)
		return;

	*base = rd_long(&tihw.rom[0x12000 + 0x88 + 0xC8]);
	*size = rd_long(&tihw.rom[((*base-4) & 0x0fffff)]);
}

/*
	Given a ROM call ID, retrieve address of ROM call.
*/
void romcalls_get_symbol_address(int id, uint32_t *addr)
{
	uint32_t base;

	base = rd_long(&tihw.rom[0x12000 + 0x88 + 0xC8]);
	*addr = rd_long(&tihw.rom[(base & 0x0fffff) + 4*id]); 
}

/* =========== */

/* Load a list of ROM calls symbols (addr & id), TIGCC formatted. */
static void load_tigcc_file_type(FILE *f)    // os.h TIGCC file: ".set acos, 0xF5"
{
    char buf[256];
    char *name, *p;
    int n;
    int number;

    while(!feof(f))
    {
        if (!fgets(buf, sizeof(buf), f))
            break;

        // no '.set' ?
        if(*buf != '.')
            continue;

        // get function name
        name = strdup(buf+5);
        p = strchr(name, ',');
        if(p == NULL)
			continue;
        else
			*p++ = '\0';

        // get function number
        n = sscanf(p, "%x", &number);
        if((n < 1) || (number > 0x7ff))
        {
            free(name);
            continue;
        }

		// and store
		table[number].name = name;
		table[number].id = number;
    }
}

/* Load a list of ROM calls symbols (addr & id), tthdex formatted. */
static void load_lionel_file_type(FILE *f)   // Lionel Debroux formatted file: 2E:ScrToHome
{
    char str[256];
    int number;

    while(!feof(f))
    {
        gchar **array;

        // get line
        if (!fgets(str, sizeof(str), f) || feof(f))
             break;
		for (number = strlen(str) - 1; str[number] == '\n' || str[number] == '\r'; number--) str[number] = '\0';

        if(!strchr(str, ':'))
            continue;

        // split
        array = g_strsplit(str, ":", 2);
        if(!array[0] || !array[1])
        {
            g_strfreev(array);
            continue;
        }

        // get values and store
        sscanf(array[0], "%x", &number);
		table[number].name = strdup(array[1]);
		table[number].id = number;

        g_strfreev(array);
    }
}

/*
	Load ROM calls (id & addr) from file. Don't touch addr field.
*/
static int load_from_file(const char *filename)
{
	FILE *f;
	char tmp[32];

	printf("Loading ROM calls from file <%s>... ", filename);
    memset(table, 0, sizeof(table));

    f = fopen(filename, "rt");
    if(f == NULL
       || !fgets(tmp, sizeof(tmp), f)
       || !fgets(tmp, sizeof(tmp), f))
	{
		printf("Failed to open <%s> with error %s (%d)\n", 
			       filename, strerror(errno), errno);
        return -1;
	}

	rewind(f);

    if(!strncmp(tmp, ".set", strlen(".set")))
        load_tigcc_file_type(f);
	else
		load_lionel_file_type(f);

    fclose(f);
	printf("Done !\n");

	return 0;
}

/*
	Fill the addr field from ROM calls located in FLASH. Don't touch other fields !
	And construct list from ROM call table.
*/
static int merge_from_flash(void)
{
	uint32_t addr;
	int size;
	int i;

	if(list != NULL)
	{
		g_list_free(list);
		list = NULL;
	}

	romcalls_get_table_infos(&addr, (uint32_t *)&size);
	if(size == 0)
		return -1;

	printf("Parsing ROM calls from flash memory (%i entries at $%06x)... ", size, addr);

	for(i = 0; i < size; i++)
	{
		if(table[i].name == NULL)
			table[i].name = strdup("unknown");

		table[i].addr = rd_long(&tihw.rom[(addr & 0x0fffff) + (i << 2)]); 

		list = g_list_append (list, &table[i]);
	}
	
	//list = g_list_reverse(list);
	printf("Done !\n");

	return 0;
}

/*
	Load ROM calls from file and FLASH and merge.
	Return value:
	 0 if successful
	-1 if error
	-2 if no image
	-3 if TI92
	-4 if already loaded
*/
int romcalls_load(const char* filename)
{
	IMG_INFO *img = &img_infos;

	// check whether parsing is possible
	if(!img_loaded) return -2;
	if(img->calc_type == TI92) return -3;

	// check for reload
	if(!img_changed)
		return -4;
	else
		img_changed = 0;

	if(load_from_file(filename))
		return -1;

	if(merge_from_flash())
		return -1;

	loaded = !0;

    return 0;
}

int romcalls_is_loaded(void)
{
	return loaded;
}

/* =========== */

// negative value if a < b; zero if a = b; positive value if a > b
static gint compare_func_by_id(gconstpointer a, gconstpointer b)
{
	ROM_CALL *aa = (ROM_CALL *)a;
	ROM_CALL *bb = (ROM_CALL *)b;

	if(aa->id == bb->id)
		return 0;
	else if(aa->id > bb->id)
		return 1;
	else return -1;
}

GList* romcalls_sort_by_id(void)
{
	return g_list_sort(list, compare_func_by_id);
}

// negative value if a < b; zero if a = b; positive value if a > b
static gint compare_func_by_addr(gconstpointer a, gconstpointer b)
{
	ROM_CALL *aa = (ROM_CALL *)a;
	ROM_CALL *bb = (ROM_CALL *)b;

	if(aa->addr == bb->addr)
		return 0;
	else if(aa->addr > bb->addr)
		return 1;
	else return -1;
}

GList* romcalls_sort_by_addr(void)
{
	return list = g_list_sort(list, compare_func_by_addr);
}

// negative value if a < b; zero if a = b; positive value if a > b
static gint compare_func_by_name(gconstpointer a, gconstpointer b)
{
	ROM_CALL *aa = (ROM_CALL *)a;
	ROM_CALL *bb = (ROM_CALL *)b;

	return strcmp(aa->name, bb->name);
}

GList* romcalls_sort_by_name(void)
{
	return list = g_list_sort(list, compare_func_by_name);
}

// negative value if a < b; zero if a = b; positive value if a > b
static gint compare_func_by_iname(gconstpointer a, gconstpointer b)
{
	ROM_CALL *aa = (ROM_CALL *)a;
	ROM_CALL *bb = (ROM_CALL *)b;

	return g_ascii_strcasecmp(aa->name, bb->name);
}

GList* romcalls_sort_by_iname(void)
{
	return list = g_list_sort(list, compare_func_by_iname);
}

/* =========== */

// cache last search (disasm)
static int last_id = 0;	

// returns id or -1
int romcalls_is_addr(uint32_t addr)
{
	int i;

	if(!loaded)	return -1;

	for(i = 0; i < (int)g_list_length(list); i++)
	{
		if(addr == table[i].addr)
			return last_id = i;
	}

	return -1;
}

// returns id or -1
int romcalls_is_name(const char *name)
{
	int i;

	if(!loaded)	return -1;

	for(i = 0; i < (int)g_list_length(list); i++)
	{
		if(!strcmp(name, table[i].name))
			return i;
	}

	return -1;
}

const char* romcalls_get_name(int id)
{
	if(!loaded)	return "not loaded";
	return table[id].name;
}

uint32_t romcalls_get_addr(int id)
{
	return table[id].addr;
}
