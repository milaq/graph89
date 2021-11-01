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
#include <stdarg.h>
#include "tilemdb.h"

/* Test if TEXT contains a correctly-formatted number WIDTH characters
   wide.  If PAD = 0, number is padded on left with zeroes; if PAD =
   1, number is padded on left with spaces; if PAD = -1, number is
   padded on right with spaces. */
static int match_num(const char* text, int width, int pad, int base,
		     int* value)
{
	int start, end;
	char* p;

	if ((int) strlen(text) < width)
		return 0;

	start = 0;
	while (start < width && text[start] == ' ')
		start++;

	end = width;
	while (end > start && text[end - 1] == ' ')
		end--;

	if (start == end)
		return 0;

	if (pad != 1 && start != 0)
		return 0;

	if (pad != -1 && end != width)
		return 0;

	if (pad != 0 && text[start] == '0' && start != end - 1)
		return 0;

	*value = strtol(text + start, &p, base);

	return (p == text + end);
}

/* Test if string exactly matches a printf-like format string.
   %<pad><width>d, %<pad><width>x match decimal and hex integers;
   %<width>s matches any string of exactly that width. */
static int match_pattern(const char* text, const char* pattern, int* datasize,
			 byte* data, ...)
{
	int width, pad, value, *ip;
	char **sp;
	char *end;
	va_list ap;
	int failed = 0, lastdb = 0;

	va_start(ap, data);

	*datasize = 0;

	while (!failed && *pattern) {
		if (*pattern != '%') {
			if (*text == *pattern)
				text++;
			else
				failed = 1;
			pattern++;
		}
		else {
			pattern++;
			if (*pattern == '0') {
				pad = 0;
				pattern++;
			}
			else if (*pattern == '-') {
				pad = -1;
				pattern++;
			}
			else
				pad = 1;

			width = strtol(pattern, &end, 10);
			pattern = end;
			if (!width)
				width = strlen(text);
			else if (width > (int) strlen(text)) {
				failed = 1;
				break;
			}

			switch (*pattern) {
			case '%':
				if (*text == '%')
					text++;
				else
					failed = 1;
				break;

			case 'B':
				if (text[0] == ' ' && text[1] == ' ') {
					lastdb = 1;
					text += 2;
					break;
				}
			case 'b':
				if (match_num(text, width, pad, 16, &value)
				    && !lastdb) {
					data[*datasize] = value;
					(*datasize)++;
					text += width;
				}
				else {
					failed = 1;
				}
				break;

			case 'd':
				ip = va_arg(ap, int *);
				if (match_num(text, width, pad, 10, ip)) {
					text += width;
				}
				else {
					failed = 1;
				}
				break;

			case 'x':
				ip = va_arg(ap, int *);
				if (match_num(text, width, pad, 16, ip)) {
					text += width;
				}
				else {
					failed = 1;
				}
				break;

			case 's':
				sp = va_arg(ap, char **);
				*sp = (char*) text;
				text += width;
				break;
			}
			pattern++;
		}
	}

	va_end(ap);

	return (!failed && *text == 0);
}

static int get_tasm_depth(const char* s)
{
	if (!strncmp(s, "+++", 3))
		return 4;
	else if (!strncmp(s, "++ ", 3))
		return 3;
	else if (!strncmp(s, "+  ", 3))
		return 2;
	else if (!strncmp(s, "   ", 3))
		return 1;
	else
		return 0;
}

static int parse_listing(const char* line, int* linenum, int* addr, int* depth,
			 int* datasize, byte* data, char** text, int* isexp,
			 const TilemListingLine* prevline)
{
	int dummy;
	char *p, *q;

	/* tasm */

	if (match_pattern(line, "%04d%3s%04x%1s%02B %02B %02B %02B %s",
			  datasize, data, linenum, &p, addr, &q, text)) {
		*depth = get_tasm_depth(p);
		*isexp = 0;
		return 1;
	}
	if (match_pattern(line, "%04d%3s%04x%1s%02b %02b %02b ",
			  datasize, data, linenum, &p, addr, &q)
	    || match_pattern(line, "%04d%3s%04x%1s%02b %02b ",
			     datasize, data, linenum, &p, addr, &q)
	    || match_pattern(line, "%04d%3s%04x%1s%02b ",
			     datasize, data, linenum, &p, addr, &q)) {
		*depth = get_tasm_depth(p);
		*text = NULL;
		*isexp = 0;
		return 1;
	}

	/* zmasm */

	if (match_pattern(line, "%08x %02B %02B %02B %02B %02B %02B   %4s %5d %s",
			  datasize, data, addr, &p, linenum, text)
	    && p[0] >= 'A' && p[0] <= 'Z') {
		*depth = p[0] - 'A' + 1;
		*isexp = (p[1] == '+');
		return 1;
	}
	if (match_pattern(line, "%08x %02B %02B %02B %02B %02B %02B   %4s %5d",
			  datasize, data, addr, &p, linenum)
	    && p[0] >= 'A' && p[0] <= 'Z') {
		*text = NULL;
		*depth = p[0] - 'A' + 1;
		*isexp = (p[1] == '+');
		return 1;
	}
	if (match_pattern(line, "                             %4s %5d %s",
			  datasize, data, &p, linenum, text)
	    && p[0] >= 'A' && p[0] <= 'Z') {
		*addr = prevline->address + prevline->datasize;
		*depth = p[0] - 'A' + 1;
		*isexp = (p[1] == '+');
		return 1;
	}
	if (match_pattern(line, "                   %08x  %4s %5d %s",
			  datasize, data, &dummy, &p, linenum, text)
	    && p[0] >= 'A' && p[0] <= 'Z') {
		*addr = prevline->address + prevline->datasize;
		*depth = p[0] - 'A' + 1;
		*isexp = (p[1] == '+');
		return 1;
	}

	/* spasm - old and new versions */

	if (match_pattern(line, "%5d %04x: %02b %02b %02b %02b %s",
			  datasize, data, linenum, addr, text)
	    || match_pattern(line, "%5d %04x: %02b %02b %02b -  %s",
			     datasize, data, linenum, addr, text)
	    || match_pattern(line, "%5d %04x: %02b %02b -  -  %s",
			     datasize, data, linenum, addr, text)
	    || match_pattern(line, "%5d %04x: %02b -  -  -  %s",
			     datasize, data, linenum, addr, text)
	    || match_pattern(line, "%5d %04x: -  -  -  -  %s",
			     datasize, data, linenum, addr, text)

	    || match_pattern(line, "%5d %02x:%04x %02b %02b %02b %02b %s",
			     datasize, data, linenum, &dummy, addr, text)
	    || match_pattern(line, "%5d %02x:%04x %02b %02b %02b -  %s",
			     datasize, data, linenum, &dummy, addr, text)
	    || match_pattern(line, "%5d %02x:%04x %02b %02b -  -  %s",
			     datasize, data, linenum, &dummy, addr, text)
	    || match_pattern(line, "%5d %02x:%04x %02b -  -  -  %s",
			     datasize, data, linenum, &dummy, addr, text)
	    || match_pattern(line, "%5d %02x:%04x -  -  -  -  %s",
			     datasize, data, linenum, &dummy, addr, text)) {
		*depth = *isexp = 0;
		return 1;
	}
	if (match_pattern(line, "            %02b %02b %02b %02b %s",
			  datasize, data, text)
	    || match_pattern(line, "            %02b %02b %02b -  %s",
			     datasize, data, text)
	    || match_pattern(line, "            %02b %02b -  -  %s",
			     datasize, data, text)
	    || match_pattern(line, "            %02b -  -  -  %s",
			     datasize, data, text)

	    || match_pattern(line, "              %02b %02b %02b %02b %s",
			     datasize, data, text)
	    || match_pattern(line, "              %02b %02b %02b -  %s",
			     datasize, data, text)
	    || match_pattern(line, "              %02b %02b -  -  %s",
			     datasize, data, text)
	    || match_pattern(line, "              %02b -  -  -  %s",
			     datasize, data, text)) {
		*linenum = prevline->srclinenum;
		*addr = prevline->address + prevline->datasize;
		*depth = *isexp = 0;
		return 1;
	}

	/* tpasm or miniasm */

	if (match_pattern(line, "%-5d %08x %02B %02B %02B %02B %02B %2s%s",
			  datasize, data, linenum, addr, &p, text)
	    || match_pattern(line, "%5d %08x %02B %02B %02B %02B %02B %2s%s",
			     datasize, data, linenum, addr, &p, text)) {
		*depth = 0;
		*isexp = (*p == 'm' || *p == 'a');
		return 1;
	}
	if (match_pattern(line, "%-5d %08x (%08x)     %2s%s",
			  datasize, data, linenum, addr, &dummy, &p, text)
	    || match_pattern(line, "%5d %08x (%08x)     %2s%s",
			     datasize, data, linenum, addr, &dummy, &p, text)) {
		*depth = 0;
		*isexp = (*p == 'm' || *p == 'a');
		return 1;
	}
	if (match_pattern(line, "               %02B %02B %02B %02B %02B %1s",
			  datasize, data, &p)) {
		*text = NULL;
		*linenum = prevline->srclinenum;
		*addr = prevline->address + prevline->datasize;
		*depth = 0;
		*isexp = (*p == 'm' || *p == 'a');
		return 1;
	}
	if (match_pattern(line, "               %02b %02b %02b %02b %02b",
			  datasize, data)
	    || match_pattern(line, "               %02b %02b %02b %02b",
			     datasize, data)
	    || match_pattern(line, "               %02b %02b %02b",
			     datasize, data)
	    || match_pattern(line, "               %02b %02b",
			     datasize, data)
	    || match_pattern(line, "               %02b",
			     datasize, data)) {
		*text = NULL;
		*linenum = prevline->srclinenum;
		*addr = prevline->address + prevline->datasize;
		*depth = *isexp = 0;
		return 1;
	}

	printf("***\t%s\n", line);

	return 0;
}

int tilem_listing_read_file(TilemListing* lst, FILE* lstfile)
{
	char buf[1024];
	int linenum, addr, depth, isexp, datasize;
	byte data[TILEM_MAX_LINE_BYTES];
	char *text;
	int i, status = 1;
	TilemListingLine prevline;

	prevline.text = NULL;
	prevline.srclinenum = 0;
	prevline.address = 0xffffffff;
	prevline.depth = 0;
	prevline.datasize = 0;

	while (fgets(buf, sizeof(buf), lstfile)) {
		i = strlen(buf);
		while (i > 0 && (buf[i - 1] == '\r' || buf[i - 1] == '\n'))
			i--;
		buf[i] = 0;
    
		if (parse_listing(buf, &linenum, &addr, &depth, &datasize,
				  data, &text, &isexp, &prevline)) {

			if (linenum)
				status = 0;

			tilem_listing_append_line(lst, linenum, addr, depth,
						  datasize, data, text, isexp);
			prevline = lst->lines[lst->nlines - 1];
		}
	}

	return status;
}

