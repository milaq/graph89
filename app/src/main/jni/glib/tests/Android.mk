LOCAL_PATH := $(call my-dir)

TESTS_TOP := $(LOCAL_PATH)

include $(CLEAR_VARS)

include $(TESTS_TOP)/mainloop-test.mk
include $(TESTS_TOP)/array-test.mk
