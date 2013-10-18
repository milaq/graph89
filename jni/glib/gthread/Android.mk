LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    gthread-impl.c         

LOCAL_STATIC_LIBRARIES := glib-2.0

LOCAL_MODULE:= gthread-2.0

LOCAL_CFLAGS := 		\
	-I$(LOCAL_PATH)		\
	-I$(GLIB_TOP)		\
	-I$(GLIB_TOP)/android	\
	-I$(GLIB_TOP)/glib

LOCAL_CFLAGS += \
    -DG_LOG_DOMAIN=\"GThread\"      \
    -D_POSIX4_DRAFT_SOURCE          \
    -D_POSIX4A_DRAFT10_SOURCE       \
    -U_OSF_SOURCE                   \
    -DG_DISABLE_DEPRECATED 

include $(BUILD_STATIC_LIBRARY)
