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
#include <stdio.h>
#include <tilem.h>
#include <androidlog.h>


JNIEXPORT jint JNICALL Java_com_graph89_emulationcore_EmulatorActivity_nativeTilemLoadImage(JNIEnv * env, jobject obj, jstring image_file)
{
	const char * filename = (*env)->GetStringUTFChars(env, image_file, 0);
	int code = tilem_load_image(filename);
	(*env)->ReleaseStringUTFChars(env, image_file, filename);
	LOGI("Tilem LoadImage %d", code);
	return (jint)code;
}


JNIEXPORT jint JNICALL Java_com_graph89_emulationcore_EmulatorActivity_nativeTilemReset(JNIEnv * env, jobject obj)
{
	int code = tilem_reset();
	LOGI("Tilem Reset");
	return code;
}

JNIEXPORT void JNICALL Java_com_graph89_emulationcore_EmulatorActivity_nativeTilemTurnScreenOn(JNIEnv * env, jobject obj)
{
	tilem_turn_screen_ON();
	LOGI("Tilem Turn Screen ON");
}

JNIEXPORT void JNICALL Java_com_graph89_emulationcore_EmulatorActivity_nativeTilemRunEngine(JNIEnv * env, jobject obj)
{
	tilem_run_engine();
}

JNIEXPORT jint JNICALL Java_com_graph89_emulationcore_EmulatorActivity_nativeTilemLoadState(JNIEnv * env, jobject obj, jstring str)
{
	const char * filename = (*env)->GetStringUTFChars(env, str, 0);

	int code = tilem_load_state(filename);

	(*env)->ReleaseStringUTFChars(env, str, filename);
	LOGI("Tilem LoadState %d", code);

	return (jint)code;
}


JNIEXPORT jint JNICALL Java_com_graph89_emulationcore_EmulatorActivity_nativeTilemSaveState(JNIEnv * env, jobject obj, jstring romfilename, jstring statefilename)
{
	const char* romfile = (*env)->GetStringUTFChars(env, romfilename, 0);
	const char* statefile = (*env)->GetStringUTFChars(env, statefilename, 0);

	int code = tilem_save_state(romfile, statefile);

	(*env)->ReleaseStringUTFChars(env, romfilename, romfile);
	(*env)->ReleaseStringUTFChars(env, statefilename, statefile);

	LOGI("Tilem SaveState %d", code);
	jint ret = (jint)code;
	return ret;
}

JNIEXPORT jint JNICALL Java_com_graph89_emulationcore_EmulatorActivity_nativeTilemUploadFile(JNIEnv * env, jobject obj, jstring str)
{
	const char * filename = (*env)->GetStringUTFChars(env, str, 0);
	int code = tilem_send_file(filename);
	(*env)->ReleaseStringUTFChars(env, str, filename);

	LOGI("Tilem Upload File");
	return (jint)code;
}


JNIEXPORT void JNICALL Java_com_graph89_emulationcore_EmulatorActivity_nativeTilemSyncClock(JNIEnv * env, jobject obj)
{
	tilem_sync_clock();
	LOGI("Tilem Sync Clock");
}
