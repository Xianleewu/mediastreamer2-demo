LOCAL_PATH := $(call my-dir)


include $(CLEAR_VARS)

LOCAL_MODULE := remotebuffer

LOCAL_SRC_FILES := libremotebuffer.so

include $(PREBUILT_SHARED_LIBRARY)




include $(CLEAR_VARS)


LOCAL_SRC_FILES := RemoteClientN.cpp

LOCAL_LDLIBS    += -llog

LOCAL_SHARED_LIBRARIES :=  remotebuffer

LOCAL_MODULE := RemoteClientN

include $(BUILD_EXECUTABLE)
