/* Hey EMACS -*- linux-c -*- */
/* $Id: screenshot.c 2620 2007-08-01 05:17:09Z kevinkofler $ */

/*  TiEmu - a TI emulator
 *  Copyright (c) 2005, Julien Blache
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
 *  Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
# include <tiemuconfig.h>
#endif

#include <stdio.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include <zlib.h>

#include "screenshot.h"
#include "version.h"
#include "struct.h"
#include "../core/ti68k_int.h"
#include "intl.h"

/*
 * Utility function for the EPS and PDF output
 */
static gboolean write_compressed_a85_screen(FILE *fp, GdkPixbuf *pixbuf, GError **error)
{
	guchar *ubuf, *cbuf;
	int cbuflen;
	int r, i, j;
	int h, w;
	int rlen;
	int ret;
	z_stream s;
	int flush;
	int outlen;
	int a85count;
	unsigned long a85tuple;
	guchar a85block[6];

	ubuf = gdk_pixbuf_get_pixels(pixbuf);
	rlen = gdk_pixbuf_get_rowstride(pixbuf);
	w = gdk_pixbuf_get_width(pixbuf);
	h = gdk_pixbuf_get_height(pixbuf);

	a85count = 0;
	a85tuple = 0;
	a85block[5] = '\0';

	/* buffer length = length + 0.1 * length + 12 (mandatory) */
	cbuflen = outlen = rlen + rlen / 10 + 12;
	cbuf = g_malloc(cbuflen);

	if (cbuf == NULL) {
		g_set_error(error, 0, 0, _("Couldn't allocate memory!"));
		return FALSE;
	}

	s.zalloc = Z_NULL;
	s.zfree = Z_NULL;
	s.opaque = Z_NULL;

	ret = deflateInit(&s, Z_DEFAULT_COMPRESSION);

	if (ret != Z_OK) {
		g_set_error(error, 0, 0, _("zlib init error"));
		g_free(cbuf);
		return FALSE;
	}
	for (r = 0; r < h; r++) {
		s.avail_in = w * 3;
		s.next_in = ubuf;
		do {
			s.avail_out = outlen;
			s.next_out = cbuf;

			flush = (r == (h - 1)) ? Z_FINISH : Z_NO_FLUSH;

			ret = deflate(&s, flush);

			if (ret == Z_STREAM_ERROR) {
				g_set_error(error, 0, 0, _("zlib deflate error"));
				g_free(cbuf);
				return FALSE;
			}

			cbuflen = outlen - s.avail_out;
			/* ASCII85 (base 85) encoding */
			for (i = 0; i < cbuflen; i++) {
				switch (a85count) {
				case 0:
					a85tuple |= (cbuf[i] << 24);
					a85count++;
					break;
				case 1:
					a85tuple |= (cbuf[i] << 16);
					a85count++;
					break;
				case 2:
					a85tuple |= (cbuf[i] << 8);
					a85count++;
					break;
				case 3:
					a85tuple |= (cbuf[i] << 0);

					if (a85tuple == 0) {
						a85block[0] = 'z';
						a85block[1] = '\0';
					}
					else {
						/* The ASCII chars must be written in reverse order,
						 * hence -> a85block[4-j]
						 */
						for (j = 0; j < 5; j++) {
							a85block[4-j] = (unsigned char)(a85tuple % 85 + '!');
							a85tuple /= 85;
						}
					}
					fprintf(fp, "%s", a85block);

					a85count = 0;
					a85tuple = 0;
					break;
				default:
					break;
				}

				if ((i > 0) && (i % 32 == 0)) {
					fprintf(fp, "\n");
				}
			}
		} while (s.avail_out == 0);
		ubuf += rlen;
	}

	if (a85count > 0) {
		a85count++;
		for (j = 0; j <= a85count; j++) {
			a85block[j] = (unsigned char)(a85tuple % 85 + '!');
			a85tuple /= 85;
		}
                /* Reverse order */
		for (j--; j > 0; j--) {
			fprintf(fp, "%c", a85block[j]);
		}
	}

        /* ASCII85 EOD marker + newline*/
	fprintf(fp, "~>\n");

	deflateEnd(&s);

	g_free(cbuf);

	return TRUE;
}


/*
 * Write out an Encapsulated PostScript file.
 */
gboolean tiemu_screen_write_eps(const gchar *filename, GdkPixbuf *pixbuf, GError **error)
{
	int h, w;
	FILE *fp;
	time_t t;
	gboolean ret;
	GError *err = NULL;

	fp = fopen(filename, "wb");
	if (fp == NULL) {
		g_set_error(error, 0, 0, _("Couldn't open destination file for writing!"));
		return FALSE;
	}

	h = gdk_pixbuf_get_height(pixbuf);
	w = gdk_pixbuf_get_width(pixbuf);

	time(&t);

	fprintf(fp, "%%!PS-Adobe-3.0 EPSF-3.0\n");
	fprintf(fp, "%%%%Creator: TiEmu %s / PostScript output Copyright (C) 2005 Julien BLACHE\n", TIEMU_VERSION);
	fprintf(fp, "%%%%Title: TiEmu %s screenshot\n",
		ti68k_calctype_to_string(tihw.calc_type));
	fprintf(fp, "%%%%CreationDate: %s", ctime(&t));
	fprintf(fp, "%%%%LanguageLevel: 3\n");
	fprintf(fp, "%%%%BoundingBox: 0 0 %d %d\n", w, h);
	fprintf(fp, "\n");
	fprintf(fp, "%d %d scale\n", w, h);

	fprintf(fp, "%d %d 8 [%d 0 0 -%d 0 %d] currentfile /ASCII85Decode filter /FlateDecode filter false 3 colorimage\n", w, h, w, h, h);
	ret = write_compressed_a85_screen(fp, pixbuf, &err);

	if (!ret) {
		g_propagate_error(error, err);
		fclose(fp);
		unlink(filename);
		return FALSE;
	}

	fprintf(fp, "%%%%EOF\n");
	fclose(fp);

	return TRUE;
}

/*
 * Write out a PDF file.
 */
gboolean tiemu_screen_write_pdf(const gchar *filename, GdkPixbuf *pixbuf, GError **error)
{
	int h, w;
	FILE *fp;
	long obj5, obj6, obj7, xref, slen, slenp;
	struct tm *t;
	time_t tt;
	gboolean ret;
	GError *err;

	fp = fopen(filename, "wb");
	if (fp == NULL) {
		g_set_error(error, 0, 0, _("Couldn't open destination file for writing!"));
		return FALSE;
	}

	h = gdk_pixbuf_get_height(pixbuf);
	w = gdk_pixbuf_get_width(pixbuf);

	tt = time(NULL);
	t = gmtime(&tt);

	fprintf(fp, "%%PDF-1.5\n");
	fprintf(fp, "\n");
	fprintf(fp, "1 0 obj\n");
	fprintf(fp, "   << /Type /Catalog\n");
	fprintf(fp, "      /Outlines 2 0 R\n");
	fprintf(fp, "      /Pages 3 0 R\n");
	fprintf(fp, "   >>\n");
	fprintf(fp, "endobj\n");
	fprintf(fp, "\n");
	fprintf(fp, "2 0 obj\n");
	fprintf(fp, "   << /Type /Outlines\n");
	fprintf(fp, "      /Count 0\n");
	fprintf(fp, "   >>\n");
	fprintf(fp, "endobj\n");
	fprintf(fp, "\n");
	fprintf(fp, "3 0 obj\n");
	fprintf(fp, "   << /Type /Pages\n");
	fprintf(fp, "      /Kids [4 0 R]\n");
	fprintf(fp, "      /Count 1\n");
	fprintf(fp, "   >>\n");
	fprintf(fp, "endobj\n");
	fprintf(fp, "\n");
	fprintf(fp, "4 0 obj\n");
	fprintf(fp, "    << /Type /Page\n");
	fprintf(fp, "       /Parent 3 0 R\n");
	fprintf(fp, "       /MediaBox [0 0 %d %d]\n", w, h);
	fprintf(fp, "       /Contents 5 0 R\n");
	fprintf(fp, "       /Resources << /ProcSet 6 0 R >>\n");
	fprintf(fp, "    >>\n");
	fprintf(fp, "endobj\n");
	fprintf(fp, "\n");

	/* Offset of object 5, for xref */
	obj5 = ftell(fp);

	fprintf(fp, "5 0 obj\n");
	fprintf(fp, "    << /Length          >>\n");

	/* Position of the stream length, to be written later on */
	slenp = ftell(fp) - 12;

	fprintf(fp, "stream\n");

	/* Start of the stream data */
	slen = ftell(fp);

	fprintf(fp, "q\n");
	fprintf(fp, "%d 0 0 %d 0 0 cm\n", w, h);
	fprintf(fp, "BI\n");
	fprintf(fp, "  /W %d\n", w);
	fprintf(fp, "  /H %d\n", h);

	/* RGB, 8 bits per component, ASCIIHex encoding */
	fprintf(fp, "  /CS /RGB\n");
	fprintf(fp, "  /BPC 8\n");
	fprintf(fp, "  /F [/A85 /FlateDecode]\n");
	fprintf(fp, "ID\n");

	ret = write_compressed_a85_screen(fp, pixbuf, &err);

	if (!ret) {
		g_propagate_error(error, err);
		fclose(fp);
		unlink(filename);
		return FALSE;
	}

	fprintf(fp, "EI\n");
	fprintf(fp, "Q\n");

	/* Go back and write the length of the stream */
	slen = ftell(fp) - slen - 1;
	fseek(fp, slenp, SEEK_SET);
	fprintf(fp, "%lu", slen);
	fseek(fp, 0L, SEEK_END);

	fprintf(fp, "endstream\n");
	fprintf(fp, "endobj\n");
	fprintf(fp, "\n");

	/* Offset of object 6, for xref */
	obj6 = ftell(fp);

	fprintf(fp, "6 0 obj\n");
	fprintf(fp, "    [/PDF]\n");
	fprintf(fp, "endobj\n");
	fprintf(fp, "\n");

	/* Offset of object 7, for xref */
	obj7 = ftell(fp);

	fprintf(fp, "7 0 obj\n");
	fprintf(fp, "   << /Title (TiEmu %s screenshot)\n",
		ti68k_calctype_to_string(tihw.calc_type));
	fprintf(fp, "      /Creator (TiEmu / PDF output Copyright (C) 2005 Julien BLACHE)\n");
	fprintf(fp, "      /Producer (TiEmu %s)\n", TIEMU_VERSION);
	fprintf(fp, "      /CreationDate (D:%04d%02d%02d%02d%02d%02d+00'00')\n",
		1900 + t->tm_year, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
	fprintf(fp, "   >>\n");
	fprintf(fp, "endobj\n");
	fprintf(fp, "\n");

	/* Offset of xref, for startxref below */
	xref = ftell(fp);

	fprintf(fp, "xref\n");
	fprintf(fp, "0 8\n");
	fprintf(fp, "0000000000 65535 f \n");
	fprintf(fp, "0000000010 00000 n \n");
	fprintf(fp, "0000000094 00000 n \n");
	fprintf(fp, "0000000153 00000 n \n");
	fprintf(fp, "0000000229 00000 n \n");
	fprintf(fp, "%010lu 00000 n \n", obj5);
	fprintf(fp, "%010lu 00000 n \n", obj6);
	fprintf(fp, "%010lu 00000 n \n", obj7);
	fprintf(fp, "\n");
	fprintf(fp, "trailer\n");
	fprintf(fp, "    << /Size 8\n");
	fprintf(fp, "       /Root 1 0 R\n");
	fprintf(fp, "       /Info 7 0 R\n");
	fprintf(fp, "    >>\n");
	fprintf(fp, "startxref\n");
	fprintf(fp, "%lu\n", xref);
	fprintf(fp, "%%%%EOF\n");

	fclose(fp);

	return TRUE;
}
