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

enum {
	TI81_SLOT_AUTO = -1,
	TI81_SLOT_0, TI81_SLOT_1, TI81_SLOT_2, TI81_SLOT_3, TI81_SLOT_4,
	TI81_SLOT_5, TI81_SLOT_6, TI81_SLOT_7, TI81_SLOT_8, TI81_SLOT_9,
	TI81_SLOT_A, TI81_SLOT_B, TI81_SLOT_C, TI81_SLOT_D, TI81_SLOT_E,
	TI81_SLOT_F, TI81_SLOT_G, TI81_SLOT_H, TI81_SLOT_I, TI81_SLOT_J,
	TI81_SLOT_K, TI81_SLOT_L, TI81_SLOT_M, TI81_SLOT_N, TI81_SLOT_O,
	TI81_SLOT_P, TI81_SLOT_Q, TI81_SLOT_R, TI81_SLOT_S, TI81_SLOT_T,
	TI81_SLOT_U, TI81_SLOT_V, TI81_SLOT_W, TI81_SLOT_X, TI81_SLOT_Y,
	TI81_SLOT_Z, TI81_SLOT_THETA
};

#define TI81_SLOT_MAX TI81_SLOT_THETA

typedef struct _TI81ProgInfo {
	int slot;               /* program slot number */
	int size;               /* size of program contents */
	dword addr;		/* address of program contents */
	byte name[8];           /* program name, tokens */
} TI81ProgInfo;

typedef struct _TI81Program {
	TI81ProgInfo info;
	byte *data;
} TI81Program;

/* Error codes */
enum {
	TI81_ERR_FILE_IO = 1,       /* File I/O error */
	TI81_ERR_INVALID_FILE,      /* PRG file is invalid */
	TI81_ERR_MEMORY,            /* Not enough memory to load program */
	TI81_ERR_SLOTS_FULL,        /* No free program slots */
	TI81_ERR_BUSY,              /* Calculator is busy and unable
	                               to load/save programs */
	TI81_ERR_INTERNAL
};

/* Create a new TI81Program with the given size. */
TI81Program * ti81_program_new(int size)
	TILEM_ATTR_MALLOC;

/* Free a TI81Program. */
void ti81_program_free(TI81Program *prgm);

/* Get information about the program in the given slot. */
int ti81_get_program_info(const TilemCalc *calc, int slot, TI81ProgInfo *info);

/* Retrieve a program from calculator memory.  Free the resulting
   program with ti81_program_free() when you're done with it. */
int ti81_get_program(const TilemCalc *calc, int slot, TI81Program **prgm);

/* Load a program into calculator memory. */
int ti81_load_program(TilemCalc *calc, const TI81Program *prgm);

/* Read a program from a PRG file.  Free the resulting program with
   ti81_program_free() when you're done with it. */
int ti81_read_prg_file(FILE *f, TI81Program **prgm);

/* Write a program to a PRG file. */
int ti81_write_prg_file(FILE *f, const TI81Program *prgm);

/* Convert program slot number into a UTF-8 string.  Free the result
   with tilem_free() when you're done with it. */
char * ti81_program_slot_to_string(int slot)
	TILEM_ATTR_MALLOC;

/* Convert program name to a UTF-8 string.  Free the result with
   tilem_free() when you're done with it. */
char * ti81_program_name_to_string(const byte *prgname)
	TILEM_ATTR_MALLOC;
