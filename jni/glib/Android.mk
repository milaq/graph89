# This file is the top android makefile for all sub-modules.

LOCAL_PATH := $(call my-dir)

GLIB_TOP := $(LOCAL_PATH)

include $(CLEAR_VARS)

include $(GLIB_TOP)/glib/Android.mk
include $(GLIB_TOP)/gmodule/Android.mk
include $(GLIB_TOP)/gthread/Android.mk
include $(GLIB_TOP)/gobject/Android.mk

# Unfortunately, we can't really build executables with the NDK, can we? FIXME:
# write a java program that runs the test through some JNI magic
# include $(GLIB_TOP)/tests/Android.mk

