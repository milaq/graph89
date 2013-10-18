/*
 * TilEm II
 *
 * Copyright (c) 2010-2012 Thibault Duponchelle
 * Copyright (c) 2012 Benjamin Moody
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
 *
 */
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <ticalcs.h>
#include <tilem.h>

#include "gui.h"
#include "filedlg.h"
#include "emucore.h"


/* Allocate a new TilemMacro structure which is empty */
static TilemMacro* tilem_macro_new() {
	TilemMacro *macro = g_new(TilemMacro, 1);
	macro->n = 0;
	macro->actions = NULL;
	return macro;
}

/* New or renew the table of actions (each TilemMacroAtom is an action) */
static TilemMacroAtom** tilem_macro_actions_new(TilemMacro *macro, int n) {
	TilemMacroAtom **atom; 

	if(n == 0) {
		atom =  g_new(TilemMacroAtom*, n);
	} else {
		atom =  g_renew(TilemMacroAtom*, macro->actions, n);
	}
	return atom;
}

/* Try to destroy the TilemMacro if really allocated */
static void tilem_macro_finalize(TilemMacro* macro) {
	if(macro) {
		if(macro->actions)
			g_free(macro->actions);
		g_free(macro);
	}
}

/* Firstly free the memory then create a new TilemMacro */
void tilem_macro_start(TilemCalcEmulator *emu) {
	emu->isMacroRecording = TRUE;

	/* Firstly destroy the macro if exists */
	tilem_macro_finalize(emu->macro);

	/* Then allocate a new one */	
	emu->macro = tilem_macro_new(emu);
}

/* Add an action to the macro. The action could be :
 * A keypress (type == 0) 
 * A file load (type == 1)
 * Or something else if I implement it :)
 */
void tilem_macro_add_action(TilemMacro* macro, int type, char * value) {
	
	int n = macro->n;

	/* We want to allocate for 1 object, but index is 0 */
	macro->actions = tilem_macro_actions_new(macro, n + 1);

	/* Then we need to save the action */	
	macro->actions[n] =  g_new(char, strlen(value)); /* FIXME : gcc says : "assignment from incompatible pointer type" ??? */
	macro->actions[n]->value = g_strdup(value);
	macro->actions[n]->type = type;
	macro->n++;
}

/* Stop the macro */
void tilem_macro_stop(TilemCalcEmulator *emu)
{
	if(emu->isMacroRecording)
		emu->isMacroRecording = FALSE;
}

/* Print the macro actions content (debug) */
void tilem_macro_print(TilemMacro *macro) {
	int i = 0;

	printf("macro->n : %d\n", macro->n);
	for(i = 0; i < macro->n; i++ ){
		printf("type : %d    value : %s\n", macro->actions[i]->type, macro->actions[i]->value);
	}
}

/* Write a file using TilemMacro structure */
void tilem_macro_write_file(TilemCalcEmulator *emu) {
	char *dir, *filename;
	tilem_config_get("macro",
                 "directory/f", &dir,
                 NULL);

	filename = prompt_save_file(_("Save macro"), 
				    GTK_WINDOW(emu->ewin->window),
				    NULL, 
				    dir,
	                            _("Macro files"), "*.txt",
	                            _("All files"), "*",
				    NULL);
	if(filename) {
		FILE * fp = g_fopen(filename, "w");
		if(fp) {
			int i = 0;
			for(i = 0; i< emu->macro->n; i++ ){
				printf("type : %d    value : %s\n", emu->macro->actions[i]->type, emu->macro->actions[i]->value);
				/* Test if it's a key press or a file loading action */
				if(emu->macro->actions[i]->type == 1) {
					char * lengthchar = g_new0(char, 4);
					int length = strlen(emu->macro->actions[i]->value);
					fwrite("file=", 1, 5, fp);
					sprintf(lengthchar, "%04d", strlen(emu->macro->actions[i]->value));
					fwrite(lengthchar, 1, sizeof(int), fp);
					fwrite("-", 1, 1, fp);
					fwrite(emu->macro->actions[i]->value, 1, length, fp);
					g_free(lengthchar);
				} else {
					fwrite(emu->macro->actions[i]->value, 1, 4, fp);
					fwrite(",", 1, 1, fp);
				}
			}
			tilem_config_set("macro", "directory/f", g_path_get_dirname(filename), NULL);
			fclose(fp);
		}
		g_free(filename);
		g_free(dir);
	}
}

#define MACRO_KEYPRESS 0
#define MACRO_FILE 1

/* Play the macro (macro should be created or loaded before else it does nothing) */
static gboolean tilem_macro_play_main(TilemCalcEmulator *emu, G_GNUC_UNUSED gpointer data) {

	if(!emu->macro) {	
		printf(_("Nothing to play\n"));
		return FALSE;
	}

	int i;
	for(i = 0; i < emu->macro->n; i++ ){
		if(emu->macro->actions[i]->type == MACRO_FILE) {
			/* Type == 1 is load file */
			struct TilemSendFileInfo *sf;
			sf = g_slice_new0(struct TilemSendFileInfo);
			sf->filename = g_strdup(emu->macro->actions[i]->value);
			sf->display_name = g_filename_display_basename(emu->macro->actions[i]->value);
			sf->slot = -1;
			sf->first = TRUE;
			sf->last = TRUE;
			send_file_main(emu, sf);
	
		} else {
			/* type == 0 is keypress */
			int code = atoi(emu->macro->actions[i]->value);
			tilem_em_unlock(emu);
			run_with_key_slowly(emu->calc, code);			
			tilem_em_lock(emu);
			
		}
	}


	return TRUE;
}


static void tilem_macro_play_finished(G_GNUC_UNUSED TilemCalcEmulator *emu, G_GNUC_UNUSED gpointer data,
                               G_GNUC_UNUSED gboolean cancelled) {

}

/* Play the macro */
void tilem_macro_play(TilemCalcEmulator* emu) {

	tilem_calc_emulator_begin(emu, &tilem_macro_play_main, &tilem_macro_play_finished, NULL);	
}

/* Load a macro (when finished, task manager will normally call tilem_macro_play) */
static gboolean tilem_macro_load_main(TilemCalcEmulator* emu, gpointer data) {

	char* filename = (char*) data;
	char c = 'a';
	
	if(filename) {
		FILE * fp = g_fopen(filename, "r");
		/* printf("filename : %s\n", filename); */
		
		tilem_macro_start(emu);	
		while(c != EOF) {	
			char* codechar = g_new0(char, 4);
			fread(codechar, 1, 4, fp);
			if(strcmp(codechar, "file") == 0) {
				c = fgetc(fp); /* Drop the "="*/
				char *lengthchar = g_new0(char, 4);
				fread(lengthchar, 1, 4, fp);
				c = fgetc(fp); /* Drop the "-"*/
				int length = atoi(lengthchar);
				char* filetoload= g_new0(char, length);
				fread(filetoload, 1, length, fp);
				tilem_macro_add_action(emu->macro, 1, filetoload);	
				g_free(lengthchar);
				g_free(filetoload);
			} else {
				int code = atoi(codechar);
				if(code <= 0)
					break;
				/*printf("code : %d, codechar : %s\n",code,  codechar); */
				tilem_macro_add_action(emu->macro, 0, codechar);	
				c = fgetc(fp);
			}
		}
		tilem_macro_stop(emu);
		fclose(fp);
	}
	
	return TRUE;
}

/* When the macro is totally loaded, then we can play it ! */
static void tilem_macro_load_finished(G_GNUC_UNUSED TilemCalcEmulator *emu, G_GNUC_UNUSED gpointer data,
                               G_GNUC_UNUSED gboolean cancelled)
{
	tilem_calc_emulator_begin(emu, &tilem_macro_play_main, &tilem_macro_play_finished, NULL);
}

/* Load a macro from filename. 
 * If filename == NULL prompt the user
 */
void tilem_macro_load(TilemCalcEmulator *emu, char* filename) {
	
	/* printf("tilem_macro_load : filename : %s\n", filename); */
	if(!filename) {
		char *dir;
		tilem_config_get("macro", "directory/f", &dir, NULL);

		filename = prompt_open_file(_("Open macro"), 
					    GTK_WINDOW(emu->ewin->window),
					    dir,
		                            _("Macro files"), "*.txt",
		                            _("All files"), "*",
					    NULL);
		if(dir)
			g_free(dir);
	}
	tilem_calc_emulator_begin(emu, &tilem_macro_load_main, &tilem_macro_load_finished, filename);	
}
	
