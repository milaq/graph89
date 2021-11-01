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


#ifndef TILEM_WRAPPER_H
#define TILEM_WRAPPER_H

	void tilem_init();
	void tilem_clean();
	int  tilem_read_emulated_screen (uint8_t *return_flags);
	int  tilem_install_rom(const char* source, const char* destination, int calc_type, int is_rom);
	int  tilem_load_image(const char * image_path);
	int  tilem_reset();
	void tilem_run_engine();
	void tilem_turn_screen_ON();

	int tilem_load_state(const char* state_file);
	int tilem_save_state(const char* rom_file, const char* state_file);

	void tilem_send_key(int key_code, int is_pressed);
	void tilem_send_keys(int* key_codes, int length);

	int tilem_send_file(const char* filename);
	void tilem_sync_clock();

#endif

