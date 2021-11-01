/* Hey EMACS -*- linux-c -*- */
/* $Id: iodefs.c 2372 2007-02-25 21:43:23Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Li�vin
 *  Copyright (c) 2005, Romain Li�vin
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
    IO ports definitions loader/parser.
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "ti68k_int.h"
#include "iodefs.h"
#include <androidlog.h>

GNode*	tree = NULL;
extern int img_changed;

static const char* iodefs_calc2str(int calc_type)
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

static const char* iodefs_get_filename()
{
	static char s[256] = "";

	sprintf(s, "iodefs_%s_hw%i.txt", 
		iodefs_calc2str(tihw.calc_type), tihw.hw_type);

	return s;
}

// get section name [section]
static char* get_section(char *s)
{
	char *b, *e;

	b = strchr(s, '[');
	e = strrchr(s, ']');

	if(!b ||!e)
	{
		fprintf(stdout, "Missing '[' or ']' token in section name!\n");
		return NULL;
	}

	b++;
	*e = '\0';

	return b;
}

static char* get_name(char *s)
{
	char *t;

	while(*s == ' ') s++;

	t = strchr(s, '\t');
	if(t) *t = '\0';

	return s;
}

// convert "ro", "wo", "rw" into value
static int get_type(const char* s)
{
	while(*s == ' ') s++;

	if(s[0] == 'r' && s[1] == 'o')
		return IO_RO;
	else if(s[0] == 'w' && s[1] == 'o')
		return IO_WO;
	else if(s[0] == 'r' && s[1] == 'w')
		return IO_RW;

	return 0;
}

// parse <..5.....> entry and returns number of available bits
static int get_bits(const char *s, IO_DEF* t)
{
	char *b, *e;
	int i, j, nbits = 8 * t->size;
	int all;

	while(*s == ' ') s++;

	b = strchr(s, '<');
	e = strrchr(s, '>');

	if(!b ||!e)
	{
		fprintf(stdout, "Missing '<' or '>' token !\n");
		return -1;
	}

	b++;
	e--;
	all = !strncmp(b, "all", 3);

	if((e - b + 1) != nbits && !all)
	{
		fprintf(stdout, "Number of bits does not match size !\n");
		return -1;
	}

	memset(t->bits, 0, nbits);
	for(i = 0, j = 0; i < nbits; i++)
	{
		if(b[i] == '.')
		{
		}
		else if(isdigit(b[i]) || all)
		{
			t->bits[j++] = nbits - i - 1;
		}
		else
		{
			fprintf(stdout, "Wrong character in bit sequence (digit or '.') !\n");
			return -1;
		}
	}

	t->nbits = j;

	return 0;
}

static int get_bit_names(char *s, IO_DEF *t)
{
	int i;
	gchar **split;

	if(s == NULL)
		return 0;

	while(*s == ' ') s++;

	split = g_strsplit(s, ",", t->nbits);
	if(split[0] == NULL)
		return 0;

	for(i = 0; i < t->nbits; i++)
	{
		if(split[i] != NULL)
			t->bit_name[i] = strdup(split[i]);
	}
	
	return 0;
}

// get bit string <76543210>
static char* get_bit_str(char *s)
{
	char *b, *e;

	b = strchr(s, '<');
	e = strrchr(s, '>');

	if(!b ||!e)
	{
		fprintf(stdout, "Missing '<' or '>' token in bit mask !\n");
		return NULL;
	}

	*++e = '\0';

	return b;
}

/*
	Unload information on I/O ports (free resources).
*/
static gboolean free_node(GNode *node, gpointer data)
{
	if (node)
		if(node->data)
			free(node->data);

	return FALSE;
}

int iodefs_unload(void)
{
  if(tree != NULL) 
  {
		g_node_traverse(tree, G_IN_ORDER, G_TRAVERSE_ALL, -1, free_node, NULL);
		g_node_destroy(tree);
		tree = NULL;
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

	File naming scheme : "iodefs_model.txt" => iodefs_89.txt
*/
int iodefs_load(const char* path)
{
	FILE *f;
	gchar *filename;
	int n;
	char line[1024];
	
	GNode* parent = NULL;
	GNode* node;

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

	if(tree)
		iodefs_unload();
	
	filename = g_strconcat(path, iodefs_get_filename(), NULL);
	f = fopen(filename, "rb");
	if(f == NULL)
	{
		g_free(filename);
		return -1;
	}

	tree = g_node_new(NULL);

	for(n = 0; !feof(f);)
	{
		gchar **split;
		IO_DEF *s;

		if (!fgets(line, sizeof(line), f))
			break;
		/* An empty string means end of file. Don't clobber other stack
		   variables with the underflow. */
		if (!*line)
			break;
		line[strlen(line) - 2] = '\0';

		if(feof(f))
			break;

		if(line[0] == ';')
			continue;
		else if(line[0] == '[')
		{
			char *name = get_section(line);
			if(name == NULL) return -1;

			s = (IO_DEF*)calloc(1, sizeof(IO_DEF));
			s->name = strdup(name);

			parent = g_node_new(s);
			g_node_append(tree, parent);

			continue;
		}
		else if(line[0] == '#')
		{
		}
		else if(line[0] != '$')
			continue;
		

		split = g_strsplit(line, "|", 6);
		if(!split[0] || !split[1] || !split[2] || !split[3] || !split[4] )
		{
			fprintf(stderr, "Error at line %i: malformed line !\n", n);
			return -1;
		}

		s = (IO_DEF*)calloc(1, sizeof(IO_DEF));

		sscanf(split[0], "$%06x", &s->addr);
		sscanf(split[1], "%i", &s->size);
		s->type = get_type(split[2]);
		s->bit_str = strdup(get_bit_str(split[3]));
		if(get_bits(split[3], s) == -1)
			return -1;
		s->all_bits = (s->nbits == (8 * s->size));
		s->name = strdup(get_name(split[4]));

		if(get_bit_names(split[5], s) == -1)
			return -1;

		if(parent == NULL)
		{
			fprintf(stderr, "Error at line %i: no section defined !\n", n);
			return -1;
		}

		node = g_node_new(s);
		g_node_append(parent, node);

		n++;
	}

	g_free(filename);
	fclose(f);

	LOGI("loading I/O defs: %s (%i entries)", iodefs_get_filename(), n);
    return 0;
}

GNode* iodefs_tree(void)
{
	return tree;
}
