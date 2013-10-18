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
#include <tiemuwrapper.h>
#include <androidlog.h>

extern JNIEnv * DbusJNIenv;
JNIEXPORT void JNICALL Java_com_graph89_emulationcore_EmulatorActivity_nativeTiEmuStep1LoadDefaultConfig(JNIEnv * env, jobject obj)
{
	tiemu_step1_load_defaultconfig();
	LOGI("TiEmu LoadDefaultConfig");
}

JNIEXPORT jint JNICALL Java_com_graph89_emulationcore_EmulatorActivity_nativeTiEmuStep2LoadImage(JNIEnv * env, jobject obj, jstring image_file)
{
	const char * filename = (*env)->GetStringUTFChars(env, image_file, 0);
	int code = tiemu_step2_load_image(filename);
	(*env)->ReleaseStringUTFChars(env, image_file, filename);
	LOGI("TiEmu LoadImage %d", code);
	return (jint)code;
}

JNIEXPORT jint JNICALL Java_com_graph89_emulationcore_EmulatorActivity_nativeTiEmuStep3Init(JNIEnv * env, jobject obj)
{
	int code = tiemu_step3_init();
	LOGI("TiEmu Init %d", code);
	return (jint)code;
}

JNIEXPORT jint JNICALL Java_com_graph89_emulationcore_EmulatorActivity_nativeTiEmuStep4Reset (JNIEnv * env, jobject obj)
{
	int code = tiemu_step4_reset();
	LOGI("TiEmu Reset %d", code);
	return (jint)code;
}

JNIEXPORT jint JNICALL Java_com_graph89_emulationcore_EmulatorActivity_nativeTiEmuSaveState(JNIEnv * env, jobject obj, jstring state_file)
{
	const char* filename = (*env)->GetStringUTFChars(env, state_file, 0);
	int code = tiemu_save_state(filename);
	(*env)->ReleaseStringUTFChars(env, state_file, filename);
	LOGI("SaveState %d", code);
	return (jint)code;
}

JNIEXPORT void JNICALL Java_com_graph89_emulationcore_EmulatorActivity_nativeTiEmuTurnScreenOn(JNIEnv * env, jobject obj)
{
	tiemu_turn_screen_ON();
	LOGI("TiEmu Turn Screen ON");
}

JNIEXPORT void JNICALL Java_com_graph89_emulationcore_EmulatorActivity_nativeTiEmuPatch(JNIEnv * env, jobject obj, jstring num, jstring vernum)
{
	const char * serial = (*env)->GetStringUTFChars(env, num, 0);
	const char * ver = (*env)->GetStringUTFChars(env, vernum, 0);

	tiemu_patch(serial, ver);

	(*env)->ReleaseStringUTFChars(env, num, serial);
	(*env)->ReleaseStringUTFChars(env, vernum, ver);
}

JNIEXPORT jint JNICALL Java_com_graph89_emulationcore_EmulatorActivity_nativeTiEmuUploadFile(JNIEnv * env, jobject obj, jstring str)
{
	const char * filename = (*env)->GetStringUTFChars(env, str, 0);
	int code = tiemu_upload_file(filename);
	(*env)->ReleaseStringUTFChars(env, str, filename);

	LOGI("TiEmu Upload File");
	return (jint)code;
}

JNIEXPORT jint JNICALL Java_com_graph89_emulationcore_EmulatorActivity_nativeTiEmuLoadState(JNIEnv * env, jobject obj, jstring str)
{
	const char * filename = (*env)->GetStringUTFChars(env, str, 0);

	int code = tiemu_load_state(filename);

	(*env)->ReleaseStringUTFChars(env, str, filename);
	LOGI("TiEmu LoadState %d", code);

	return (jint)code;
}

JNIEXPORT void JNICALL Java_com_graph89_emulationcore_EmulatorActivity_nativeTiEmuSyncClock(JNIEnv * env, jobject obj)
{
	tiemu_sync_clock();
}

JNIEXPORT void JNICALL Java_com_graph89_emulationcore_EmulatorActivity_nativeTiEmuRunEngine(JNIEnv * env, jobject obj)
{
	DbusJNIenv = env;
	tiemu_run_engine();
}
