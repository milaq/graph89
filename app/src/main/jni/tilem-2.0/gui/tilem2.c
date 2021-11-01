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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <ticalcs.h>
#include <tilem.h>

#include "gui.h"
#include "files.h"
#include "icons.h"
#include "msgbox.h"

/* CMD LINE OPTIONS */
static gchar* cl_romfile = NULL;
static gchar* cl_skinfile = NULL;
static gchar* cl_model = NULL;
static gchar* cl_statefile = NULL;
static gchar** cl_files_to_load = NULL;
static gboolean cl_skinless_flag = FALSE;
static gboolean cl_reset_flag = FALSE;
static gchar* cl_getvar = NULL;
static gchar* cl_macro_to_run = NULL;
static gboolean cl_debug_flag = FALSE;
static gboolean cl_normalspeed_flag = FALSE;
static gboolean cl_fullspeed_flag = FALSE;
static gchar* cl_link_cable = NULL;
static gboolean cl_audio_flag = FALSE;


static GOptionEntry entries[] =
{
	{ "rom", 'r', 0, G_OPTION_ARG_FILENAME, &cl_romfile, N_("The rom file to run"), N_("FILE") },
	{ "skin", 'k', 0, G_OPTION_ARG_FILENAME, &cl_skinfile, N_("The skin file to use"), N_("FILE") },
	{ "model", 'm', 0, G_OPTION_ARG_STRING, &cl_model, N_("The model to use"), N_("NAME") },
	{ "state-file", 's', 0, G_OPTION_ARG_FILENAME, &cl_statefile, N_("The state-file to use"), N_("FILE") },
	{ "without-skin", 'l', 0, G_OPTION_ARG_NONE, &cl_skinless_flag, N_("Start in skinless mode"), NULL },
	{ "reset", 0, 0, G_OPTION_ARG_NONE, &cl_reset_flag, N_("Reset the calc at startup"), NULL },
	{ "get-var", 0, 0, G_OPTION_ARG_STRING, &cl_getvar, N_("Get a var at startup"), N_("FILE") },
	{ "play-macro", 'p', 0, G_OPTION_ARG_FILENAME, &cl_macro_to_run, N_("Run this macro at startup"), N_("FILE") },
	{ "debug", 'd', 0, G_OPTION_ARG_NONE, &cl_debug_flag, N_("Launch debugger"), NULL },
	{ "normal-speed", 0, 0, G_OPTION_ARG_NONE, &cl_normalspeed_flag, N_("Run at normal speed"), NULL },
	{ "full-speed", 0, 0, G_OPTION_ARG_NONE, &cl_fullspeed_flag, N_("Run at maximum speed"), NULL },
	{ "cable", 'c', 0, G_OPTION_ARG_STRING, &cl_link_cable, N_("Connect to an external link cable"), N_("TYPE[:PORT]") },
	{ "audio", 'a', 0, G_OPTION_ARG_NONE, &cl_audio_flag, N_("Enable audio output"), NULL },
	{ G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &cl_files_to_load, NULL, N_("FILE") },
	{ 0, 0, 0, 0, 0, 0, 0 }
};


/* #########  MAIN  ######### */

/* Order of preference for automatic model selection. */
static const char model_search_order[] =
	{ TILEM_CALC_TI81,
	  TILEM_CALC_TI73,
	  TILEM_CALC_TI82,
	  TILEM_CALC_TI83,
	  TILEM_CALC_TI76,
	  TILEM_CALC_TI84P_SE,
	  TILEM_CALC_TI84P,
	  TILEM_CALC_TI83P_SE,
	  TILEM_CALC_TI83P,
	  TILEM_CALC_TI84P_NSPIRE,
	  TILEM_CALC_TI85,
	  TILEM_CALC_TI86, 0 };

/* Check if given calc model should be used for these file types. */
static gboolean check_file_types(int calc_model,
                                 const int *file_models,
                                 int nfiles)
{
	/* Only choose a calc model if it supports all of the given
	   file types, and at least one of the files is of the calc's
	   "preferred" type.  This means if we have a mixture of 82Ps
	   and 83Ps, we can use either a TI-83 or TI-76.fr ROM image,
	   but not a TI-83 Plus. */

	gboolean preferred = FALSE;
	int i;

	calc_model = model_to_base_model(calc_model);

	for (i = 0; i < nfiles; i++) {
		if (file_models[i] == calc_model)
			preferred = TRUE;
		else if (!model_supports_file(calc_model, file_models[i]))
			return FALSE;
	}

	return preferred;
}

static void load_initial_rom(TilemCalcEmulator *emu,
                             const char *cmdline_rom_name,
                             const char *cmdline_state_name,
                             char **cmdline_files,
                             int model)
{
	GError *err = NULL;
	char *modelname;
	int nfiles, *file_models, i;

	/* If a ROM file is specified on the command line, use that
	   (and no other) */

	if (cmdline_rom_name) {
		if (tilem_calc_emulator_load_state(emu, cmdline_rom_name,
		                                   cmdline_state_name,
		                                   model, &err))
			return;
		else if (!err)
			exit(0);
		else {
			g_printerr("%s\n", err->message);
			exit(1);
		}
	}

	/* Choose model by file names */

	if (!model && cmdline_files) {
		nfiles = g_strv_length(cmdline_files);
		file_models = g_new(int, nfiles);

		/* determine model for each filename */
		for (i = 0; i < nfiles; i++)
			file_models[i] = file_to_model(cmdline_files[i]);

		/* iterate over all known models... */
		for (i = 0; model_search_order[i]; i++) {
			model = model_search_order[i];

			/* check if this model supports the named files */
			if (!check_file_types(model, file_models, nfiles))
				continue;

			/* try to load model, but no error message if
			   no ROM is present in config */
			if (tilem_calc_emulator_load_state(emu, NULL, NULL,
			                                   model, &err)) {
				g_free(file_models);
				return;
			}
			else if (!err)
				exit(0);
			else if (!g_error_matches(err, TILEM_EMULATOR_ERROR,
			                          TILEM_EMULATOR_ERROR_NO_ROM)) {
				messagebox01(NULL, GTK_MESSAGE_ERROR,
				             _("Unable to load calculator state"),
				             "%s", err->message);
			}
			g_clear_error(&err);
		}

		g_free(file_models);
		model = 0;
	}

	/* If no model specified on command line (either explicitly or
	   implicitly), then choose the most recently used model */

	if (!model && !cmdline_files) {
		tilem_config_get("recent", "last_model/s", &modelname, NULL);
		if (modelname)
			model = name_to_model(modelname);
		g_free(modelname);
	}

	/* Try to load the most recently used ROM for chosen model */

	if (model) {
		if (tilem_calc_emulator_load_state(emu, NULL, NULL,
		                                   model, &err))
			return;
		else if (!err)
			exit(0);
		else {
			messagebox01(NULL, GTK_MESSAGE_ERROR,
			             _("Unable to load calculator state"),
			             "%s", err->message);
			g_clear_error(&err);
		}
	}

	/* Prompt user for a ROM file */

	while (!emu->calc) {
		if (!tilem_calc_emulator_prompt_open_rom(emu))
			exit(0);
	}
}

static int parse_link_cable(CableOptions *options, const char *str)
{
	const char *portstr;
	char *modelstr = NULL;
	char *end;

	if ((portstr = strchr(str, ':'))) {
		modelstr = g_strndup(str, portstr - str);
		str = modelstr;
		portstr++;
	}
	else {
		portstr = "0";
	}

	if (!g_ascii_strcasecmp(str, "gry"))
		options->model = CABLE_GRY;
	else if (!g_ascii_strcasecmp(str, "blk")
	         || !g_ascii_strcasecmp(str, "ser"))
		options->model = CABLE_BLK;
	else if (!g_ascii_strcasecmp(str, "par"))
		options->model = CABLE_PAR;
	else if (!g_ascii_strcasecmp(str, "slv"))
		options->model = CABLE_SLV;
	else if (!g_ascii_strcasecmp(str, "tie"))
		options->model = CABLE_TIE;
	else if (!g_ascii_strcasecmp(str, "vti"))
		options->model = CABLE_VTI;
	else
		options->model = ticables_string_to_model(str);

	g_free(modelstr);

	if (options->model == CABLE_NUL)
		return 0;

	options->port = strtol(portstr, &end, 10);
	if (end == portstr || *end != 0)
		return 0;

	if (options->port == 0 && options->model != CABLE_TIE)
		options->port = 1;

	options->timeout = DFLT_TIMEOUT;
	options->delay = DFLT_DELAY;
	return 1;
}

int main(int argc, char **argv)
{
	TilemCalcEmulator* emu;
	char *menurc_path;
	GOptionContext *context;
	GError *error = NULL;
	int model = 0;
	CableOptions cable_options;

	g_thread_init(NULL);

#ifdef ENABLE_NLS
	setlocale(LC_ALL, "");
	bindtextdomain(GETTEXT_PACKAGE, get_locale_dir());
# ifdef HAVE_BIND_TEXTDOMAIN_CODESET
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
# endif
	textdomain(GETTEXT_PACKAGE);
#endif
	gtk_init(&argc, &argv);
	set_program_path(argv[0]);
	g_set_application_name("TilEm");

	menurc_path = get_shared_file_path("menurc", NULL);
	if (menurc_path)
		gtk_accel_map_load(menurc_path);
	g_free(menurc_path);

	init_custom_icons();
	gtk_window_set_default_icon_name("tilem");

	emu = tilem_calc_emulator_new();

	context = g_option_context_new(NULL);
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_add_group(context, gtk_get_option_group(TRUE));
	if (!g_option_context_parse(context, &argc, &argv, &error))
	{
		g_printerr(_("%s: %s\n"), g_get_prgname(), error->message);
		exit (1);
	}

	if (cl_model) {
		model = name_to_model(cl_model);
		if (!model) {
			g_printerr(_("%s: unknown model %s\n"),
			           g_get_prgname(), cl_model);
			return 1;
		}
	}

	if (cl_link_cable) {
		if (!parse_link_cable(&cable_options, cl_link_cable)) {
			g_printerr(_("%s: invalid link cable %s\n"),
			           g_get_prgname(), cl_link_cable);
			return 1;
		}
	}
	else {
		cable_options.model = CABLE_NUL;
		cable_options.port = 0;
	}

	load_initial_rom(emu, cl_romfile, cl_statefile, cl_files_to_load, model);

	emu->ewin = tilem_emulator_window_new(emu);

	if (cl_skinless_flag)
		tilem_emulator_window_set_skin_disabled(emu->ewin, TRUE);
	else if (cl_skinfile) {
		tilem_emulator_window_set_skin(emu->ewin, cl_skinfile);
		tilem_emulator_window_set_skin_disabled(emu->ewin, FALSE);
	}

	gtk_widget_show(emu->ewin->window);

	ticables_library_init();
	tifiles_library_init();
	ticalcs_library_init();
	/* the above functions modify textdomain */
	textdomain(GETTEXT_PACKAGE);

	tilem_audio_device_init();

	if (cl_reset_flag)
		tilem_calc_emulator_reset(emu);

	if (cl_fullspeed_flag)
		tilem_calc_emulator_set_limit_speed(emu, FALSE);
	else if (cl_normalspeed_flag)
		tilem_calc_emulator_set_limit_speed(emu, TRUE);

	tilem_calc_emulator_set_audio(emu, cl_audio_flag);
	tilem_calc_emulator_set_link_cable(emu, &cable_options);

	if (cl_files_to_load)
		load_files_cmdline(emu->ewin, cl_files_to_load);
	if (cl_macro_to_run)
		tilem_macro_load(emu, cl_macro_to_run);
	if (cl_getvar)
		tilem_link_receive_matching(emu, cl_getvar, ".");

	if (cl_debug_flag)
		launch_debugger(emu->ewin);
	else
		tilem_calc_emulator_run(emu);

	g_signal_connect(emu->ewin->window, "destroy",
	                 G_CALLBACK(gtk_main_quit), NULL);

	gtk_main();

	tilem_calc_emulator_pause(emu);

	tilem_emulator_window_free(emu->ewin);
	tilem_calc_emulator_free(emu);

	menurc_path = get_config_file_path("menurc", NULL);
	gtk_accel_map_save(menurc_path);
	g_free(menurc_path);

	ticables_library_exit();
	tifiles_library_exit();
	ticalcs_library_exit();
	tilem_audio_device_exit();

	return 0;
}
