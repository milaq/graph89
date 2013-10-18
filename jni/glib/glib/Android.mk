LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    ./libcharset/localcharset.c \
    garray.c        \
    gasyncqueue.c   \
    gatomic.c       \
    gbacktrace.c    \
    gbase64.c       \
    gbookmarkfile.c \
    gcache.c        \
    gcompletion.c   \
    gconvert.c      \
    gdataset.c      \
    gdate.c         \
    gdir.c          \
    gerror.c        \
    gfileutils.c    \
    ghash.c         \
    ghook.c         \
    giochannel.c    \
    gkeyfile.c      \
    glist.c         \
    gmain.c         \
    gmappedfile.c   \
    gmarkup.c       \
    gmem.c          \
    gmessages.c     \
    gnode.c         \
    goption.c       \
    gpattern.c      \
    gprimes.c       \
    gqsort.c        \
    gqueue.c        \
    grel.c          \
    grand.c         \
    gscanner.c      \
    gsequence.c     \
    gshell.c        \
    gslice.c        \
    gslist.c        \
    gstdio.c        \
    gstrfuncs.c     \
    gstring.c       \
    gthread.c       \
    gthreadpool.c   \
    gtimer.c        \
    gtree.c         \
    guniprop.c      \
    gutf8.c         \
    gunibreak.c     \
    gunicollate.c   \
    gunidecomp.c    \
    gutils.c        \
    gprintf.c       \
    giounix.c       \
    gspawn.c

LOCAL_MODULE:= glib-2.0

# ./glib private macros, copy from Makefile.am
LOCAL_CFLAGS := \
    -Ijni/glib			\
    -Ijni/glib/android		\
    -I$(LOCAL_PATH)/libcharset		\
    -I$(LOCAL_PATH)/gnulibi		\
    -I$(LOCAL_PATH)/pcre		\
    -DLIBDIR=\"$(libdir)\"          \
    -DHAVE_CONFIG_H                 \
    \
    -DG_LOG_DOMAIN=\"GLib-GRegex\" \
    -DSUPPORT_UCP \
    -DSUPPORT_UTF8 \
    -DNEWLINE=-1 \
    -DMATCH_LIMIT=10000000 \
    -DMATCH_LIMIT_RECURSION=10000000 \
    -DMAX_NAME_SIZE=32 \
    -DMAX_NAME_COUNT=10000 \
    -DMAX_DUPLENGTH=30000 \
    -DLINK_SIZE=2 \
    -DEBCDIC=0 \
    -DPOSIX_MALLOC_THRESHOLD=10 \
    -DG_DISABLE_DEPRECATED \
    -DGLIB_COMPILATION 
    
#include $(BUILD_STATIC_LIBRARY)
include $(BUILD_SHARED_LIBRARY)
