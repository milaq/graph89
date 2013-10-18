/* Hey EMACS -*- linux-c -*- */
/* $Id: vat.c 2268 2006-11-06 17:18:51Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Liévin
 *  Copyright (c) 2005-2006, Romain Liévin, Kevin Kofler
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
    Variable Allocation Table access routines
*/

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "ti68k_def.h"
#include "ti68k_int.h"

#include "handles.h"
#include "vat.h"

typedef struct 
{
	char name[8]; 
	uint16_t compat; 
	union { 
		uint16_t flags_n; 
		struct { 
			uint16_t busy : 1, local : 1, flag1_5 : 1, flag1_4 : 1, collapsed : 1, twin : 1, archived : 1, in_view : 1; 
			uint16_t folder : 1, overwritten : 1, checked : 1, hidden : 1, locked : 1, statvar : 1, graph_ref_1 : 1, graph_ref_0 : 1; 
		} bits; 
	} flags; 
	uint16_t handle; 
} TI89_SYM_ENTRY;

typedef struct 
{ 
	uint16_t folder; 
	uint16_t offset; 
} TI89_HSym;

/*
	"a" To "z", "A" To "Z", Chr(128), Chr(129), Chr(130), Chr(131), Chr(132), Chr(133), Chr(134), 
	Chr(135), Chr(136), Chr(137), Chr(138), Chr(139), Chr(141), Chr(142), Chr(143), Chr(144), 
	Chr(145), Chr(146), Chr(147), Chr(148), "À" To "Ö", "Ø" To "ö", "ø" To "ÿ", "_", Chr(154), 
	Chr(155), Chr(178), "\"
	and "0" To "9" (apart from first position)
*/
static int valid_chars[256] = {
	0, 0, 0, 0, 0, 0, 0, 0,		0, 0, 0, 0, 0, 0, 0, 0,	// 0x00
	0, 0, 0, 0, 0, 0, 0, 0,		0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,		0, 0, 0, 0, 0, 0, 0, 0,	// 0x20
	1, 1, 1, 1, 1, 1, 1, 1,		1, 1, 0, 0, 0, 0, 0, 0,
	0, 1, 1, 1, 1, 1, 1, 1,		1, 1, 1, 1, 1, 1, 1, 1,	// 0x40
	1, 1, 1, 1, 1, 1, 1, 1,		1, 1, 1, 0, 1, 0, 0, 1,
	0, 1, 1, 1, 1, 1, 1, 1,		1, 1, 1, 1, 1, 1, 1, 1,	// 0x60
	1, 1, 1, 1, 1, 1, 1, 1,		1, 1, 1, 0, 0, 0, 0, 0,
	1, 1, 1, 1, 1, 1, 1, 1,		1, 1, 1, 1, 0, 1, 1, 1,	// 0x80
	1, 1, 1, 1, 1, 0, 0, 0,		0, 0, 1, 1, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,		0, 0, 0, 0, 0, 0, 0, 0,	// 0xa0
	0, 0, 1, 0, 0, 0, 0, 0,		0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 1, 1, 1, 1, 1, 0,		1, 1, 1, 1, 1, 1, 1, 1,	// 0xc0
	1, 1, 1, 1, 1, 1, 1, 0,		1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 0,		1, 1, 1, 1, 1, 1, 1, 1,	// 0xe0
	1, 1, 1, 1, 1, 1, 1, 0,		1, 1, 1, 1, 1, 1, 1, 1,
};

#define is_allowed_char(x)	(valid_chars[(x) & 0xff])
//#define is_alnum(c)			isalnum((char)(c))

// Return TRUE if there is a NULL terminated string starting at *mem.
static int is_varname(uint8_t *mem)
{
	int i=0;

	//printf("%i %i %i %02x\n", is_alnum(mem[i]) ? 1 : 0, is_allowed_char(mem[i]), mem[i], mem[i]);

	// The VAT can contain names starting with a number, they are hidden in Var-Link.
	if(!is_allowed_char(mem[0]))
		return 0;

	for(i=1; i<8; i++)
		if(!is_allowed_char(mem[i]) && mem[i])
			return 0;

	return !0;
}

static int get_folder_list_handle(void)
{
	int h, i;

	// search for memory blocks which have a string at [5]
	for(h = 1; h < HEAP_MAX_SIZE; h++)
	{
		uint32_t addr;
		uint16_t size;

		heap_get_block_addr_and_size(h, &addr, &size);
		if(is_varname(ti68k_get_real_address(addr + 4)))
		{
			// next, be sure we have found the folder list by comparing the number of (possible)
			// folders with the number of identified folders
			int nfolders = mem_rd_word(addr+2);
			
			// The folder list always contains at least the main folder.
			if(nfolders == 0)
				continue;

			for(i = 0; i < nfolders; i++)
				if(!is_varname(ti68k_get_real_address(addr + 4 + i*sizeof(TI89_SYM_ENTRY))))
					break;

			// not valid, so try the next handle
			if(i < nfolders)
				continue;

			printf("handle $%i, #folders = %i\n", h, nfolders);
			return h;
		}
	}

	return -1;
}

static int parse_vat_89(GNode *node_top)
{
	uint32_t fa, va, pa;
	uint16_t fs, vs, ps;
	int nfolders, nvars;
	int i, j;
	VatSymEntry *vse;
	GNode *node_fol, *node_var;
	int handle = 0x08;

	if(tihw.calc_type == TI92)
		return -1;	

	// handle: names and handles of all folders (including "main")
	if(strcmp(img_infos.version, "2.00") >= 0)
		handle = get_folder_list_handle();	// AMS2 (dynamic)
	else
		handle = 0x08;	// AMS1 (static)

	if(handle == -1)
		return -1;
	else
		heap_get_block_addr_and_size(handle, &fa, &fs);

	// skip maximum number of folders before handle #$B needs to be resized
	// and actual number of folders 
	nfolders = mem_rd_word(fa+2);
	fa += 4;

	// now, we read a list of SYM_ENTRY structs (list of folders)
	for(i=0; i<nfolders; i++)
	{
		TI89_SYM_ENTRY se;
		
		// read struct
		memcpy(&se, ti68k_get_real_address(fa + i * sizeof(TI89_SYM_ENTRY)), sizeof(TI89_SYM_ENTRY));
		se.handle = GUINT16_FROM_BE(se.handle);

		// add node
		vse = g_malloc0(sizeof(VatSymEntry));
		strcpy(vse->name, se.name);	vse->handle = se.handle;
		g_node_append(node_top, node_fol = g_node_new(vse));

		// handle xxxx: names and handles of all variables
		heap_get_block_addr_and_size(se.handle, &va, &vs);

		// skip max num and actual num of vars
		nvars = mem_rd_word(va+2);
		va += 4;

		for(j=0; j<nvars; j++)
		{
			TI89_SYM_ENTRY se;

			// read struct
			memcpy(&se, ti68k_get_real_address(va + j * sizeof(TI89_SYM_ENTRY)), sizeof(TI89_SYM_ENTRY));
			se.handle = GUINT16_FROM_BE(se.handle);

			// add node
			vse = g_malloc0(sizeof(VatSymEntry));
			strcpy(vse->name, se.name);	vse->handle = se.handle;
			g_node_append(node_fol, node_var = g_node_new(vse));

			// handle: variable content
			heap_get_block_addr_and_size(se.handle, &pa, &ps);
		}
	}

	return 0;
}

/*
	An excellent doc on VAT: http://doors.ticalc.org/tips.htm#VAT
*/

typedef struct 
{
	char name[9]; 
	uint8_t state;
	uint16_t handle; 
} TI92_SYM_ENTRY;

// tested: OK.
static int parse_vat_92(GNode *node_top)
{
	uint32_t fa, va, pa;
	uint16_t fs, vs, ps;
	int nfolders, nvars;
	int i, j;
	VatSymEntry *vse;
	GNode *node_fol, *node_var;

	if(tihw.calc_type != TI92)
		return -1;

	// handle 000B:	names and handles of all folders (including "main")
	heap_get_block_addr_and_size(0xb, &fa, &fs);

	// skip maximum number of folders before handle #$B needs to be resized
	// and actual number of folders 
	nfolders = mem_rd_word(fa+2);
	fa += 4;

	// now, we read a list of SYM_ENTRY structs (list of folders)
	for(i=0; i<nfolders; i++)
	{
		TI92_SYM_ENTRY se;
		
		// read struct
		memcpy(&se, ti68k_get_real_address(fa + i * sizeof(TI92_SYM_ENTRY)), sizeof(TI92_SYM_ENTRY));
		se.handle = GUINT16_FROM_BE(se.handle);

		// add node
		vse = g_malloc0(sizeof(VatSymEntry));
		strcpy(vse->name, se.name);	vse->handle = se.handle;
		g_node_append(node_top, node_fol = g_node_new(vse));

		// handle xxxx: names and handles of all variables
		heap_get_block_addr_and_size(se.handle, &va, &vs);

		// skip max num and actual num of vars
		nvars = mem_rd_word(va+2);
		va += 4;

		for(j=0; j<nvars; j++)
		{
			TI92_SYM_ENTRY se;

			// read struct
			memcpy(&se, ti68k_get_real_address(va + j * sizeof(TI92_SYM_ENTRY)), sizeof(TI92_SYM_ENTRY));
			se.handle = GUINT16_FROM_BE(se.handle);

			// add node
			vse = g_malloc0(sizeof(VatSymEntry));
			strcpy(vse->name, se.name);	vse->handle = se.handle;
			g_node_append(node_fol, node_var = g_node_new(vse));

			// handle: variable content
			heap_get_block_addr_and_size(se.handle, &pa, &ps);
		}
	}

	return 0;
}

/*
	Allocate and create a tree
*/
int vat_parse(GNode **tree)
{
	VatSymEntry *vse;

	if(tree == NULL)
		return -1;

	vse = g_malloc0(sizeof(VatSymEntry));
	strcpy(vse->name, "home");
	*tree = g_node_new(vse);

	if(tihw.calc_type == TI92)
		parse_vat_92(*tree);
	else
		parse_vat_89(*tree);

	return 0;
}


static int sym_find_handle_89(const char *dirname, const char *filename)
{
	uint32_t fa, va;
	uint16_t fs, vs;
	int nfolders, nvars;
	int i, j;
	int handle = 0x08;

	if(tihw.calc_type == TI92)
		return -1;	

	// handle: names and handles of all folders (including "main")
	if(strcmp(img_infos.version, "2.00") >= 0)
		handle = get_folder_list_handle();	// AMS2 (dynamic)
	else
		handle = 0x08;	// AMS1 (static)

	if(handle == -1)
		return 0;
	else
		heap_get_block_addr_and_size(handle, &fa, &fs);

	// skip maximum number of folders before handle #$B needs to be resized
	// and actual number of folders 
	nfolders = mem_rd_word(fa+2);
	fa += 4;

	// now, we read a list of SYM_ENTRY structs (list of folders)
	for(i=0; i<nfolders; i++)
	{
		TI89_SYM_ENTRY se;
		
		// read struct
		memcpy(&se, ti68k_get_real_address(fa + i * sizeof(TI89_SYM_ENTRY)), sizeof(TI89_SYM_ENTRY));
		se.handle = GUINT16_FROM_BE(se.handle);

		if (strncmp (se.name, dirname, 8)) continue;

		// handle xxxx: names and handles of all variables
		heap_get_block_addr_and_size(se.handle, &va, &vs);

		// skip max num and actual num of vars
		nvars = mem_rd_word(va+2);
		va += 4;

		for(j=0; j<nvars; j++)
		{
			TI89_SYM_ENTRY se;

			// read struct
			memcpy(&se, ti68k_get_real_address(va + j * sizeof(TI89_SYM_ENTRY)), sizeof(TI89_SYM_ENTRY));
			se.handle = GUINT16_FROM_BE(se.handle);

			// add node
			if (strncmp (se.name, filename, 8)) continue;

			return se.handle;
		}
	}

	return 0;
}

static int sym_find_handle_92(const char *dirname, const char *filename)
{
	uint32_t fa, va;
	uint16_t fs, vs;
	int nfolders, nvars;
	int i, j;

	if(tihw.calc_type != TI92)
		return 0;

	// handle 000B:	names and handles of all folders (including "main")
	heap_get_block_addr_and_size(0xb, &fa, &fs);

	// skip maximum number of folders before handle #$B needs to be resized
	// and actual number of folders 
	nfolders = mem_rd_word(fa+2);
	fa += 4;

	// now, we read a list of SYM_ENTRY structs (list of folders)
	for(i=0; i<nfolders; i++)
	{
		TI92_SYM_ENTRY se;
		
		// read struct
		memcpy(&se, ti68k_get_real_address(fa + i * sizeof(TI92_SYM_ENTRY)), sizeof(TI92_SYM_ENTRY));
		se.handle = GUINT16_FROM_BE(se.handle);

		if (strncmp (se.name, dirname, 8)) continue;

		// handle xxxx: names and handles of all variables
		heap_get_block_addr_and_size(se.handle, &va, &vs);

		// skip max num and actual num of vars
		nvars = mem_rd_word(va+2);
		va += 4;

		for(j=0; j<nvars; j++)
		{
			TI92_SYM_ENTRY se;

			// read struct
			memcpy(&se, ti68k_get_real_address(va + j * sizeof(TI92_SYM_ENTRY)), sizeof(TI92_SYM_ENTRY));
			se.handle = GUINT16_FROM_BE(se.handle);

			// add node
			if (strncmp (se.name, filename, 8)) continue;

			return se.handle;
		}
	}

	return 0;
}

/*
	Allocate and create a tree
*/
int sym_find_handle(const char *dirname, const char *filename)
{
	if(tihw.calc_type == TI92)
		return sym_find_handle_92(dirname, filename);
	else
		return sym_find_handle_89(dirname, filename);
}

static gboolean free_vse(GNode *node, gpointer data)
{
	if (node) 
		g_free(node->data);
	return FALSE;
}

/*
	Free a previously allocated tree
*/
int vat_free(GNode **tree)
{
	if (*tree != NULL) 
	{
	    g_node_traverse(*tree, G_IN_ORDER, G_TRAVERSE_ALL, -1, free_vse, NULL);
		g_node_destroy(*tree);
		*tree = NULL;
	}

	return 0;
}
