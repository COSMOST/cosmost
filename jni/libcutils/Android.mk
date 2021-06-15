LOCAL_PATH := $(call my-dir)

# Prebuilt libcutils
include $(CLEAR_VARS)
LOCAL_MODULE := libcutils
LOCAL_SRC_FILES := $(LOCAL_PATH)/libcutils.so
include $(PREBUILT_SHARED_LIBRARY)
