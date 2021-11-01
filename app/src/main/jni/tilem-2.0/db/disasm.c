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

typedef struct _TilemDisasmSymbol {
	char* name;
	dword value;
} TilemDisasmSymbol;

typedef struct _TilemDisasmSymTable {
	int nsyms;
	int nsyms_a;
	TilemDisasmSymbol* syms;
} TilemDisasmSymTable;

struct _TilemDisasm {
	TilemDisasmSymTable labels;
	TilemDisasmSymTable romcalls;
	TilemDisasmSymTable flags;
	TilemDisasmSymTable macros;
};

TilemDisasm* tilem_disasm_new()
{
	TilemDisasm* dasm = tilem_new0(TilemDisasm, 1);
	dasm->labels.syms = NULL;
	dasm->romcalls.syms = NULL;
	dasm->flags.syms = NULL;
	dasm->macros.syms = NULL;
	return dasm;
}

static void tilem_disasm_sym_table_free(TilemDisasmSymTable* stab)
{
	int i;

	for (i = 0; i < stab->nsyms; i++)
		tilem_free(stab->syms[i].name);
	tilem_free(stab->syms);
}

void tilem_disasm_free(TilemDisasm* dasm)
{
	if (!dasm)
		return;

	tilem_disasm_sym_table_free(&dasm->labels);
	tilem_disasm_sym_table_free(&dasm->romcalls);
	tilem_disasm_sym_table_free(&dasm->flags);
	tilem_disasm_sym_table_free(&dasm->macros);
	tilem_free(dasm);
}

/* Find symbol in the given table, if any */
static TilemDisasmSymbol* find_symbol(const TilemDisasmSymTable* stab,
				      dword value)
{
	int start, end, i;

	start = 0;
	end = stab->nsyms;
	while (start < end) {
		i = (start + end) / 2;
		if (stab->syms[i].value == value)
			return &stab->syms[i];
		else if (stab->syms[i].value <= value)
			start = i + 1;
		else
			end = i;
	}
	return NULL;
}

/* Find previous symbol in the given table, if any */
static TilemDisasmSymbol* find_prev_symbol(const TilemDisasmSymTable* stab,
					   dword value)
{
	int start, end, i;

	start = 0;
	end = stab->nsyms;
	while (start < end) {
		i = (start + end) / 2;
		if (stab->syms[i].value <= value)
			start = i + 1;
		else
			end = i;
	}
	if (start > 0)
		return &stab->syms[start - 1];
	else
		return NULL;
}

/* Find symbol with given name */
static TilemDisasmSymbol* find_symbol_by_name(const TilemDisasmSymTable* stab,
					      const char* name)
{
	int i;
	for (i = 0; i < stab->nsyms; i++)
		if (!strcmp(stab->syms[i].name, name))
			return &stab->syms[i];
	return NULL;
}

/* Find a given symbol in the table, or create a new one */
static TilemDisasmSymbol* add_symbol(TilemDisasmSymTable* stab,
				     dword value)
{
	int start, end, i;
	TilemDisasmSymbol* syms;

	start = 0;
	end = stab->nsyms;

	while (start < end) {
		i = (start + end) / 2;
		if (stab->syms[i].value == value)
			return &stab->syms[i];
		else if (stab->syms[i].value < value)
			start = i + 1;
		else
			end = i;
	}

	/* insert new label into the array */
	if (stab->nsyms < stab->nsyms_a) {
		if (start < stab->nsyms)
			memmove(&stab->syms[start + 1], &stab->syms[start],
				((stab->nsyms - start)
				 * sizeof(TilemDisasmSymbol)));
	}
	else {
		stab->nsyms_a = (stab->nsyms + 1) * 2;
		syms = tilem_new(TilemDisasmSymbol, stab->nsyms_a);
		if (start > 0)
			memcpy(syms, stab->syms,
			       start * sizeof(TilemDisasmSymbol));
		if (start < stab->nsyms)
			memcpy(syms + start + 1, stab->syms + start,
			       ((stab->nsyms - start)
				* sizeof(TilemDisasmSymbol)));
		tilem_free(stab->syms);
		stab->syms = syms;
	}
  
	stab->nsyms++;

	stab->syms[start].value = value;
	stab->syms[start].name = NULL;
	return &stab->syms[start];
}

/* Remove a symbol from the table */
static void del_symbol(TilemDisasmSymTable* stab,
		       TilemDisasmSymbol* sym)
{
	int n = sym - stab->syms;

	tilem_free(sym->name);

	if (n < stab->nsyms - 1) {
		memmove(sym, sym + 1,
			(stab->nsyms - n - 1) * sizeof(TilemDisasmSymbol));
	}

	stab->nsyms--;
}

static void set_symbol(TilemDisasmSymTable* stab,
		       const char* name, dword value)
{
	TilemDisasmSymbol* sym;

	if ((sym = find_symbol_by_name(stab, name))) {
		if (sym->value == value)
			return;
		else
			del_symbol(stab, sym);
	}

	sym = add_symbol(stab, value);
	tilem_free(sym->name);
	sym->name = tilem_new_atomic(char, strlen(name) + 1);
	strcpy(sym->name, name);
}

static char* skipws(char* p)
{
	while (*p == ' ' || *p == '\t')
		p++;
	return p;
}

static char* skipwc(char* p)
{
	while ((unsigned char) *p > ' ')
		p++;
	return p;
}

static int parse_sym_value(const char* text, dword* value)
{
	char* p;
	dword x;

	if (text[0] >= '0' && text[0] <= '7' && text[1] == ',') {
		x = strtol(text + 2, &p, 16);
		*value = 0x1000 + (x << 4) + (text[0] - '0');
	}
	else {
		*value = strtol(text, &p, 16);
	}

	return (p != text && *p == 0);
}

static int parse_sym_line(TilemDisasmSymTable* stab, char* line)
{
	char *w1end, *w2start, *w2end, *name;
	dword value;

	if (line[0] == '#' || line[0] == ';')
		return 1;

	w1end = skipwc(line);
	w2start = skipws(w1end);
	w2end = skipwc(w2start);

	if (w1end == line || w2start == w1end || w2end == w2start)
		return 1;
	if (*w2end)
		return 1;

	*w1end = *w2end = 0;

	if (*line >= '0' && *line <= '9') {
		name = w2start;
		if (!parse_sym_value(line, &value))
			return 1;
	}
	else {
		name = line;
		if (!parse_sym_value(w2start, &value))
			return 1;
	}

	set_symbol(stab, name, value);
	return 0;
}

int tilem_disasm_read_symbol_file(TilemDisasm* dasm, FILE* symfile)
{
	char buf[1024];
	char* p;
	TilemDisasmSymTable* curtbl;
	int status = 1;

	curtbl = &dasm->labels;

	while (fgets(buf, sizeof(buf), symfile)) {
		p = buf + strlen(buf);
		while (p != buf && (p[-1] == '\n' || p[-1] == '\r'))
			p--;
		*p = 0;

		if (!strcmp(buf, "[labels]"))
			curtbl = &dasm->labels;
		else if (!strcmp(buf, "[romcalls]"))
			curtbl = &dasm->romcalls;
		else if (!strcmp(buf, "[flags]"))
			curtbl = &dasm->flags;
		else if (!strcmp(buf, "[macros]"))
			curtbl = &dasm->macros;
		else if (!parse_sym_line(curtbl, buf))
			status = 0;
	}

	return status;
}

void tilem_disasm_set_label(TilemDisasm* dasm, const char* name,
			     dword value)
{
	set_symbol(&dasm->labels, name, value);
}

int tilem_disasm_get_label(const TilemDisasm* dasm, const char* name,
			   dword* value)
{
	TilemDisasmSymbol* sym = find_symbol_by_name(&dasm->labels, name);

	if (!sym)
		return 0;
	else if (value)
		*value = sym->value;
	return 1;
}

const char* tilem_disasm_get_label_at_address(const TilemDisasm* dasm,
					      dword addr)
{
	TilemDisasmSymbol* sym = find_symbol(&dasm->labels, addr);

	if (sym)
		return sym->name;
	else
		return NULL;
}

typedef struct _TilemDisasmInstruction {
	int length;
	const char* pattern;
} TilemDisasmInstruction;

static const TilemDisasmInstruction insts_main[256] = {
	{1,"NOP"},       {3,"LD~BC,%w"},  {1,"LD~(BC),A"},   {1,"INC~BC"},
	{1,"INC~B"},     {1,"DEC~B"},     {2,"LD~B,%b"},     {1,"RLCA"},
	{1,"EX~AF,AF'"}, {1,"ADD~HL,BC"}, {1,"LD~A,(BC)"},   {1,"DEC~BC"},
	{1,"INC~C"},     {1,"DEC~C"},     {2,"LD~C,%b"},     {1,"RRCA"},
	{2,"DJNZ~%r"},   {3,"LD~DE,%w"},  {1,"LD~(DE),A"},   {1,"INC~DE"},
	{1,"INC~D"},     {1,"DEC~D"},     {2,"LD~D,%b"},     {1,"RLA"},
	{2,"JR~%r"},     {1,"ADD~HL,DE"}, {1,"LD~A,(DE)"},   {1,"DEC~DE"},
	{1,"INC~E"},     {1,"DEC~E"},     {2,"LD~E,%b"},     {1,"RRA"},
	{2,"JR~NZ,%r"},  {3,"LD~HL,%w"},  {3,"LD~(%a),HL"},  {1,"INC~HL"},
	{1,"INC~H"},     {1,"DEC~H"},     {2,"LD~H,%b"},     {1,"DAA"},
	{2,"JR~Z,%r"},   {1,"ADD~HL,HL"}, {3,"LD~HL,(%a)"},  {1,"DEC~HL"},
	{1,"INC~L"},     {1,"DEC~L"},     {2,"LD~L,%b"},     {1,"CPL"},
	{2,"JR~NC,%r"},  {3,"LD~SP,%w"},  {3,"LD~(%a),A"},   {1,"INC~SP"},
	{1,"INC~(HL)"},  {1,"DEC~(HL)"},  {2,"LD~(HL),%b"},  {1,"SCF"},
	{2,"JR~C,%r"},   {1,"ADD~HL,SP"}, {3,"LD~A,(%a)"},   {1,"DEC~SP"},
	{1,"INC~A"},     {1,"DEC~A"},     {2,"LD~A,%b"},     {1,"CCF"},

	{1,"LD~B,B"},    {1,"LD~B,C"},    {1,"LD~B,D"},    {1,"LD~B,E"},
	{1,"LD~B,H"},    {1,"LD~B,L"},    {1,"LD~B,(HL)"}, {1,"LD~B,A"},
	{1,"LD~C,B"},    {1,"LD~C,C"},    {1,"LD~C,D"},    {1,"LD~C,E"},
	{1,"LD~C,H"},    {1,"LD~C,L"},    {1,"LD~C,(HL)"}, {1,"LD~C,A"},
	{1,"LD~D,B"},    {1,"LD~D,C"},    {1,"LD~D,D"},    {1,"LD~D,E"},
	{1,"LD~D,H"},    {1,"LD~D,L"},    {1,"LD~D,(HL)"}, {1,"LD~D,A"},
	{1,"LD~E,B"},    {1,"LD~E,C"},    {1,"LD~E,D"},    {1,"LD~E,E"},
	{1,"LD~E,H"},    {1,"LD~E,L"},    {1,"LD~E,(HL)"}, {1,"LD~E,A"},
	{1,"LD~H,B"},    {1,"LD~H,C"},    {1,"LD~H,D"},    {1,"LD~H,E"},
	{1,"LD~H,H"},    {1,"LD~H,L"},    {1,"LD~H,(HL)"}, {1,"LD~H,A"},
	{1,"LD~L,B"},    {1,"LD~L,C"},    {1,"LD~L,D"},    {1,"LD~L,E"},
	{1,"LD~L,H"},    {1,"LD~L,L"},    {1,"LD~L,(HL)"}, {1,"LD~L,A"},
	{1,"LD~(HL),B"}, {1,"LD~(HL),C"}, {1,"LD~(HL),D"}, {1,"LD~(HL),E"},
	{1,"LD~(HL),H"}, {1,"LD~(HL),L"}, {1,"HALT"},      {1,"LD~(HL),A"},
	{1,"LD~A,B"},    {1,"LD~A,C"},    {1,"LD~A,D"},    {1,"LD~A,E"},
	{1,"LD~A,H"},    {1,"LD~A,L"},    {1,"LD~A,(HL)"}, {1,"LD~A,A"},

	{1,"ADD~A,B"}, {1,"ADD~A,C"}, {1,"ADD~A,D"},    {1,"ADD~A,E"},
	{1,"ADD~A,H"}, {1,"ADD~A,L"}, {1,"ADD~A,(HL)"}, {1,"ADD~A,A"},
	{1,"ADC~A,B"}, {1,"ADC~A,C"}, {1,"ADC~A,D"},    {1,"ADC~A,E"},
	{1,"ADC~A,H"}, {1,"ADC~A,L"}, {1,"ADC~A,(HL)"}, {1,"ADC~A,A"},
	{1,"SUB~B"},   {1,"SUB~C"},   {1,"SUB~D"},      {1,"SUB~E"},
	{1,"SUB~H"},   {1,"SUB~L"},   {1,"SUB~(HL)"},   {1,"SUB~A"},
	{1,"SBC~A,B"}, {1,"SBC~A,C"}, {1,"SBC~A,D"},    {1,"SBC~A,E"},
	{1,"SBC~A,H"}, {1,"SBC~A,L"}, {1,"SBC~A,(HL)"}, {1,"SBC~A,A"},
	{1,"AND~B"},   {1,"AND~C"},   {1,"AND~D"},      {1,"AND~E"},
	{1,"AND~H"},   {1,"AND~L"},   {1,"AND~(HL)"},   {1,"AND~A"},
	{1,"XOR~B"},   {1,"XOR~C"},   {1,"XOR~D"},      {1,"XOR~E"},
	{1,"XOR~H"},   {1,"XOR~L"},   {1,"XOR~(HL)"},   {1,"XOR~A"},
	{1,"OR~B"},    {1,"OR~C"},    {1,"OR~D"},       {1,"OR~E"},
	{1,"OR~H"},    {1,"OR~L"},    {1,"OR~(HL)"},    {1,"OR~A"},
	{1,"CP~B"},    {1,"CP~C"},    {1,"CP~D"},       {1,"CP~E"},
	{1,"CP~H"},    {1,"CP~L"},    {1,"CP~(HL)"},    {1,"CP~A"},

	{1,"RET~NZ"},      {1,"POP~BC"},   {3,"JP~NZ,%j"},  {3,"JP~%j"},
	{3,"CALL~NZ,%j"},  {1,"PUSH~BC"},  {2,"ADD~A,%b"},  {1,"RST~%z"},
	{1,"RET~Z"},       {1,"RET"},      {3,"JP~Z,%j"},   {1,0},
	{3,"CALL~Z,%j"},   {3,"CALL~%j"},  {2,"ADC~A,%b"},  {1,"RST~%z"},
	{1,"RET~NC"},      {1,"POP~DE"},   {3,"JP~NC,%j"},  {2,"OUT~(%b),A"},
	{3,"CALL~NC,%j"},  {1,"PUSH~DE"},  {2,"SUB~%b"},    {1,"RST~%z"},
	{1,"RET~C"},       {1,"EXX"},      {3,"JP~C,%j"},   {2,"IN~A,(%b)"},
	{3,"CALL~C,%j"},   {1,0},          {2,"SBC~A,%b"},  {1,"RST~%z"},
	{1,"RET~PO"},      {1,"POP~HL"},   {3,"JP~PO,%j"},  {1,"EX~(SP),HL"},
	{3,"CALL~PO,%j"},  {1,"PUSH~HL"},  {2,"AND~%b"},    {1,"RST~%z"},
	{1,"RET~PE"},      {1,"JP~(HL)"},  {3,"JP~PE,%j"},  {1,"EX~DE,HL"},
	{3,"CALL~PE,%j"},  {1,0},          {2,"XOR~%b"},    {1,"RST~%z"},
	{1,"RET~P"},       {1,"POP~AF"},   {3,"JP~P,%j"},   {1,"DI"},
	{3,"CALL~P,%j"},   {1,"PUSH~AF"},  {2,"OR~%b"},     {1,"RST~%z"},
	{1,"RET~M"},       {1,"LD~SP,HL"}, {3,"JP~M,%j"},   {1,"EI"},
	{3,"CALL~M,%j"},   {1,0},          {2,"CP~%b"},     {1,"RST~%z"}};

static const TilemDisasmInstruction insts_ddfd[256] = {
	{1,0},            {1,0},            {1,0},              {1,0},
	{1,0},            {1,0},            {1,0},              {1,0},
	{1,0},            {2,"ADD~%i,BC"},  {1,0},              {1,0},
	{1,0},            {1,0},            {1,0},              {1,0},
	{1,0},            {1,0},            {1,0},              {1,0},
	{1,0},            {1,0},            {1,0},              {1,0},
	{1,0},            {2,"ADD~%i,DE"},  {1,0},              {1,0},
	{1,0},            {1,0},            {1,0},              {1,0},
	{1,0},            {4,"LD~%i,%w"},   {4,"LD~(%a),%i"},   {2,"INC~%i"},
	{2,"INC~%iH"},    {2,"DEC~%iH"},    {3,"LD~%iH,%b"},    {1,0},
	{1,0},            {2,"ADD~%i,%i"},  {4,"LD~%i,(%a)"},   {2,"DEC~%i"},
	{2,"INC~%iL"},    {2,"DEC~%iL"},    {3,"LD~%iL,%b"},    {1,0},
	{1,0},            {1,0},            {1,0},              {1,0},
	{3,"INC~(%i%s)"}, {3,"DEC~(%i%s)"}, {4,"LD~(%i%s),%b"}, {1,0},
	{1,0},            {2,"ADD~%i,SP"},  {1,0},              {1,0},
	{1,0},            {1,0},            {1,0},              {1,0},

	{1,0},             {1,0},             {1,0},             {1,0},
	{2,"LD~B,%iH"},    {2,"LD~B,%iL"},    {3,"LD~B,(%i%s)"}, {1,0},
	{1,0},             {1,0},             {1,0},             {1,0},
	{2,"LD~C,%iH"},    {2,"LD~C,%iL"},    {3,"LD~C,(%i%s)"}, {1,0},
	{1,0},             {1,0},             {1,0},             {1,0},
	{2,"LD~D,%iH"},    {2,"LD~D,%iL"},    {3,"LD~D,(%i%s)"}, {1,0},
	{1,0},             {1,0},             {1,0},             {1,0},
	{2,"LD~E,%iH"},    {2,"LD~E,%iL"},    {3,"LD~E,(%i%s)"}, {1,0},
	{2,"LD~%iH,B"},    {2,"LD~%iH,C"},    {2,"LD~%iH,D"},    {2,"LD~%iH,E"},
	{2,"LD~%iH,%iH"},  {2,"LD~%iH,%iL"},  {3,"LD~H,(%i%s)"}, {2,"LD~%iH,A"},
	{2,"LD~%iL,B"},    {2,"LD~%iL,C"},    {2,"LD~%iL,D"},    {2,"LD~%iL,E"},
	{2,"LD~%iL,%iH"},  {2,"LD~%iL,%iL"},  {3,"LD~L,(%i%s)"}, {2,"LD~%iL,A"},
	{3,"LD~(%i%s),B"}, {3,"LD~(%i%s),C"}, {3,"LD~(%i%s),D"}, {3,"LD~(%i%s),E"},
	{3,"LD~(%i%s),H"}, {3,"LD~(%i%s),L"}, {1,0},             {3,"LD~(%i%s),A"},
	{1,0},             {1,0},             {1,0},             {1,0},
	{2,"LD~A,%iH"},    {2,"LD~A,%iL"},    {3,"LD~A,(%i%s)"}, {1,0},

	{1,0},           {1,0},           {1,0},              {1,0},
	{2,"ADD~A,%iH"}, {2,"ADD~A,%iL"}, {3,"ADD~A,(%i%s)"}, {1,0},
	{1,0},           {1,0},           {1,0},              {1,0},
	{2,"ADC~A,%iH"}, {2,"ADC~A,%iL"}, {3,"ADC~A,(%i%s)"}, {1,0},
	{1,0},           {1,0},           {1,0},              {1,0},
	{2,"SUB~%iH"},   {2,"SUB~%iL"},   {3,"SUB~(%i%s)"},   {1,0},
	{1,0},           {1,0},           {1,0},              {1,0},
	{2,"SBC~A,%iH"}, {2,"SBC~A,%iL"}, {3,"SBC~A,(%i%s)"}, {1,0},
	{1,0},           {1,0},           {1,0},              {1,0},
	{2,"AND~%iH"},   {2,"AND~%iL"},   {3,"AND~(%i%s)"},   {1,0},
	{1,0},           {1,0},           {1,0},              {1,0},
	{2,"XOR~%iH"},   {2,"XOR~%iL"},   {3,"XOR~(%i%s)"},   {1,0},
	{1,0},           {1,0},           {1,0},              {1,0},
	{2,"OR~%iH"},    {2,"OR~%iL"},    {3,"OR~(%i%s)"},    {1,0},
	{1,0},           {1,0},           {1,0},              {1,0},
	{2,"CP~%iH"},    {2,"CP~%iL"},    {3,"CP~(%i%s)"},    {1,0},

	{1,0}, {1,0},          {1,0}, {1,0},
	{1,0}, {1,0},          {1,0}, {1,0},
	{1,0}, {1,0},          {1,0}, {1,0},
	{1,0}, {1,0},          {1,0}, {1,0},
	{1,0}, {1,0},          {1,0}, {1,0},
	{1,0}, {1,0},          {1,0}, {1,0},
	{1,0}, {1,0},          {1,0}, {1,0},
	{1,0}, {1,0},          {1,0}, {1,0},
	{1,0}, {2,"POP~%i"},   {1,0}, {2,"EX~(SP),%i"},
	{1,0}, {2,"PUSH~%i"},  {1,0}, {1,0},
	{1,0}, {2,"JP~(%i)"},  {1,0}, {1,0},
	{1,0}, {1,0},          {1,0}, {1,0},
	{1,0}, {1,0},          {1,0}, {1,0},
	{1,0}, {1,0},          {1,0}, {1,0},
	{1,0}, {2,"LD~SP,%i"}, {1,0}, {1,0},
	{1,0}, {1,0},          {1,0}, {1,0}};

static const TilemDisasmInstruction insts_cb[256] = {
	{2,"RLC~B"},  {2,"RLC~C"},  {2,"RLC~D"},     {2,"RLC~E"},
	{2,"RLC~H"},  {2,"RLC~L"},  {2,"RLC~(HL)"},  {2,"RLC~A"},
	{2,"RRC~B"},  {2,"RRC~C"},  {2,"RRC~D"},     {2,"RRC~E"},
	{2,"RRC~H"},  {2,"RRC~L"},  {2,"RRC~(HL)"},  {2,"RRC~A"},
	{2,"RL~B"},   {2,"RL~C"},   {2,"RL~D"},      {2,"RL~E"},
	{2,"RL~H"},   {2,"RL~L"},   {2,"RL~(HL)"},   {2,"RL~A"},
	{2,"RR~B"},   {2,"RR~C"},   {2,"RR~D"},      {2,"RR~E"},
	{2,"RR~H"},   {2,"RR~L"},   {2,"RR~(HL)"},   {2,"RR~A"},
	{2,"SLA~B"},  {2,"SLA~C"},  {2,"SLA~D"},     {2,"SLA~E"},
	{2,"SLA~H"},  {2,"SLA~L"},  {2,"SLA~(HL)"},  {2,"SLA~A"},
	{2,"SRA~B"},  {2,"SRA~C"},  {2,"SRA~D"},     {2,"SRA~E"},
	{2,"SRA~H"},  {2,"SRA~L"},  {2,"SRA~(HL)"},  {2,"SRA~A"},
	{2,"SLIA~B"}, {2,"SLIA~C"}, {2,"SLIA~D"},    {2,"SLIA~E"},
	{2,"SLIA~H"}, {2,"SLIA~L"}, {2,"SLIA~(HL)"}, {2,"SLIA~A"},
	{2,"SRL~B"},  {2,"SRL~C"},  {2,"SRL~D"},     {2,"SRL~E"},
	{2,"SRL~H"},  {2,"SRL~L"},  {2,"SRL~(HL)"},  {2,"SRL~A"},

	{2,"BIT~0,B"}, {2,"BIT~0,C"}, {2,"BIT~0,D"},    {2,"BIT~0,E"},
	{2,"BIT~0,H"}, {2,"BIT~0,L"}, {2,"BIT~0,(HL)"}, {2,"BIT~0,A"},
	{2,"BIT~1,B"}, {2,"BIT~1,C"}, {2,"BIT~1,D"},    {2,"BIT~1,E"},
	{2,"BIT~1,H"}, {2,"BIT~1,L"}, {2,"BIT~1,(HL)"}, {2,"BIT~1,A"},
	{2,"BIT~2,B"}, {2,"BIT~2,C"}, {2,"BIT~2,D"},    {2,"BIT~2,E"},
	{2,"BIT~2,H"}, {2,"BIT~2,L"}, {2,"BIT~2,(HL)"}, {2,"BIT~2,A"},
	{2,"BIT~3,B"}, {2,"BIT~3,C"}, {2,"BIT~3,D"},    {2,"BIT~3,E"},
	{2,"BIT~3,H"}, {2,"BIT~3,L"}, {2,"BIT~3,(HL)"}, {2,"BIT~3,A"},
	{2,"BIT~4,B"}, {2,"BIT~4,C"}, {2,"BIT~4,D"},    {2,"BIT~4,E"},
	{2,"BIT~4,H"}, {2,"BIT~4,L"}, {2,"BIT~4,(HL)"}, {2,"BIT~4,A"},
	{2,"BIT~5,B"}, {2,"BIT~5,C"}, {2,"BIT~5,D"},    {2,"BIT~5,E"},
	{2,"BIT~5,H"}, {2,"BIT~5,L"}, {2,"BIT~5,(HL)"}, {2,"BIT~5,A"},
	{2,"BIT~6,B"}, {2,"BIT~6,C"}, {2,"BIT~6,D"},    {2,"BIT~6,E"},
	{2,"BIT~6,H"}, {2,"BIT~6,L"}, {2,"BIT~6,(HL)"}, {2,"BIT~6,A"},
	{2,"BIT~7,B"}, {2,"BIT~7,C"}, {2,"BIT~7,D"},    {2,"BIT~7,E"},
	{2,"BIT~7,H"}, {2,"BIT~7,L"}, {2,"BIT~7,(HL)"}, {2,"BIT~7,A"},

	{2,"RES~0,B"}, {2,"RES~0,C"}, {2,"RES~0,D"},    {2,"RES~0,E"},
	{2,"RES~0,H"}, {2,"RES~0,L"}, {2,"RES~0,(HL)"}, {2,"RES~0,A"},
	{2,"RES~1,B"}, {2,"RES~1,C"}, {2,"RES~1,D"},    {2,"RES~1,E"},
	{2,"RES~1,H"}, {2,"RES~1,L"}, {2,"RES~1,(HL)"}, {2,"RES~1,A"},
	{2,"RES~2,B"}, {2,"RES~2,C"}, {2,"RES~2,D"},    {2,"RES~2,E"},
	{2,"RES~2,H"}, {2,"RES~2,L"}, {2,"RES~2,(HL)"}, {2,"RES~2,A"},
	{2,"RES~3,B"}, {2,"RES~3,C"}, {2,"RES~3,D"},    {2,"RES~3,E"},
	{2,"RES~3,H"}, {2,"RES~3,L"}, {2,"RES~3,(HL)"}, {2,"RES~3,A"},
	{2,"RES~4,B"}, {2,"RES~4,C"}, {2,"RES~4,D"},    {2,"RES~4,E"},
	{2,"RES~4,H"}, {2,"RES~4,L"}, {2,"RES~4,(HL)"}, {2,"RES~4,A"},
	{2,"RES~5,B"}, {2,"RES~5,C"}, {2,"RES~5,D"},    {2,"RES~5,E"},
	{2,"RES~5,H"}, {2,"RES~5,L"}, {2,"RES~5,(HL)"}, {2,"RES~5,A"},
	{2,"RES~6,B"}, {2,"RES~6,C"}, {2,"RES~6,D"},    {2,"RES~6,E"},
	{2,"RES~6,H"}, {2,"RES~6,L"}, {2,"RES~6,(HL)"}, {2,"RES~6,A"},
	{2,"RES~7,B"}, {2,"RES~7,C"}, {2,"RES~7,D"},    {2,"RES~7,E"},
	{2,"RES~7,H"}, {2,"RES~7,L"}, {2,"RES~7,(HL)"}, {2,"RES~7,A"},

	{2,"SET~0,B"}, {2,"SET~0,C"}, {2,"SET~0,D"},    {2,"SET~0,E"},
	{2,"SET~0,H"}, {2,"SET~0,L"}, {2,"SET~0,(HL)"}, {2,"SET~0,A"},
	{2,"SET~1,B"}, {2,"SET~1,C"}, {2,"SET~1,D"},    {2,"SET~1,E"},
	{2,"SET~1,H"}, {2,"SET~1,L"}, {2,"SET~1,(HL)"}, {2,"SET~1,A"},
	{2,"SET~2,B"}, {2,"SET~2,C"}, {2,"SET~2,D"},    {2,"SET~2,E"},
	{2,"SET~2,H"}, {2,"SET~2,L"}, {2,"SET~2,(HL)"}, {2,"SET~2,A"},
	{2,"SET~3,B"}, {2,"SET~3,C"}, {2,"SET~3,D"},    {2,"SET~3,E"},
	{2,"SET~3,H"}, {2,"SET~3,L"}, {2,"SET~3,(HL)"}, {2,"SET~3,A"},
	{2,"SET~4,B"}, {2,"SET~4,C"}, {2,"SET~4,D"},    {2,"SET~4,E"},
	{2,"SET~4,H"}, {2,"SET~4,L"}, {2,"SET~4,(HL)"}, {2,"SET~4,A"},
	{2,"SET~5,B"}, {2,"SET~5,C"}, {2,"SET~5,D"},    {2,"SET~5,E"},
	{2,"SET~5,H"}, {2,"SET~5,L"}, {2,"SET~5,(HL)"}, {2,"SET~5,A"},
	{2,"SET~6,B"}, {2,"SET~6,C"}, {2,"SET~6,D"},    {2,"SET~6,E"},
	{2,"SET~6,H"}, {2,"SET~6,L"}, {2,"SET~6,(HL)"}, {2,"SET~6,A"},
	{2,"SET~7,B"}, {2,"SET~7,C"}, {2,"SET~7,D"},    {2,"SET~7,E"},
	{2,"SET~7,H"}, {2,"SET~7,L"}, {2,"SET~7,(HL)"}, {2,"SET~7,A"}};

static const TilemDisasmInstruction insts_ddfdcb[256] = {
	{4,"RLC~B,(%i%s)"},  {4,"RLC~C,(%i%s)"},  {4,"RLC~D,(%i%s)"},  {4,"RLC~E,(%i%s)"},
	{4,"RLC~H,(%i%s)"},  {4,"RLC~L,(%i%s)"},  {4,"RLC~(%i%s)"},    {4,"RLC~A,(%i%s)"},
	{4,"RRC~B,(%i%s)"},  {4,"RRC~C,(%i%s)"},  {4,"RRC~D,(%i%s)"},  {4,"RRC~E,(%i%s)"},
	{4,"RRC~H,(%i%s)"},  {4,"RRC~L,(%i%s)"},  {4,"RRC~(%i%s)"},    {4,"RRC~A,(%i%s)"},
	{4,"RL~B,(%i%s)"},   {4,"RL~C,(%i%s)"},   {4,"RL~D,(%i%s)"},   {4,"RL~E,(%i%s)"},
	{4,"RL~H,(%i%s)"},   {4,"RL~L,(%i%s)"},   {4,"RL~(%i%s)"},     {4,"RL~A,(%i%s)"},
	{4,"RR~B,(%i%s)"},   {4,"RR~C,(%i%s)"},   {4,"RR~D,(%i%s)"},   {4,"RR~E,(%i%s)"},
	{4,"RR~H,(%i%s)"},   {4,"RR~L,(%i%s)"},   {4,"RR~(%i%s)"},     {4,"RR~A,(%i%s)"},
	{4,"SLA~B,(%i%s)"},  {4,"SLA~C,(%i%s)"},  {4,"SLA~D,(%i%s)"},  {4,"SLA~E,(%i%s)"},
	{4,"SLA~H,(%i%s)"},  {4,"SLA~L,(%i%s)"},  {4,"SLA~(%i%s)"},    {4,"SLA~A,(%i%s)"},
	{4,"SRA~B,(%i%s)"},  {4,"SRA~C,(%i%s)"},  {4,"SRA~D,(%i%s)"},  {4,"SRA~E,(%i%s)"},
	{4,"SRA~H,(%i%s)"},  {4,"SRA~L,(%i%s)"},  {4,"SRA~(%i%s)"},    {4,"SRA~A,(%i%s)"},
	{4,"SLIA~B,(%i%s)"}, {4,"SLIA~C,(%i%s)"}, {4,"SLIA~D,(%i%s)"}, {4,"SLIA~E,(%i%s)"},
	{4,"SLIA~H,(%i%s)"}, {4,"SLIA~L,(%i%s)"}, {4,"SLIA~(%i%s)"},   {4,"SLIA~A,(%i%s)"},
	{4,"SRL~B,(%i%s)"},  {4,"SRL~C,(%i%s)"},  {4,"SRL~D,(%i%s)"},  {4,"SRL~E,(%i%s)"},
	{4,"SRL~H,(%i%s)"},  {4,"SRL~L,(%i%s)"},  {4,"SRL~(%i%s)"},    {4,"SRL~A,(%i%s)"},

	{4,"BIT~%f*"}, {4,"BIT~%f*"}, {4,"BIT~%f*"}, {4,"BIT~%f*"},
	{4,"BIT~%f*"}, {4,"BIT~%f*"}, {4,"BIT~%f"},  {4,"BIT~%f*"},
	{4,"BIT~%f*"}, {4,"BIT~%f*"}, {4,"BIT~%f*"}, {4,"BIT~%f*"},
	{4,"BIT~%f*"}, {4,"BIT~%f*"}, {4,"BIT~%f"},  {4,"BIT~%f*"},
	{4,"BIT~%f*"}, {4,"BIT~%f*"}, {4,"BIT~%f*"}, {4,"BIT~%f*"},
	{4,"BIT~%f*"}, {4,"BIT~%f*"}, {4,"BIT~%f"},  {4,"BIT~%f*"},
	{4,"BIT~%f*"}, {4,"BIT~%f*"}, {4,"BIT~%f*"}, {4,"BIT~%f*"},
	{4,"BIT~%f*"}, {4,"BIT~%f*"}, {4,"BIT~%f"},  {4,"BIT~%f*"},
	{4,"BIT~%f*"}, {4,"BIT~%f*"}, {4,"BIT~%f*"}, {4,"BIT~%f*"},
	{4,"BIT~%f*"}, {4,"BIT~%f*"}, {4,"BIT~%f"},  {4,"BIT~%f*"},
	{4,"BIT~%f*"}, {4,"BIT~%f*"}, {4,"BIT~%f*"}, {4,"BIT~%f*"},
	{4,"BIT~%f*"}, {4,"BIT~%f*"}, {4,"BIT~%f"},  {4,"BIT~%f*"},
	{4,"BIT~%f*"}, {4,"BIT~%f*"}, {4,"BIT~%f*"}, {4,"BIT~%f*"},
	{4,"BIT~%f*"}, {4,"BIT~%f*"}, {4,"BIT~%f"},  {4,"BIT~%f*"},
	{4,"BIT~%f*"}, {4,"BIT~%f*"}, {4,"BIT~%f*"}, {4,"BIT~%f*"},
	{4,"BIT~%f*"}, {4,"BIT~%f*"}, {4,"BIT~%f"},  {4,"BIT~%f*"},

	{4,"RES~0,B,(%i%s)"}, {4,"RES~0,C,(%i%s)"}, {4,"RES~0,D,(%i%s)"}, {4,"RES~0,E,(%i%s)"},
	{4,"RES~0,H,(%i%s)"}, {4,"RES~0,L,(%i%s)"}, {4,"RES~%f"},         {4,"RES~0,A,(%i%s)"},
	{4,"RES~1,B,(%i%s)"}, {4,"RES~1,C,(%i%s)"}, {4,"RES~1,D,(%i%s)"}, {4,"RES~1,E,(%i%s)"},
	{4,"RES~1,H,(%i%s)"}, {4,"RES~1,L,(%i%s)"}, {4,"RES~%f"},         {4,"RES~1,A,(%i%s)"},
	{4,"RES~2,B,(%i%s)"}, {4,"RES~2,C,(%i%s)"}, {4,"RES~2,D,(%i%s)"}, {4,"RES~2,E,(%i%s)"},
	{4,"RES~2,H,(%i%s)"}, {4,"RES~2,L,(%i%s)"}, {4,"RES~%f"},         {4,"RES~2,A,(%i%s)"},
	{4,"RES~3,B,(%i%s)"}, {4,"RES~3,C,(%i%s)"}, {4,"RES~3,D,(%i%s)"}, {4,"RES~3,E,(%i%s)"},
	{4,"RES~3,H,(%i%s)"}, {4,"RES~3,L,(%i%s)"}, {4,"RES~%f"},         {4,"RES~3,A,(%i%s)"},
	{4,"RES~4,B,(%i%s)"}, {4,"RES~4,C,(%i%s)"}, {4,"RES~4,D,(%i%s)"}, {4,"RES~4,E,(%i%s)"},
	{4,"RES~4,H,(%i%s)"}, {4,"RES~4,L,(%i%s)"}, {4,"RES~%f"},         {4,"RES~4,A,(%i%s)"},
	{4,"RES~5,B,(%i%s)"}, {4,"RES~5,C,(%i%s)"}, {4,"RES~5,D,(%i%s)"}, {4,"RES~5,E,(%i%s)"},
	{4,"RES~5,H,(%i%s)"}, {4,"RES~5,L,(%i%s)"}, {4,"RES~%f"},         {4,"RES~5,A,(%i%s)"},
	{4,"RES~6,B,(%i%s)"}, {4,"RES~6,C,(%i%s)"}, {4,"RES~6,D,(%i%s)"}, {4,"RES~6,E,(%i%s)"},
	{4,"RES~6,H,(%i%s)"}, {4,"RES~6,L,(%i%s)"}, {4,"RES~%f"},         {4,"RES~6,A,(%i%s)"},
	{4,"RES~7,B,(%i%s)"}, {4,"RES~7,C,(%i%s)"}, {4,"RES~7,D,(%i%s)"}, {4,"RES~7,E,(%i%s)"},
	{4,"RES~7,H,(%i%s)"}, {4,"RES~7,L,(%i%s)"}, {4,"RES~%f"},         {4,"RES~7,A,(%i%s)"},

	{4,"SET~0,B,(%i%s)"}, {4,"SET~0,C,(%i%s)"}, {4,"SET~0,D,(%i%s)"}, {4,"SET~0,E,(%i%s)"},
	{4,"SET~0,H,(%i%s)"}, {4,"SET~0,L,(%i%s)"}, {4,"SET~%f"},         {4,"SET~0,A,(%i%s)"},
	{4,"SET~1,B,(%i%s)"}, {4,"SET~1,C,(%i%s)"}, {4,"SET~1,D,(%i%s)"}, {4,"SET~1,E,(%i%s)"},
	{4,"SET~1,H,(%i%s)"}, {4,"SET~1,L,(%i%s)"}, {4,"SET~%f"},         {4,"SET~1,A,(%i%s)"},
	{4,"SET~2,B,(%i%s)"}, {4,"SET~2,C,(%i%s)"}, {4,"SET~2,D,(%i%s)"}, {4,"SET~2,E,(%i%s)"},
	{4,"SET~2,H,(%i%s)"}, {4,"SET~2,L,(%i%s)"}, {4,"SET~%f"},         {4,"SET~2,A,(%i%s)"},
	{4,"SET~3,B,(%i%s)"}, {4,"SET~3,C,(%i%s)"}, {4,"SET~3,D,(%i%s)"}, {4,"SET~3,E,(%i%s)"},
	{4,"SET~3,H,(%i%s)"}, {4,"SET~3,L,(%i%s)"}, {4,"SET~%f"},         {4,"SET~3,A,(%i%s)"},
	{4,"SET~4,B,(%i%s)"}, {4,"SET~4,C,(%i%s)"}, {4,"SET~4,D,(%i%s)"}, {4,"SET~4,E,(%i%s)"},
	{4,"SET~4,H,(%i%s)"}, {4,"SET~4,L,(%i%s)"}, {4,"SET~%f"},         {4,"SET~4,A,(%i%s)"},
	{4,"SET~5,B,(%i%s)"}, {4,"SET~5,C,(%i%s)"}, {4,"SET~5,D,(%i%s)"}, {4,"SET~5,E,(%i%s)"},
	{4,"SET~5,H,(%i%s)"}, {4,"SET~5,L,(%i%s)"}, {4,"SET~%f"},         {4,"SET~5,A,(%i%s)"},
	{4,"SET~6,B,(%i%s)"}, {4,"SET~6,C,(%i%s)"}, {4,"SET~6,D,(%i%s)"}, {4,"SET~6,E,(%i%s)"},
	{4,"SET~6,H,(%i%s)"}, {4,"SET~6,L,(%i%s)"}, {4,"SET~%f"},         {4,"SET~6,A,(%i%s)"},
	{4,"SET~7,B,(%i%s)"}, {4,"SET~7,C,(%i%s)"}, {4,"SET~7,D,(%i%s)"}, {4,"SET~7,E,(%i%s)"},
	{4,"SET~7,H,(%i%s)"}, {4,"SET~7,L,(%i%s)"}, {4,"SET~%f"},         {4,"SET~7,A,(%i%s)"}};

static const TilemDisasmInstruction insts_ed[256] = {
	{2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0},
	{2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0},
	{2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0},
	{2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0},
	{2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0},
	{2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0},
	{2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0},
	{2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0},

	{2,"IN~B,(C)"}, {2,"OUT~(C),B"}, {2,"SBC~HL,BC"}, {4,"LD~(%a),BC"},
	{2,"NEG"},      {2,"RETN"},      {2,"IM~0"},      {2,"LD~I,A"},
	{2,"IN~C,(C)"}, {2,"OUT~(C),C"}, {2,"ADC~HL,BC"}, {4,"LD~BC,(%a)"},
	{2,"NEG*"},     {2,"RETI"},      {2,"IM~0*"},     {2,"LD~R,A"},
	{2,"IN~D,(C)"}, {2,"OUT~(C),D"}, {2,"SBC~HL,DE"}, {4,"LD~(%a),DE"},
	{2,"NEG*"},     {2,"RETN*"},     {2,"IM~1"},      {2,"LD~A,I"},
	{2,"IN~E,(C)"}, {2,"OUT~(C),E"}, {2,"ADC~HL,DE"}, {4,"LD~DE,(%a)"},
	{2,"NEG*"},     {2,"RETN*"},     {2,"IM~2"},      {2,"LD~A,R"},
	{2,"IN~H,(C)"}, {2,"OUT~(C),H"}, {2,"SBC~HL,HL"}, {4,"LD~(%a),HL*"},
	{2,"NEG*"},     {2,"RETN*"},     {2,"IM~0*"},     {2,"RRD"},
	{2,"IN~L,(C)"}, {2,"OUT~(C),L"}, {2,"ADC~HL,HL"}, {4,"LD~HL,(%a)*"},
	{2,"NEG*"},     {2,"RETN*"},     {2,"IM~0*"},     {2,"RLD"},
	{2,"IN~(C)"},   {2,"OUT~(C),0"}, {2,"SBC~HL,SP"}, {4,"LD~(%a),SP"},
	{2,"NEG*"},     {2,"RETN*"},     {2,"IM~1*"},     {2,0},
	{2,"IN~A,(C)"}, {2,"OUT~(C),A"}, {2,"ADC~HL,SP"}, {4,"LD~SP,(%a)"},
	{2,"NEG*"},     {2,"RETN*"},     {2,"IM~2*"},     {2,0},

	{2,0},      {2,0},      {2,0},      {2,0},
	{2,0},      {2,0},      {2,0},      {2,0},
	{2,0},      {2,0},      {2,0},      {2,0},
	{2,0},      {2,0},      {2,0},      {2,0},
	{2,0},      {2,0},      {2,0},      {2,0},
	{2,0},      {2,0},      {2,0},      {2,0},
	{2,0},      {2,0},      {2,0},      {2,0},
	{2,0},      {2,0},      {2,0},      {2,0},
	{2,"LDI"},  {2,"CPI"},  {2,"INI"},  {2,"OUTI"},
	{2,0},      {2,0},      {2,0},      {2,0},
	{2,"LDD"},  {2,"CPD"},  {2,"IND"},  {2,"OUTD"},
	{2,0},      {2,0},      {2,0},      {2,0},
	{2,"LDIR"}, {2,"CPIR"}, {2,"INIR"}, {2,"OTIR"},
	{2,0},      {2,0},      {2,0},      {2,0},
	{2,"LDDR"}, {2,"CPDR"}, {2,"INDR"}, {2,"OTDR"},
	{2,0},      {2,0},      {2,0},      {2,0},

	{2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0},
	{2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0},
	{2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0},
	{2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0},
	{2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0},
	{2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0},
	{2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0},
	{2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0}};

/* Count number of bytes of arguments for a given instruction/macro
   pattern */
static int pattern_arg_size(const char* pattern)
{
	char* p;
	int count = 0, offs;

	while (*pattern) {
		if (*pattern != '%')
			pattern++;
		else {
			pattern++;

			if (*pattern >= '0' && *pattern <= '9') {
				offs = strtol(pattern, &p, 10);
				pattern = p;
			}
			else {
				offs = count;
			}

			switch (*pattern) {
			case 0:
				pattern--;
				break;

			case 'b':
			case 'C':
			case 'r':
			case 's':
				offs++;
				break;

			case 'a':
			case 'c':
			case 'f':
			case 'j':
			case 'w':
				offs += 2;
				break;
			}

			pattern++;
			if (offs > count)
				count = offs;
		}
	}

	return count;
}

static void get_instruction_info(const TilemDisasm* dasm,
				 const byte* instr,
				 int* length, int* argbase,
				 const char** pattern)
{
	const TilemDisasmSymbol* sym;
	const TilemDisasmInstruction* ii;
	dword mvalue;
	int i;

	mvalue = 0;
	for (i = 0; instr[0] && i < 4; i++) {
		mvalue = (mvalue << 8) | instr[i];
		if ((sym = find_symbol(&dasm->macros, mvalue))) {
			*pattern = sym->name;
			*length = i + 1 + pattern_arg_size(sym->name);
			*argbase = i + 1;
			return;
		}
	}

	if (instr[0] == 0xed) {
		ii = &insts_ed[instr[1]];
		*argbase = 2;
	}
	else if (instr[0] == 0xdd || instr[0] == 0xfd) {
		if (instr[1] == 0xcb) {
			ii = &insts_ddfdcb[instr[3]];
		}
		else {
			ii = &insts_ddfd[instr[1]];
		}
		*argbase = 2;
	}
	else if (instr[0] == 0xcb) {
		ii = &insts_cb[instr[1]];
		*argbase = 2;
	}
	else {
		ii = &insts_main[instr[0]];
		*argbase = 1;
	}

	*length = ii->length;

	if (ii->pattern) {
		*pattern = ii->pattern;
	}
	else {
		*argbase = 0;
		if (ii->length == 1)
			*pattern = "DB~%b";
		else
			*pattern = "DB~%b,%b";
	}
}

static void TILEM_ATTR_PRINTF(3, 4)
printv(char** buf, int* bsize, const char* fmt, ...)
{
	va_list ap;
	int n;

	if (*bsize == 0)
		return;

	va_start(ap, fmt);
	n = vsnprintf(*buf, *bsize, fmt, ap);
	va_end(ap);

	if (n >= *bsize) {
		*buf += *bsize - 1;
		**buf = 0;
		*bsize = 0;
	}
	else {
		*buf += n;
		**buf = 0;
		*bsize -= n;
	}
}

static void print_byte(char** buf, int* bsize, unsigned int b)
{
	printv(buf, bsize, "$%02X", b);
}

static void print_word(const TilemDisasm* dasm, char** buf, int* bsize,
		       dword w, int autonum, int autodiff)
{
	TilemDisasmSymbol* sym;

	if (autonum && w < 0x100) {
		printv(buf, bsize, "$%04X", w);
		return;
	}

	sym = find_prev_symbol(&dasm->labels, w);

	if (sym && !strcmp(sym->name, "flags")) {
		w -= sym->value;
		sym = find_symbol(&dasm->flags, w);
		if (sym) {
			printv(buf, bsize, "flags + %s", sym->name);
		}
		else {
			printv(buf, bsize, "flags + $%02X", w);
		}
	}
	else if (sym && w == sym->value) {
		printv(buf, bsize, "%s", sym->name);
	}
	else if (sym && autodiff && w > 0x8000 && w - sym->value < 64) {
		printv(buf, bsize, "%s + %d", sym->name, w - sym->value);
	}
	else {
		printv(buf, bsize, "$%04X", w);
	}
}

static void print_romcall(const TilemDisasm* dasm, char** buf, int* bsize,
			  dword w)
{
	TilemDisasmSymbol* sym;

	sym = find_symbol(&dasm->romcalls, w);
	if (sym) {
		printv(buf, bsize, "%s", sym->name);
	}
	else {
		printv(buf, bsize, "$%04X", w);
	}
}

static void print_flag(const TilemDisasm* dasm, char** buf, int* bsize,
		       unsigned int bit, unsigned int offset,
		       unsigned int prefix)
{
	TilemDisasmSymbol* sym;
	int i;

	if (prefix == 0xfd) {
		sym = find_symbol(&dasm->flags, 0x1000 + (offset << 4) + bit);
		if (sym) {
			for (i = 0; sym->name[i]; i++) {
				printv(buf, bsize, "%c", sym->name[i]);
				if (sym->name[i] == ',')
					printv(buf, bsize, " (IY + ");
			}
			printv(buf, bsize, ")");
			return;
		}

		sym = find_symbol(&dasm->flags, offset);
		if (sym) {
			printv(buf, bsize, "%d, (IY + %s)", bit, sym->name);
			return;
		}
	}

	printv(buf, bsize, "%d, (%s", bit, (prefix == 0xfd ? "IY" : "IX"));

	if (offset & 0x80) {
		printv(buf, bsize, " - $%02X", 0x100 - offset);
	}
	else if (offset) {
		printv(buf, bsize, " + $%02X", offset);
	}

	printv(buf, bsize, ")");
}

static void disassemble_pattern(const TilemDisasm* dasm, const char* pattern,
				const byte* ibuf, dword pc, int argbase,
				char** buf, int* bsize)
{
	int argidx, offs;
	char* p;
	dword w;
	TilemDisasmSymbol* sym;

	argidx = argbase;

	while (*bsize && *pattern) {
		if (*pattern == '~')
			printv(buf, bsize, "\t");
		else if (*pattern == ',') {
			printv(buf, bsize, ", ");
		}
		else if (*pattern != '%') {
			printv(buf, bsize, "%c", *pattern);
		}
		else {
			pattern++;
			if (*pattern >= '0' && *pattern <= '9') {
				offs = argbase + strtol(pattern, &p, 10);
				pattern = p;
			}
			else {
				offs = argidx;
			}

			switch (*pattern) {
			case 0:
				pattern--;
				break;

			case '%':
				printv(buf, bsize, "%%");
				break;

			case 'a':
				/* %a: word value, always an address */
				w = ibuf[offs] | (ibuf[offs + 1] << 8);
				print_word(dasm, buf, bsize, w, 0, 1);
				offs += 2;
				break;

			case 'b':
				/* %b: byte value */
				print_byte(buf, bsize, ibuf[offs]);
				offs++;
				break;

			case 'c':
				/* %c: word value, always a ROM call number */
				w = ibuf[offs] | (ibuf[offs + 1] << 8);
				print_romcall(dasm, buf, bsize, w);
				offs += 2;
				break;

			case 'C':
				/* %C: byte value, always a ROM call number */
				print_romcall(dasm, buf, bsize, ibuf[offs]);
				offs++;
				break;

			case 'f':
				/* %f: flag value */
				print_flag(dasm, buf, bsize,
					   (ibuf[offs + 1] >> 3) & 7,
					   ibuf[offs], ibuf[0]);
				offs += 2;
				break;

			case 'i':
				/* %i: IX or IY by instruction prefix */
				if (ibuf[0] == 0xdd)
					printv(buf, bsize, "IX");
				else
					printv(buf, bsize, "IY");
				break;

			case 'j':
				/* %j: word value, always a jump address */
				w = ibuf[offs] | (ibuf[offs + 1] << 8);
				print_word(dasm, buf, bsize, w, 0, 0);
				offs += 2;
				break;

			case 'r':
				/* %r: one-byte PC-relative value */
				if (ibuf[offs] & 0x80)
					w = pc + offs - 0xff + ibuf[offs];
				else
					w = pc + offs + 1 + ibuf[offs];
				print_word(dasm, buf, bsize, w, 0, 0);
				offs++;
				break;

			case 's':
				/* %s: one-byte signed displacement */
				if (ibuf[0] == 0xfd
				    && (sym = find_symbol(&dasm->flags,
							  ibuf[offs]))) {
					printv(buf, bsize, " + %s", sym->name);
				}
				else if (ibuf[offs] & 0x80) {
					printv(buf, bsize, " - ");
					print_byte(buf, bsize,
						   0x100 - ibuf[offs]);
				}
				else if (ibuf[offs]) {
					printv(buf, bsize, " + ");
					print_byte(buf, bsize, ibuf[offs]);
				}
				offs++;
				break;

			case 'w':
				/* %w: word value */
				w = ibuf[offs] | (ibuf[offs + 1] << 8);
				print_word(dasm, buf, bsize, w, 1, 1);
				offs += 2;
				break;
				
			case 'z':
				/* %z: RST target address */
				print_word(dasm, buf, bsize, ibuf[0] & 0x38,
					   0, 0);
				break;
			}
			if (offs > argidx)
				argidx = offs;
		}

		pattern++;
	}
}

void tilem_disasm_disassemble(const TilemDisasm* dasm, TilemCalc* calc,
			      int phys, dword addr, dword* nextaddr,
			      char* buffer, int bufsize)
{
	byte ibuf[64];
	dword a, addr_l, max;
	int length, argbase, i;
	const char* pattern;

	if (phys) {
		max = calc->hw.romsize + calc->hw.ramsize;
		for (i = 0; i < 4; i++) {
			a = (addr + i) % max;
			ibuf[i] = calc->mem[a];
		}

		addr_l = (*calc->hw.mem_ptol)(calc, addr);
		if (addr_l == 0xffffffff)
			addr_l = (addr & 0x3fff) | 0x4000;
	}
	else {
		max = 0x10000;
		for (i = 0; i < 4; i++) {
			a = (addr + i) & 0xffff;
			ibuf[i] = calc->mem[(*calc->hw.mem_ltop)(calc, a)];
		}

		addr_l = addr;
	}

	get_instruction_info(dasm, ibuf, &length, &argbase, &pattern);

	if (phys) {
		for (i = 0; i < length; i++) {
			ibuf[i] = calc->mem[addr];
			addr = (addr + 1) % max;
		}
	}
	else {
		for (i = 0; i < length; i++) {
			ibuf[i] = calc->mem[(*calc->hw.mem_ltop)(calc, addr)];
			addr = (addr + 1) & 0xffff;
		}
	}

	if (nextaddr)
		*nextaddr = addr;

	if (buffer) {
		disassemble_pattern(dasm, pattern, ibuf, addr_l, argbase,
				    &buffer, &bufsize);
	}
}
