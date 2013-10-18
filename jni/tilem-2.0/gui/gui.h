/*
 * TilEm II
 *
 * Copyright (c) 2010-2011 Thibault Duponchelle
 * Copyright (c) 2010-2012 Benjamin Moody
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

#include "animation.h"
#include "audiodev.h"
#include "emulator.h"
#include "skinops.h"
#include "emuwin.h"
#include "debugger.h"
#include "gettext.h"

#include "gtk-compat.h"

/* This struture is a wrapper for VarEntry with additionnal informations used by tilem */
typedef struct {
	int model;

	VarEntry *ve;   	/* Original variable info retrieved
	                	   from calculator */
	int slot;       	/* Slot number */

	/* Strings for display (UTF-8) */
	char *name_str; 	/* Variable name */
	char *type_str; 	/* Variable type */
	char *slot_str; 	/* Program slot */
	char *file_ext; 	/* Default file extension */
	char *filetype_desc; 	/* File format description */

	int size;            	/* Variable size */
	gboolean archived;   	/* Is archived */
	gboolean can_group;  	/* Can be stored in group file */

} TilemVarEntry;


#define LABEL_X_ALIGN 0.0


/* ###### event.c ##### */

/* Dialog mesg */
void show_about();



/* ##### tool.c ##### */

/* Get model name (abbreviation) for a TilEm model ID. */
const char * model_to_name(int model);

/* Convert model name to a model ID. */
int name_to_model(const char *name);

/* Convert TilEm model ID to tifiles2 model ID. */
CalcModel model_to_calcmodel(int model);

/* Convert tifiles2 model ID to TilEm model ID. */
int calcmodel_to_model(CalcModel model);

/* Get model ID for a given file. */
int file_to_model(const char *name);

/* Get "base" model for file type support. */
int model_to_base_model(int calc_model);

/* Check if calc is compatible with given file type. */
gboolean model_supports_file(int calc_model, int file_model);


/* Convert UTF-8 to filename encoding.  Use ASCII digits in place of
   subscripts if necessary.  If conversion fails utterly, fall back to
   the UTF-8 name, which is broken but better than nothing. */
char * utf8_to_filename(const char *utf8str);

/* Convert UTF-8 to a subset of UTF-8 that is compatible with the
   locale */
char * utf8_to_restricted_utf8(const char *utf8str);

/* Generate default filename (UTF-8) for a variable */
char * get_default_filename(const TilemVarEntry *tve);


/* ##### config.c ##### */

/* Retrieve settings from configuration file.  GROUP is the
   configuration group; following arguments are a series of OPTION
   strings, each followed by a pointer to a variable that will receive
   the value.  The list of options is terminated by NULL.

   Each OPTION is a string of the form "KEY/TYPE" or "KEY/TYPE=VALUE",
   where KEY is the name of the configuration property, and TYPE is
   either 'f' for a filename (char*), 's' for a UTF-8 string (char*),
   'i' for an integer (int), 'r' for a real number (double), or 'b'
   for a boolean (int).

   VALUE, if specified, is the default value for the option if it has
   not been defined by the user.  If no VALUE is specified, the option
   defaults to zero or NULL.

   Strings returned by this function must be freed by the caller
   (using g_free().) */
void tilem_config_get(const char *group, const char *option, ...)
	G_GNUC_NULL_TERMINATED;

/* Save settings to the configuration file.  Arguments are a series of
   option names, as above, each followed by the new value of the
   option.  The list is terminated by NULL. */
void tilem_config_set(const char *group, const char *option, ...)
	G_GNUC_NULL_TERMINATED;


/* ##### link.c ##### */

/* This structure is used to send a file (usually slot=-1, first=TRUE, last=TRUE)*/
struct TilemSendFileInfo {
	char *filename;
	char *display_name;
	int slot;
	int first;
	int last;
	char *error_message;
};

/* This structure is used to receive a file */
struct TilemReceiveFileInfo {
	GSList *entries;
	char* destination;
	char *error_message;
	gboolean output_tig;
};

/* Copy a TilemVarEntry structure */
TilemVarEntry *tilem_var_entry_copy(const TilemVarEntry *tve);

/* Free a previous allocated TilemVarEntry */
void tilem_var_entry_free(TilemVarEntry *tve);

/* Send a file to the calculator through the GUI.  SLOT is the
   destination program slot (for TI-81.)  FIRST must be true if this
   is the first variable in a series; LAST must be true if this is the
   last in a series. */
int tilem_link_send_file(TilemCalcEmulator *emu, const char *filename,
                          int slot, gboolean first, gboolean last);

/* The effective send file function. If there's no good reason, use tilem_link_send_file instead. */
gboolean send_file_main(TilemCalcEmulator *emu, gpointer data);

/* Request directory listing. */
void tilem_link_get_dirlist(TilemCalcEmulator *emu);

/* Get the calc model as needed by ticalcs functions */
int get_calc_model(TilemCalc *calc);

/* Show error */
void show_error(TilemCalcEmulator *emu, const char *title, const char *message);

/* Receive a variable and write it to a file. */
void tilem_link_receive_file(TilemCalcEmulator *emu,
                             const TilemVarEntry* varentry,
                             const char* destination);

/* Receive a list of variables (GSList of TilemVarEntries) and save
   them to a group file. */
void tilem_link_receive_group(TilemCalcEmulator *emu,
                              GSList *entries,
                              const char *destination);

/* Receive variables with names matching a pattern.  PATTERN is a
   glob-like pattern in UTF-8.  Files will be written out to
   DESTDIR. */
void tilem_link_receive_matching(TilemCalcEmulator *emu,
                                 const char *pattern,
                                 const char *destdir);


/* ##### pbar.c ##### */

/* Create or update the progress bar */
void progress_bar_update(TilemCalcEmulator* emu);



/* ##### animatedgif.c ##### */

/* Save a TilemAnimation to a GIF file. */
void tilem_animation_write_gif(TilemAnimation *anim, byte* palette, int palette_size, FILE *fp);


/* ##### gifencod.c ##### */

/* Encode gif data */
void GifEncode(FILE *fout, unsigned char *pixels, int depth, int siz);



