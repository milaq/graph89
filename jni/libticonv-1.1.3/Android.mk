LOCAL_PATH:= $(call my-dir)
CONV_TOP := jni/libticonv-1.1.3
GLIB_TOP := jni/glib

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	src/ticonv.c  \
	src/charset.c \
	src/filename.c \
	src/tokens.c  

LOCAL_MODULE:= ticonv-1.1.3

LOCAL_CFLAGS := \
    -I$(CONV_TOP)/src	 \
    \
    -I$(GLIB_TOP)	\
    -I$(GLIB_TOP)/glib   \
    -I$(GLIB_TOP)/android	\
    \
    -DHAVE_CONFIG_H \
    -DTICONV_EXPORTS \
    -O3
        
LOCAL_SHARED_LIBRARIES := glib-2.0    

include $(BUILD_SHARED_LIBRARY)