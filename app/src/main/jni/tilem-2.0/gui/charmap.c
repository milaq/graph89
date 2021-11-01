/*
 * TilEm II
 *
 * Copyright (c) 2011 Benjamin Moody
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <ticonv.h>
#include <tilem.h>

#include "charmap.h"

#define UNDEF 0xfffd

static const unsigned long ti81chars[256] = {
	0,      ' ',    0x2192, 0x2191, 0x2193, 0x25B6, '<',    0x2264,
	'=',    0x2260, '>',    0x2265, 0x3D20DE, 0x207C, '+',  '-',
	'*',    '/',    '^',    0x221A, '(',    ')',    '[',    ']',
	'{',    '}',    '?',    '!',    ':',    ',',    0x2026, 0x207B00B9,
	0x207B, 0xB7,   0x2070, 0xB9,   0xB2,   0xB3,   0x2074, 0x2075,
	0x2076, 0x2077, 0x2078, 0x2079, 'E',    0x2081, 0x2082, 0x2083,
	0x2084, 0x23E8, 0x209C, '"',    0x207B, '.',    '0',    '1',
	'2',    '3',    '4',    '5',    '6',    '7',    '8',    '9',
	'E',    0x2B3,  0xB0,   0x3B8,  'R',    'T',    0x2E3,  0x2B8,
	0x780305, 0x790305, 0x3A3, 0x3C3, 0x3C0, 'A',   'B',    'C',
	'D',    'E',    'F',    'G',    'H',    'I',    'J',    'K',
	'L',    'M',    'N',    'O',    'P',    'Q',    'R',    'S',
	'T',    'U',    'V',    'W',    'X',    'Y',    'Z',    0x3B8,
	'a',    'b',    'c',    'd',    'e',    'f',    'g',    'h',
	'i',    'l',    'm',    'n',    'o',    'p',    'q',    'r',
	's',    't',    'u',    'v',    'w',    'x',    'y',    0xD7,
	0x2588, 0x219120DE, 0x4120DE, '_', 0x21910332, 0x410332, UNDEF, UNDEF,
	UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF,
	UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF,
	UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF,
	UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF,
	UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF,
	UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF,
	UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF,
	UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF,
	UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF,
	UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF,
	UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF,
	UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF,
	UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF,
	UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF,
	UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF };

static const unsigned long * getmap(int model)
{
	switch (model) {
	case TILEM_CALC_TI73:
		return ti73_charset;
	case TILEM_CALC_TI81:
		return ti81chars;
	case TILEM_CALC_TI82:
		return ti82_charset;
	case TILEM_CALC_TI76:
	case TILEM_CALC_TI83:
		return ti83_charset;
	case TILEM_CALC_TI83P:
	case TILEM_CALC_TI83P_SE:
	case TILEM_CALC_TI84P:
	case TILEM_CALC_TI84P_SE:
	case TILEM_CALC_TI84P_NSPIRE:
		return ti83p_charset;
	case TILEM_CALC_TI85:
		return ti85_charset;
	case TILEM_CALC_TI86:
		return ti86_charset;
	default:
		return ti83p_charset;
	}
}

/* Convert a byte value from the calculator large-font character set
   into a printable UTF-8 string. */
char *ti_to_unicode(int model, unsigned int value)
{
	const unsigned long *map = getmap(model);
	unsigned long v;
	char buf[12];
	int n;

	v = map[value];
	if (v == 0)
		v = 0x2400;	/* SYMBOL FOR NULL */
	else if (v == '\n')
		v = 0x240A;	/* SYMBOL FOR LINE FEED */
	else if (v == ' ')
		v = 0x2423;	/* OPEN BOX */

	/* in the ticonv character tables, non-BMP characters are
	   represented by a surrogate pair */
	if ((v & 0xfc00fc00) == 0xd800dc00) {
		v = (((v & 0x3ff0000) >> 6) | (v & 0x3ff)) + 0x10000;
		n = g_unichar_to_utf8(v, buf);
	}
	else if (v & 0xffff0000) {
		n = g_unichar_to_utf8(v >> 16, buf);
		n += g_unichar_to_utf8(v & 0xffff, buf + n);
	}
	else {
		n = g_unichar_to_utf8(v, buf);
	}

	return g_strndup(buf, n);
}
