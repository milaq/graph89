LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

CALCS_TOP := jni/libticalcs2-1.1.7
FILES_TOP := jni/libtifiles2-1.1.5
CONV_TOP := jni/libticonv-1.1.3
GLIB_TOP := jni/glib
CABLES_TOP := jni/libticables2-1.3.3

LOCAL_SRC_FILES:= \
	src/backup.c \
	src/calc_00.c \
	src/calc_xx.c \
	src/calc_73.c \
	src/calc_82.c \
	src/calc_83.c \
	src/calc_85.c \
	src/calc_86.c \
	src/calc_89.c \
	src/calc_92.c \
	src/calc_84p.c \
	src/calc_89t.c \
	src/calc_nsp.c \
	src/clock.c \
	src/cmd73.c \
	src/cmd82.c \
	src/cmd85.c \
	src/cmd89.c \
	src/cmd92.c \
	src/dusb_cmd.c \
	src/nsp_cmd.c \
	src/dirlist.c \
	src/error.c \
	src/keys73.c \
	src/keys83.c \
	src/keys83p.c \
	src/keys86.c \
	src/keys89.c \
	src/keys92p.c \
	src/logging.c \
	src/dbus_pkt.c \
	src/dusb_rpkt.c \
	src/dusb_vpkt.c \
	src/nsp_rpkt.c \
	src/nsp_vpkt.c \
	src/probe.c \
	src/romdump.c \
	src/ticalcs.c \
	src/tikeys.c \
	src/type2str.c \
	src/update.c 
	
LOCAL_MODULE:= ticalcs2-1.1.7

LOCAL_CFLAGS := \
    -I$(CALCS_TOP)/src	 \
    \
    -I$(GLIB_TOP)	\
    -I$(GLIB_TOP)/glib   \
    -I$(GLIB_TOP)/android	\
    \
    -I$(FILES_TOP)/src \
    -I$(CABLES_TOP)/src \
    -I$(CONV_TOP)/src \
    \
    -DHAVE_CONFIG_H \
    -O3 \
   
    
LOCAL_SHARED_LIBRARIES := glib-2.0 ticonv-1.1.3 ticables2-1.3.3 tifiles2-1.1.5

include $(BUILD_SHARED_LIBRARY)