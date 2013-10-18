LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

WRAPPER_TOP := jni/wrapper
TIEMU_TOP := jni/tiemu-3.03
CALCS_TOP := jni/libticalcs2-1.1.7
FILES_TOP := jni/libtifiles2-1.1.5
CONV_TOP := jni/libticonv-1.1.3
GLIB_TOP := jni/glib
CABLES_TOP := jni/libticables2-1.3.3
TILEM_TOP := jni/tilem-2.0

LOCAL_SRC_FILES:= wrappercommon.c \
tilemwrapper.c \
tiemuwrapper.c \
wrappercommonjni.c \
tiemuwrapperjni.c \
tilemwrapperjni.c \
wabbitvar.c\
wabbitlink.c\
bootimage.c

LOCAL_CFLAGS := \
    -I$(WRAPPER_TOP)	 \
    \
    -I$(GLIB_TOP)	\
    -I$(GLIB_TOP)/glib   \
    -I$(GLIB_TOP)/android	\
    \
    -I$(TIEMU_TOP)/src \
    -I$(TIEMU_TOP)/src/core \
    -I$(TIEMU_TOP)/src/core/ti_hw \
    -I$(TIEMU_TOP)/src/core/ti_sw	 \
    -I$(TIEMU_TOP)/src/core/dbg	 \
    -I$(TIEMU_TOP)/src \
    -I$(CABLES_TOP)/src \
    -I$(FILES_TOP)/src \
    -I$(CALCS_TOP)/src \
    -I$(CONV_TOP)/src\
    -I$(TILEM_TOP)/emu\
    -I$(TILEM_TOP)/gui\
    -O3
   
LOCAL_MODULE:=wrapper

LOCAL_SHARED_LIBRARIES := glib-2.0 ticonv-1.1.3 ticables2-1.3.3 tifiles2-1.1.5 ticalcs2-1.1.7 tiemu-3.03 tilem-2.0

LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog

#include $(BUILD_STATIC_LIBRARY)
include $(BUILD_SHARED_LIBRARY)