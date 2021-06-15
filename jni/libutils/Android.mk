LOCAL_PATH := $(call my-dir)

# Prebuilt libutils
include $(CLEAR_VARS)
LOCAL_MODULE := libutils
LOCAL_SRC_FILES := $(LOCAL_PATH)/libutils.so
include $(PREBUILT_SHARED_LIBRARY)
