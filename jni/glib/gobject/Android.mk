LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    gboxed.c                \
    genums.c                \
    gparam.c                \
    gsignal.c               \
    gtypemodule.c           \
    gtypeplugin.c           \
    gvalue.c                \
    gvaluetypes.c           \
    gclosure.c              \
    gobject.c               \
    gparamspecs.c           \
    gtype.c                 \
    gvaluearray.c           \
    gvaluetransform.c       \
    gsourceclosure.c

LOCAL_STATIC_LIBRARIES := glib-2.0

LOCAL_MODULE:= gobject-2.0

LOCAL_CFLAGS := 		\
	-I$(LOCAL_PATH)		\
	-I$(GLIB_TOP)		\
	-I$(GLIB_TOP)/android	\
	-I$(GLIB_TOP)/glib

LOCAL_CFLAGS += \
    -DG_LOG_DOMAIN=\"GLib-GObject\" \
    -DGOBJECT_COMPILATION           \
    -DG_DISABLE_CONST_RETURNS       \
    -DG_DISABLE_DEPRECATED 

include $(BUILD_STATIC_LIBRARY)
