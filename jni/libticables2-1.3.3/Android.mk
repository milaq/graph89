LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
GLIB_TOP := jni/glib
CABLES_TOP := jni/libticables2-1.3.3

LOCAL_SRC_FILES:= \
	src/data_log.c \
	src/detect.c  \
	src/error.c \
	src/ioports.c \
	src/link_gry.c \
	src/link_nul.c \
	src/link_par.c \
	src/link_blk.c \
	src/link_usb.c \
	src/link_tie.c \
	src/link_vti.c \
	src/link_xxx.c \
	src/log_dbus.c \
	src/log_dusb.c \
	src/log_hex.c \
	src/log_nsp.c \
	src/hex2dbus.c \
	src/hex2dusb.c \
	src/hex2nsp.c \
	src/logging.c \
	src/probe.c \
	src/ticables.c \
	src/type2str.c \
	src/none.c

LOCAL_MODULE:= ticables2-1.3.3

LOCAL_CFLAGS := \
    -I$(CABLES_TOP)/src	 \
    \
    -I$(GLIB_TOP)\
    -I$(GLIB_TOP)/glib   \
    -I$(GLIB_TOP)/android	\
    \
    -DHAVE_CONFIG_H\
    -DNO_CABLE_BLK\
	-DNO_CABLE_GRY\
	-DNO_CABLE_PAR\
	-DNO_CABLE_SLV\
	-DNO_CABLE_VTI\
	-DNO_CABLE_TIE \
	-O3
    
LOCAL_SHARED_LIBRARIES := glib-2.0
    

include $(BUILD_SHARED_LIBRARY)