/*
 * TilEm II
 *
 * Copyright (c) 2010-2011 Thibault Duponchelle
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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <ticalcs.h>
#include <tilem.h>

#include "gui.h"
#include "files.h"
#include "msgbox.h"

#ifndef CONFIG_FILE
#define CONFIG_FILE "config.ini"
#endif

#define MAX_RECENT_FILES 10

/* Store a filename in a GKeyFile.  Any control characters or
   non-UTF-8 filenames are stored in octal.  Note that
   g_key_file_set/get_string() can't be used because they only allow
   UTF-8 */
static void key_file_set_filename(GKeyFile *gkf, const char *group,
                                  const char *key, const char *value)
{
	char *escaped;
	const char *p;
	char *q;
	gunichar uc;
	int b;

	q = escaped = g_new(char, strlen(value) * 4 + 1);

	while (*value != 0) {
		uc = g_utf8_get_char_validated(value, -1);
		if (uc < 0x20 || uc == 0x7F || !g_unichar_validate(uc)) {
			b = (unsigned char) *value;
			q[0] = '\\';
			q[1] = '0' + (b >> 6);
			q[2] = '0' + ((b >> 3) & 7);
			q[3] = '0' + (b & 7);
			q += 4;
			value++;
		}
		else if (uc == '\\') {
			q[0] = q[1] = '\\';
			q += 2;
			value++;
		}
		else {
			p = g_utf8_next_char(value);
			while (value != p)
				*q++ = *value++;
		}
	}

	*q = 0;

	g_key_file_set_value(gkf, group, key, escaped);
	g_free(escaped);
}

/* Retrieve a filename from a GKeyFile. */
static char *key_file_get_filename(GKeyFile *gkf, const char *group,
                                   const char *key, GError **error)
{
	char *value, *unescaped;

	value = g_key_file_get_value(gkf, group, key, error);
	if (!value)
		return NULL;

	unescaped = g_strcompress(value);
	g_free(value);
	return unescaped;
}

/* Load and parse the configuration file. */
static GKeyFile *load_config(gboolean writable)
{
	static gboolean warned;
	GKeyFile *gkf;
	GKeyFileFlags flags;
	char *cfname, *dname;
	GError *err = NULL;

	gkf = g_key_file_new();

	cfname = get_shared_file_path(CONFIG_FILE, NULL);
	if (!cfname)
		return gkf;

	if (writable)
		flags = (G_KEY_FILE_KEEP_COMMENTS
		         | G_KEY_FILE_KEEP_TRANSLATIONS);
	else
		flags = 0;

	if (!g_key_file_load_from_file(gkf, cfname, flags, &err)) {
		/* don't bother the user more than once */
		if (!warned) {
			dname = g_filename_display_name(cfname);
			messagebox02(NULL, GTK_MESSAGE_ERROR,
			             _("Unable to read settings"),
			             _("An error occurred while reading %s: %s"),
			             dname, err->message);
			g_free(dname);
			warned = TRUE;
		}
		g_error_free(err);
	}

	g_free(cfname);
	return gkf;
}

/* Save the configuration file. */
static void save_config(GKeyFile *gkf)
{
	static gboolean warned;
	char *cfname, *dname;
	char *data;
	gsize length;
	GError *err = NULL;

	data = g_key_file_to_data(gkf, &length, NULL);
	
	cfname = get_config_file_path(CONFIG_FILE, NULL);

	if (!g_file_set_contents(cfname, data, length, &err)) {
		/* don't bother the user more than once */
		if (!warned) {
			dname = g_filename_display_name(cfname);
			messagebox02(NULL, GTK_MESSAGE_ERROR,
			             _("Unable to save settings"),
			             _("An error occurred while writing %s: %s"),
			             dname, err->message);
			g_free(dname);
			warned = TRUE;
		}
		g_error_free(err);
	}

	g_free(cfname);
	g_free(data);
}

/* Retrieve settings from the configuration file. */
void tilem_config_get(const char *group, const char *option, ...)
{
	va_list ap;
	GKeyFile *gkf;
	const char *type, *defvalue;
	GError *err = NULL;
	char *key, *p;
	char **strp;
	int *intp;
	double *dblp;
	GdkColor *colorp;

	g_return_if_fail(group != NULL);
	g_return_if_fail(option != NULL);

	gkf = load_config(FALSE);

	va_start(ap, option);
	while (option != NULL) {
		type = strrchr(option, '/');
		if (type == NULL || type[1] == 0
		    || (type[2] != 0 && type[2] != '=')) {
			g_critical(_("invalid argument\n"));
			break;
		}

		if (type[2] == '=')
			defvalue = &type[3];
		else
			defvalue = NULL;

		key = g_strndup(option, type - option);

		if (type[1] == 'f') {
			strp = va_arg(ap, char **);
			*strp = key_file_get_filename(gkf, group, key, &err);
			if (err && defvalue)
				*strp = g_strdup(defvalue);
		}
		else if (type[1] == 's') {
			strp = va_arg(ap, char **);
			*strp = g_key_file_get_string(gkf, group, key, &err);
			if (err && defvalue)
				*strp = g_strdup(defvalue);
		}
		else if (type[1] == 'i') {
			intp = va_arg(ap, int *);
			*intp = g_key_file_get_integer(gkf, group, key, &err);
			if (err && defvalue)
				*intp = g_ascii_strtoll(defvalue, NULL, 10);
		}
		else if (type[1] == 'r') {
			dblp = va_arg(ap, double *);
			*dblp = g_key_file_get_double(gkf, group, key, &err);
			if (err && defvalue)
				*dblp = g_ascii_strtod(defvalue, NULL);
		}
		else if (type[1] == 'b') {
			intp = va_arg(ap, int *);
			*intp = g_key_file_get_boolean(gkf, group, key, &err);
			if (err && defvalue)
				*intp = g_ascii_strtoll(defvalue, NULL, 10);
		}
		else if (type[1] == 'c') {
			colorp = va_arg(ap, GdkColor *);
			p = g_key_file_get_string(gkf, group, key, &err);
			if (p == NULL || !gdk_color_parse(p, colorp)) {
				if (defvalue) {
					gdk_color_parse(defvalue, colorp);
				}
				else {
					colorp->red = 0;
					colorp->green = 0;
					colorp->blue = 0;
				}
			}
			g_free(p);
		}
		else {
			g_critical(_("invalid argument\n"));
			g_free(key);
			break;
		}

		g_clear_error(&err);
		g_free(key);
		option = va_arg(ap, const char *);
	}
	va_end(ap);

	g_key_file_free(gkf);
}

/* Save settings to the configuration file. */
void tilem_config_set(const char *group, const char *option, ...)
{
	va_list ap;
	GKeyFile *gkf;
	const char *type;
	char *key;
	const char *strv;
	int intv;
	double dblv;
	const GdkColor *colorv;
	char *p;

	g_return_if_fail(group != NULL);
	g_return_if_fail(option != NULL);

	gkf = load_config(TRUE);

	va_start(ap, option);
	while (option != NULL) {
		type = strrchr(option, '/');
		if (type == NULL || type[1] == 0 || type[2] != 0) {
			g_critical(_("invalid argument\n"));
			break;
		}

		key = g_strndup(option, type - option);

		if (type[1] == 'f') {
			strv = va_arg(ap, const char *);
			key_file_set_filename(gkf, group, key, strv);
		}
		else if (type[1] == 's') {
			strv = va_arg(ap, const char *);
			g_key_file_set_string(gkf, group, key, strv);
		}
		else if (type[1] == 'i') {
			intv = va_arg(ap, int);
			g_key_file_set_integer(gkf, group, key, intv);
		}
		else if (type[1] == 'r') {
			dblv = va_arg(ap, double);
			g_key_file_set_double(gkf, group, key, dblv);
		}
		else if (type[1] == 'b') {
			intv = va_arg(ap, int);
			g_key_file_set_boolean(gkf, group, key, !!intv);
		}
		else if (type[1] == 'c') {
			colorv = va_arg(ap, const GdkColor *);
			p = g_strdup_printf("#%02x%02x%02x",
			                    colorv->red >> 8,
			                    colorv->green >> 8,
			                    colorv->blue >> 8);
			g_key_file_set_string(gkf, group, key, p);
			g_free(p);
		}
		else {
			g_critical(_("invalid argument\n"));
			g_free(key);
			break;
		}

		g_free(key);

		option = va_arg(ap, const char *);
	}
	va_end(ap);

	save_config(gkf);
	g_key_file_free(gkf);
}

