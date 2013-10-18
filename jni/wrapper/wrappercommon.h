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


#ifndef WRAPPER_COMMON_H
#define WRAPPER_COMMON_H

	#include <stdint.h>
	#include <stdbool.h>

	#define	CALC_TYPE_TI89		 	1
	#define	CALC_TYPE_TI89T		 	2
	#define	CALC_TYPE_V200		 	3
	#define	CALC_TYPE_TI92		 	4
	#define	CALC_TYPE_TI92PLUS		5
	#define	CALC_TYPE_TI84PLUS_SE	6
	#define	CALC_TYPE_TI84PLUS	   	7
	#define	CALC_TYPE_TI83PLUS_SE  	8
	#define	CALC_TYPE_TI83PLUS  	9
	#define	CALC_TYPE_TI83  	    10

	typedef struct
	{
		uint8_t r;
		uint8_t g;
		uint8_t b;
	}RGB_struct;

	typedef struct
	{
		uint8_t a;
		uint8_t r;
		uint8_t g;
		uint8_t b;
	}ARGB_struct;

	typedef union
	{
		uint32_t color_int;
		ARGB_struct color_struct;
	}color;

	typedef struct
	{
		uint32_t pixel_on;
		uint32_t pixel_off;
		uint32_t grid_color;

		uint32_t grid_on_color; //derived

	}skin_colors_struct;

	typedef struct
	{
		uint32_t* buffer;
		uint32_t  length;

		int width;
		int height;

	}display_buffer_struct;


	typedef struct
	{
		int calc_type;
		double speed_coefficient;
		int screen_zoom;
		bool is_grayscale;
		bool is_grid;
		skin_colors_struct skin_colors;
		display_buffer_struct display_buffer_not_zoomed;

		uint8_t* grid_mask;
	}graph89_emulator_params_struct;


	extern bool is_tilem;
	extern bool is_tiemu;
	extern uint32_t g89_crc_table[];
	extern const uint8_t g89_shift_table[];
	extern graph89_emulator_params_struct graph89_emulator_params;

	void graph89_init_commons(int calc_type, int screen_width, int screen_height, int zoom, bool is_grayscale, bool is_grid, uint32_t pixel_on_color,
			uint32_t pixel_off_color, uint32_t grid_color, double speed_coefficient, const char* tmp_dir);
	void graph89_clean_commons();
	int  graph89_read_emulated_screen (uint8_t *return_flags);
	void graph89_get_emulated_screen (uint32_t* out_buffer, int out_buffer_length);
	void graph89_update_screen_zoom(int screen_zoom);
	int  graph89_install_rom(const char* source, const char* destination, int calc_type, int is_rom);
	void graph89_send_key(int key_code, int is_pressed);
	void graph89_send_keys(int* key_codes, int length);

#endif
