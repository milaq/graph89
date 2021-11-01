/*
 *   Graph89 - Emulator for Android
 *
 *	 Copyright (C) 2012-2013  Dritan Hashorva
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.

 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>
 */


#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <wrappercommon.h>
#include <tiemuwrapper.h>
#include <tilemwrapper.h>
#include <androidlog.h>

const uint8_t g89_shift_table[]={0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1};
uint32_t g89_crc_table[256];

bool is_tiemu = false;
bool is_tilem = false;

graph89_emulator_params_struct graph89_emulator_params = {0};

static void calc_type_parse(int calc_type);
static bool calc_type_is_tiemu(int calc_type);
static bool calc_type_is_tilem(int calc_type);
static void init_engines();
static void init_display_buffer(display_buffer_struct* buffer, int width, int height);
static void free_display_buffers();
static void free_display_buffer(display_buffer_struct* disp);
static void build_crc_table(void);
static void scale_line(uint32_t *target, uint32_t *source, int src_width, int tgt_width);
static void scale_rect(uint32_t *target, uint32_t *source, int src_width, int src_height, int tgt_width, int tgt_height);
static void set_display_colors(uint32_t pixel_on, uint32_t pixel_off, uint32_t grid_color);
static int  average_colors(uint32_t col1, uint32_t col2);
static void build_grid_mask();

void graph89_init_commons(int calc_type, int screen_width, int screen_height, int screen_zoom, bool is_grayscale, bool is_grid, uint32_t pixel_on_color,
		uint32_t pixel_off_color, uint32_t grid_color, double speed_coefficient, const char* tmp_dir)
{
	graph89_clean_commons();

	build_crc_table();

	graph89_emulator_params.calc_type = calc_type;
	init_display_buffer(&graph89_emulator_params.display_buffer_not_zoomed, screen_width, screen_height);
	graph89_emulator_params.screen_zoom = screen_zoom;
	graph89_emulator_params.is_grayscale = is_grayscale;
	graph89_emulator_params.is_grid = is_grid;
	graph89_emulator_params.speed_coefficient = speed_coefficient;

	set_display_colors(pixel_on_color, pixel_off_color, grid_color);

	if (is_grid) build_grid_mask();

	calc_type_parse(calc_type);

	if (is_tiemu)
	{
		tiemu_set_tmp_dir(tmp_dir);
	}

	init_engines();
}

void graph89_clean_commons()
{
	tiemu_clean();
	tilem_clean();
	free_display_buffers();

	is_tiemu = false;
	is_tilem = false;
}

int graph89_read_emulated_screen (uint8_t *return_flags)
{
	if (is_tiemu)
	{
		return tiemu_read_emulated_screen(return_flags);
	}
	else if (is_tilem)
	{
		return tilem_read_emulated_screen(return_flags);
	}
}

void graph89_get_emulated_screen (uint32_t* out_buffer, int out_buffer_length)
{
	int zoom = graph89_emulator_params.screen_zoom;
	uint32_t* not_scaled_buffer = graph89_emulator_params.display_buffer_not_zoomed.buffer;
	uint32_t  not_scaled_buffer_length = graph89_emulator_params.display_buffer_not_zoomed.length;
	int not_scaled_buffer_width = graph89_emulator_params.display_buffer_not_zoomed.width;
	int not_scaled_buffer_height = graph89_emulator_params.display_buffer_not_zoomed.height;

	if (out_buffer_length != not_scaled_buffer_width * not_scaled_buffer_height * zoom * zoom)
	{
		LOGI("Error: buffer size doesn't match %d with %d", out_buffer_length, not_scaled_buffer_width * not_scaled_buffer_height * zoom * zoom);
		return;
	}

	scale_rect(out_buffer, not_scaled_buffer, not_scaled_buffer_width, not_scaled_buffer_height, not_scaled_buffer_width * zoom, not_scaled_buffer_height * zoom);

	if (graph89_emulator_params.is_grid)
	{
		int i;
		uint32_t avgColor = graph89_emulator_params.skin_colors.grid_on_color;
		uint32_t pixel_on = graph89_emulator_params.skin_colors.pixel_on;
		uint32_t grid_color = graph89_emulator_params.skin_colors.grid_color;
		uint8_t* grid_mask = graph89_emulator_params.grid_mask;

		for (i = 0; i < out_buffer_length; ++i)
		{
			if (!grid_mask[i])
			{
				if (out_buffer[i] == pixel_on)
				{
					out_buffer[i] = avgColor;
				}
				else
				{
					out_buffer[i] = grid_color;
				}
			}
		}
	}
}

void graph89_update_screen_zoom(int screen_zoom)
{
	graph89_emulator_params.screen_zoom = screen_zoom;

	if (graph89_emulator_params.is_grid)
	{
		free(graph89_emulator_params.grid_mask);
		build_grid_mask();
	}
}

int graph89_install_rom(const char* source, const char* destination, int calc_type, int is_rom)
{
	if (calc_type_is_tiemu(calc_type))
	{
		return tiemu_install_rom(source, destination, calc_type, is_rom);
	}
	else if (calc_type_is_tilem(calc_type))
	{
		return tilem_install_rom(source, destination, calc_type, is_rom);
	}

	return -1;
}

void graph89_send_key(int key_code, int is_pressed)
{
	if (is_tiemu)
	{
		tiemu_send_key(key_code, is_pressed);
	}
	else if (is_tilem)
	{
		tilem_send_key(key_code, is_pressed);
	}
}

void graph89_send_keys(int* key_codes, int length)
{
	if (is_tiemu)
	{
		tiemu_send_keys(key_codes, length);
	}
	else if (is_tilem)
	{
		tilem_send_keys(key_codes, length);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void calc_type_parse(int calc_type)
{
	is_tiemu = false;
	is_tilem = false;

	is_tiemu = calc_type_is_tiemu(calc_type);

	if (!is_tiemu)
	{
		is_tilem = calc_type_is_tilem(calc_type);
	}
}

static bool calc_type_is_tiemu(int calc_type)
{
	switch (calc_type)
	{
		case CALC_TYPE_TI89:
		case CALC_TYPE_TI89T:
		case CALC_TYPE_V200:
		case CALC_TYPE_TI92PLUS:
		case CALC_TYPE_TI92:
			return true;
	}

	return false;
}

static bool calc_type_is_tilem(int calc_type)
{
	switch (calc_type)
	{
		case CALC_TYPE_TI84PLUS_SE:
		case CALC_TYPE_TI84PLUS:
		case CALC_TYPE_TI83PLUS_SE:
		case CALC_TYPE_TI83PLUS:
		case CALC_TYPE_TI83:
			return true;
	}

	return false;
}

static void init_engines()
{
	if (is_tiemu)
	{
		tiemu_init();
	}
	else if (is_tilem)
	{
		tilem_init();
	}
}

static void init_display_buffer(display_buffer_struct* buffer, int width, int height)
{
	buffer->width = width;
	buffer->height = height;
	buffer->length = width * height;
	buffer->buffer = (uint32_t*) malloc(buffer->length * sizeof(uint32_t));
}

static void free_display_buffers()
{
	free(graph89_emulator_params.display_buffer_not_zoomed.buffer);
	graph89_emulator_params.display_buffer_not_zoomed.buffer = NULL;
	graph89_emulator_params.display_buffer_not_zoomed.length = 0;

	free (graph89_emulator_params.grid_mask);
	graph89_emulator_params.grid_mask = NULL;
}

static void set_display_colors(uint32_t pixel_on, uint32_t pixel_off, uint32_t grid_color)
{
	graph89_emulator_params.skin_colors.pixel_on = pixel_on;
	graph89_emulator_params.skin_colors.pixel_off = pixel_off;
	graph89_emulator_params.skin_colors.grid_color = grid_color;

	graph89_emulator_params.skin_colors.grid_on_color = average_colors(grid_color, pixel_on);
}

static void build_crc_table(void)
{
	uint32_t i,j;

    for (i = 0; i < 256; i++)
    {
        uint32_t c = i;
        for (j = 0; j < 8; j++)
        {
            c = (c & 1) ? (0xEDB88320 ^ (c >> 1)) : (c >> 1);
        }
        g89_crc_table[i] = c;
    }
}

static void scale_line(uint32_t *target, uint32_t *source, int src_width, int tgt_width)
{
  int NumPixels = tgt_width;
  int IntPart = src_width / tgt_width;
  int FractPart = src_width % tgt_width;
  int E = 0;
  while (NumPixels-- > 0)
  {
    *target++ = *source;
    source += IntPart;
    E += FractPart;
    if (E >= tgt_width)
    {
      E -= tgt_width;
      source++;
    }
  }
}

static void scale_rect(uint32_t *target, uint32_t *source, int src_width, int src_height, int tgt_width, int tgt_height)
{
  int NumPixels = tgt_height;
  int IntPart = (src_height / tgt_height) * src_width;
  int FractPart = src_height % tgt_height;
  int E = 0;
  uint32_t *PrevSource = NULL;
  while (NumPixels-- > 0)
  {
    if (source == PrevSource)
    {
      memcpy(target, target-tgt_width, tgt_width*sizeof(*target));
    }
    else
    {
      scale_line(target, source, src_width, tgt_width);
      PrevSource = source;
    }
    target += tgt_width;
    source += IntPart;
    E += FractPart;
    if (E >= tgt_height)
    {
      E -= tgt_height;
      source += src_width;
    }
  }
}

static int average_colors(uint32_t col1, uint32_t col2)
{
	uint8_t alpha1 =   	(col1 & 0xFF000000) >> 24;
	uint8_t red1 =   	(col1 & 0x00FF0000) >> 16;
	uint8_t green1 = 	(col1 & 0x0000FF00) >> 8;
	uint8_t blue1 =   	col1 & 0x000000FF;

	uint8_t alpha2 =   	(col2 & 0xFF000000) >> 24;
	uint8_t red2 =   	(col2 & 0x00FF0000) >> 16;
	uint8_t green2 = 	(col2 & 0x0000FF00) >> 8;
	uint8_t blue2 =   	col2 & 0x000000FF;

	uint8_t out_alpha =   	(uint8_t) (alpha1 + ((int)alpha2 - (int)alpha1) / 4);
	uint8_t out_red =   	(uint8_t) (red1 + ((int)red2 - (int)red1) / 4);
	uint8_t out_green = 	(uint8_t) (green1 + ((int)green2 - (int)green1) / 4);
	uint8_t out_blue =  	(uint8_t) (blue1 + ((int)blue2 - (int)blue1) / 4);

	uint32_t out = out_alpha << 24 | out_red << 16 | out_green << 8 | out_blue;
	return out;
}

static void build_grid_mask()
{
	int zoom = graph89_emulator_params.screen_zoom;
	int width = graph89_emulator_params.display_buffer_not_zoomed.width * zoom;
	int height = graph89_emulator_params.display_buffer_not_zoomed.height * zoom;
	graph89_emulator_params.grid_mask = malloc(width * height * sizeof(uint8_t));

	uint8_t blank_lines = zoom / 5 + 1;
	uint8_t on = zoom - blank_lines;

	int i, j, k;

	uint8_t *mask;
	mask = calloc(zoom, sizeof(uint8_t));

	for (i = blank_lines / 2; i < on + blank_lines / 2; ++i)
	{
		mask[i] = 1;
	}

	for (j = 0; j < height; ++j)
	{
		int l = j * width;

		if (mask[j % zoom])
		{
			for (i = 0; i < width; ++i)
			{
				graph89_emulator_params.grid_mask[l + i] = mask[i % zoom];
			}
		}
		else
		{
			for (i = 0; i < width; ++i)
			{
				graph89_emulator_params.grid_mask[l + i] = 0;
			}
		}
	}

	free(mask);
}
