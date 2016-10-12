LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
    bionic \
    system/core/include \
    $(LOCAL_PATH)/src/remote \
    app/newcdr/include/display	\
    app/newcdr/include/fwk_msg

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libutils \
    libbinder	\
    libfwk_msg

LOCAL_CFLAGS := -Wno-error=non-virtual-dtor

LOCAL_SRC_FILES := \
    src/remote/IRemoteBufferClient.cpp \
    src/remote/IRemoteBuffer.cpp \
    src/remote/RemoteBuffer.cpp \
    src/remote/RemoteBufferClient.cpp

LOCAL_MODULE := libremotebuffer

LOCAL_MODULE_OWNER := cody
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := src/remote_client/cmd.cpp

LOCAL_C_INCLUDES := \
    bionic \
    frameworks/native/include/binder \
    system/core/include \
    app/newcdr/src/remote \
    app/newcdr/include/display \
    $(LOCAL_PATH)

LOCAL_SHARED_LIBRARIES:= liblog libcutils libutils libbinder libremotebuffer

LOCAL_MODULE := cmd.cgi

include $(BUILD_EXECUTABLE)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := src/remote_client/screenrecord.cpp

LOCAL_C_INCLUDES := \
    bionic \
    frameworks/native/include/binder \
    system/core/include \
    app/newcdr/src/remote \
    app/newcdr/include/display \
    $(LOCAL_PATH)/src/remote_client

LOCAL_SHARED_LIBRARIES:= liblog libcutils libutils libbinder libremotebuffer

LOCAL_MODULE := libscreenrecord

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
#strict compile option
#LOCAL_CFLAGS += -Werror  -Wall -Wno-unused-parameter -Wno-reorder
LOCAL_CFLAGS += -Wno-narrowing


ENABLE_PLAYBACK := true
ifeq ($(ENABLE_PLAYBACK), true)
LOCAL_CFLAGS += -DMEDIAPLAYER_READY
endif
ENABLE_DISPLAY := true
ifeq ($(ENABLE_DISPLAY), true)
LOCAL_CFLAGS += -DDISPLAY_READY
endif
ENABLE_EVENTMANAGER := true
ifeq ($(ENABLE_EVENTMANAGER), true)
LOCAL_CFLAGS += -DEVENTMANAGER_READY
endif
ENABLE_POWERMANAGER := true
ifeq ($(ENABLE_POWERMANAGER), true)
LOCAL_CFLAGS += -DPOWERMANAGER_READY
endif
ENABLE_DEEPSLEEP := false
ifeq ($(ENABLE_DEEPSLEEP), true)
LOCAL_CFLAGS += -DPOWERMANAGER_ENABLE_DEEPSLEEP
endif
ifeq ($(BOARD_HAS_WATERMARK), true)
LOCAL_CFLAGS += -DWATERMARK_ENABLE
endif
ifeq ($(DISABLE_MODEM), false)
LOCAL_CFLAGS += -DMODEM_ENABLE
endif

ifeq ($(strip $(DEVICE_USE_NEWUI)), true)
LOCAL_CFLAGS += -DUSE_NEWUI
endif

ifeq ($(strip $(FEAT_MESSAGE_FRAMEWORK)), true)
LOCAL_CFLAGS += -DMESSAGE_FRAMEWORK
endif

ifeq ($(strip $(PRODUCT_HAVE_3G_REMOTE)), true)
LOCAL_CFLAGS += -DREMOTE_3G
endif

# Feature control macros
#LOCAL_CFLAGS += -DIMPLEMENTED_UnicodeConverter
LOCAL_CFLAGS += -DIMPLEMENTED_setMuteMode
#LOCAL_CFLAGS += -DIMPLEMENTED_setImpactFileDuration
LOCAL_CFLAGS += -DIMPLEMENTED_setVideoEncodingBitRate
#LOCAL_CFLAGS += -DIMPLEMENTED_V4L2_CTRL
LOCAL_CFLAGS += -DIMPLEMENTED_recordListener
LOCAL_CFLAGS += -DHAVE_takePicture
#LOCAL_CFLAGS += -DADAS_ENABLE
#LOCAL_CFLAGS += -DTVDECODER_ENABLE
LOCAL_CFLAGS += -DMOTION_DETECTION_ENABLE
LOCAL_CFLAGS += -DIMPLEMENT_IMPACT

LOCAL_STATIC_LIBRARIES := \
	libjsoncpp

LOCAL_SHARED_LIBRARIES := \
	libminigui_ths \
	libcutils \
	libutils \
	libbinder \
	libdatabase \
	libstorage \
	libhardware \
	libhardware_legacy \
	libmedia

LOCAL_SHARED_LIBRARIES += libstlport libsysutils liblog \
                          libcrypto libdl \
                          liblogwrap

LOCAL_SHARED_LIBRARIES += \
	libmedia_proxy

LOCAL_SHARED_LIBRARIES += \
	libion

LOCAL_SRC_FILES := \
	src/misc/cdr_misc.cpp \
	src/misc/posixTimer.cpp \
	src/misc/Message2Str.cpp \
	src/misc/resourceManager.cpp \
	src/misc/Dialog.cpp \
	src/window/RegisterCDRWindow.cpp \
	src/window/MainWindow.cpp \
	src/window/RecordPreview.cpp \
	src/widget/RegisterWidgets.cpp \
	src/widget/MenuList.cpp \
	src/widget/dateSetting.cpp \
	src/widget/PopUpMenu.cpp \
	src/widget/TipLabel.cpp \
	src/widget/PicObj.cpp \
	src/camera/CdrCamera.cpp \
	src/camera/CdrMediaRecorder.cpp \
	src/storage/StorageManager.cpp \
	src/storage/Process.cpp \
	src/misc/Rtc.cpp \
	src/gps/GpsController.cpp

#newui
ifeq ($(strip $(DEVICE_USE_NEWUI)), true)
LOCAL_SRC_FILES += \
		src/window/NewStatusBar.cpp \
        	src/widget/NewSubMenu.cpp \
	        src/window/NewMenu.cpp \
        	src/widget/NewShowMessageBox.cpp \
		src/widget/NewCdrProgressBar.cpp
else
LOCAL_SRC_FILES += \
                src/window/StatusBar.cpp \
        	src/widget/SubMenu.cpp \
                src/window/Menu.cpp \
        	src/widget/showMessageBox.cpp \
		src/widget/CdrProgressBar.cpp
endif

ifeq ($(DISABLE_MODEM), false)
LOCAL_SRC_FILES += \
	src/cellular/CellularController.cpp \
	src/cellular/util.cpp \
	src/cellular/simIo.cpp
endif

#network
LOCAL_SRC_FILES += \
	src/network/hotspot/hotspot.cpp \
        src/network/hotspot/hotspotIf.cpp \
        src/network/hotspot/hotspotCli.cpp \
        src/network/hotspot/hotspotSetting.cpp \
        src/network/net/net.cpp \
        src/network/net/ConnectivityServer.cpp \
	src/network/sta/wifiHal.cpp \
	src/network/sta/wpaCtl.cpp \
	src/network/sta/wifiMonitor.cpp \
	src/network/sta/wifiIf.cpp

LOCAL_SHARED_LIBRARIES += \
	libwpa_client \
	libnetutils

ifeq ($(ENABLE_POWERMANAGER), true)
LOCAL_SRC_FILES += \
	src/power/PowerManager.cpp

LOCAL_SHARED_LIBRARIES += \
	libcpuinfo \
	libbattery
endif

ifeq ($(strip $(FEAT_MESSAGE_FRAMEWORK)), true)
	LOCAL_SHARED_LIBRARIES += \
	libfwk_msg
endif

ifeq ($(strip $(PRODUCT_HAVE_3G_REMOTE)), true)
	LOCAL_SHARED_LIBRARIES += \
	libcurl	\
	libqiniu
endif

ifeq ($(ENABLE_EVENTMANAGER), true)
LOCAL_SRC_FILES += \
	src/event/EventManager.cpp \
	src/event/ConfigData.cpp
endif

#ifeq ($(ENABLE_DISPLAY), true)
LOCAL_SRC_FILES += \
	src/display/CdrDisplay.cpp \
	src/display/layerBmpShow.cpp
#endif

ifeq ($(ENABLE_PLAYBACK), true)
LOCAL_SRC_FILES += \
	src/window/PlayBackPreview.cpp \
	src/window/PlayBack.cpp \
	src/window/AudioPlayBack.cpp
endif

ifeq ($(strip $(FEAT_MESSAGE_FRAMEWORK)), true)
LOCAL_SRC_FILES += \
	src/fwk_msg/view/fwk_view.cpp \
	src/fwk_msg/controller/fwk_controller.cpp
endif

LOCAL_SRC_FILES += \
	src/misc/cdrLang.cpp

ifeq ($(strip $(PRODUCT_HAVE_3G_REMOTE)), true)
	LOCAL_SRC_FILES += \
		src/remote_client/network_connection.cpp	\
		src/fwk_msg/controller/HerbMediaRecorderForRemote.cpp \
		src/fwk_msg/controller/remote_network_upload.cpp
endif

LOCAL_C_INCLUDES :=
LOCAL_C_INCLUDES += \
	frameworks/native/include/binder \

#LOCAL_C_INCLUDES := \
	bionic \
	frameworks/native/include \
	external/stlport/stlport	\
	external/jsoncpp/include
LOCAL_C_INCLUDES += \
	external/openssl/include

# local include paths
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/include/window \
	$(LOCAL_PATH)/include/camera \
	$(LOCAL_PATH)/include/misc \
	$(LOCAL_PATH)/include/storage \
	$(LOCAL_PATH)/src/widget/include	\
	$(LOCAL_PATH)/include
ifeq ($(strip $(PRODUCT_HAVE_3G_REMOTE)), true)
	LOCAL_C_INCLUDES += \
		$(LOCAL_PATH)/include/curl	\
		$(LOCAL_PATH)/include/csdkqiniu
endif

#ifeq ($(ENABLE_DISPLAY), true)
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/include/display \
	frameworks/av/display \
	frameworks/include/include_media/display \
	system/core/libion/include \
	system/core/libion/kernel-headers \
	hardware/rockchip/hwcomposer
#endif
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/include/event
ifeq ($(ENABLE_POWERMANAGER), true)
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/include/power
endif

LOCAL_C_INCLUDES += \
	frameworks/include/include_media/media

#LOCAL_C_INCLUDES += \
	frameworks/av/media_proxy \
	frameworks/av/media_proxy/core/hardware \
	frameworks/av/media_proxy/media/media \
	frameworks/include/include_media/media

LOCAL_C_INCLUDES += \
	frameworks/include \
	frameworks/include/include_database \
	frameworks/include/include_battery \
	frameworks/include/include_storage

LOCAL_C_INCLUDES += \
	system/core/include/cutils \
	app/newcdr/src/remote

LOCAL_SHARED_LIBRARIES += \
    libremotebuffer \
	

ifeq ($(strip $(FEAT_MESSAGE_FRAMEWORK)), true)
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/include/fwk_msg
endif

LOCAL_MODULE_TAGS := eng
LOCAL_MODULE := newcdr

include external/stlport/libstlport.mk
include $(BUILD_EXECUTABLE)


ifeq ($(strip $(FEAT_MESSAGE_FRAMEWORK)), true)
#
# libfwk_msg.so for target.
#
include $(CLEAR_VARS)
LOCAL_SRC_FILES := src/fwk_msg/libfwk/libfwk_msg.so
LOCAL_MODULE:= libfwk_msg
LOCAL_MODULE_OWNER := jdy
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_SUFFIX := $(TARGET_SHLIB_SUFFIX)
LOCAL_MULTILIB := both
include $(BUILD_PREBUILT)
endif

ifeq ($(strip $(PRODUCT_HAVE_3G_REMOTE)), true)
#
# libqiniu.so for target.
#
include $(CLEAR_VARS)
LOCAL_SRC_FILES := src/remote_client/third_party/libqiniu.so
LOCAL_MODULE:= libqiniu
LOCAL_MODULE_OWNER := jdy
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_SUFFIX := $(TARGET_SHLIB_SUFFIX)
LOCAL_MULTILIB := both
include $(BUILD_PREBUILT)
endif
