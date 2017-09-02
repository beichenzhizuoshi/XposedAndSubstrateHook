LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE    := hookbridge #生成的模块名
LOCAL_SRC_FILES := hookbridge.cpp #源文件名
LOCAL_LDLIBS +=  -llog -ldl

include $(BUILD_SHARED_LIBRARY)