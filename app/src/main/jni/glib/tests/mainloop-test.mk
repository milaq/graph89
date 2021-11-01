LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    mainloop-test.c

LOCAL_SHARED_LIBRARIES := \
	libglib-2.0  \
	libgthread-2.0

LOCAL_C_INCLUDES := 		\
	$(GLIB_TOP)		\
	$(GLIB_TOP)/android	\
	$(GLIB_TOP)/glib

LOCAL_MODULE:= mainloop-test

include $(BUILD_EXECUTABLE)
