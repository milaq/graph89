LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

GLIB_TOP := jni/glib
CABLES_TOP := jni/libticables2-1.3.3
TILEM_TOP := jni/tilem-2.0
CALCS_TOP := jni/libticalcs2-1.1.7
FILES_TOP := jni/libtifiles2-1.1.5
CONV_TOP := jni/libticonv-1.1.3

LOCAL_SRC_FILES:= \
	emu/calcs.c \
	emu/z80.c \
	emu/state.c \
	emu/rom.c \
	emu/flash.c \
	emu/link.c \
	emu/keypad.c \
	emu/lcd.c  \
	emu/cert.c \
	emu/md5.c \
	emu/timers.c \
	emu/monolcd.c \
	emu/graylcd.c \
	emu/grayimage.c \
	emu/graycolor.c \
	\
	emu/x7/x7_init.c    \
	emu/x7/x7_io.c    \
	emu/x7/x7_memory.c    \
	emu/x7/x7_subcore.c    \
	emu/x1/x1_init.c    \
	emu/x1/x1_io.c    \
	emu/x1/x1_memory.c    \
	emu/x1/x1_subcore.c    \
	emu/x2/x2_init.c    \
	emu/x2/x2_io.c    \
	emu/x2/x2_memory.c    \
	emu/x2/x2_subcore.c    \
	emu/x3/x3_init.c    \
	emu/x3/x3_io.c    \
	emu/x3/x3_memory.c    \
	emu/x3/x3_subcore.c    \
	emu/xp/xp_init.c    \
	emu/xp/xp_io.c    \
	emu/xp/xp_memory.c    \
	emu/xp/xp_subcore.c    \
	emu/xs/xs_init.c    \
	emu/xs/xs_io.c    \
	emu/xs/xs_memory.c    \
	emu/xs/xs_subcore.c    \
	emu/x4/x4_init.c    \
	emu/x4/x4_io.c    \
	emu/x4/x4_memory.c    \
	emu/x4/x4_subcore.c    \
	emu/xz/xz_init.c    \
	emu/xz/xz_io.c    \
	emu/xz/xz_memory.c    \
	emu/xz/xz_subcore.c    \
	emu/xn/xn_init.c    \
	emu/xn/xn_io.c    \
	emu/xn/xn_memory.c    \
	emu/xn/xn_subcore.c    \
	emu/x5/x5_init.c    \
	emu/x5/x5_io.c    \
	emu/x5/x5_memory.c    \
	emu/x5/x5_subcore.c    \
	emu/x6/x6_init.c    \
	emu/x6/x6_io.c    \
	emu/x6/x6_memory.c    \
	emu/x6/x6_subcore.c \
	gui/memory.c \
	gui/link.c \
	gui/emucore.c \
	gui/tool.c \
	gui/ti81prg.c\
    gui/emulator.c

LOCAL_CFLAGS := \
    \
    -I$(GLIB_TOP)	\
    -I$(GLIB_TOP)/glib   \
    -I$(GLIB_TOP)/android	\
    \
    -I$(TILEM_TOP) \
    -I$(TILEM_TOP)/emu\
    -I$(TILEM_TOP)/gui\
    -I$(CABLES_TOP)/src \
    -I$(FILES_TOP)/src \
    -I$(CALCS_TOP)/src \
    -I$(CONV_TOP)/src\
    \
    -DHAVE_CONFIG_H \
    -DDEBUGGER \
    -DNO_GDB \
    -DNO_SOUND \
    -O3\

   
LOCAL_MODULE:=tilem-2.0
    
LOCAL_SHARED_LIBRARIES :=  glib-2.0 ticonv-1.1.3 ticables2-1.3.3 tifiles2-1.1.5 ticalcs2-1.1.7

LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog

include $(BUILD_SHARED_LIBRARY)