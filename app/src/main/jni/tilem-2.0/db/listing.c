/*
 * libtilemdb - Utilities for debugging Z80 assembly programs
 *
 * Copyright (C) 2010 Benjamin Moody
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tilemdb.h"

struct _TilemListingLineCache {
	dword minaddr;
	dword maxaddr;
	int* order;
};

static void sort_lines(TilemListing* lst,
		       int start, int end)
{
	int pivot, ncstart, ncend, n;
	dword addr, pivotaddr;
	int* order = lst->linecache->order;

	pivot = order[start];
	pivotaddr = lst->lines[pivot].address;

	ncstart = start + 1;
	ncend = end;

	while (ncstart < ncend) {
		n = order[ncstart];
		addr = lst->lines[n].address;
		if (addr < pivotaddr) {
			ncstart++;
		}
		else {
			order[ncstart] = order[ncend - 1];
			order[ncend - 1] = n;
			ncend--;
		}
	}

	ncstart--;
	order[start] = order[ncstart];
	order[ncstart] = pivot;

	if (ncstart > start + 1)
		sort_lines(lst, start, ncstart);
	if (end > ncend + 1)
		sort_lines(lst, ncend, end);
}

static void ensure_order(TilemListing* lst)
{
	int i;

	if (lst->nlines == 0) {
		lst->linecache->minaddr = 0xffff;
		lst->linecache->maxaddr = 0;
	}
	else if (!lst->linecache->order) {
		lst->linecache->order = tilem_new_atomic(int, lst->nlines);
		for (i = 0; i < lst->nlines; i++)
			lst->linecache->order[i] = i;
		sort_lines(lst, 0, lst->nlines);

		i = lst->linecache->order[0];
		lst->linecache->minaddr = lst->lines[i].address;
		i = lst->linecache->order[lst->nlines - 1];
		lst->linecache->maxaddr = lst->lines[i].address;
	}
}

static void order_destroyed(TilemListing* lst)
{
	tilem_free(lst->linecache->order);
	lst->linecache->order = NULL;
}

static int first_at_addr(TilemListing* lst, dword addr)
{
	int start, end, i;

	start = 0;
	end = lst->nlines;
	while (start < end) {
		i = (start + end) / 2;
		if (lst->lines[lst->linecache->order[i]].address < addr)
			start = i + 1;
		else
			end = i;
	}
	return start;
}

TilemListing* tilem_listing_new()
{
	TilemListing* lst = tilem_new0(TilemListing, 1);
	lst->lines = NULL;
	lst->linecache = tilem_new0(TilemListingLineCache, 1);
	lst->linecache->order = NULL;
	return lst;
}

void tilem_listing_free(TilemListing* lst)
{
	int i;

	if (!lst)
		return;

	for (i = 0; i < lst->nlines; i++)
		tilem_free(lst->lines[i].text);
	tilem_free(lst->lines);
	tilem_free(lst->linecache->order);
	tilem_free(lst->linecache);
	tilem_free(lst);
}

void tilem_listing_file_clear(TilemListing* lst)
{
	order_destroyed(lst);
	lst->nlines = 0;
	lst->nlines_a = 0;
	tilem_free(lst->lines);
	lst->lines = NULL;
}

void tilem_listing_append_line(TilemListing* lst, int srclinenum, dword address,
			       int depth, int datasize, const byte* data,
			       const char* text, int is_expansion)
{
	TilemListingLine* line;
	int i;

	order_destroyed(lst);
	lst->nlines++;
	if (lst->nlines >= lst->nlines_a) {
		lst->nlines_a = lst->nlines * 2;
		lst->lines = tilem_renew(TilemListingLine,
					 lst->lines, lst->nlines_a);
	}

	line = &lst->lines[lst->nlines - 1];

	line->listing = lst;
	line->srclinenum = srclinenum;
	line->address = address & 0xffff;
	line->depth = depth;

	if (datasize > TILEM_MAX_LINE_BYTES)
		datasize = TILEM_MAX_LINE_BYTES;

	line->datasize = datasize;
	for (i = 0; i < datasize; i++)
		line->data[i] = data[i];

	if (text) {
		line->text = tilem_new_atomic(char, strlen(text) + 1);
		strcpy(line->text, text);

		if ((text[0] >= 'A' && text[0] <= 'Z')
		    || (text[0] >= 'a' && text[0] <= 'z')
		    || (text[0] & 0x80))
			line->is_label = 1;
		else
			line->is_label = 0;
	}
	else {
		line->text = NULL;
		line->is_label = 0;
	}

	line->is_expansion = is_expansion;
}

void tilem_listing_get_address_range(TilemListing* lst, dword* min, dword* max)
{
	ensure_order(lst);
	if (min)
		*min = lst->linecache->minaddr;
	if (max)
		*max = lst->linecache->maxaddr;
}

TilemListingLine* tilem_listing_line_get_next(TilemListingLine* line)
{
	if (!line)
		return NULL;
	
	if (line != line->listing->lines + line->listing->nlines)
		return line + 1;
	else
		return NULL;
}

TilemListingLine* tilem_listing_line_get_prev(TilemListingLine* line)
{
	if (!line)
		return NULL;

	if (line != line->listing->lines)
		return line - 1;
	else
		return NULL;

}

TilemListingLine* tilem_listing_get_loaded_line_at_addr(TilemListing* lst,
							dword address,
							TilemCalc* calc,
							int match_internal)
{
	int i;
	TilemListingLine *line;

	ensure_order(lst);
	i = first_at_addr(lst, address + 1);

	while (--i >= 0) {
		line = &lst->lines[lst->linecache->order[i]];

		if (match_internal) {
			if (line->address + TILEM_MAX_LINE_BYTES <= address)
				return NULL;
			else if (line->address + line->datasize <= address)
				continue;
		}
		else {
			if (line->address != address)
				return NULL;
		}

		if (tilem_listing_line_is_loaded(line, calc))
			return line;
	}
	return NULL;
}

static inline unsigned int getbyte(TilemCalc* calc, dword addr)
{
	dword pa = (*calc->hw.mem_ltop)(calc, addr & 0xffff);
	return calc->mem[pa];
}

static inline int line_load_count(const TilemListingLine* line, TilemCalc* calc)
{
	int i, n;

	for (i = n = 0; i < line->datasize; i++)
		if (getbyte(calc, line->address + i) == line->data[i])
			n++;
	return n;
}

static inline TilemListingLine* get_next_local(TilemListingLine* line)
{
	TilemListingLine* nline = tilem_listing_line_get_next(line);

	if (nline && nline->address != line->address + line->datasize)
		return NULL;
	else
		return nline;
}

static inline TilemListingLine* get_prev_local(TilemListingLine* line)
{
	TilemListingLine* nline = tilem_listing_line_get_prev(line);

	if (nline && nline->address != line->address + line->datasize)
		return NULL;
	else
		return nline;
}

int tilem_listing_line_is_loaded(TilemListingLine* line, TilemCalc* calc)
{
	int nbytes, ngood;
	TilemListingLine *nline;

	while (line->datasize == 0 && (nline = get_next_local(line)))
		line = nline;

	if (line_load_count(line, calc) != line->datasize)
		return 0;
	else {
		nbytes = ngood = line->datasize;

		nline = line;
		while ((nline = get_next_local(nline))
		       && nline->address < line->address + 32) {
			nbytes += nline->datasize;
			ngood += line_load_count(nline, calc);
		}

		nline = line;
		while ((nline = get_prev_local(nline))
		       && line->address < nline->address + 32) {
			nbytes += nline->datasize;
			ngood += line_load_count(nline, calc);
		}

		return (ngood > nbytes / 2);
	}
}

static int bptest_listing_line(TilemCalc* calc, dword addr TILEM_ATTR_UNUSED,
			       void* data)
{
	return tilem_listing_line_is_loaded(data, calc);
}

int tilem_listing_line_add_breakpoint(TilemListingLine* line,
				      TilemCalc* calc, int bptype,
				      int match_internal)
{
	dword max;

	if (match_internal && line->datasize > 0)
		max = line->address + line->datasize - 1;
	else
		max = line->address;

	return tilem_z80_add_breakpoint(calc, bptype, line->address, max,
					0xffff, &bptest_listing_line,
					(void*) line);
}

static int bptest_listing_int(TilemCalc* calc, dword addr, void* data)
{
	TilemListing* lst = data;

	if (tilem_listing_get_loaded_line_at_addr(lst, addr, calc, 1))
		return 1;
	else
		return 0;
}

static int bptest_listing_top(TilemCalc* calc, dword addr, void* data)
{
	TilemListing* lst = data;

	if (tilem_listing_get_loaded_line_at_addr(lst, addr, calc, 0))
		return 1;
	else
		return 0;
}

int tilem_listing_add_breakpoint(TilemListing* lst, TilemCalc* calc,
				 int bptype, int match_internal)
{
	dword min, max;

	tilem_listing_get_address_range(lst, &min, &max);
	return tilem_z80_add_breakpoint(calc, bptype, min, max, 0xffff,
					(match_internal
					 ? &bptest_listing_int
					 : &bptest_listing_top), lst);
}

