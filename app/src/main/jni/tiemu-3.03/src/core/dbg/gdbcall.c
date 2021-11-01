/* Hey EMACS -*- linux-c -*- */
/* $Id: gdbcall.c 2211 2006-08-16 20:02:42Z kevinkofler $ */

/*  TiEmu - a TI emulator
 *
 *  This file Copyright (c) 2005, Kevin Kofler
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
    Gdbcall: GDB interfacing functions
*/

#ifndef NO_GDB

#include "gdbcall.h"

#include <stdio.h>
#include <string.h>

/* These come from GDB's exceptions.h, which I don't want to include, because it
   pulls in ui-out.h, which in turn won't compile without many other headers
   from directories not even in our include path. */
#define RETURN_MASK_ALL -2
typedef int return_mask;
typedef void (catch_command_errors_ftype) (char *, int);
extern int catch_command_errors (catch_command_errors_ftype *func, char *command, int from_tty, return_mask);
struct ui_file;
struct cmd_list_element;
#include "../gdb/gdb/top.h"

/* This is for Insight. */
extern int No_Update;

static void gdbcall_exec_command(char *command_str)
{
  No_Update = 0; /* Tell Insight to refresh itself. */
  catch_command_errors (execute_command, command_str, 0, RETURN_MASK_ALL); 
}

void gdbcall_run(void)
{
  char command[4]="run";
  gdbcall_exec_command(command);
}

void gdbcall_continue(void)
{
  char command[2]="c";
  gdbcall_exec_command(command);
}

void gdb_add_symbol_file(const char *filename, unsigned address)
{
  char command[(strlen(filename) << 1) + 28], *p=command+17;
  const char *q=filename;
  strcpy(command, "add-symbol-file \"");
  while (*q) {
    if (*q == '\\' || *q == '\'' || *q == '\"') *(p++)='\\';
    *(p++)=*(q++);
  }
  sprintf(p, "\" %u", address);
  gdbcall_exec_command(command);
}

void gdb_hbreak(const char *funcname)
{
  char command[strlen(funcname) + 8];
  sprintf(command, "hbreak %s", funcname);
  gdbcall_exec_command(command);
}

#endif
