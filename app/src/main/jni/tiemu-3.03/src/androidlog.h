#ifndef ANDROID_LOG
#define ANDROID_LOG
	#include <android/log.h>

	#define DEBUG_NAME "Graph89"

	#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG , DEBUG_NAME, __VA_ARGS__)
	#define LOGI(...) __android_log_print(ANDROID_LOG_INFO , DEBUG_NAME, __VA_ARGS__)
	#define LOGW(...) __android_log_print(ANDROID_LOG_WARN , DEBUG_NAME, __VA_ARGS__)
	#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR , DEBUG_NAME, __VA_ARGS__)
	#define LOGF(...) __android_log_print(ANDROID_LOG_FATAL , DEBUG_NAME, __VA_ARGS__)

#endif
