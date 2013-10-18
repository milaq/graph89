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

/* Set the name used to invoke this program.  Data files can be
   located relative to this path if the package is not installed */
void set_program_path(const char *path);

/* Locate an existing configuration or data file.  Arguments will be
   concatenated, separated by / or \, as with g_build_filename().
   NULL is returned if the file isn't found.  Free result with
   g_free(). */
char * get_shared_file_path(const char *name, ...)
	G_GNUC_NULL_TERMINATED;

/* Locate an existing configuration or data directory.  NULL is
   returned if the file isn't found.  Free result with g_free(). */
char * get_shared_dir_path(const char *name, ...)
	G_GNUC_NULL_TERMINATED;

/* Get the full path where a configuration file should be written;
   attempt to create the directory if it doesn't exist.  This function
   will always return a valid filename (although it may not actually
   be writable.)  Free result with g_free(). */
char * get_config_file_path(const char *name, ...)
	G_GNUC_NULL_TERMINATED;

/* Get the full path to locale data files */
char * get_locale_dir(void);
