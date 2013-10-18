LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

FILES_TOP := jni/libtifiles2-1.1.5
CONV_TOP := jni/libticonv-1.1.3
GLIB_TOP := jni/glib

LOCAL_SRC_FILES:= \
	src/comments.c \
	src/error.c \
	src/files8x.c \
	src/files9x.c \
	src/filesnsp.c \
	src/filesxx.c \
	src/filetypes.c \
	src/grouped.c \
	src/intelhex.c \
	src/logging.c \
	src/misc.c \
	src/rwfile.c \
	src/tifiles.c \
	src/tigroup.c \
	src/type2str.c \
	src/types73.c \
	src/types82.c \
	src/types83.c \
	src/types83p.c \
	src/types84p.c \
	src/types85.c \
	src/types86.c \
	src/types89.c \
	src/types89t.c \
	src/types92.c \
	src/types92p.c \
	src/typesv2.c \
	src/typesnsp.c \
	src/typesxx.c \
	src/ve_fp.c \
	src/minizip/ioapi.c \
	src/minizip/miniunz.c \
	src/minizip/minizip.c \
	src/minizip/unzip.c \
	src/minizip/zip.c
	
LOCAL_MODULE:= tifiles2-1.1.5

LOCAL_CFLAGS := \
    -I$(FILES_TOP)/src	 \
    \
    -I$(GLIB_TOP)	\
    -I$(GLIB_TOP)/glib   \
    -I$(GLIB_TOP)/android	\
    \
    -I$(CONV_TOP)/src   \
    \
    -DHAVE_CONFIG_H\
    -DSTDC\
    -O3 
       
LOCAL_LDLIBS := -lz 
    
LOCAL_SHARED_LIBRARIES := glib-2.0 ticonv-1.1.3
    
#include $(BUILD_STATIC_LIBRARY)
include $(BUILD_SHARED_LIBRARY)