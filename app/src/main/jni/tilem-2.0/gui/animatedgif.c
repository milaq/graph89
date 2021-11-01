/*
 * TilEm II
 *
 * Copyright (c) 2010-2011 Thibault Duponchelle
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <gtk/gtk.h>
#include <ticalcs.h>
#include <tilem.h>
#include "gui.h"


static void write_global_header(FILE* fp, int width, int height, byte* palette, int palette_size);
static void write_global_footer(FILE* fp);
static void write_extension_block(FILE* fout, word delay);
static void write_image_block_start(FILE *fp, int width, int height);
static void write_image_block_end(FILE *fp);
static void write_comment(FILE* fp);
static void write_application_extension(FILE * fp) ;

static void write_global_header(FILE* fp, int width, int height, byte* palette, int palette_size) {
	
	/* Magic number for Gif file format */
    	char global_header_magic_number[] = {'G', 'I', 'F', '8', '9', 'a'};
    	/* Size of canvas width on 2 bytes, heigth on 2 bytes */
	char global_header_canvas[] = {96, 0, 64, 0 };

	global_header_canvas[0] = width; 
	global_header_canvas[1] = (width >> 8) ; 
	global_header_canvas[2] = height; 
	global_header_canvas[3] = (height >> 8); 

	/* Flag */
	/* The 11th byte is a set of flags  : 
	bit 0:    Global Color Table Flag (GCTF)
        bit 1..3: Color Resolution
        bit 4:    Sort Flag to Global Color Table
        bit 5..7: Size of Global Color Table: 2^(1+n)
	It means "use the GCT wich is given after (from the size bit 5..7) and a resolution bit 1..3 
	The Background color is an index in the Global Color Table
	*/
	/* FIXME : if we change the palette size, we need to change this flag too and I don't do this currently */
    	char global_header_flag[] = { 0xf7 };
	/* The index in global color table */
	char global_header_background_index[] = {0x00};
	/* Aspect pixel ratio (unknown) */
	char global_header_aspect_pixel_ratio[] = {0x00};
	
	
	fwrite(global_header_magic_number, 6, 1, fp);
	fwrite(global_header_canvas, 4, 1, fp);
	fwrite(global_header_flag, 1, 1, fp);
	fwrite(global_header_background_index, 1, 1, fp);
	fwrite(global_header_aspect_pixel_ratio, 1, 1, fp);
	
	//byte* palette = tilem_color_palette_new_packed(255, 255, 255, 0, 0, 0, 2.2);
	
	fwrite(palette, palette_size * 3, 1, fp);
}

static void write_global_footer(FILE* fp) {

	/* This value means end of gif file */	
	char footer_trailer[1] = { 0x3b};
	
	fwrite(footer_trailer, 1, 1,fp);
}


static void write_extension_block(FILE* fp, word delay) {

	/* Extension block introduced by 0x21 ('!'), size before extension_block_terminator, flag byte, delay (10/100) 2 bytes   */
	char extension_block_header[2] = {0x21, 0xf9};
	/* Size before extension_block_terminator */
	char extension_block_size[1] = { 0x04} ;
	/* Flag (unknown) */
	char extension_block_flag[1] = { 0x00} ;
	/* Delay (x/100 sec) on 2 bytes*/
	char extension_block_delay[2] = {10, 0} ;
	extension_block_delay[0] = delay;
	/* The index designed by this variable become transparent even if palette gives a black(or something else) color. */ 
	char extension_block_transparent_index[1] = {0xff};
	/* End of extension block */
	char extension_block_terminator[1] = {0x00};

	fwrite(extension_block_header, 2, 1, fp);
    	fwrite(extension_block_size, 1, 1, fp);
    	fwrite(extension_block_flag, 1, 1, fp);
    	fwrite(extension_block_delay, 2, 1, fp);
    	fwrite(extension_block_transparent_index, 1, 1, fp);
    	fwrite(extension_block_terminator, 1, 1, fp);

}

static void write_image_block_start(FILE *fp, int width, int height) {

	/* Header */
	char image_block_header[] = { 0x2c};
	/* Left corner x (2 bytes), left corner y (2 bytes), width (2 bytes), height (2 bytes) */
	char image_block_canvas[] = { 0, 0, 0, 0, 96, 0, 64, 0};
	
	image_block_canvas[4] = width; 
	image_block_canvas[5] = (width >> 8) ; 
	image_block_canvas[6] = height; 
	image_block_canvas[7] = (height >> 8); 
	/* Flag */
	char image_block_flag[] = { 0x00 };

        fwrite(image_block_header, 1, 1, fp);
    	fwrite(image_block_canvas, 8, 1, fp);
    	fwrite(image_block_flag, 1, 1, fp);

}

static void write_image_block_end(FILE *fp) {
	
 	/* Give an end to the image block */
	char image_block_end[1] = {0x00};

	fwrite(image_block_end, 1, 1,fp);
}

static void write_comment(FILE* fp) {

	char comment[] = {0x21, 0xfe, 8, 'T', 'i', 'l', 'E', 'm', '2', 0, 0, 0};
	fwrite(comment, 12, 1, fp);
}

static void write_application_extension(FILE * fp) {

	/* Magic number to start the block */
	char application_extension_magic_number[] = { 0x21, 0xff, 0x0b };
	/* Application name */
	char application_extension_application_name[] = { 'N', 'E', 'T', 'S', 'C', 'A', 'P', 'E', '2', '.', '0' };
	/* magic number */
	char application_extension_data_follow[] = { 0x03, 0x01 };
	/* 0 to 65535 loop */
	char application_extension_number_of_loop[] = { 0xff, 0xff};
	/* the end of the block */	
	char application_extension_terminator[] = { 0x00 };
	
	//char gif_infos[31] = {
        //0x21, 0xff, 0x0b, 'N', 'E', 'T', 'S', 'C', 'A', 'P', 'E', '2', '.', '0', 3, 1, 0xff, 0xff, 0x00	};

	//fwrite(gif_infos, 19, 1, fp);
	fwrite(application_extension_magic_number, 3, 1, fp);
	fwrite(application_extension_application_name, 11, 1, fp);
	fwrite(application_extension_data_follow, 2, 1, fp);
	fwrite(application_extension_number_of_loop, 2, 1, fp);
	fwrite(application_extension_terminator, 1, 1, fp);
}

/* Apparently, most current web browsers are seriously and
   deliberately broken in their handling of animated GIFs.  Internet
   Explorer does not allow any frame to be shorter than 60 ms, and
   Gecko does not allow any frame shorter than 20 ms.  Furthermore,
   rather than simply imposing a lower limit, or skipping frames,
   these browsers take any frame they deem "too short" and extend it
   to a full 100 ms out of sheer spite.

   If we want animations to look correct in all web browsers (which
   is, after all, the main reason for using GIF animations in the
   first place), we have to limit ourselves to 60-ms frames or
   longer. */
#define MIN_FRAME_DELAY 6

void tilem_animation_write_gif(TilemAnimation *anim, byte* palette, int palette_size, FILE *fp)
{
	GdkPixbufAnimation *ganim;
	int width, height, delay, n;
	gdouble time_stretch, t;
	byte *image;
	TilemAnimFrame *frm, *next;
	gboolean is_static;

	g_return_if_fail(TILEM_IS_ANIMATION(anim));
	g_return_if_fail(fp != NULL);

	ganim = GDK_PIXBUF_ANIMATION(anim);
	width = gdk_pixbuf_animation_get_width(ganim);
	height = gdk_pixbuf_animation_get_height(ganim);
	is_static = gdk_pixbuf_animation_is_static_image(ganim);
	time_stretch = 1.0 / tilem_animation_get_speed(anim);

	frm = tilem_animation_next_frame(anim, NULL);
	g_return_if_fail(frm != NULL);

	write_global_header(fp, width, height, palette, palette_size);

	if (!is_static)
		write_application_extension(fp);

	write_comment(fp);

	t = MIN_FRAME_DELAY * 5.0;

	/* FIXME: combine multiple frames by averaging rather than
	   simply taking the last one */

	while (frm) {
		next = tilem_animation_next_frame(anim, frm);

		if (!is_static) {
			delay = tilem_anim_frame_get_duration(frm);
			t += delay * time_stretch;
			n = t / 10.0;

			if (n < MIN_FRAME_DELAY && next != NULL) {
				frm = next;
				continue;
			}

			t -= n * 10.0;
			if (n > 0xffff)
				n = 0xffff;
			else if (n < MIN_FRAME_DELAY)
				n = MIN_FRAME_DELAY;
			write_extension_block(fp, n);
		}

		tilem_animation_get_indexed_image(anim, frm, &image,
		                                  &width, &height);
		write_image_block_start(fp, width, height);
		GifEncode(fp, image, 8, width * height);
		write_image_block_end(fp);
		g_free(image);

		frm = next;
	}

	write_global_footer(fp);
}
