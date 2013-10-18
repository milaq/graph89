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


#include <jni.h>
#include <wrappercommon.h>
#include <androidlog.h>

JNIEXPORT void JNICALL Java_com_graph89_emulationcore_EmulatorActivity_nativeInitGraph89(JNIEnv * env, jobject obj, jint calc_type, jint screen_width, jint screen_height, jint zoom,
		jint is_grayscale, jint is_grid, jint pixel_on_color, jint pixel_off_color, jint grid_color, jdouble speed_coefficient, jstring tmp_dir)
{
	const char * tmpDir = (*env)->GetStringUTFChars(env, tmp_dir, 0);
	graph89_init_commons(calc_type, screen_width, screen_height, zoom, is_grayscale, is_grid, pixel_on_color, pixel_off_color, grid_color, speed_coefficient, tmpDir);
	LOGI("Init Graph89");
	(*env)->ReleaseStringUTFChars(env, tmp_dir, tmpDir);
}

JNIEXPORT void JNICALL Java_com_graph89_emulationcore_EmulatorActivity_nativeCleanGraph89(JNIEnv * env, jobject obj)
{
	graph89_clean_commons();
	LOGI("Clean Graph89");
}

JNIEXPORT jint JNICALL Java_com_graph89_emulationcore_EmulatorActivity_nativeReadEmulatedScreen(JNIEnv * env, jobject obj, jbyteArray jFlags)
{
	uint8_t *flags = (*env)->GetByteArrayElements(env, jFlags, 0);
	int crc = graph89_read_emulated_screen(flags);
	(*env)->ReleaseByteArrayElements(env, jFlags, flags, 0);
	return (jint)crc;
}

JNIEXPORT void JNICALL Java_com_graph89_emulationcore_EmulatorActivity_nativeGetEmulatedScreen(JNIEnv * env, jobject obj , jintArray jScreenData)
{
	uint32_t *lcd_out = (*env)->GetIntArrayElements(env, jScreenData, 0);
	jsize lcd_out_len = (*env)->GetArrayLength(env, jScreenData);
	graph89_get_emulated_screen(lcd_out, lcd_out_len);
	(*env)->ReleaseIntArrayElements(env, jScreenData, lcd_out, 0);
}

JNIEXPORT void JNICALL Java_com_graph89_emulationcore_EmulatorActivity_nativeUpdateScreenZoom(JNIEnv * env, jobject obj , jint screen_zoom)
{
	graph89_update_screen_zoom((int)screen_zoom);
}

JNIEXPORT jint JNICALL Java_com_graph89_emulationcore_EmulatorActivity_nativeInstallROM(JNIEnv * env, jobject obj, jstring source_file, jstring dest_file, jint calc_type, jint is_rom)
{
	const char* source = (*env)->GetStringUTFChars(env, source_file, 0);
	const char* destination = (*env)->GetStringUTFChars(env, dest_file, 0);

	int code = graph89_install_rom(source, destination, (int) calc_type, (int)is_rom);

	(*env)->ReleaseStringUTFChars(env, source_file, source);
	(*env)->ReleaseStringUTFChars(env, dest_file, destination);

	LOGI("Graph89 Install ROM %d", code);

	return (jint) code;
}

JNIEXPORT void JNICALL Java_com_graph89_emulationcore_EmulatorActivity_nativeSendKey(JNIEnv * env, jobject obj , jint key, jint active)
{
	graph89_send_key((int)key, (int)active);
}

JNIEXPORT void JNICALL Java_com_graph89_emulationcore_EmulatorActivity_nativeSendKeys(JNIEnv * env, jobject obj , jintArray arr)
{
	jsize len = (*env)->GetArrayLength(env, arr);
	int *keys = (*env)->GetIntArrayElements(env, arr, 0);

	graph89_send_keys(keys, (int)len);

	(*env)->ReleaseIntArrayElements(env, arr, keys, 0);
}
