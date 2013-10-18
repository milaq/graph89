/*
 * TilEm II
 *
 * Copyright (c) 2011-2012 Benjamin Moody
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
#include <string.h>
#include <tilem.h>

#include "ti81prg.h"
#include "gettext.h"

#define tSpace  0x56
#define t0      0x10
#define t9      0x19
#define tDecPt	0x1A
#define tA      0x59
#define tZ      0x72
#define tTheta  0x73

#define ramStart      0xE000
#define cxCurApp      0xE347
#define cxOldApp      (cxCurApp + 1)
#define DimX_old      0xEEF9
#define DimX_new      0xF12D
#define prgm0Name     0xF1D3
#define prgm0Start    0xF2FB
#define prgm0End      (prgm0Start + 2)
#define prgmThetaEnd  (prgm0End + 36*2)
#define progMem       0xF347
#define progMemEnd    0xFCA6

#define cxPrgmEdit  2
#define cxPrgmExec  10
#define cxMenu      11

TI81Program * ti81_program_new(int size)
{
	TI81Program *prgm = tilem_new0(TI81Program, 1);

	prgm->info.slot = TI81_SLOT_AUTO;
	memset(prgm->info.name, tSpace, 8);

	if (size > 0) {
		prgm->info.size = size;
		prgm->data = tilem_new_atomic(byte, size);
	}

	return prgm;
}

void ti81_program_free(TI81Program *prgm)
{
	if (!prgm)
		return;

	if (prgm->data)
		tilem_free(prgm->data);
	tilem_free(prgm);
}

static byte *get_byte_ptr(const TilemCalc *calc, dword addr)
{
	if (addr < ramStart || addr > 0xffff)
		return NULL;

	return &calc->ram[addr - ramStart];
}

static int read_byte(const TilemCalc *calc, dword addr)
{
	const byte *p = get_byte_ptr(calc, addr);

	if (!p)
		return -1;
	else
		return *p;
}

static dword read_word(const TilemCalc *calc, dword addr)
{
	const byte *p = get_byte_ptr(calc, addr);

	if (!p)
		return 0;
	else
		return (p[0] | p[1] << 8);
}

static void write_word(TilemCalc *calc, dword addr, dword value)
{
	byte *p = get_byte_ptr(calc, addr);

	if (p) {
		p[0] = value & 0xff;
		p[1] = (value >> 8) & 0xff;
	}
}

static int check_busy(const TilemCalc *calc)
{
	int cur, old;

	cur = read_byte(calc, cxCurApp);
	old = read_byte(calc, cxOldApp);

	if (cur == cxPrgmEdit || cur == cxPrgmExec)
		return 1;
	else if (cur == cxMenu && (old == cxPrgmEdit || old == cxPrgmExec))
		return 1;
	else
		return 0;
}

static dword get_free_mem_end(const TilemCalc *calc)
{
	const byte *p;
	int n, i;

	p = get_byte_ptr(calc, DimX_new);

	/* DimX is always a small positive integer, so the first byte
	   must be between 80h and 82h, and the last five bytes must
	   always be zero.  On 1.1K, DimX_new is part of textShadow,
	   so none of these byte values make any sense. */

	if (p[0] < 0x80 || p[0] > 0x82 || p[7] != 0)
		p = get_byte_ptr(calc, DimX_old);

	if (p[0] < 0x80 || p[0] > 0x82)
		return 0;

	for (i = 3; i < 7; i++)
		if (p[i])
			return 0;

	n = ((p[2] & 0xf)
	     + ((p[2] >> 4) * 10)
	     + ((p[1] & 0xf) * 100)
	     + ((p[1] >> 4) * 1000));

	for (i = p[0]; i < 0x83; i++) {
		if (n % 10)
			return 0;
		n /= 10;
	}

	return (progMemEnd + 1 - 16 * n);
}

int ti81_get_program_info(const TilemCalc *calc, int slot, TI81ProgInfo *info)
{
	const byte *p;
	dword progstart, progend;

	if (slot < 0 || slot > TI81_SLOT_MAX)
		return TI81_ERR_INTERNAL;

	if (check_busy(calc))
		return TI81_ERR_BUSY;

	progstart = read_word(calc, prgm0Start + 2 * slot);
	progend = read_word(calc, prgm0Start + 2 * slot + 2);

	if (progstart < ramStart || progend < ramStart || progend < progstart)
		return TI81_ERR_BUSY;

	info->slot = slot;
	info->size = progend - progstart;
	info->addr = progstart;

	p = get_byte_ptr(calc, prgm0Name + 8 * slot);
	if (!p) return TI81_ERR_INTERNAL;
	memcpy(info->name, p, 8);

	return 0;
}

int ti81_get_program(const TilemCalc *calc, int slot, TI81Program **prgm)
{
	TI81ProgInfo info;
	const byte *p;
	int s;

	if ((s = ti81_get_program_info(calc, slot, &info))) {
		*prgm = NULL;
		return s;
	}

	*prgm = ti81_program_new(info.size);
	(*prgm)->info = info;
	if (info.size > 0 && (p = get_byte_ptr(calc, info.addr)))
		memcpy((*prgm)->data, p, info.size);

	return 0;
}

int ti81_load_program(TilemCalc *calc, const TI81Program *prgm)
{
	TI81ProgInfo info;
	int slot = prgm->info.slot;
	int s, i;
	dword progs_start, progs_end, mem_end, x;
	byte *p;

	if (slot == TI81_SLOT_AUTO) {
		for (slot = 0; slot <= TI81_SLOT_MAX; slot++) {
			if ((s = ti81_get_program_info(calc, slot, &info)))
				return s;
			if (info.size == 0 && info.name[0] == tSpace)
				break;
		}

		if (slot > TI81_SLOT_MAX)
			return TI81_ERR_SLOTS_FULL;
	}

	if ((s = ti81_get_program_info(calc, slot, &info)))
		return s;

	/* move later programs forward/backward in memory */

	progs_start = info.addr + info.size;
	progs_end = read_word(calc, prgmThetaEnd);
	if (progs_end < progs_start)
		return TI81_ERR_BUSY;

	mem_end = get_free_mem_end(calc);
	if (progs_end + prgm->info.size - info.size > mem_end)
		return TI81_ERR_MEMORY;

	if (prgm->info.size != info.size && progs_start != progs_end) {
		p = get_byte_ptr(calc, progs_start);
		if (!p) return TI81_ERR_INTERNAL;
		memmove(p + prgm->info.size - info.size, p,
		        progs_end - progs_start);
	}

	/* update program pointers */

	for (i = slot; i <= TI81_SLOT_MAX; i++) {
		x = read_word(calc, prgm0End + 2 * i);
		write_word(calc, prgm0End + 2 * i,
		           x + prgm->info.size - info.size);
	}

	/* copy program data */

	if (prgm->info.size != 0) {
		p = get_byte_ptr(calc, info.addr);
		if (!p) return TI81_ERR_INTERNAL;
		memcpy(p, prgm->data, prgm->info.size);
	}

	/* copy program name */

	p = get_byte_ptr(calc, prgm0Name + 8 * slot);
	if (!p) return TI81_ERR_INTERNAL;
	memcpy(p, prgm->info.name, 8);

	return 0;
}

int ti81_read_prg_file(FILE *f, TI81Program **prgm)
{
	byte buf[20];
	unsigned int size, i;
	unsigned int sum = 0;
	TI81Program *p;

	*prgm = NULL;

	if (fread(buf, 1, 20, f) != 20)
		return TI81_ERR_INVALID_FILE;

	if (strcmp((char *) buf, "**TI81**") || buf[9] != 0x6e)
		return TI81_ERR_INVALID_FILE;

	size = buf[10] | buf[11] << 8;

	p = ti81_program_new(size);

	memcpy(p->info.name, buf + 12, 8);

	for (i = 0; i < 8; i++)
		sum += buf[12 + i];

	if (fread(p->data, 1, size, f) != size) {
		ti81_program_free(p);
		return TI81_ERR_INVALID_FILE;
	}

	for (i = 0; i < size; i++)
		sum += p->data[i];

	if (fread(buf, 1, 2, f) != 2) {
		ti81_program_free(p);
		return TI81_ERR_INVALID_FILE;
	}

	sum -= (buf[0] | buf[1] << 8);
	if (sum & 0xffff)
		fprintf(stderr, _("warning: checksum incorrect\n"));

	*prgm = p;
	return 0;
}

int ti81_write_prg_file(FILE *f, const TI81Program *prgm)
{
	byte buf[20];
	unsigned int size, i;
	unsigned int sum = 0;

	memcpy(buf, "**TI81**\0n", 10);
	size = prgm->info.size;
	buf[10] = size & 0xff;
	buf[11] = (size >> 8) & 0xff;

	memcpy(buf + 12, prgm->info.name, 8);

	for (i = 0; i < 8; i++)
		sum += buf[12 + i];

	if (fwrite(buf, 1, 20, f) != 20)
		return TI81_ERR_FILE_IO;

	if (fwrite(prgm->data, 1, size, f) != size)
		return TI81_ERR_FILE_IO;

	for (i = 0; i < size; i++)
		sum += prgm->data[i];

	buf[0] = sum & 0xff;
	buf[1] = (sum >> 8) & 0xff;
	if (fwrite(buf, 1, 2, f) != 2)
		return TI81_ERR_FILE_IO;

	return 0;
}

char * ti81_program_slot_to_string(int slot)
{
	char buf[50];
	char *s;

	if (slot == TI81_SLOT_AUTO)
		strcpy(buf, _("Automatic"));
	else if (slot < 0 || slot > 36)
		strcpy(buf, "?");
	else if (slot < 10)
		sprintf(buf, "Prgm%c", slot + '0');
	else if (slot < 36)
		sprintf(buf, "Prgm%c", slot + 'A' - 10);
	else
		strcpy(buf, "Prgm\316\270");

	s = tilem_new_atomic(char, strlen(buf) + 1);
	strcpy(s, buf);
	return s;
}

char * ti81_program_name_to_string(const byte *prgname)
{
	char buf[50];
	char *s;
	int i, j;

	for (i = j = 0; i < 8; i++) {
		if (prgname[i] == tSpace)
			buf[j++] = '_';
		else if (prgname[i] == tDecPt)
			buf[j++] = '.';
		else if (prgname[i] == tTheta) {
			buf[j++] = '\316';
			buf[j++] = '\270';
		}
		else if (prgname[i] >= t0 && prgname[i] <= t9)
			buf[j++] = '0' + prgname[i] - t0;
		else if (prgname[i] >= tA && prgname[i] <= tZ)
			buf[j++] = 'A' + prgname[i] - tA;
		else
			buf[j++] = '?';
	}

	while (j > 0 && buf[j - 1] == '_')
		j--;
	buf[j] = 0;

	s = tilem_new_atomic(char, strlen(buf) + 1);
	strcpy(s, buf);
	return s;
}
