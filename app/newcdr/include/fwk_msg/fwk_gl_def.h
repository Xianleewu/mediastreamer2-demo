/*   ----------------------------------------------------------------------
Copyright (C) 2014-2016 Fuzhou Rockchip Electronics Co., Ltd

     Sec Class: Rockchip Confidential

V1.0 Dayao Ji <jdy@rock-chips.com>
---------------------------------------------------------------------- */


#ifndef _FWK_GL_DEF_837689389163_
#define _FWK_GL_DEF_837689389163_

#include <stdio.h>
#include <sys/msg.h>


enum
{
    FWK_MOD_FWK = 1,
    FWK_MOD_CONTROL,
    FWK_MOD_VIEW,
    FWK_MOD_CAMERA,/*Camera*/
    FWK_MOD_PARAMETER, /*Parameter*/
    FWK_MOD_STORAGE,/*Storage*/
    FWK_MOD_GPS, /*GPS*/
    FWK_MOD_WIFI, /*WiFi*/
    FWK_MOD_CELLULAR,/*Cellular*/
    FWK_MOD_WRTC, /*Web RTC*/
    FWK_MOD_Reserverd1,
    FWK_MOD_Reserverd2,
    FWK_MOD_Reserverd3,
    FWK_MOD_Reserverd4,
    FWK_MOD_Reserverd5,
    FWK_MOD_Reserverd6,
    FWK_MOD_NUM
};




typedef struct
{
    int size;
    int source;
    int msg_id;
    int dest;
    char  para[1];
} FWK_MSG;




#define MSG_GET_NO_WAIT			IPC_NOWAIT
#define MSG_GET_WAIT_ROREVER		0


typedef enum
{
    COMMU_TYPE_GPS = 1,
    COMMU_TYPE_WIFI,
    COMMU_TYPE_CELLULAR,
    COMMU_TYPE_WATERMARK,
    COMMU_TYPE_MOVING,
    COMMU_TYPE_FORMAT,
    COMMU_TYPE_FACTORY_RESET,
    COMMU_TYPE_RECORD,
    COMMU_TYPE_VOICE
}CommuCmdType;

typedef  struct 
{
   int cmdType; /*1:GPS      2:WIFI    3:Cellular*/
   int operType; /*0: OFF     1: ON*/
} CommuReq;

typedef  struct 
{
   int cmdType; /*1:GPS      2:WIFI    3:Cellular*/
   int operType; /*0: OFF     1: ON*/
   int result; /*0:  Success      others: failed*/
} CommuRsp;

typedef enum
{
    VALUE_TYPE_RESOLUTION = 1,
    VALUE_TYPE_RECORD_TIME,
    VALUE_TYPE_EXPOSURE,
    VALUE_TYPE_WB,
    VALUE_TYPE_CONTRAST
}ValueCmdType;

typedef enum
{
    WB_VALUE_AUTO = 0,
    WB_VALUE_DAYLIGHT,
    WB_VALUE_CLOUDY,
    WB_VALUE_INCANDESCENT,
    WB_VALUE_FLUORESCENT
}ValueWB;



typedef struct
{
	int cmdType;
	int value;
}ValueReq;

typedef struct
{
	int cmdType;
	int value;
	int result;
}ValueRsp;

typedef struct{
	unsigned int year;
	unsigned int month;
	unsigned int day;
	unsigned int hour;
	unsigned int minute;
	unsigned int second;
}WRTC_DATE_TIME;

typedef struct{
	WRTC_DATE_TIME dt;
	int result;
}WRTC_DT_RSP;


#define WRTC_MAX_NAME_LEN			32
#define WRTC_MAX_VALUE_LEN			32
#define WRTC_MAX_SETTING_ITEMS		22
#define WRTC_MAX_FILE_NAME_LEN		512
#define WRTC_MAX_URL_LEN			512

typedef struct{
	char itemName[WRTC_MAX_NAME_LEN+1];
	int valueType;/*0:int   1:char*  2 bool  3 date*/
	int value;
	bool bValue;
	char valueStr[WRTC_MAX_VALUE_LEN+1];
	WRTC_DATE_TIME dt;
}SETTING_ITEM;

typedef struct{
	char szName[WRTC_MAX_NAME_LEN+1];
	int nCount;
	SETTING_ITEM  items[WRTC_MAX_SETTING_ITEMS];
}WRTC_SETTINGS;

typedef struct{
		unsigned char path[WRTC_MAX_FILE_NAME_LEN+1];
		int size;
		int result;
}WRTC_TAKE_PICTURE_NOTIFY;

typedef struct{
	char pwd[WRTC_MAX_VALUE_LEN+1];
}WRTC_WIFI_PASSWD_REQ;

typedef struct{
	int result;
}WRTC_WIFI_PASSWD_RSP;

typedef struct{
	char ssid[WRTC_MAX_VALUE_LEN+1];
}WRTC_WIFI_SSID_REQ;

typedef struct{
	int result;
}WRTC_WIFI_SSID_RSP;

typedef enum{
	WRTC_GET_FILES_PICTURE = 1,
	WRTC_GET_FILES_VIDEO
}REQ_TYPE_T;

typedef struct{
	REQ_TYPE_T reqType;
}WRTC_GET_FILES_REQ;

typedef struct{
	REQ_TYPE_T reqType;
	int result;
}WRTC_GET_FILES_RSP;

typedef struct{
	char file_name[WRTC_MAX_FILE_NAME_LEN+1];
}WRTC_GET_FILE_INFO_T;

typedef struct{
	char file_name[WRTC_MAX_FILE_NAME_LEN+1];
	int result;
}WRTC_GET_FILE_INFO_RSP_T;

typedef enum{
	WRTC_GET_FILES_D_VIDEO = 1,
	WRTC_GET_FILES_D_PICTURE,
	WRTC_GET_FILES_D_EVENT_VIDEO
}FILE_LIST_REQ_D_TYPE_T;

typedef struct{
	FILE_LIST_REQ_D_TYPE_T reqType;
}WRTC_GET_FILE_LIST_D_REQ;

typedef struct{
	FILE_LIST_REQ_D_TYPE_T reqType;
	int result;
}WRTC_GET_FILE_LIST_D_RSP;




#define WRTC_CMD_TAKE_PICTURE			("TakePicture")

#define WRTC_CMD_SET_RESOLUTION			("Resolutionl")
#define WRTC_VALUE_RESOLUTION_720P		("720P")
#define WRTC_VALUE_RESOLUTION_1080P		("1080P")

#define WRTC_CMD_SET_PICQUALITY			("PicQuality")
#define WRTC_VALUE_PICQUALITY_QQVGA		("QQVGA")
#define	WRTC_VALUE_PICQUALITY_QVGA		("QVGA")
#define WRTC_VALUE_PICQUALITY_720P		("720P")
#define WRTC_VALUE_PICQUALITY_1080P		("1080P")


#define WRTC_CMD_SET_RECORD_TIME			("RecordTime")
#define WRTC_VALUE_RECORD_TIME_MINUTE		(60)
#define WRTC_VALUE_RECORD_TIME_2MINUTE		(120)


#define WRTC_CMD_SET_EXPOSURE			("Exposure")
#define WRTC_VALUE_EXPOSURE_MINUS_3		(-3)
#define WRTC_VALUE_EXPOSURE_MINUS_2		(-2)
#define WRTC_VALUE_EXPOSURE_MINUS_1		(-1)
#define WRTC_VALUE_EXPOSURE_ZERO		(0)
#define WRTC_VALUE_EXPOSURE_1			(1)
#define WRTC_VALUE_EXPOSURE_2			(2)
#define WRTC_VALUE_EXPOSURE_3			(3)


#define WRTC_CMD_SET_WB					("WhiteBalance")
#define WRTC_VALUE_WB_AUTO				("Auto")
#define WRTC_VALUE_WB_DAYLIGHT			("Daylight")
#define WRTC_VALUE_WB_CLOUDY			("Cloudy")
#define WRTC_VALUE_WB_INCANDESCENT		("Incandescent")
#define WRTC_VALUE_WB_FLUORESCENT		("Fluorescent")


#define WRTC_CMD_SET_CONTRAST			("Contrast")
#define WRTC_VALUE_CONTRAST_0			(0)
#define WRTC_VALUE_CONTRAST_1			(1)
#define WRTC_VALUE_CONTRAST_2			(2)
#define WRTC_VALUE_CONTRAST_3			(3)
#define WRTC_VALUE_CONTRAST_4			(4)
#define WRTC_VALUE_CONTRAST_5			(5)


#define WRTC_CMD_SET_WATERMARK			("WaterMark")
#define WRTC_VALUE_BOOL_TRUE			("true")
#define WRTC_VALUE_BOOL_FALSE			("false")


#define WRTC_CMD_SET_MOVING				("Moving")


#define WRTC_CMD_SET_GPS				("Gps")


#define WRTC_CMD_SET_MOBILE				("Mobile")


#define WRTC_CMD_SET_FORMAT				("Format")


#define WRTC_CMD_SET_RESET_FACTORY		("ResetFactory")


#define WRTC_CMD_GET_ALL_PARAMS	("GetSettings")

#define WRTC_CMD_SETTINGS		("Settings")

#define WRTC_CMD_SET_RECORD      ("Record")
#define WRTC_CMD_SET_SOUND        ("Sound")
#define WRTC_CMD_SET_DATE_TIME        ("DateTime")
#define WRTC_CMD_SET_WIFI_PASSWD	("WfiPasswd")
#define WRTC_CMD_SET_WIFI_SSID	("WfiSsid")

#define WRTC_TAKE_PICTURE_RSP		("TakePictureLocation")

#define WRTC_CMD_GET_PICTURE_FILES	("PictureList")
#define WRTC_CMD_GET_VIDEO_FILES	("VideoList")
#define WRTC_CMD_GET_FILE_INFO		("FileInfo")

#define WRTC_SDCARD_PICTURE_PATH	("/mnt/external_sd/photo")
#define WRTC_SDCARD_VIDEO_PATH		("/mnt/external_sd/video")
#define WRTC_SDCARD_VIDEO_R_PATH		("/mnt/external_sd/video_r")
#define WRTC_SDCARD_VIDEO_F_ALT_PATH		("/mnt/external_sd/video_f_rlt")

#define WRTC_CMD_GET_FILE_LIST_D			("FileListD")
#define WRTC_CMD_GET_FILE_LIST_TYPE_V		("Video")
#define WRTC_CMD_GET_FILE_LIST_TYPE_P		("Picture")
#define WRTC_CMD_GET_FILE_LIST_TYPE_EV		("EventVideo")


#define WRTC_CMD_DEL_FILE			("DelFile")

typedef struct{
	char path[WRTC_MAX_FILE_NAME_LEN+1];
}WRTC_DEL_FILE_REQ;

typedef struct{
	char path[WRTC_MAX_FILE_NAME_LEN+1];
	int result;
}WRTC_DEL_FILE_RSP;


#define WRTC_CMD_GET_DEVICE_INFO				("DeviceInfo")
#define WRTC_DEVICE_INFO_CHIP_NAME				("x3-C3230RK")

typedef struct{
	char deviceid[WRTC_MAX_NAME_LEN+1];
	char version[WRTC_MAX_FILE_NAME_LEN+1];
	char cputype[WRTC_MAX_NAME_LEN+1];
	char cpu[WRTC_MAX_NAME_LEN+1];
	char manufacturer[WRTC_MAX_NAME_LEN+1];
}WRTC_GET_DEVICE_INFO_RSP;


#define WRTC_CMD_GET_AP_INFO				("ApInfo")

typedef struct{
	char ssid[WRTC_MAX_NAME_LEN+1];
	char pwd[WRTC_MAX_VALUE_LEN+1];
	int result;
}WRTC_GET_AP_INFO_RSP;


#define WRTC_CMD_GET_CAM_NUMBER				("CamNumber")
typedef struct{
	int number;
	int result;
}WRTC_GET_CAM_NUMBER_RSP;


#define WRTC_CMD_GET_RTSP_URL				("RtspURL")
typedef struct{
	char camfront[WRTC_MAX_URL_LEN+1];
	char camback[WRTC_MAX_URL_LEN+1];
	int result;
}WRTC_GET_RTSP_URL_RSP;


typedef enum
{
	REMOTE_NETWORK_TYPE_INVALID = 0,
	REMOTE_NETWORK_TYPE_REALTIME = 1,
	REMOTE_NETWORK_TYPE_ALARM
}REMOTE_NETWORK_TYPE;

typedef struct
{
	char sid[WRTC_MAX_NAME_LEN+1];
	REMOTE_NETWORK_TYPE type;
	long timestamp;
}REMOTE_NETWORK_LOCATION_REQ_T;

typedef struct
{
	char sid[WRTC_MAX_NAME_LEN+1];
	REMOTE_NETWORK_TYPE type;
	long timestamp;
	int result;
}REMOTE_NETWORK_LOCATION_RSP_T;

typedef struct
{
	char sid[WRTC_MAX_NAME_LEN+1];
	REMOTE_NETWORK_TYPE type;
	int duration;
	int video_foramt;
	long timestamp;
}REMOTE_NETWORK_VIDEO_REQ_T;

typedef struct
{
	char sid[WRTC_MAX_NAME_LEN+1];
	REMOTE_NETWORK_TYPE type;
	int duration;
	int video_foramt;
	long timestamp;
	char file[WRTC_MAX_FILE_NAME_LEN+1];
	double latitude;
	double longtitude;
	char url_info[WRTC_MAX_URL_LEN+1];
	char addr_info[WRTC_MAX_URL_LEN+1];
	int result;
}REMOTE_NETWORK_VIDEO_RSP_T;

typedef struct
{
	char sid[WRTC_MAX_NAME_LEN+1];
	REMOTE_NETWORK_TYPE type;
	int img_foramt;
	long timestamp;
}REMOTE_NETWORK_IMG_REQ_T;

typedef struct
{
	char sid[WRTC_MAX_NAME_LEN+1];
	REMOTE_NETWORK_TYPE type;
	int img_foramt;
	long timestamp;
	char file[WRTC_MAX_FILE_NAME_LEN+1];
	double latitude;
	double longtitude;
	char url_info[WRTC_MAX_URL_LEN+1];
	char addr_info[WRTC_MAX_URL_LEN+1];
	int result;
}REMOTE_NETWORK_IMG_RSP_T;

typedef enum
{
	REMOTE_STATUS_TYPE_INVALID = 0,
	REMOTE_STATUS_TYPE_RECORDING = 1,
	REMOTE_STATUS_TYPE_UPLOADING,
	REMOTE_STATUS_TYPE_REJECTED
}REMOTE_STATUS_TYPE;

typedef enum
{
	REMOTE_NETWORK_DATA_TYPE_INVALID = 0,
	REMOTE_NETWORK_DATA_TYPE_VIDEO = 1,
	REMOTE_NETWORK_DATA_TYPE_IMG
}REMOTE_NETWORK_DATA_TYPE;

typedef struct
{
	REMOTE_STATUS_TYPE type;	
}REMOTE_NETWORK_UPDATE_STATUS_IND_T;

typedef struct
{
	char file_name[WRTC_MAX_FILE_NAME_LEN+1];
	long timestamp;
	REMOTE_NETWORK_TYPE type;
	REMOTE_NETWORK_DATA_TYPE data_type;
}REMOTE_NETWORK_UPLOAD_REQ_T;

typedef struct
{
	char file_name[WRTC_MAX_FILE_NAME_LEN+1];
	long timestamp;
	REMOTE_NETWORK_TYPE type;
	REMOTE_NETWORK_DATA_TYPE data_type;
}REMOTE_NETWORK_GET_STORAGE_TOKEN_REQ_T;

typedef struct
{
	char file_name[WRTC_MAX_FILE_NAME_LEN+1];
	long timestamp;
	REMOTE_NETWORK_TYPE type;
	REMOTE_NETWORK_DATA_TYPE data_type;
	char storage_token[WRTC_MAX_FILE_NAME_LEN+1];
	char storage_domain[WRTC_MAX_FILE_NAME_LEN+1];
	int result;
}REMOTE_NETWORK_GET_STORAGE_TOKEN_RSP_T;

typedef struct
{
	char file_name[WRTC_MAX_FILE_NAME_LEN+1];
	long timestamp;
	REMOTE_NETWORK_TYPE type;
	REMOTE_NETWORK_DATA_TYPE data_type;
	char storage_token[WRTC_MAX_FILE_NAME_LEN+1];
	char storage_domain[WRTC_MAX_FILE_NAME_LEN+1];
	int result;
	char storage_upload_url[WRTC_MAX_FILE_NAME_LEN+1];
}REMOTE_NETWORK_UPLOAD_STORAGE_DONE_T;


typedef struct
{
	char storage_token[WRTC_MAX_FILE_NAME_LEN+1];
	char storage_domain[WRTC_MAX_FILE_NAME_LEN+1];
	int result;
}REMOTE_NETWORK_STORAGE_TOKEN_INFO_T;


#define REMOTE_NETWORK_S_ID						("deviceSeriesNo")
#define REMOTE_NETWORK_GET_GPS_INFO_CMD			("remote_location")
#define REMOTE_NETWORK_GET_VIDEO_CMD			("remote_video")
#define REMOTE_NETWORK_GET_IMAGE_CMD			("remote_img")
#define REMOTE_NETWORK_GPS_LONGITUDE			("longitude")
#define REMOTE_NETWORK_GPS_LATITUDE				("latitude")
#define REMOTE_NETWORK_S_TYPE					("deviceType")
#define REMOTE_NETWORK_S_TYPE_VALUE				("TACHOGRAPH")

#define REMOTE_NETWORK_GPS_TYPE_NORMAL			("REAL_TIME")
#define REMOTE_NETWORK_GPS_TYPE_ALARM			("WARNING")

#define REMOTE_NETWORK_STATUS_RECORDING			("RECORDING")
#define REMOTE_NETWORK_STATUS_UPLOADING			("UPLOADING")
#define REMOTE_NETWORK_STATUS_REJECTED			("REJECTED")



typedef enum
{
	REOMOTE_NETWORK_CMD_INVALID,
	REOMOTE_NETWORK_CMD_LOCATION,
	REOMOTE_NETWORK_CMD_GPS,
    REOMOTE_NETWORK_CMD_WIFI,
    REOMOTE_NETWORK_CMD_CELLULAR,
    REOMOTE_NETWORK_CMD_WATERMARK,
    REOMOTE_NETWORK_CMD_MOVING,
    REOMOTE_NETWORK_CMD_FORMAT,
    REOMOTE_NETWORK_CMD_FACTORY_RESET,
    REOMOTE_NETWORK_CMD_RECORD,
    REOMOTE_NETWORK_CMD_VOICE,
    REOMOTE_NETWORK_CMD_RESOLUTION,
    REOMOTE_NETWORK_CMD_RECORD_TIME,
    REOMOTE_NETWORK_CMDEXPOSURE,
    REOMOTE_NETWORK_CMD_WB,
    REOMOTE_NETWORK_CMD_CONTRAST,
    REOMOTE_NETWORK_CMD_DATE_TIME,
    REOMOTE_NETWORK_CMD_HOTSPOT_PASSWD,
    REOMOTE_NETWORK_CMD_TAKE_PICTURE,
    REOMOTE_NETWORK_CMD_GET_PARAMS,
    REOMOTE_NETWORK_CMD_GET_PICTURE_FILES_LIST,
    REOMOTE_NETWORK_CMD_GET_VIDEO_FILES_LIST,
    REOMOTE_NETWORK_CMD_GET_FILE_INFO,
    REOMOTE_NETWORK_CMD_GET_FILE_LIST_D_VIDEO,
    REOMOTE_NETWORK_CMD_GET_FILE_LIST_D_PICTURE,
    REOMOTE_NETWORK_CMD_GET_FILE_LIST_D_EVENT_VIDEO,
    REOMOTE_NETWORK_CMD_DEL_FILE,
    REOMOTE_NETWORK_CMD_DEVICE_INFO,
    REOMOTE_NETWORK_CMD_AP_INFO,
    REOMOTE_NETWORK_CMD_CAM_NUMBER,
    REOMOTE_NETWORK_CMD_RTSP_URL,
    REOMOTE_NETWORK_CMD_GET_PROFILE,
    REOMOTE_NETWORK_CMD_UPDATE_STATUS,
    REOMOTE_NETWORK_CMD_UPLOAD_DONE,
    REOMOTE_NETWORK_CMD_GET_STORAGE_TOKEN
}REOMOTE_NETWORK_CMD_TYPE;

#define REMOTE_NETWORK_BUFFER_LEN			(1024)
typedef struct{
	char buffer[REMOTE_NETWORK_BUFFER_LEN+1];
	REOMOTE_NETWORK_CMD_TYPE type;
}REMOTE_NETWORK_NOTIFY_T;

typedef enum
{
	REMOTE_NETWORK_ALARM_TYPE_INVALID,
	REMOTE_NETWORK_ALARM_TYPE_MD
}REMOTE_NETWORK_ALARM_TYPE;

typedef struct{
	REMOTE_NETWORK_ALARM_TYPE type;
}REMOTE_NETWORK_ALARM_T;

#endif /* _FWK_GL_DEF_837689389163_*/
