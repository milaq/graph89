/*
 *   Graph89 - Emulator for Android
 *	 Copyright (C) 2012-2013  Dritan Hashorva
 *
 *   libtilemcore - Graphing calculator emulation library
 *   Copyright (C) 2010 Benjamin Moody
 *
 *   WabbitEmu
 *   Copyright (C)  http://wabbit.codeplex.com/
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


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <wrappercommon.h>
#include <tilemwrapper.h>
#include <tilem.h>
#include <graph89_interface.h>
#include <ticalcs.h>
#include <wabbit.h>

TilemLCDBuffer* tilemlcdbuf = NULL;
TilemGrayLCD *glcd = NULL;

TilemCalcEmulator* emu = NULL;

static uint32_t pixelOnColor;
static uint32_t pixelOffColor;
static int raw_width = 0;
static int raw_height = 0;
static uint32_t* lcd_buffer_x1 = NULL;
static uint32_t lcd_buffer_x1_length = 0;
static bool is_grayscale = 0;
static dword* palette = NULL;
static dword cpalette[129];
static byte old_contrast = 0xFF;

static bool is_busy();
static void link_update_nop();
static TilemCalcEmulator* tilem_calc_emulator_new();
static void get_contrast_settings(unsigned int contrast, int *cbase, int *cfact);

static char rom_file[512];
static char sav_file[512];


void tilem_init()
{
	tilem_clean();

	emu = tilem_calc_emulator_new();

	ticables_library_init();
	tifiles_library_init();
	ticalcs_library_init();

	raw_width = graph89_emulator_params.display_buffer_not_zoomed.width;
	raw_height = graph89_emulator_params.display_buffer_not_zoomed.height;
	lcd_buffer_x1 = graph89_emulator_params.display_buffer_not_zoomed.buffer;
	lcd_buffer_x1_length = graph89_emulator_params.display_buffer_not_zoomed.length;

	pixelOnColor = graph89_emulator_params.skin_colors.pixel_on;
	pixelOffColor = graph89_emulator_params.skin_colors.pixel_off;

	is_grayscale = graph89_emulator_params.is_grayscale;

	if (is_grayscale)
	{
		glcd = tilem_gray_lcd_new(emu->calc, 4, 200);

		palette = tilem_color_palette_new(pixelOffColor >> 16 & 0xFF, pixelOffColor >> 8 & 0xFF, pixelOffColor & 0xFF, pixelOnColor >> 16 & 0xFF, pixelOnColor >> 8 & 0xFF, pixelOnColor & 0xFF, 2.2);
	}

	tilemlcdbuf = tilem_lcd_buffer_new();
}

void tilem_clean()
{
	tilem_lcd_buffer_free(tilemlcdbuf);
	tilemlcdbuf = NULL;
	tilem_gray_lcd_free(glcd);
	glcd = NULL;
	free(palette);
	palette = NULL;
	if (emu != NULL)
	{
		tilem_calc_free(emu->calc);
		g_queue_free(emu->task_queue);
		free(emu->link_update);
		emu = NULL;
	}
}

int tilem_load_image(const char * image_path)
{
	strncpy(rom_file, image_path, sizeof(rom_file) - 1);
	int ret = tilem_calc_load_ROM(emu->calc, image_path);
	return ret;
}


void state_userpages(TilemCalc* calc, upages_t *upages);

int tilem_reset()
{
	upages_t upages;
	state_userpages(emu->calc, &upages);
	if (upages.start == -1)
			return;

	if (upages.start >= 0)
	{
		memset(emu->calc->mem  + upages.end * 0x4000, 0xFF, (upages.start - upages.end + 1) * 0x4000);

		FILE* dest = fopen(rom_file, "wb");
		if (!dest)
		{
			return -4;
		}
		fwrite(emu->calc->mem, 1, emu->calc->hw.romsize, dest);
		fclose(dest);
	}

	tilem_calc_reset(emu->calc);
	return 0;
}

void tilem_run_engine()
{
	emu->calc->flash.unlock = TRUE;
	tilem_z80_run(emu->calc, 700000 * graph89_emulator_params.speed_coefficient, NULL);
}

void tilem_sync_clock()
{
	sync_clock_tilem(emu);
}

extern byte ti84pse_boot_image_1[];
extern byte ti84p_boot_image_1[];
extern int boot_image_84_size;

extern byte ti83pse_boot_image_1[];
extern byte ti83p_boot_image_1[];
extern int boot_image_83_size;


extern byte boot_image_2[];
extern int boot_image_2_start;
extern int boot_image_2_size;
extern LINK_ERR forceload_os(TilemCalc* calc, TIFILE_t *tifile);

int tilem_install_rom(const char* source, const char* destination, int calc_type, int is_rom)
{
	if(is_rom)
	{
		uint8_t buf[8192];
		size_t size;

		FILE* src = fopen(source, "rb");
		FILE* dest = fopen(destination, "wb");

		if (!src || !dest)
		{
			return -1;
		}

		while ((size = fread(buf, 1, 8192, src)) > 0)
		{
			fwrite(buf, 1, size, dest);
		}

		fclose(src);
		fclose(dest);
	}
	else
	{
		tilem_clean();
		graph89_emulator_params.calc_type = calc_type;
		emu = tilem_calc_emulator_new();
		ticables_library_init();
		tifiles_library_init();
		ticalcs_library_init();

		memset(emu->calc->mem, 0xFF, emu->calc->hw.romsize);
		switch(graph89_emulator_params.calc_type)
		{
			case CALC_TYPE_TI84PLUS_SE:
				memcpy(emu->calc->mem + emu->calc->hw.romsize - 0x4000, ti84pse_boot_image_1, boot_image_84_size);
				break;
			case CALC_TYPE_TI84PLUS:
				memcpy(emu->calc->mem + emu->calc->hw.romsize - 0x4000, ti84p_boot_image_1, boot_image_84_size);
				break;
			case CALC_TYPE_TI83PLUS_SE:
				memcpy(emu->calc->mem + emu->calc->hw.romsize - 0x4000, ti83pse_boot_image_1, boot_image_83_size);
				break;
			case CALC_TYPE_TI83PLUS:
				memcpy(emu->calc->mem + emu->calc->hw.romsize - 0x4000, ti83p_boot_image_1, boot_image_83_size);
				break;
			default:
				return -1;
		}

		memcpy(emu->calc->mem + emu->calc->hw.romsize - 0x4000 + boot_image_2_start, boot_image_2, boot_image_2_size);

		TIFILE_t *tifile = importvar(source, FALSE);

		if (tifile == NULL || tifile->type != FLASH_TYPE) return -2;
		LINK_ERR err = forceload_os(emu->calc, tifile);

		if (err != LERR_SUCCESS) return -3;

		calc_erase_certificate(emu->calc->mem, emu->calc->hw.romsize);

		FILE* dest = fopen(destination, "wb");
		if (!dest)
		{
			return -4;
		}
		fwrite(emu->calc->mem, 1, emu->calc->hw.romsize, dest);
		fclose(dest);
		tilem_clean();
	}

	return 0;
}

int tilem_read_emulated_screen (uint8_t *return_flags)
{
	int i, j, v;
	uint32_t crc = 0xFFFFFFFF;
	int cbase, cfact;

	if (!emu->calc->lcd.active)
	{
		int len = emu->calc->hw.lcdwidth * emu->calc->hw.lcdheight;
		int length = 0;
		for (i = 0; i < len; ++i)
		{
			crc = g89_crc_table[(crc ^ 0x00) & 0xFF] ^ (crc >> 8);

			lcd_buffer_x1[length++] = pixelOffColor;
		}
	}
	else
	{
		if (is_grayscale)
		{
			tilem_gray_lcd_get_frame(glcd, tilemlcdbuf);

			if (tilemlcdbuf->contrast != old_contrast)
			{
				old_contrast = tilemlcdbuf->contrast;
				get_contrast_settings(old_contrast, &cbase, &cfact);

				for (i = 0; i <= 128; i++)
				{
					v = ((i * cfact) >> 7) + cbase;
					cpalette[i] = palette[v];
				}
			}

		}
		else
		{
			tilem_lcd_get_frame(emu->calc, tilemlcdbuf);
		}

		int len = emu->calc->hw.lcdwidth * emu->calc->hw.lcdheight;

		int length = 0;

		if (is_grayscale)
		{
			for (i = 0; i < len; ++i)
			{
				byte b =  tilemlcdbuf->data[i];

				crc = g89_crc_table[(crc ^ b) & 0xFF] ^ (crc >> 8);

				lcd_buffer_x1[length++] = 0xFF000000 | cpalette[b];
			}
		}
		else
		{
			for (i = 0; i < len; ++i)
			{
				byte b =  tilemlcdbuf->data[i];

				crc = g89_crc_table[(crc ^ b) & 0xFF] ^ (crc >> 8);

				lcd_buffer_x1[length++] = b == 0 ? pixelOffColor : pixelOnColor;
			}
		}
	}

	return_flags[0] = !emu->calc->lcd.active;
	return_flags[1] = is_busy();

	crc ^= 0xFFFFFFFF;
	return crc;
}

void tilem_turn_screen_ON()
{
	//press ON button
	tilem_z80_run(emu->calc, 6000000, NULL);
	tilem_keypad_press_key(emu->calc, 0x29);
	tilem_z80_run(emu->calc, 4000000, NULL);
	tilem_keypad_release_key(emu->calc, 0x29);
}

int tilem_load_state(const char* state_file)
{
	return tilem_calc_load_STATE(emu->calc, state_file);
}

int tilem_save_state(const char* rom_file, const char* state_file)
{
	strncpy(sav_file, state_file, sizeof(sav_file) - 1);
	return tilem_calc_save_state(emu->calc, rom_file, state_file);
}

void tilem_send_key(int key_code, int is_pressed)
{
	if (is_pressed)
	{
		tilem_keypad_press_key(emu->calc, key_code);
	}
	else
	{
		tilem_keypad_release_key(emu->calc, key_code);
	}
}

void tilem_send_keys(int* key_codes, int length)
{
	int i;
	for (i = 0; i < length; ++i)
	{
		if (key_codes[i] == 0xFF) continue;

		tilem_keybufferPush(key_codes[i]);
	}
}

extern LINK_ERR WabbitSendFile(TilemCalc *calc, const char* filename);
extern LINK_ERR forceload_app(TilemCalc* calc, TIFILE_t *tifile);
int tilem_send_file(const char* filename)
{
	SEND_FLAG destination = SEND_CUR;

	TIFILE_t *var = importvar(filename, destination);

	if ((graph89_emulator_params.calc_type == CALC_TYPE_TI84PLUS_SE || graph89_emulator_params.calc_type == CALC_TYPE_TI84PLUS || graph89_emulator_params.calc_type == CALC_TYPE_TI83PLUS_SE)
			&&  var->type == FLASH_TYPE && var->flash->type == FLASH_TYPE_APP)
	{
		int msize = (emu->calc->hw.romsize + emu->calc->hw.ramsize + emu->calc->hw.lcdmemsize);

		char* membackup = (char*) malloc(msize);
		memcpy(membackup, emu->calc->mem, msize);

		int ret = forceload_app(emu->calc, var);

		if (ret == LERR_SUCCESS)
		{
			free (membackup);
			membackup = NULL;

			FILE* dest = fopen(rom_file, "wb");
			if (!dest)
			{
				return -4;
			}
			fwrite(emu->calc->mem, 1, emu->calc->hw.romsize, dest);
			fclose(dest);

			return 0;
		}
		else
		{
			memcpy(emu->calc->mem, membackup, msize);
			free(membackup);
			membackup = NULL;
		}
	}

	return tilem_link_send_file(emu, filename, -1, TRUE, TRUE) != 0 ? -10 : 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

static bool is_busy()
{
	if (is_grayscale) return false;

	bool is_busy = false;
	int i;

	for (i = 1; i <= 8; ++i)
	{
		if (lcd_buffer_x1[i * raw_width -1] == pixelOnColor)
			return true;
	}

	return false;
}

static void link_update_nop()
{
}

static TilemCalcEmulator* tilem_calc_emulator_new()
{
	TilemCalcEmulator* emu = g_new0(TilemCalcEmulator, 1);
	CalcUpdate *update;
	int rate, channels;
	double latency, volume;
	char *driver;

	update = g_new0(CalcUpdate, 1);
	update->start = &link_update_nop;
	update->stop = &link_update_nop;
	update->refresh = &link_update_nop;
	update->pbar = &link_update_nop;
	update->label = &link_update_nop;
	emu->link_update = update;

	emu->ext_cable_in = -1;
	emu->ext_cable_out = -1;

	emu->task_queue = g_queue_new();

	switch(graph89_emulator_params.calc_type)
	{
		case CALC_TYPE_TI84PLUS_SE:
			emu->calc = tilem_calc_new(TILEM_CALC_TI84P_SE);
			break;
		case CALC_TYPE_TI84PLUS:
			emu->calc = tilem_calc_new(TILEM_CALC_TI84P);
			break;
		case CALC_TYPE_TI83PLUS_SE:
			emu->calc = tilem_calc_new(TILEM_CALC_TI83P_SE);
			break;
		case CALC_TYPE_TI83PLUS:
			emu->calc = tilem_calc_new(TILEM_CALC_TI83P);
			break;
		case CALC_TYPE_TI83:
			emu->calc = tilem_calc_new(TILEM_CALC_TI83);
			break;
	}

	return emu;
}

static void get_contrast_settings(unsigned int contrast,
                                  int *cbase, int *cfact)
{
	if (contrast < 32) {
		*cbase = 0;
		*cfact = contrast * 8;
	}
	else {
		*cbase = (contrast - 32) * 8;
		*cfact = 255 - *cbase;
	}
}
