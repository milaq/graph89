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

#ifndef TIEMU_WRAPPER_H
#define TIEMU_WRAPPER_H

	void tiemu_init();
	void tiemu_clean();
	void tiemu_step1_load_defaultconfig();
	int  tiemu_step2_load_image(const char * image_path);
	int  tiemu_step3_init();
	int  tiemu_step4_reset();
	int  tiemu_load_state(const char* state_file);
	int  tiemu_save_state(const char* state_file);
	int  tiemu_upload_file(const char* file_name);
	void tiemu_send_key(int key_code, int is_pressed);
	void tiemu_send_keys(int* key_codes, int length);
	void tiemu_turn_screen_ON();
	void tiemu_sync_clock();
	void tiemu_patch(const char* num, const char* vernum);

	void tiemu_run_engine();
	int  tiemu_read_emulated_screen (uint8_t *return_flags);
	void tiemu_set_tmp_dir(const char* tmp_dir);
	int  tiemu_install_rom(const char* source, const char* destination, int calc_type, int is_rom);

#endif
