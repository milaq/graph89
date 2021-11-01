/*
 *   Graph89 - Emulator for Android
 *	 Copyright (C) 2012-2013  Dritan Hashorva
 *
 *	 TiEmu - Tiemu Is an EMUlator
 *   Copyright (c) 2000, Thomas Corvazier, Romain Lievin
 *   Copyright (c) 2001-2002, Romain Lievin, Julien Blache
 *   Copyright (c) 2003-2004, Romain Liévin
 *   Copyright (c) 2005-2006, Kevin Kofler
 *
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



#include <ti68k_def.h>
#include <wrappercommon.h>
#include <tiemuwrapper.h>
#include <androidlog.h>
#include <string.h>

extern CalcHandle*  calc_handle;
extern int enable_grayscale;
extern uint32_t lcd_planes[];
extern uint8_t *lcd_planebufs[];
extern int lcd_changed;
extern int ngc;

static int raw_width = 0;
static int raw_height = 0;
static uint32_t* lcd_buffer_x1 = NULL;
static uint32_t* lcd_buffer_tmp_x1 = NULL;
static uint32_t lcd_buffer_x1_length = 0;

static uint32_t pixelOnColor;
static uint32_t pixelOffColor;

static int read_screen_blank();
static int read_screen_BW();
static int read_screen_grayscale();
static void compute_convtable(void);
static void compute_grayscale(void);

void tiemu_init()
{
	tiemu_clean();

	raw_width = graph89_emulator_params.display_buffer_not_zoomed.width;
	raw_height = graph89_emulator_params.display_buffer_not_zoomed.height;
	lcd_buffer_x1 = graph89_emulator_params.display_buffer_not_zoomed.buffer;
	lcd_buffer_x1_length = graph89_emulator_params.display_buffer_not_zoomed.length;

	pixelOnColor = graph89_emulator_params.skin_colors.pixel_on;
	pixelOffColor = graph89_emulator_params.skin_colors.pixel_off;

	enable_grayscale = graph89_emulator_params.is_grayscale;

	if (enable_grayscale)
	{
		compute_convtable();
		compute_grayscale();

		lcd_buffer_tmp_x1 = (uint32_t*)malloc(LCDMEM_W * LCDMEM_H * sizeof(uint32_t));
	}
}

void tiemu_clean()
{
	free(lcd_buffer_tmp_x1);
	lcd_buffer_tmp_x1 = NULL;

	ti68k_exit();

	memset(&params, 0, sizeof(Ti68kParameters));
	memset(&tihw, 0, sizeof(Ti68kHardware));
	memset(&linkp, 0, sizeof(Ti68kLinkPort));
	memset(&bkpts, 0, sizeof(Ti68kBreakpoints));
	memset(&logger, 0, sizeof(Ti68kLogging));

	calc_handle = NULL;
}

void tiemu_step1_load_defaultconfig()
{
	ti68k_config_load_default();
}

int tiemu_step2_load_image(const char * image_path)
{
	return ti68k_load_image(image_path);
}

int tiemu_step3_init()
{
	return ti68k_init();
}

int tiemu_step4_reset()
{
	int code = ti68k_reset();

	if (enable_grayscale)
	{
		lcd_planes[0] = tihw.lcd_adr;
		lcd_planebufs[0] = &tihw.ram[tihw.lcd_adr];
		ngc = 1;
		lcd_changed = 1;
	}

	return code;
}

int tiemu_save_state(const char* state_file)
{
	if (tihw.ram)
	{
		return ti68k_state_save(state_file);
	}
	else
	{
		return -1;
	}
}

int tiemu_load_state(const char* state_file)
{
	return ti68k_state_load(state_file);
}

int tiemu_upload_file(const char* file_name)
{
	return ti68k_linkport_send_file(file_name);
}

void tiemu_send_key(int key_code, int is_pressed)
{
	ti68k_kbd_set_key(key_code, is_pressed);
}

void tiemu_send_keys(int* key_codes, int length)
{
	int i;
	for (i = 0; i < length; ++i)
	{
		if (key_codes[i] == 0xFF) continue;

		KeyBufferPush(key_codes[i]);
	}
}

void tiemu_sync_clock()
{
	sync_clock();
}

void tiemu_turn_screen_ON()
{
	hw_m68k_irq(6);
}

void tiemu_run_engine()
{
	int cpu_cycles = engine_num_cycles_per_loop() * graph89_emulator_params.speed_coefficient;
	hw_m68k_run(cpu_cycles / 4);
}

int tiemu_read_emulated_screen (uint8_t *return_flags)
{
	int i, j, k;
	uint32_t crc = 0xFFFFFFFF;

	tihw.lcd_ptr = (char *) &tihw.ram[tihw.lcd_adr];

	int widthdiv8 = ((int)raw_width) / 8;
	int height = (int)raw_height;

	int CRC = 0;

	if (!tihw.on_off)
	{
		CRC = read_screen_blank();
	}
	else if (!enable_grayscale)
	{
		CRC = read_screen_BW();
	}
	else
	{
		CRC = read_screen_grayscale();
	}

	return_flags[0] = tihw.on_off == 0; //is screen off
	return_flags[1] = tihw.lcd_ptr[(height - 1) * LCDMEM_W / 8 + widthdiv8 - 1] & g89_shift_table[7];  //is calculator busy

	return CRC;
}

void tiemu_patch(const char* num, const char* vernum)
{
	 uint32_t addr;
	        romcalls_get_symbol_address(0x2A1, &addr);

	        addr -= tihw.rom_base;
	        addr &= tihw.rom_size - 1;

	        //LEA 4(PC), A0
	        tihw.rom[addr+0]= 0x41;
	        tihw.rom[addr+1]= 0xFA;
	        tihw.rom[addr+2]= 0x00;
	        tihw.rom[addr+3]= 0x04;

	        //RTS
	        tihw.rom[addr+4]= 0x4E;
	        tihw.rom[addr+5]= 0x75;

	        tihw.rom[addr+6]= num[0];
	        tihw.rom[addr+7]= num[1];
	        tihw.rom[addr+8]= num[2];
	        tihw.rom[addr+9]= num[3];
	        tihw.rom[addr+10]= num[4];

	        tihw.rom[addr+11]= num[5];
	        tihw.rom[addr+12]= num[6];
	        tihw.rom[addr+13]= num[7];
	        tihw.rom[addr+14]= num[8];
	        tihw.rom[addr+15]= num[9];
	        tihw.rom[addr+16]= 0x00;

	        romcalls_get_symbol_address(0x16d, &addr);

	        addr -= tihw.rom_base;
	        addr &= tihw.rom_size - 1;

	        int vnum = strtol(vernum, (char **)NULL, 16);

	        //move.w #$xxxx, d0
	        tihw.rom[addr+0]= 0x30;
	        tihw.rom[addr+1]= 0x3C;

	        tihw.rom[addr+2]= (uint8_t)(vnum >> 8);
	        tihw.rom[addr+3]= (uint8_t)(vnum & 0xFF);

	        //RTS
	        tihw.rom[addr+4]= 0x4E;
	        tihw.rom[addr+5]= 0x75;

}

void tiemu_set_tmp_dir(const char* tmp_dir)
{
	TMP_DIR = malloc((strlen(tmp_dir) + 1)*sizeof(char));
	strcpy(TMP_DIR, tmp_dir);
}

int tiemu_install_rom(const char* source, const char* destination, int calc_type, int is_rom)
{
	int code = 0, dummy = 0;

	if (is_rom)
	{
		code = ti68k_convert_rom_to_image(source, destination, &dummy);
	}
	else
	{
		code = ti68k_convert_tib_to_image(source, destination, -1, &dummy);
	}

	return code;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int read_screen_blank()
{
	int i;

	for (i = 0; i < lcd_buffer_x1_length; ++i)
	{
		lcd_buffer_x1[i] = pixelOffColor;
	}

	return -1;
}

static int read_screen_BW()
{
	int crc = 0xFFFFFFFF;

	int i, j, k;
	uint8_t *lcd_bitmap = &tihw.ram[tihw.lcd_adr];

	int widthdiv8 = ((int)raw_width) / 8;
	int height = (int)raw_height;

	int length = 0;

	for (j = 0; j < height; ++j)
	{
		int l = j * LCDMEM_W / 8;
		for (i = 0; i < widthdiv8; ++i)
		{
			uint8_t c = tihw.lcd_ptr[l + i];
			crc = g89_crc_table[(crc ^ c) & 0xFF] ^ (crc >> 8);

			for (k = 0; k < 8; ++k)
			{
				lcd_buffer_x1[length++] = c & g89_shift_table[k] ? pixelOnColor : pixelOffColor;
			}
		}
	}

	return crc ^ 0xFFFFFFFF;
}


#define BPP 8               // 8 bits per pixel
#define NGS 16              // number of gray scales (contrast level)
#define filter(v, l, h) (v<l ? l : (v>h ? h : v))

extern volatile int lcd_flag;

static uint32_t convtab[512];
static RGB_struct grayscales[16];

static int		lcd_state = -1;     // screen state

static int contrast = NGS;          // current contrast level
static int old_contrast = 0;        // previous contrast level
static int new_contrast = NGS;		// new contrast level

static int max_plane = 0;         	// number of grayscales to emulate

static int shot_cnt = 0;					// number of captures
static int skip_cnt = 0;					// number of frames to skip

// gray plane sequences in relation with gscales.c
static const int gp_seq[9][8] = {
	{ -1 },						// unused
	{ 0, -1 },					// b&w		(1 plane)
	{ -1 },						// unused
	{ 0, 0, 1, -1 },			// 4 colors (2 planes)
	{ -1 },						// unused
	{ -1 },						// unused
	{ -1 },						// unused
	{ 2, 0, 1, 0, 1, 0, -1 },	// 7 colors (3 planes)
	{ 1, 0, 2, 0, 0, 1, 0, -1 },// 8 colors (3 planes)
};

static int read_screen_grayscale()
{
	int crc = 0xFFFFFFFF;

	if(lcd_flag || (tihw.hw_type >= HW2))
	{
		int i, j, k, l;
		uint8_t *lcd_bitmap = &tihw.ram[tihw.lcd_adr];
		uint8_t *lcd_buf = (uint8_t *)lcd_buffer_tmp_x1;

		if(!tihw.lcd_ptr)
			goto exit;

		if(lcd_state != tihw.on_off)
		{
			lcd_state = tihw.on_off;
			lcd_changed = 1;
		}

		// Check for contrast change (from TI HW)
		if(contrast != tihw.contrast)
		{
			gint c = contrast = tihw.contrast;

			new_contrast = (c + old_contrast) / 2;
			old_contrast = c;

			compute_grayscale();

			lcd_changed = 1;
		}

		// Check for gray plane change (menu/hw)
		if(max_plane != ngc)
		{
			max_plane = ngc;
			compute_grayscale();
		}

		// LCD off or unchanged: don't refresh !
		if(!lcd_state || !lcd_changed)
		{
			goto exit;
		}

		// Reset LCD changed flag.
		lcd_changed = 0;

		// Convert the bitmap screen to a bytemap screen and grayscalize
		memset(lcd_buffer_tmp_x1, 0, LCDMEM_W * LCDMEM_H * sizeof(uint32_t));

		for(l = 0; l < 8; l++)
		{
			int pp = gp_seq[ngc][l];
			if(pp == -1) break;

			lcd_bitmap = lcd_planebufs[pp];

			for(j = 0, k = 0; k < LCDMEM_H; k++)
			{
				for(i = 0; i < LCDMEM_W/8; i++, lcd_bitmap++)
				{
					lcd_buffer_tmp_x1[j++] += convtab[(*lcd_bitmap << 1)  ];
					lcd_buffer_tmp_x1[j++] += convtab[(*lcd_bitmap << 1)+1];
				}
			}
		}

		int index = 0;
		for (j = 0; j < raw_height; ++j)
		{
			int scanline = j * LCDMEM_W;

			for (i = 0; i < raw_width; ++i)
			{
				RGB_struct pxx = grayscales[lcd_buf[scanline + i]];

				uint32_t col = 0xFF << 24 | pxx.r << 16 | pxx.g << 8 | pxx.b;

				crc = g89_crc_table[(crc ^ col) & 0xFF] ^ (crc >> 8);

				lcd_buffer_x1[index++] = col;
			}
		}
	}
exit:
	lcd_flag = 0;
	if(tihw.hw_type >= HW2)	lcd_hook_hw2(TRUE);
	return crc ^ 0xFFFFFFFF;
}

void compute_convtable(void)
{
  	int i, j;
  	uint8_t k;
  	uint8_t *tab = (uint8_t *)convtab;

  	for(i=0, j=0; i<256; i++)
    {
      	for(k = 1<<7; k; k>>=1)
		{
			tab[j++] = (i & k) ? 1 : 0;
		}
    }
}

void compute_grayscale(void)
{
  	int i;
  	int sr, sg, sb;
  	int er, eg, eb;
  	int r, g ,b;
    uint32_t white = pixelOffColor;
    uint32_t black = pixelOnColor;

	//printf("# planes: %i | contrast: %i\n", max_plane, contrast);

	// Compute RBG bsaic values
  	sr = (white & 0xff0000) >> 8;
  	sg = (white & 0x00ff00);
  	sb = (white & 0x0000ff) << 8;

  	er = (black & 0xff0000) >> 8;
  	eg = (black & 0x00ff00);
  	eb = (black & 0x0000ff) << 8;

	// Compute RGB values tuned with contrast
  	if(contrast < NGS)
    {
      	sr = sr - (sr-er) * (NGS - contrast)/NGS;
      	sg = sg - (sg-eg) * (NGS - contrast)/NGS;
      	sb = sb - (sb-eb) * (NGS - contrast)/NGS;
    }
  	else
    {
      	er = er - (er-sr)*(contrast - NGS)/NGS;
      	eg = eg - (eg-sg)*(contrast - NGS)/NGS;
      	eb = eb - (eb-sb)*(contrast - NGS)/NGS;
    }

  	r = sr;
  	g = sg;
  	b = sb;

  	if(lcd_state)
    {
      	for(i = 0; i <= (max_plane+1); i++)
		{
	  		grayscales[i].r = filter(r, 0x0000, 0xfff0) >> 8;
	  		grayscales[i].g = filter(g, 0x0000, 0xff00) >> 8;
	  		grayscales[i].b = filter(b, 0x0000, 0xff00) >> 8;

	  		r -= ((sr-er) / (max_plane+1));
	  		g -= ((sg-eg) / (max_plane+1));
	  		b -= ((sb-eb) / (max_plane+1));
		}
    }

	// Compute grayscale palette
	if(!max_plane)
		return;

	for(i = 0; i <= max_plane; i++)
	{
		grayscales[i].r = ((sr-er)/max_plane * (max_plane-i) + er) >> 8;
		grayscales[i].g = ((sg-eg)/max_plane * (max_plane-i) + eg) >> 8;
		grayscales[i].b = ((sb-eb)/max_plane * (max_plane-i) + eb) >> 8;
	}
}
