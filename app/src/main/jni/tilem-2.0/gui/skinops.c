/*
 *   skinedit - a skin editor for the TiEmu emulator
 *   Copyright (C) 2002 Julien BLACHE <jb@tilp.info>
 *   Copyright (C) 2012 Benjamin Moody
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
contra-sh :
   This file is a (quasi) perfect copy of the tiemu skinops.c file ...
   Thank's to rom's and JB for this wonderful work.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "skinops.h"

#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include "gettext.h"

#define SKIN_ERROR g_quark_from_static_string("skin-error")
enum {
	SKIN_ERROR_INVALID
};

/*
	Determine skin type
*/
int skin_get_type(SKIN_INFOS *si, const char *filename)
{
	FILE *fp;
	char str[17];

	fp = g_fopen(filename, "rb");
	if (fp == NULL) {
		fprintf(stderr, _("Unable to open this file: <%s>\n"), filename);
		return -1;
	}

	memset(str, 0, sizeof(str));
	fread(str, 16, 1, fp);

	if(!strncmp(str, "TiEmu v2.00", 16))
		si->type = SKIN_TYPE_TIEMU;
	else if(!strncmp(str, "TilEm v2.00 ", 16))
		si->type = SKIN_TYPE_TIEMU;
	else if(!strncmp(str, "VTIv2.1 ", 8))
		si->type = SKIN_TYPE_OLD_VTI;
	else if(!strncmp(str, "VTIv2.5 ", 8))
		si->type = SKIN_TYPE_VTI;
	else {
		fprintf(stderr, _("Bad skin format\n"));
		return -1;
	}

	return 0;
}

/*
  Read TilEm skin informations (header)
*/
int skin_read_header(SKIN_INFOS *si, FILE *fp)
{
	int i;
	uint32_t endian;
	uint32_t jpeg_offset;
	uint32_t length;
	char str[17];

	/* signature & offsets */
	fread(str, 16, 1, fp);
	if ((strncmp(str, "TilEm v2.00", 16))
	    && (strncmp(str, "TiEmu v2.00", 16))) {
		return -1;
	}
	fread(&endian, 4, 1, fp);
	fread(&jpeg_offset, 4, 1, fp);

	if (endian != ENDIANNESS_FLAG)
		jpeg_offset = GUINT32_SWAP_LE_BE(jpeg_offset);

	/* Skin name */
	fread(&length, 4, 1, fp);
	if (endian != ENDIANNESS_FLAG)
		length = GUINT32_SWAP_LE_BE(length);

	if (length > 0) {
		si->name = (char *)malloc(length + 1);
		if (si->name == NULL)
			return -1;

		memset(si->name, 0, length + 1);
		fread(si->name, length, 1, fp);
	}

	/* Skin author */
	fread(&length, 4, 1, fp);
	if (endian != ENDIANNESS_FLAG)
		length = GUINT32_SWAP_LE_BE(length);

	if (length > 0) {
		si->author = (char *)malloc(length + 1);
		if (si->author == NULL)
			return -1;

		memset(si->author, 0, length + 1);
		fread(si->author, length, 1, fp);
	}

	/* LCD colors */
	fread(&si->colortype, 4, 1, fp);
	fread(&si->lcd_white, 4, 1, fp);
	fread(&si->lcd_black, 4, 1, fp);

	/* Calc type */
	fread(si->calc, 8, 1, fp);

	/* LCD position */
	fread(&si->lcd_pos.left, 4, 1, fp);
	fread(&si->lcd_pos.top, 4, 1, fp);
	fread(&si->lcd_pos.right, 4, 1, fp);
	fread(&si->lcd_pos.bottom, 4, 1, fp);

	/* Number of RECT struct to read */
	fread(&length, 4, 1, fp);
	if (endian != ENDIANNESS_FLAG)
		length = GUINT32_SWAP_LE_BE(length);

	if (length > SKIN_KEYS)
		return -1;

	for (i = 0; i < (int)length; i++) {
		fread(&si->keys_pos[i].left, 4, 1, fp);
		fread(&si->keys_pos[i].top, 4, 1, fp);
		fread(&si->keys_pos[i].right, 4, 1, fp);
		fread(&si->keys_pos[i].bottom, 4, 1, fp);
	}

	if (endian != ENDIANNESS_FLAG) {
		si->colortype = GUINT32_SWAP_LE_BE(si->colortype);
		si->lcd_white = GUINT32_SWAP_LE_BE(si->lcd_white);
		si->lcd_black = GUINT32_SWAP_LE_BE(si->lcd_black);

		si->lcd_pos.top = GUINT32_SWAP_LE_BE(si->lcd_pos.top);
		si->lcd_pos.left = GUINT32_SWAP_LE_BE(si->lcd_pos.left);
		si->lcd_pos.bottom = GUINT32_SWAP_LE_BE(si->lcd_pos.bottom);
		si->lcd_pos.right = GUINT32_SWAP_LE_BE(si->lcd_pos.right);

		for (i = 0; i < (int)length; i++) {
			si->keys_pos[i].top = GUINT32_SWAP_LE_BE(si->keys_pos[i].top);
			si->keys_pos[i].bottom = GUINT32_SWAP_LE_BE(si->keys_pos[i].bottom);
			si->keys_pos[i].left = GUINT32_SWAP_LE_BE(si->keys_pos[i].left);
			si->keys_pos[i].right = GUINT32_SWAP_LE_BE(si->keys_pos[i].right);
		}
	}

	si->jpeg_offset = ftell(fp);

	return 0;
}

/*
  Read skin image (pure jpeg data)
*/
int skin_read_image(SKIN_INFOS *si, FILE *fp, GError **err)
{
	GdkPixbufLoader *loader;
	gboolean result;
	guchar *buf;
	gsize count;
	struct stat st;

	// Extract image from skin
	fseek(fp, si->jpeg_offset, SEEK_SET);
	fstat(fileno(fp), &st);
	count = st.st_size - si->jpeg_offset;

	buf = g_malloc(count * sizeof(guchar));
	count = fread(buf, sizeof(guchar), count, fp);

	// Feed the pixbuf loader with our jpeg data
	loader = gdk_pixbuf_loader_new();
	result = gdk_pixbuf_loader_write(loader, buf, count, err);
	g_free(buf);

	if(result == FALSE) {
		g_object_unref(loader);
		return -1;
	}

	result = gdk_pixbuf_loader_close(loader, err);
	if(result == FALSE) {
		g_object_unref(loader);
		return -1;
	}

	// and get the pixbuf
	si->raw = gdk_pixbuf_loader_get_pixbuf(loader);
	if(si->raw == NULL) {
		g_set_error(err, SKIN_ERROR, SKIN_ERROR_INVALID,
		            _("Unable to load background image"));
		g_object_unref(loader);
		return -1;
	}

	si->sx = si->sy = 1.0;
	si->image = g_object_ref(si->raw);
	g_object_ref(si->raw);

	// Get new skin size
	si->width = gdk_pixbuf_get_width(si->image);
	si->height = gdk_pixbuf_get_height(si->image);

	g_object_unref(loader);

	return 0;
}

/* Load a skin (TilEm v2.00 only) */
int skin_load(SKIN_INFOS *si, const char *filename, GError **err)
{
	FILE *fp;
	int ret = 0, errnum;
	char *dname;

	g_return_val_if_fail(err == NULL || *err == NULL, -1);

	fp = g_fopen(filename, "rb");
	if (fp == NULL) {
		errnum = errno;
		dname = g_filename_display_basename(filename);
		g_set_error(err, G_FILE_ERROR, g_file_error_from_errno(errnum),
		            _("Unable to open %s for reading: %s"),
		            dname, g_strerror(errnum));
		g_free(dname);
		return -1;
	}

	ret = skin_read_header(si, fp);
	if (ret) {
		fclose(fp);
		dname = g_filename_display_basename(filename);
		g_set_error(err, SKIN_ERROR, SKIN_ERROR_INVALID,
		            _("The file %s is not a valid skin."), dname);
		g_free(dname);
		return -1;
	}

	ret = skin_read_image(si, fp, err);

	fclose(fp);

	return ret;
}

/* Unload skin by freeing allocated memory */
int skin_unload(SKIN_INFOS *si)
{
	if (si->image != NULL) {
		g_object_unref(si->image);
		si->image = NULL;
	}

	if (si->raw) {
		g_object_unref(si->raw);
		si->raw = NULL;
	}

	free(si->name);
	free(si->author);

	memset(si, 0, sizeof(SKIN_INFOS));

	return 0;
}

