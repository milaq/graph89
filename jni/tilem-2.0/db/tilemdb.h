/*
 * libtilemdb - Utilities for debugging Z80 assembly programs
 *
 * Copyright (C) 2010-2012 Benjamin Moody
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef _TILEMDB_H
#define _TILEMDB_H

#include <tilem.h>

#ifdef __cplusplus
extern "C" {
#endif


/* Disassembler */

typedef struct _TilemDisasm TilemDisasm;

/* Create a new disassembly context. */
TilemDisasm* tilem_disasm_new(void);

/* Free a disassembly context. */
void tilem_disasm_free(TilemDisasm* dasm);

/* Read symbols from SYMFILE. */
int tilem_disasm_read_symbol_file(TilemDisasm* dasm, FILE* symfile);

/* Set symbol NAME to value VALUE. */
void tilem_disasm_set_label(TilemDisasm* dasm, const char* name,
			    dword value);

/* Check if symbol NAME is defined.  If symbol is defined and VALUE is
   non-null, set *VALUE to symbol's value. */
int tilem_disasm_get_label(const TilemDisasm* dasm, const char* name,
			   dword* value);

/* Check if a label is defined at the given address. */
const char* tilem_disasm_get_label_at_address(const TilemDisasm* dasm,
					      dword addr);

/* Disassemble a line starting at address ADDR.  Store text (up to
   BUFSIZE characters) in BUFFER, and set *NEXTADDR to the address of
   the following line.  If PHYS is 0, use logical addresses; otherwise
   use physical addresses. */
void tilem_disasm_disassemble(const TilemDisasm* dasm, TilemCalc* calc,
			      int phys, dword addr, dword* nextaddr,
			      char* buffer, int bufsize);

/* Determine the most likely address for the instruction preceding
   this one. */
dword tilem_disasm_guess_prev_address(const TilemDisasm* dasm,
                                      TilemCalc* calc, int phys, dword addr);


/* Assembly listing files */

typedef struct _TilemListing TilemListing;
typedef struct _TilemListingLine TilemListingLine;
typedef struct _TilemListingLineCache TilemListingLineCache;

#define TILEM_MAX_LINE_BYTES 6

struct _TilemListingLine {
	TilemListing* listing;		 /* Listing to which this line
					    belongs */
	char* text;			 /* Text of source line */
	int srclinenum;			 /* Line number in original
					    source file */
	dword address;			 /* Address */
	int depth;			 /* Source file inclusion
					    depth */
	byte datasize;			 /* Number of data bytes */
	unsigned is_label : 1;		 /* = 1 if line appears to
					    contain a label */
	unsigned is_expansion : 1;	 /* = 1 if line is part of a
					    macro expansion */
	byte data[TILEM_MAX_LINE_BYTES]; /* Data bytes on this line */
};

struct _TilemListing {
	int nlines;
	int nlines_a;
	TilemListingLine* lines;
	TilemListingLineCache* linecache;
};

/* Create new assembly listing. */
TilemListing* tilem_listing_new(void);

/* Free listing data. */
void tilem_listing_free(TilemListing* lst);

/* Clear listing file contents. */
void tilem_listing_clear(TilemListing* lst);

/* Add a line to the end of the listing file. */
void tilem_listing_append_line(TilemListing* lst, int srclinenum, dword address,
			       int depth, int datasize, const byte* data,
			       const char* text, int is_expansion);

/* Calculate minimum and maximum address used by a listing file. */
void tilem_listing_get_address_range(TilemListing* lst, dword* min, dword* max);

/* Get next line, if any. */
TilemListingLine* tilem_listing_line_get_next(TilemListingLine* line);

/* Get previous line, if any. */
TilemListingLine* tilem_listing_line_get_prev(TilemListingLine* line);

/* Find the line (if any) currently loaded at the given address.  If
   MATCH_INTERNAL = 0, find only lines that begin at that address. */
TilemListingLine* tilem_listing_get_loaded_line_at_addr(TilemListing* lst,
							dword address,
							TilemCalc* calc,
							int match_internal);

/* Check if given line is currently loaded (and mapped into Z80 memory
   space.) */
int tilem_listing_line_is_loaded(TilemListingLine* line, TilemCalc* calc);

/* Set a breakpoint to be triggered on the given line. */
int tilem_listing_line_add_breakpoint(TilemListingLine* line,
				      TilemCalc* calc, int bptype,
				      int match_internal);

/* Set a breakpoint to be triggered on any line in the listing. */
int tilem_listing_add_breakpoint(TilemListing* lst, TilemCalc* calc,
				 int bptype, int match_internal);

/* Read assembly listing from LSTFILE. */
int tilem_listing_read_file(TilemListing* lst, FILE* lstfile);


#ifdef __cplusplus
}
#endif

#endif
