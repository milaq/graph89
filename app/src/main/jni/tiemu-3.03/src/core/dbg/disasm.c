/* Hey EMACS -*- linux-c -*- */
/* $Id: disasm.c 2841 2009-05-15 15:26:47Z roms $ */

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

#ifdef HAVE_CONFIG_H
# include <tiemuconfig.h>
#endif

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "ti68k_int.h"

/* ti68k_debug_disassemble is a wrapper around the GDB or UAE disassembler, so
   we can use the GDB disassembler in GDB-enabled builds and the UAE one in
   non-GDB builds. The abstraction also allows plugging in any other
   disassembler instead at any moment. */

#ifndef NO_GDB

/* GDB enabled: use the GDB disassembler */

#include "../../gdb/include/dis-asm.h"
struct gdbarch;
struct ui_file;
struct disassemble_info gdb_disassemble_info (struct gdbarch *gdbarch, struct ui_file *file);
int print_insn_m68k (bfd_vma memaddr, disassemble_info *info);

uint32_t ti68k_debug_disassemble(uint32_t addr, char **line)
{
	static struct disassemble_info di;
	static char *output = NULL;
	uint32_t offset;
	gchar **split;
	gchar *p;

	if (!output)
	{
		di = gdb_disassemble_info (NULL, NULL);
		output = di.stream;
	}

	*output = 0;
	offset = print_insn_m68k(addr, &di);
	split = g_strsplit(output, " ", 2);

	if(split[1])
		for(p = split[1]; *p == ' '; p++);
	else
		p = "";

	*line = g_strdup_printf("%06x: %s %s", addr, split[0] ? split[0] : "", p);
	g_strfreev(split);

	return offset;
}

#else

extern int m68k_disasm (char *output, uint32_t addr);

static const char* instr[] = { 
	"ORSR",			/* ORI  #<data>,SR		*/
	"ANDSR",		/* ANDI #<data>,SR		*/
	"EORSR",		/* EORI #<data>,SR		*/
	"MVSR2",		/* MOVE SR,<ea>			*/
	"MV2SR",		/* MOVE <ea>,SR			*/
	"MVR2USP.L",	/* MOVE An,USP			*/
	"MVUSP2R.L",	/* MOVE USP,An			*/
	"MVMLE",		/* MOVEM <ea>,<list>  	*/
	"MVMEL",		/* MOVEM <list>,<ea>	*/
	"MVPMR",		/* MOVEP <Dx>,<(d16,Ay)>*/
	"MVPRM",		/* MOVEP <d16,Ay>,<Dx>	*/
	"TRAP.L",		/* TRAP	#<vector>		*/
	"RESET.L",
	"NOP.L",
	"STOP.L",
	"RTE.L",
	"RTS.L",
	"JMP.L",
	"LEA.L",
	"BTST",			/* do nothing			*/
	"BT",			/* BRA					*/

	NULL
};

static int match_opcode(const char *opcode)
{
	int i;

	if(opcode == NULL)
		return -1;

	for(i = 0; instr[i] != NULL; i++)
	{
		 if(!strncmp(opcode, (char *)instr[i], strlen(instr[i])))
			return i;
	}

	return -1;
}

// testing patterns: 0x55, 0xAA, 0x02, 0xF5, 0x5F, 0xFA, 0xAF
static gchar* create_reg_list(uint8_t value, char name)
{
	gchar *str = g_strdup("");
	gchar *str2;
	int i;
	int pre_bit = 0;
	int cur_bit;
	int start = -1;
	int end = -1;

	for(i = 0; i < 8; i++, value >>= 1)
	{
		cur_bit = value & 1;

		if(pre_bit == 0 && cur_bit == 1) start = i;
		if(pre_bit == 1 && cur_bit == 0) end = i-1;

		if(start == end && start != -1)
		{
			str2 = g_strdup_printf("%s%c%i/", str, name, i-1);
			g_free(str);
			str = str2;
			end = start = -1;
		}
		else if(end > start)
		{
			str2 = g_strdup_printf("%s%c%i-%c%i/", str, name, start, name, end);
			g_free(str);
			str = str2;
			end = start = -1;
		}

		pre_bit = cur_bit;
	}

	if((end=i-1) > (start+1) && start != -1)
	{
		str2 = g_strdup_printf("%s%c%i-%i", str, name, start, end);
		g_free(str);
		str = str2;
	}
	else if(start > 0 && end > 0)
	{
		str2 = g_strdup_printf("%s%c%i", str, name, start);
		g_free(str);
		str = str2;
	}
	else
		str[strlen(str) - 1] = '\0';

	return str;
}

static gchar* create_rev_reg_list(uint8_t value, char name)
{
	gchar *str = g_strdup("");
	gchar *str2;
	int i;
	int pre_bit = 0;
	int cur_bit;
	int start = -1;
	int end = -1;

	for(i = 0; i < 8; i++, value <<= 1)
	{
		cur_bit = (value & (1 << 7)) >> 7;

		if(pre_bit == 0 && cur_bit == 1) start = i;
		if(pre_bit == 1 && cur_bit == 0) end = i-1;

		if(start == end && start != -1)
		{
			str2 = g_strdup_printf("%s%c%i/", str, name, i-1);
			g_free(str);
			str = str2;
			end = start = -1;
		}
		else if(end > start)
		{
			str2 = g_strdup_printf("%s%c%i-%c%i/", str, name, start, name, end);
			g_free(str);
			str = str2;
			end = start = -1;
		}

		pre_bit = cur_bit;
	}

	if((end=i-1) > (start+1) && start != -1)
	{
		str2 = g_strdup_printf("%s%c%i-%i", str, name, start, end);
		g_free(str);
		str = str2;
	}
	else if(start > 0 && end > 0)
	{
		str2 = g_strdup_printf("%s%c%i", str, name, start);
		g_free(str);
		str = str2;
	}
	else
		str[strlen(str) - 1] = '\0';

	return str;
}

static gchar* create_reg_lists(uint16_t value)
{
	gchar *a, *d;
	gchar *str;

	a = create_reg_list(MSB(value), 'A');
	d = create_reg_list(LSB(value), 'D');

	return str = g_strconcat(a, "/", d, NULL);
}

static char* create_rev_reg_lists(uint16_t value)
{
	gchar *a, *d;
	gchar *str;

	d = create_rev_reg_list(MSB(value), 'D');
	a = create_rev_reg_list(LSB(value), 'A');

	return str = g_strconcat(d, "/", a, NULL);
}

// do the same work as m68k_disasm but some instructions dis-assembled by the
// UAE engine use a weird/wrong naming scheme so we remap them here rather than
// touching the newcpu.c & table68k files because this file may be updated when 
// upgrading the UAE engine.
int m68k_dasm(char **line, uint32_t addr)
{
	char output[1024];
	int offset;
	gchar** split;
	int idx;

	// get UAE disassembly
	offset = m68k_disasm(output, addr);

	// split string into address, opcode and operand
	split = g_strsplit(output, " ", 3);

	// search for opcode to rewrite
	idx = match_opcode(split[1]);
	if(idx != -1)
	{
		gchar *tmp;
		
		switch(idx)
		{
		case 0:		/* ORI to SR #<data>,SR		*/
			{
				char c = split[1][5];

				g_free(split[1]);
				split[1] = g_strdup_printf("ORI.%c", c);
			
				tmp = g_strconcat(split[2], ",SR", NULL);
				g_free(split[2]);
				split[2] = tmp;
			}
			break;
		case 1:		/* ANDI to SR #<data>,SR	*/
			{
				char c = split[1][6];
			
				g_free(split[1]);
				split[1] = g_strdup_printf("ANDI.%c", c);
			
				tmp = g_strconcat(split[2], ",SR", NULL);
				g_free(split[2]);
				split[2] = tmp;
			}
			break;
		case 2:		/* EORI to SR #<data>,SR	*/
			{
				char c = split[1][6]; 
			
				g_free(split[1]);
				split[1] = g_strdup_printf("EORI.%c", c);
			
				tmp = g_strconcat(split[2], ",SR", NULL);
				g_free(split[2]);
				split[2] = tmp;
			}
			break;
		case 3:		/* MOVE from SR: SR,<ea>		*/
			{
				char c = split[1][6];

				g_free(split[1]);
				split[1] = g_strdup_printf("MOVE.%c", c);
			
				tmp = g_strconcat("SR,", split[2], NULL);
				g_free(split[2]);
				split[2] = tmp;
			}
			break;
		case 4:		/* MOVE to SR: <ea>,SR		*/
			{
				char c = split[1][6];
			
				g_free(split[1]);
				split[1] = g_strdup_printf("MOVE.%c", c);
			
				tmp = g_strconcat(split[2], ",SR", NULL);
				g_free(split[2]);
				split[2] = tmp;
			}
			break;
		case 5:	/* MOVE An,USP	*/
			g_free(split[1]);
			split[1] = g_strdup("MOVE");
			
			tmp = g_strconcat(split[2], ",USP", NULL);
			g_free(split[2]);
			split[2] = tmp;
			break;
		case 6:	/* MOVE USP,An	*/
			g_free(split[1]);
			split[1] = g_strdup("MOVE");
			
			tmp = g_strconcat("USP,", split[2], NULL);
			g_free(split[2]);
			split[2] = tmp;
			break;
		case 7:		/* MOVEM <ea>,<list>	*/
			{
				char c = split[1][6];
				char *p, *q;
				gchar *tmp;
				uint16_t mask;

				g_free(split[1]);
				split[1] = g_strdup_printf("MOVEM.%c", c);

				p = split[2];
				q = strchr(split[2], ',');
				q++;

				sscanf(p, "#$%" SCNx16, &mask);
				if(q[0] != '-')
					tmp = g_strdup_printf("%s,%s", q, create_reg_lists(mask));
				else
					tmp = g_strdup_printf("%s,%s", q, create_rev_reg_lists(mask));

				g_free(split[2]);
				split[2] = tmp;
			}
			break;
		case 8:		/* MOVEM <list>, <ea>	*/
			{
				char c = split[1][6];
				char *p, *q;
				gchar *tmp;
				uint16_t mask;

				g_free(split[1]);
				split[1] = g_strdup_printf("MOVEM.%c", c);

				p = split[2];
				q = strchr(split[2], ',');
				q++;

				sscanf(p, "#$%" SCNx16, &mask);
				if(q[0] != '-')
					tmp = g_strdup_printf("%s,%s", create_reg_lists(mask), q);
				else
					tmp = g_strdup_printf("%s,%s", q, create_rev_reg_lists(mask));

				g_free(split[2]);
				split[2] = tmp;
			}
			break;
		case 9:		/* MOVEP <Dx>,<(d16,Ay)> */
		case 10:	/* MOVEP <(d16,Ay)>,<Dx> */
			{
				char c = split[1][6];

				g_free(split[1]);
				split[1] = g_strdup_printf("MOVEP.%c", c);
			}
			break;
		case 11:	/* TRAP #<vector>	*/
		case 12:	/* RESET.L			*/
		case 13:	/* NOP.L			*/
		case 14:	/* STOP.L			*/
		case 15:	/* RTE.L			*/
		case 16:	/* RTS.L			*/
		case 17:	/* JMP.L			*/
		case 18:	/* LEA.L			*/
			{
				char *p = strchr(split[1], '.');
				if(p) *p = '\0';
			}
			break;
		case 20:	/* BRA				*/
			g_free(split[1]);
			split[1] = g_strdup("BRA");
		break;

		case 19:	/* BTST				*/
		default:
			break;
		}
	}

	// search for [value] and reject it at end of line
	if(strchr(split[2], '['))
	{
		char *p = strchr(split[2], '[');
		char *q = strrchr(split[2], ']');
		char tmp[256];

		p--;
		q++;
		strncpy(tmp, p, q - p);
		tmp[q-p] = '\0';
		strcpy(p, q);
		strcat(split[2], tmp);
	}
	
	*line = g_strdup_printf("%s %s %s", 
			split[0] ? split[0] : "", 
			split[1] ? split[1] : "",
			split[2] ? split[2] : "");
	g_strfreev(split);

	return offset;
}

uint32_t ti68k_debug_disassemble(uint32_t addr, char **line)
{
	uint32_t offset;

	offset = m68k_dasm(line, addr);

	return offset;
}
#endif
