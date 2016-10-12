LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := RemoteServer.cpp

LOCAL_C_INCLUDES := \
    bionic \
    system/core/include \
    app/newcdr/src/remote \
    $(LOCAL_PATH)

LOCAL_SHARED_LIBRARIES:= liblog libcutils libutils libbinder libremotebuffer

LOCAL_MODULE := RemoteServer

include $(BUILD_EXECUTABLE)
