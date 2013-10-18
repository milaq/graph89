LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    gmodule.c    

LOCAL_LDLIBS := \
    -ldl

LOCAL_STATIC_LIBRARIES :=	\
    glib-2.0

LOCAL_MODULE:= gmodule-2.0

LOCAL_CFLAGS := 		\
	-I$(GLIB_TOP)		\
	-I$(GLIB_TOP)/android	\
	-I$(GLIB_TOP)/glib	\
	-I$(LOCAL_PATH)/android

LOCAL_CFLAGS += \
    -DG_LOG_DOMAIN=\"GModule\"      \
    -DG_DISABLE_DEPRECATED 

include $(BUILD_STATIC_LIBRARY)
