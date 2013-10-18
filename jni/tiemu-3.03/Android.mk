LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

TIEMU_TOP := jni/tiemu-3.03
CALCS_TOP := jni/libticalcs2-1.1.7
FILES_TOP := jni/libtifiles2-1.1.5
CONV_TOP := jni/libticonv-1.1.3
GLIB_TOP := jni/glib
CABLES_TOP := jni/libticables2-1.3.3

LOCAL_SRC_FILES:= \
src/core/uae/readcpu.c \
src/core/uae/cpudefs.c \
src/core/uae/missing.c \
src/core/uae/xmalloc.c \
src/core/uae/cpustbl.c \
src/core/uae/fpp.c \
src/core/uae/cpuemu.c \
src/core/uae/newcpu.c \
src/core/error.c \
src/core/engine.c \
src/core/images.c \
src/core/interface.c \
src/core/state.c \
src/core/type2str.c \
src/core/hwpm.c \
src/core/ti_hw/dbus.c \
src/core/ti_hw/flash.c \
src/core/ti_hw/gscales.c \
src/core/ti_hw/hw.c \
src/core/ti_hw/hwprot.c \
src/core/ti_hw/kbd.c \
src/core/ti_hw/m68k.c \
src/core/ti_hw/mem.c \
src/core/ti_hw/mem89.c \
src/core/ti_hw/mem89tm.c \
src/core/ti_hw/mem92.c \
src/core/ti_hw/mem92p.c \
src/core/ti_hw/memv2.c \
src/core/ti_hw/ports.c \
src/core/ti_hw/rtc_hw3.c \
src/core/ti_hw/tichars.c \
src/core/ti_sw/er_codes.c \
src/core/ti_sw/handles.c \
src/core/ti_sw/iodefs.c \
src/core/ti_sw/mem_map.c \
src/core/ti_sw/registers.c \
src/core/ti_sw/romcalls.c \
src/core/ti_sw/timem.c \
src/core/ti_sw/vat.c \
src/core/dbg/bkpts.c \
src/core/dbg/debug.c \
src/core/dbg/disasm.c \
src/core/dbg/fpudasm.c \
src/core/dbg/gdbcall.c \
src/gui/tsource.c \
src/misc/tie_error.c 

LOCAL_MODULE:= tiemu-3.03

LOCAL_CFLAGS := \
    -I$(TIEMU_TOP)/src	 \
    -I$(TIEMU_TOP)/src/core/ti_hw	 \
    -I$(TIEMU_TOP)/src/core/ti_sw	 \
    -I$(TIEMU_TOP)/src/core/uae	 \
    -I$(TIEMU_TOP)/src/core	 \
    -I$(TIEMU_TOP)/src/core/dbg	 \
    -I$(TIEMU_TOP)/src/misc	 \
    -I$(TIEMU_TOP)/src/gui	 \
    \
    -I$(GLIB_TOP)	\
    -I$(GLIB_TOP)/glib   \
    -I$(GLIB_TOP)/android	\
    \
    -I$(FILES_TOP)/src   \
    -I$(CONV_TOP)/src    \
    -I$(CALCS_TOP)/src   \
    -I$(CABLES_TOP)/src  \
    \
    -DHAVE_CONFIG_H \
    -DDEBUGGER \
    -DNO_GDB \
    -DNO_SOUND \
    -O3 

       
LOCAL_SHARED_LIBRARIES := glib-2.0 ticonv-1.1.3 ticables2-1.3.3 tifiles2-1.1.5 ticalcs2-1.1.7 

LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog

include $(BUILD_SHARED_LIBRARY)