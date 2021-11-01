/* Hey EMACS -*- linux-c -*- */
/* $Id: keymap.c 2610 2007-07-19 21:35:03Z kevinkofler $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Liévin
 *  Copyright (c) 2005, Romain Liévin
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
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include "keydefs.h"
#include "pckeys.h"
#include "ti68k_int.h"
#include "keymap.h"
#include "logging.h"

#ifdef _MSC_VER
Pc2TiKey keymap[KEYMAP_MAX] = {0};
#else
Pc2TiKey keymap[KEYMAP_MAX] = {};
#endif

// search for key name and return key value (or -1 if not found)
int keymap_string_to_value(const KeyTuple *array, const char *key_name)
{
    KeyTuple *p;

    for(p = (KeyTuple *)array; p->value != -1; p++)
        if(!strcmp(p->name, key_name))
            return p->value;

    return -1;
}

// search for key value and return key name
const char* keymap_value_to_string(const KeyTuple *array, int key_value)
{
    KeyTuple *p;

    for(p = (KeyTuple *)array; p->value != -1; p++)
        if(key_value == p->value)
            return p->name;

    return "";
}

// return calc type or -1
int keymap_read_header(const char *filename)
{
    FILE *f;
    char line[256] = { 0 };

    // open file
    f = fopen(filename, "rt");
    if(f == NULL)
    {
        fprintf(stderr, "unable to open keymap file <%s>.\n", filename);
        return -1;
    }

    // read first line
    if (!fgets(line, sizeof(line), f))
    {
        fprintf(stderr, "No calc model found !\n");
        fclose(f);
        return -1;
    }
    line[strlen(line) - 1] = '\0';
    if(strncmp(line, "Model", strlen("Model")))
    {
        fprintf(stderr, "No calc model found !\n");
        fclose(f);
        return -1;
    }

    // close file
    fclose(f);

    // and convert
    return ti68k_string_to_calctype(line + 7);
}

int keymap_load(const char *filename)
{
    FILE *f;
    char line[256];
    int pckey, tikey, mod;
    int idx = 0;

    // open file
    f = fopen(filename, "rt");
    if(f == NULL)
    {
        fprintf(stderr, "unable to open keymap file <%s>.\n", filename);
        return -1;
    }

    if (!fgets(line, sizeof(line), f))
    {
        fprintf(stderr, "No calc model found !\n");
        fclose(f);
        return -1;
    }
    line[strlen(line) - 1] = '\0';
    if(strncmp(line, "Model", strlen("Model")))
    {
        fprintf(stderr, "No calc model found !\n");
        fclose(f);
        return -1;
    }

    // parse file
    while(!feof(f))
    {
        gchar **split = { 0 };
        gchar **split2 = { 0 };
	char *p;

        // remove cr/lf
        if (!fgets(line, sizeof(line), f))
            break;
        //line[strlen(line) - 2] = '\0';
	p = strrchr(line, '\r');
	if(p != NULL) *p = '\0';
	p = strrchr(line, '\n');
	if(p != NULL) *p = '\0';

        // skip comments
        if((line[0] == '#') || (line[0] == '/'))
            continue;

        // split string
        split = g_strsplit(line, ":", 2);
        if((split[0] == NULL) || (split[1] == NULL))
        {
            g_strfreev(split);
            continue;
        }

        // split sub-string
        split2 = g_strsplit(split[1], ",", 2);
        if(split2[0] == NULL)
        {
            g_strfreev(split2);
            continue;
        }

        // convert key names into values
        pckey = keymap_string_to_value(pckeys, split[0]);
        if(pckey == -1)
        {
            fprintf(stderr, "Warning: non-existent PC key: <%s>\n", split[0]);
            g_strfreev(split);
            continue;
        }

        tikey = keymap_string_to_value(tikeys, split2[0]);
        if(tikey == -1)
        {
            fprintf(stderr, "Warning: non-existent TI key: <%s>\n", split[1]);
            g_strfreev(split);
            continue;
        }

        if(split2[1] == NULL)
            mod = -1;
        else
            mod = keymap_string_to_value(tikeys, split2[1]);            

        // and store value
        keymap[idx].pc_key = pckey;
        keymap[idx].ti_key = tikey;
        keymap[idx].modifier = mod;
        idx++;

        // free mem
        g_strfreev(split);
        g_strfreev(split2);
    }

    // close file
    tiemu_info(_("loading keymap: %s (%i keys)"), g_basename(filename), idx);
    fclose(f);

    return 0;
}
