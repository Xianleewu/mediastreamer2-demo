LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := RemoteClient.cpp

LOCAL_C_INCLUDES := \
    bionic \
    frameworks/native/include/binder \
    system/core/include \
    app/newcdr/src/remote \
    app/newcdr/include/display \
    $(LOCAL_PATH)

LOCAL_SHARED_LIBRARIES:= liblog libcutils libutils libbinder libremotebuffer

LOCAL_MODULE := RemoteClient

include $(BUILD_EXECUTABLE)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := screenrecord.cpp

LOCAL_C_INCLUDES := \
    bionic \
    frameworks/native/include/binder \
    system/core/include \
    app/newcdr/src/remote \
    app/newcdr/include/display \
    $(LOCAL_PATH)

LOCAL_SHARED_LIBRARIES:= liblog libcutils libutils libbinder libremotebuffer

LOCAL_MODULE := libscreenrecord

include $(BUILD_SHARED_LIBRARY)
