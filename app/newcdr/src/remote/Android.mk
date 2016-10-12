LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
	bionic \
	system/core/include \
	$(LOCAL_PATH) \
    app/newcdr/include/display	\
    app/newcdr/include/fwk_msg

LOCAL_SHARED_LIBRARIES := \
	liblog \
	libutils \
	libbinder	\
	libfwk_msg

LOCAL_CFLAGS := -Wno-error=non-virtual-dtor

LOCAL_SRC_FILES := \
	IRemoteBufferClient.cpp \
	IRemoteBuffer.cpp \
	RemoteBuffer.cpp \
	RemoteBufferClient.cpp

LOCAL_MODULE := libremotebuffer

LOCAL_MODULE_OWNER := cody
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

