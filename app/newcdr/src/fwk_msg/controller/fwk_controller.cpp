/*   ----------------------------------------------------------------------
Copyright (C) 2014-2016 Fuzhou Rockchip Electronics Co., Ltd

     Sec Class: Rockchip Confidential

V1.0 Dayao Ji <jdy@rock-chips.com>
---------------------------------------------------------------------- */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/msg.h>

#include <netinet/in.h>
#include <pthread.h>
#include <termio.h>
#include <errno.h>
#include <dirent.h>
#include <linux/msg.h>



#include "fwk_gl_api.h"
#include "fwk_gl_def.h"
#include "fwk_gl_msg.h"

/*for current newcdr controller*/
#include <network/hotspot.h>
#include <network/hotspotSetting.h>
#include <network/wifiIf.h>
#include <network/config.h>
#include <gps/GpsController.h>
#ifdef MODEM_ENABLE
#include <cellular/CellularController.h>
#endif
#include "dateSetting.h"
#include <properties.h>


#undef LOG_TAG
#define LOG_TAG "fwk_controller"
#include "debug.h"

using namespace std;
#include <string>
#include <json/json.h>


static int fwk_controller_queue;
/*for MINIGUI*/
extern CdrMain *pView_cdrMain;
extern pthread_mutex_t s_view_mutex;


#ifdef REMOTE_3G
extern int record_for_remote(HerbCamera* pHC,time_t timestamp, int type);
extern void notify_is_recording(void);
extern char* remote_network_get_token(void);
extern void notify_is_rejecting(void);
#endif

void remote_preview_callback(int cam_type)
{
	pthread_mutex_lock(&s_view_mutex);
	pView_cdrMain->setCameraCallback(cam_type);
	pthread_mutex_unlock(&s_view_mutex);
}


void deal_controller_msg(FWK_MSG* msg)
{

    
    switch(msg->msg_id)
    {
    	
		case MSG_ID_FWK_CONTROL_WRTC_CALL_REQ:
			{
				#if 0
				const char* str="{\"name\":\"TakePicture\",\"type\":\"Action\"}";
				Json::Reader reader;
				Json::Value root;

				if(reader.parse(str,root))
				{
					string name=root["name"].asString();
					db_msg("jdy fwk_msg MSG_ID_FWK_CONTROL_DEMO_REQ name=%s \n", name.c_str());	
					const char* pname=root["name"].asCString();
					db_msg("jdy fwk_msg MSG_ID_FWK_CONTROL_DEMO_REQ name=%s \n", pname);
				}
				//fwk_send_message_ext(msg->dest, msg->source, MSG_ID_FWK_CONTROL_DEMO_RSP, NULL, 0);
				#endif
				//unsigned char *pbuf = (unsigned char*)msg->para;
				const char *pbuf = (const char*)msg->para;
				Json::Reader reader;
				Json::Value root;
				
				db_msg("fwk_msg MSG_ID_FWK_CONTROL_WRTC_CALL_REQ pbuf=%s \n", pbuf);	
				if(reader.parse(pbuf,root))
				{
					const char* pname=root["name"].asCString();
					const char* pOper=root["operation"].asCString();
					
					db_msg("fwk_msg MSG_ID_FWK_CONTROL_WRTC_CALL_REQ name=%s pOper=%s\n", pname, pOper);
					if(pname != NULL)
					{
						if(strcasecmp(pname,WRTC_CMD_TAKE_PICTURE) == 0)
						{
							fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_TAKE_PICTURE_REQ, NULL, 0);
						}
						else if(strcasecmp(pname,WRTC_CMD_SET_RESOLUTION) == 0)
						{
							ValueReq req;
							const char* pvalue=root["value"].asCString();

							db_msg("fwk_msg MSG_ID_FWK_CONTROL_WRTC_CALL_REQ name=%s pvalue=%s\n", pname, pvalue);
							memset(&req, 0x0, sizeof(req));
							req.cmdType = VALUE_TYPE_RESOLUTION;
							if(pvalue != NULL)
							{
								if(strcasecmp(pvalue, WRTC_VALUE_RESOLUTION_720P) == 0)
								{
									req.value = 720;
									fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_SET_VALUE_REQ, &req, sizeof(req));
								}
								else if(strcasecmp(pvalue, WRTC_VALUE_RESOLUTION_1080P) == 0)
								{
									req.value = 1080;
									fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_SET_VALUE_REQ, &req, sizeof(req));
								}
							}
							
						}
						else if(strcasecmp(pname,WRTC_CMD_SET_RECORD_TIME) == 0)
						{
							ValueReq req;
							int pvalue=root["value"].asInt();

							db_msg("fwk_msg MSG_ID_FWK_CONTROL_WRTC_CALL_REQ name=%s pvalue=%d\n", pname, pvalue);
							memset(&req, 0x0, sizeof(req));
							req.cmdType = VALUE_TYPE_RECORD_TIME;
							req.value = pvalue;
							fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_SET_VALUE_REQ, &req, sizeof(req));
						}
						else if(strcasecmp(pname,WRTC_CMD_SET_EXPOSURE) == 0)
						{
							ValueReq req;
							int pvalue=root["value"].asInt();

							db_msg("fwk_msg MSG_ID_FWK_CONTROL_WRTC_CALL_REQ name=%s pvalue=%d\n", pname, pvalue);
							memset(&req, 0x0, sizeof(req));
							req.cmdType = VALUE_TYPE_EXPOSURE;
							req.value = pvalue;
							fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_SET_VALUE_REQ, &req, sizeof(req));
						}
						else if(strcasecmp(pname,WRTC_CMD_SET_WB) == 0)
						{
							ValueReq req;
							const char* pvalue=root["value"].asCString();

							db_msg("fwk_msg MSG_ID_FWK_CONTROL_WRTC_CALL_REQ name=%s pvalue=%s\n", pname, pvalue);
							memset(&req, 0x0, sizeof(req));
							req.cmdType = VALUE_TYPE_WB;
							if(pvalue != NULL)
							{
								if(strcasecmp(pvalue, WRTC_VALUE_WB_AUTO) == 0)
								{
									req.value = WB_VALUE_AUTO;
									fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_SET_VALUE_REQ, &req, sizeof(req));
								}
								else if(strcasecmp(pvalue, WRTC_VALUE_WB_DAYLIGHT) == 0)
								{
									req.value = WB_VALUE_DAYLIGHT;
									fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_SET_VALUE_REQ, &req, sizeof(req));
								}
								else if(strcasecmp(pvalue, WRTC_VALUE_WB_CLOUDY) == 0)
								{
									req.value = WB_VALUE_CLOUDY;
									fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_SET_VALUE_REQ, &req, sizeof(req));
								}
								else if(strcasecmp(pvalue, WRTC_VALUE_WB_INCANDESCENT) == 0)
								{
									req.value = WB_VALUE_INCANDESCENT;
									fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_SET_VALUE_REQ, &req, sizeof(req));
								}
								else if(strcasecmp(pvalue, WRTC_VALUE_WB_FLUORESCENT) == 0)
								{
									req.value = WB_VALUE_FLUORESCENT;
									fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_SET_VALUE_REQ, &req, sizeof(req));
								}
							}
							
						}
						else if(strcasecmp(pname,WRTC_CMD_SET_CONTRAST) == 0)
						{
							ValueReq req;
							int pvalue=root["value"].asInt();

							db_msg("fwk_msg MSG_ID_FWK_CONTROL_WRTC_CALL_REQ name=%s pvalue=%d\n", pname, pvalue);
							memset(&req, 0x0, sizeof(req));
							req.cmdType = VALUE_TYPE_CONTRAST;
							req.value = pvalue;
							fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_SET_VALUE_REQ, &req, sizeof(req));
						}
						else if(strcasecmp(pname,WRTC_CMD_SET_WATERMARK) == 0)
						{
							CommuReq req;
							int pvalue=root["value"].asBool();

							db_msg("fwk_msg MSG_ID_FWK_CONTROL_WRTC_CALL_REQ name=%s pvalue=%d\n", pname, pvalue);
							memset(&req, 0x0, sizeof(req));
							req.cmdType = COMMU_TYPE_WATERMARK;
							req.operType = pvalue;
							fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_COMMU_SWITCH_REQ, &req, sizeof(req));
						}
						else if(strcasecmp(pname,WRTC_CMD_SET_MOVING) == 0)
						{
							CommuReq req;
							int pvalue=root["value"].asBool();

							db_msg("fwk_msg MSG_ID_FWK_CONTROL_WRTC_CALL_REQ name=%s pvalue=%d\n", pname, pvalue);
							memset(&req, 0x0, sizeof(req));
							req.cmdType = COMMU_TYPE_MOVING;
							req.operType = pvalue;
							fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_COMMU_SWITCH_REQ, &req, sizeof(req));
						}
						else if(strcasecmp(pname,WRTC_CMD_SET_GPS) == 0)
						{
							CommuReq req;
							int pvalue=root["value"].asBool();

							db_msg("fwk_msg MSG_ID_FWK_CONTROL_WRTC_CALL_REQ name=%s pvalue=%d\n", pname, pvalue);
							memset(&req, 0x0, sizeof(req));
							req.cmdType = COMMU_TYPE_GPS;
							req.operType = pvalue;
							fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_COMMU_SWITCH_REQ, &req, sizeof(req));
						}
						else if(strcasecmp(pname,WRTC_CMD_SET_MOBILE) == 0)
						{
							CommuReq req;
							int pvalue=root["value"].asBool();

							db_msg("fwk_msg MSG_ID_FWK_CONTROL_WRTC_CALL_REQ name=%s pvalue=%d\n", pname, pvalue);
							memset(&req, 0x0, sizeof(req));
							req.cmdType = COMMU_TYPE_CELLULAR;
							req.operType = pvalue;
							fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_COMMU_SWITCH_REQ, &req, sizeof(req));
						}
						else if(strcasecmp(pname,WRTC_CMD_SET_FORMAT) == 0)
						{
							CommuReq req;
							int pvalue=root["value"].asBool();

							db_msg("fwk_msg MSG_ID_FWK_CONTROL_WRTC_CALL_REQ name=%s pvalue=%d\n", pname, pvalue);
							memset(&req, 0x0, sizeof(req));
							req.cmdType = COMMU_TYPE_FORMAT;
							req.operType = pvalue;
							fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_COMMU_SWITCH_REQ, &req, sizeof(req));
						}
						else if(strcasecmp(pname,WRTC_CMD_SET_RESET_FACTORY) == 0)
						{
							CommuReq req;
							int pvalue=root["value"].asBool();
							CommuRsp rsp;

							db_msg("fwk_msg MSG_ID_FWK_CONTROL_WRTC_CALL_REQ name=%s pvalue=%d\n", pname, pvalue);
							rsp.cmdType = COMMU_TYPE_FACTORY_RESET;
							rsp.operType = pvalue;
							rsp.result = 0;
							fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_VIEW, MSG_ID_FWK_CONTROL_COMMU_SWITCH_RSP, &rsp, sizeof(rsp));
							
							memset(&req, 0x0, sizeof(req));
							req.cmdType = COMMU_TYPE_FACTORY_RESET;
							req.operType = pvalue;
							fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_COMMU_SWITCH_REQ, &req, sizeof(req));
						}
						else if(strcasecmp(pname,WRTC_CMD_GET_ALL_PARAMS) == 0)
						{
							fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_GET_PARAS_REQ, NULL, 0);
						}
						else if(strcasecmp(pname,WRTC_CMD_SET_RECORD) == 0)
						{
							CommuReq req;
							int pvalue=root["value"].asBool();

							db_msg("fwk_msg MSG_ID_FWK_CONTROL_WRTC_CALL_REQ name=%s pvalue=%d\n", pname, pvalue);
							memset(&req, 0x0, sizeof(req));
							req.cmdType = COMMU_TYPE_RECORD;
							req.operType = pvalue;
							fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_COMMU_SWITCH_REQ, &req, sizeof(req));
						}
						else if(strcasecmp(pname,WRTC_CMD_SET_SOUND) == 0)
						{
							CommuReq req;
							int pvalue=root["value"].asBool();

							db_msg("fwk_msg MSG_ID_FWK_CONTROL_WRTC_CALL_REQ name=%s pvalue=%d\n", pname, pvalue);
							memset(&req, 0x0, sizeof(req));
							req.cmdType = COMMU_TYPE_VOICE;
							req.operType = pvalue;
							fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_COMMU_SWITCH_REQ, &req, sizeof(req));
						}
						else if(strcasecmp(pname,WRTC_CMD_SET_DATE_TIME) == 0)
						{
							WRTC_DATE_TIME dt;
							
							memset(&dt, 0x0, sizeof(dt));
							dt.year = root["year"].asUInt();
							dt.month = root["month"].asUInt();
							dt.day = root["day"].asUInt();
							dt.hour = root["hour"].asUInt();
							dt.minute = root["minute"].asUInt();
							dt.second = root["second"].asUInt();
							fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_SET_DATE_TIME_REQ, &dt, sizeof(dt));
						}
						else if(strcasecmp(pname,WRTC_CMD_SET_WIFI_PASSWD) == 0)
						{
							WRTC_WIFI_PASSWD_REQ req;
							const char* pvalue=root["value"].asCString();
							WRTC_WIFI_PASSWD_RSP w_rsp;

							memset(&req, 0x0, sizeof(req));
							memset(&w_rsp, 0x0, sizeof(w_rsp));
							if(pvalue != NULL)
							{
								strncpy(req.pwd, pvalue,WRTC_MAX_VALUE_LEN);
								w_rsp.result = 0;
							}
							else
							{
								w_rsp.result = -1;
							}
							fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_VIEW, MSG_ID_FWK_CONTROL_SET_WIFIPASSWD_RSP, &w_rsp, sizeof(w_rsp));
							
							if(pvalue != NULL)
							{
								fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_SET_WIFIPASSWD_REQ, &req, sizeof(req));
							}
							
						}
						else if(strcasecmp(pname,WRTC_CMD_GET_PICTURE_FILES) == 0)
						{
							WRTC_GET_FILES_REQ req;

							memset(&req, 0x0, sizeof(req));
							req.reqType = WRTC_GET_FILES_PICTURE;
							
							fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_GET_FILES_REQ, &req, sizeof(req));
							
						}
						else if(strcasecmp(pname,WRTC_CMD_GET_VIDEO_FILES) == 0)
						{
							WRTC_GET_FILES_REQ req;

							memset(&req, 0x0, sizeof(req));
							req.reqType = WRTC_GET_FILES_VIDEO;
							
							fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_GET_FILES_REQ, &req, sizeof(req));
							
						}
						else if(strcasecmp(pname,WRTC_CMD_GET_FILE_INFO) == 0)
						{
							WRTC_GET_FILE_INFO_T req;
							const char* pvalue=root["value"].asCString();

							memset(&req, 0x0, sizeof(req));
							if(pvalue != NULL)
							{
								strncpy(req.file_name, pvalue, WRTC_MAX_FILE_NAME_LEN);
							}
							
							fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_GET_FILE_INFO_REQ, &req, sizeof(req));
							
						}
						else if(strcasecmp(pname,WRTC_CMD_GET_FILE_LIST_D) == 0)
						{
							WRTC_GET_FILE_LIST_D_REQ req;
							const char* pvalue=root["type"].asCString();

							memset(&req, 0x0, sizeof(req));
							if(pvalue != NULL)
							{
								db_msg("fwk_msg MSG_ID_FWK_CONTROL_WRTC_CALL_REQ name=%s pvalue=%d\n", pname, pvalue);
								if(strcasecmp(pvalue, WRTC_CMD_GET_FILE_LIST_TYPE_V) == 0)
								{
									req.reqType = WRTC_GET_FILES_D_VIDEO;
								}
								else if(strcasecmp(pvalue, WRTC_CMD_GET_FILE_LIST_TYPE_P) == 0)
								{
									req.reqType = WRTC_GET_FILES_D_PICTURE;
								}
								else if(strcasecmp(pvalue, WRTC_CMD_GET_FILE_LIST_TYPE_EV) == 0)
								{
									req.reqType = WRTC_GET_FILES_D_EVENT_VIDEO;
								}
								
							}
							
							
							fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_GET_FILE_LIST_D_REQ, &req, sizeof(req));
							
						}
						else if(strcasecmp(pname,WRTC_CMD_DEL_FILE) == 0)
						{
							WRTC_DEL_FILE_REQ req;
							const char* pvalue=root["value"].asCString();

							memset(&req, 0x0, sizeof(req));
							if(pvalue != NULL)
							{
								db_msg("fwk_msg MSG_ID_FWK_CONTROL_WRTC_CALL_REQ name=%s pvalue=%d\n", pname, pvalue);
								strncpy(req.path, pvalue, WRTC_MAX_FILE_NAME_LEN);
							}
							
							
							fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_DEL_FILE_REQ, &req, sizeof(req));
							
						}
						else if(strcasecmp(pname,WRTC_CMD_GET_DEVICE_INFO) == 0)
						{
							db_msg("fwk_msg MSG_ID_FWK_CONTROL_WRTC_CALL_REQ name=%s \n", pname);
							
							
							fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_DEVICE_INFO_REQ, NULL, 0);
							
						}
						else if(strcasecmp(pname,WRTC_CMD_GET_AP_INFO) == 0)
						{
							db_msg("fwk_msg WRTC_CMD_GET_AP_INFO name=%s \n", pname);
							
							
							fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_AP_INFO_REQ, NULL, 0);
							
						}
						else if(strcasecmp(pname,WRTC_CMD_GET_CAM_NUMBER) == 0)
						{
							db_msg("fwk_msg WRTC_CMD_GET_CAM_NUMBER	 name=%s \n", pname);
							
							
							fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_CAM_NUMBER_INFO_REQ, NULL, 0);
							
						}
						else if(strcasecmp(pname,WRTC_CMD_GET_RTSP_URL) == 0)
						{
							db_msg("fwk_msg WRTC_CMD_GET_RTSP_URL	 name=%s \n", pname);
							
							
							fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_RTSP_URL_REQ, NULL, 0);
							
						}
						else
						{
							db_msg("fwk_msg MSG_ID_FWK_CONTROL_WRTC_CALL_REQ Unknown wrtc cmd name=%s \n", pname);
							
						}
					}

					if(pOper != NULL)
					{
						db_msg("fwk_msg MSG_ID_FWK_CONTROL_WRTC_CALL_REQ pOper=%s \n", pOper);
						if(strcasecmp(pOper, REMOTE_NETWORK_GET_GPS_INFO_CMD) == 0)
						{
							REMOTE_NETWORK_LOCATION_REQ_T req_t;
							const char* pNo = root["deviceSeriesNo"].asCString();
							
							db_msg("fwk_msg remote_location deviceSeriesNo=%s \n", pNo);
							memset(&req_t, 0, sizeof(req_t));
							if(pNo != NULL)
							{
								strncpy(req_t.sid, pNo, WRTC_MAX_NAME_LEN);
								req_t.type = REMOTE_NETWORK_TYPE_REALTIME;
								req_t.timestamp = time(NULL);
							}
							fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_GET_GPS_INFO_REQ, &req_t, sizeof(req_t));
						}
						else if(strcasecmp(pOper, REMOTE_NETWORK_GET_VIDEO_CMD) == 0)
						{
							REMOTE_NETWORK_VIDEO_REQ_T req_t;
							const char* pNo = root["deviceSeriesNo"].asCString();
							
							db_msg("fwk_msg remote_video deviceSeriesNo=%s \n", pNo);
							memset(&req_t, 0, sizeof(req_t));
							if(pNo != NULL)
							{
								strncpy(req_t.sid, pNo, WRTC_MAX_NAME_LEN);
								req_t.type = REMOTE_NETWORK_TYPE_REALTIME;
								req_t.timestamp = time(NULL);
								req_t.duration = 15;
								req_t.video_foramt = 0;
							}
							fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_3G_REMOTE_VIDEO_REQ, &req_t, sizeof(req_t));
						}
						else if(strcasecmp(pOper, REMOTE_NETWORK_GET_IMAGE_CMD) == 0)
						{
							REMOTE_NETWORK_IMG_REQ_T req_t;
							const char* pNo = root["deviceSeriesNo"].asCString();
							
							db_msg("fwk_msg remote_img deviceSeriesNo=%s \n", pNo);
							memset(&req_t, 0, sizeof(req_t));
							if(pNo != NULL)
							{
								strncpy(req_t.sid, pNo, WRTC_MAX_NAME_LEN);
								req_t.type = REMOTE_NETWORK_TYPE_REALTIME;
								req_t.timestamp = time(NULL);
								req_t.img_foramt = 0;
							}
							fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_3G_REMOTE_IMAGE_REQ, &req_t, sizeof(req_t));
						}
						else
						{
							db_msg("fwk_msg MSG_ID_FWK_CONTROL_WRTC_CALL_REQ Unknown operation pOper=%s \n", pOper);
						}
					}
					
				}
				
						
			}
			break;

		case MSG_ID_FWK_CONTROL_SET_DATE_TIME_REQ:
			{
				WRTC_DATE_TIME* dt_req = (WRTC_DATE_TIME*)msg->para;
				WRTC_DT_RSP rsp;
				date_t date;
				
				db_msg("fwk_msg MSG_ID_FWK_CONTROL_SET_DATE_TIME_REQ year=%d hour=%d \n", dt_req->year, dt_req->hour);

				memset(&rsp ,0, sizeof(rsp));
				rsp.dt.year = dt_req->year;
				rsp.dt.month = dt_req->month;
				rsp.dt.day = dt_req->day;
				rsp.dt.hour = dt_req->hour;
				rsp.dt.minute = dt_req->minute;
				rsp.dt.second = dt_req->second;
				rsp.result = 0;


				memset(&date ,0, sizeof(date));
				date.year = dt_req->year;
				date.month = dt_req->month;
				date.day = dt_req->day;
				date.hour = dt_req->hour;
				date.minute = dt_req->minute;
				date.second = dt_req->second;
				

				setSystemDate(&date);
				fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_VIEW, MSG_ID_FWK_CONTROL_SET_DATE_TIME_RSP, &rsp, sizeof(rsp));
			}
			break;

		case MSG_ID_FWK_CONTROL_SET_WIFIPASSWD_REQ:
			{
				WRTC_WIFI_PASSWD_REQ* w_req = (WRTC_WIFI_PASSWD_REQ*)msg->para;
				WRTC_WIFI_PASSWD_RSP w_rsp;
				
				db_msg("fwk_msg MSG_ID_FWK_CONTROL_SET_WIFIPASSWD_REQ \n");
				memset(&w_rsp, 0, sizeof(w_rsp));
				w_rsp.result = apSetPasswd(w_req->pwd);
				
				//fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_VIEW, MSG_ID_FWK_CONTROL_SET_WIFIPASSWD_RSP, &w_rsp, sizeof(w_rsp));
			}
			break;
			
		case MSG_ID_FWK_CONTROL_SET_WIFISSID_REQ:
			{
				WRTC_WIFI_SSID_REQ* w_req = (WRTC_WIFI_SSID_REQ*)msg->para;
				WRTC_WIFI_SSID_RSP w_rsp;
				
				db_msg("fwk_msg MSG_ID_FWK_CONTROL_SET_WIFISSID_REQ \n");
				memset(&w_rsp, 0, sizeof(w_rsp));
				w_rsp.result = apSetSsid(w_req->ssid);
				//fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_VIEW, MSG_ID_FWK_CONTROL_SET_WIFISSID_RSP, &w_rsp, sizeof(w_rsp));
			}
			break;

		case MSG_ID_FWK_CONTROL_TAKE_PICTURE_REQ:
			{
			    char value[PROPERTY_VALUE_MAX];
				bool is_insert = false;

                property_get("persist.sys.videoimpact.status", value, "false");
				
				db_msg("fwk_msg MSG_ID_FWK_CONTROL_TAKE_PICTURE_REQ \n");

				pthread_mutex_lock(&s_view_mutex);
				is_insert = pView_cdrMain->isCardInsert();
				pthread_mutex_unlock(&s_view_mutex);
				if(!is_insert)
				{
					WRTC_TAKE_PICTURE_NOTIFY rsp;
					
					db_msg("fwk_msg MSG_ID_FWK_CONTROL_TAKE_PICTURE_REQ snapshot no sdcard \n");
					memset(&rsp, 0, sizeof(rsp));
					rsp.result = -1;
					fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_VIEW, MSG_ID_FWK_CONTROL_TAKE_PICTURE_RSP, &rsp, sizeof(rsp));
				}
				else
				{
					pthread_mutex_lock(&s_view_mutex);
					pView_cdrMain->clearSnapshotPath();
					pView_cdrMain->take_picture();
	                if((strcmp(value, "true") == 0)) {
	                    pView_cdrMain->impact_occur();
	                }
					pthread_mutex_unlock(&s_view_mutex);
				}
			}
			break;
			
		case MSG_ID_FWK_CONTROL_TAKE_THUMB_REQ:
			{
			    char value[PROPERTY_VALUE_MAX];
				
				db_msg("fwk_msg MSG_ID_FWK_CONTROL_TAKE_THUMB_REQ \n");

				pthread_mutex_lock(&s_view_mutex);
				pView_cdrMain->clearSnapshotPath();
				pView_cdrMain->take_picture();
				pthread_mutex_unlock(&s_view_mutex);
			}
			break;

		case MSG_ID_FWK_CONTROL_GET_PARAS_REQ:
			{
				WRTC_SETTINGS rsp;

				memset(&rsp, 0x0, sizeof(rsp));
				pthread_mutex_lock(&s_view_mutex);
				pView_cdrMain->getSettingsWebRTC(&rsp);
				pthread_mutex_unlock(&s_view_mutex);

				
				fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_VIEW, MSG_ID_FWK_CONTROL_GET_PARAS_RSP, &rsp, sizeof(rsp));
			}
			break;

		case MSG_ID_FWK_CONTROL_SET_VALUE_REQ:
			{
				ValueReq* value_req = (ValueReq*)msg->para;
				ValueRsp rsp;
				int value = 0;
				
				db_msg("MSG_ID_FWK_CONTROL_SET_VALUE_REQ");
				//db_msg("fwk_msg MSG_ID_FWK_CONTROL_SET_VALUE_REQ cmdTpe=%d value=%d", value_req->cmdType, value_req->value);	
				memset(&rsp, 0x0, sizeof(rsp));
				rsp.cmdType = value_req->cmdType;
				rsp.value = value_req->value;
				if(value_req->cmdType == VALUE_TYPE_RESOLUTION)
				{
					db_msg("MSG_ID_FWK_CONTROL_SET_VALUE_REQ value=%d", value_req->value);
					if(value_req->value == 1080)
					{
						pthread_mutex_lock(&s_view_mutex);
						rsp.result = pView_cdrMain->setVideoModeWebRTC(0);
						pthread_mutex_unlock(&s_view_mutex);
					}
					else if(value_req->value == 720)
					{
						pthread_mutex_lock(&s_view_mutex);
						rsp.result = pView_cdrMain->setVideoModeWebRTC(1);
						pthread_mutex_unlock(&s_view_mutex);
					}
					else
					{
						db_msg("MSG_ID_FWK_CONTROL_SET_VALUE_REQ VALUE_TYPE_RESOLUTION unknown");
					}
				}
				else if(value_req->cmdType == VALUE_TYPE_RECORD_TIME)
				{
					int index=0;

					index = (value_req->value/60)-1;
					db_msg("MSG_ID_FWK_CONTROL_SET_VALUE_REQ record value=%d index=%d", value_req->value, index);
					pthread_mutex_lock(&s_view_mutex);
					rsp.result = pView_cdrMain->setRecordTimeWebRTC(index);
					pthread_mutex_unlock(&s_view_mutex);
					
				}
				else if(value_req->cmdType == VALUE_TYPE_EXPOSURE)
				{
					int index=0;

					index = value_req->value + 3;
					db_msg("MSG_ID_FWK_CONTROL_SET_VALUE_REQ Exposure value=%d index=%d", value_req->value, index);
					pthread_mutex_lock(&s_view_mutex);
					rsp.result = pView_cdrMain->setExposureWebRTC(index);
					pthread_mutex_unlock(&s_view_mutex);
					
				}
				else if(value_req->cmdType == VALUE_TYPE_WB)
				{
					int index=0;

					index = value_req->value;
					db_msg("MSG_ID_FWK_CONTROL_SET_VALUE_REQ WB value=%d index=%d", value_req->value, index);
					pthread_mutex_lock(&s_view_mutex);
					rsp.result = pView_cdrMain->setWhiteBalanceWebRTC(index);
					pthread_mutex_unlock(&s_view_mutex);
					
				}
				else if(value_req->cmdType == VALUE_TYPE_CONTRAST)
				{
					int index=0;

					index = value_req->value;
					db_msg("MSG_ID_FWK_CONTROL_SET_VALUE_REQ Contrast value=%d index=%d", value_req->value, index);
					pthread_mutex_lock(&s_view_mutex);
					rsp.result = pView_cdrMain->setContrastWebRTC(index);
					pthread_mutex_unlock(&s_view_mutex);
					
				}
				
				fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_VIEW, MSG_ID_FWK_CONTROL_SET_VALUE_RSP, &rsp, sizeof(rsp));
			}
			break;

		case MSG_ID_FWK_CONTROL_COMMU_SWITCH_REQ:
			{
				CommuReq*  communication_req = (CommuReq*)msg->para;
				CommuRsp rsp;

				db_msg("MSG_ID_FWK_CONTROL_COMMU_SWITCH_REQ");	
				db_msg("fwk_msg MSG_ID_FWK_CONTROL_COMMU_SWITCH_REQ %s line=%d cmdType=%d operType=%d", __FUNCTION__, __LINE__,communication_req->cmdType, communication_req->operType);
				memset(&rsp, 0x0, sizeof(rsp));
				rsp.cmdType = communication_req->cmdType;
				rsp.operType = communication_req->operType;
				rsp.result = 0;
				if(communication_req->cmdType == COMMU_TYPE_GPS)
				{
					pthread_mutex_lock(&s_view_mutex);
					pView_cdrMain->setGpsWebRTC(communication_req->operType);
					pthread_mutex_unlock(&s_view_mutex);
					if(communication_req->operType)
					{
						/*ON*/
						if (enableGps())
						{
							rsp.result = 0;
						}
						else
						{
							rsp.result = -1;
						}
					}
					else
					{
						/*oFF*/
						if (disableGps())
						{
							rsp.result = 0;
						}
						else
						{
							rsp.result = -1;
						}
					}
				}
				else if(communication_req->cmdType == COMMU_TYPE_WIFI)
				{
					if(communication_req->operType)
					{
						/*ON*/
						if (!enable_wifi())
						{
							rsp.result = 0;
						}
						else
						{
							rsp.result = -1;
						}
					}
					else
					{
						/*oFF*/
						if (!disable_wifi())
						{
							rsp.result = 0;
						}
						else
						{
							rsp.result = -1;
						}
					}
				}
			#ifdef MODEM_ENABLE
				else if(communication_req->cmdType == COMMU_TYPE_CELLULAR)
				{
					pthread_mutex_lock(&s_view_mutex);
					pView_cdrMain->setCellularWebRTC(communication_req->operType);
					pthread_mutex_unlock(&s_view_mutex);
					if(communication_req->operType)
					{
						/*ON*/
						if (enableCellular())
						{
							rsp.result = 0;
						}
						else
						{
							rsp.result = -1;
						}
					}
					else
					{
						/*oFF*/
						if (disableCellular())
						{
							rsp.result = 0;
						}
						else
						{
							rsp.result = -1;
						}
					}
				}
			#endif
				else if(communication_req->cmdType == COMMU_TYPE_WATERMARK)
				{
					pthread_mutex_lock(&s_view_mutex);
					rsp.result = pView_cdrMain->setTimeWaterMarkWebRTC(communication_req->operType);
					pthread_mutex_unlock(&s_view_mutex);
					
				}
				else if(communication_req->cmdType == COMMU_TYPE_MOVING)
				{
					pthread_mutex_lock(&s_view_mutex);
					rsp.result = pView_cdrMain->setAWMDWebRTC(communication_req->operType);
					pthread_mutex_unlock(&s_view_mutex);
					
				}
				else if(communication_req->cmdType == COMMU_TYPE_FORMAT)
				{
					pthread_mutex_lock(&s_view_mutex);
					rsp.result = pView_cdrMain->setFormatWebRTC();
					pthread_mutex_unlock(&s_view_mutex);
				}
				else if(communication_req->cmdType == COMMU_TYPE_FACTORY_RESET)
				{
					pthread_mutex_lock(&s_view_mutex);
					rsp.result = pView_cdrMain->setFactoryResetWebRTC();
					pthread_mutex_unlock(&s_view_mutex);
				}
				else if(communication_req->cmdType == COMMU_TYPE_RECORD)
				{
					pthread_mutex_lock(&s_view_mutex);
					rsp.result = pView_cdrMain->setRecordWebRTC(communication_req->operType);
					pthread_mutex_unlock(&s_view_mutex);
				}
				else if(communication_req->cmdType == COMMU_TYPE_VOICE)
				{
					pthread_mutex_lock(&s_view_mutex);
					rsp.result = pView_cdrMain->setSoundWebRTC(communication_req->operType);
					pthread_mutex_unlock(&s_view_mutex);
				}

				if(communication_req->cmdType != COMMU_TYPE_FACTORY_RESET)
				{
					fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_VIEW, MSG_ID_FWK_CONTROL_COMMU_SWITCH_RSP, &rsp, sizeof(rsp));
				}
				
			}
			break;

		case MSG_ID_FWK_CONTROL_GET_FILES_REQ:
			{
				WRTC_GET_FILES_REQ* w_req = (WRTC_GET_FILES_REQ*)msg->para;
				WRTC_GET_FILES_RSP w_rsp;
				
				db_msg("fwk_msg MSG_ID_FWK_CONTROL_GET_FILES_REQ \n");
				memset(&w_rsp, 0, sizeof(w_rsp));
				w_rsp.reqType = w_req->reqType;
				
				fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_VIEW, MSG_ID_FWK_CONTROL_GET_FILES_RSP, &w_rsp, sizeof(w_rsp));
			}
			break;

		case MSG_ID_FWK_CONTROL_GET_FILE_INFO_REQ:
			{
				WRTC_GET_FILE_INFO_T* w_req = (WRTC_GET_FILE_INFO_T*)msg->para;
				WRTC_GET_FILE_INFO_RSP_T w_rsp;
				
				db_msg("fwk_msg MSG_ID_FWK_CONTROL_GET_FILE_INFO_REQ file_name=%s \n", w_req->file_name);
				memset(&w_rsp, 0, sizeof(w_rsp));
				strncpy(w_rsp.file_name, w_req->file_name, WRTC_MAX_FILE_NAME_LEN);
				
				fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_VIEW, MSG_ID_FWK_CONTROL_GET_FILE_INFO_RSP, &w_rsp, sizeof(w_rsp));
			}
			break;

		case MSG_ID_FWK_CONTROL_GET_GPS_INFO_REQ:
			{
				REMOTE_NETWORK_LOCATION_REQ_T* w_req = (REMOTE_NETWORK_LOCATION_REQ_T*)msg->para;
				REMOTE_NETWORK_LOCATION_RSP_T w_rsp;

				db_msg("fwk_msg MSG_ID_FWK_CONTROL_GET_GPS_INFO_REQ sid=%s \n", w_req->sid);
				memset(&w_rsp, 0, sizeof(w_rsp));
				strncpy(w_rsp.sid, w_req->sid, WRTC_MAX_NAME_LEN);
				w_rsp.type = w_req->type;
				w_rsp.timestamp = w_req->timestamp;
				
				fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_VIEW, MSG_ID_FWK_CONTROL_GET_GPS_INFO_RSP, &w_rsp, sizeof(w_rsp));
			}
			break;

		case MSG_ID_FWK_CONTROL_GET_FILE_LIST_D_REQ:
			{
				WRTC_GET_FILE_LIST_D_REQ* w_req = (WRTC_GET_FILE_LIST_D_REQ*)msg->para;
				WRTC_GET_FILE_LIST_D_RSP w_rsp;
				
				db_msg("fwk_msg MSG_ID_FWK_CONTROL_GET_FILE_LIST_D_REQ \n");
				memset(&w_rsp, 0, sizeof(w_rsp));
				w_rsp.reqType = w_req->reqType;
				
				fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_VIEW, MSG_ID_FWK_CONTROL_GET_FILE_LIST_D_RSP, &w_rsp, sizeof(w_rsp));
			}
			break;

		case MSG_ID_FWK_CONTROL_DEL_FILE_REQ:
			{
				WRTC_DEL_FILE_REQ* w_req = (WRTC_DEL_FILE_REQ*)msg->para;
				WRTC_DEL_FILE_RSP w_rsp;
				
				db_msg("fwk_msg MSG_ID_FWK_CONTROL_DEL_FILE_REQ path=%s\n", w_req->path);
				memset(&w_rsp, 0, sizeof(w_rsp));
				strncpy(w_rsp.path, w_req->path, WRTC_MAX_FILE_NAME_LEN);

				w_rsp.result = unlink(w_req->path);
				
				fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_VIEW, MSG_ID_FWK_CONTROL_DEL_FILE_RSP, &w_rsp, sizeof(w_rsp));
			}
			break;

		case MSG_ID_FWK_CONTROL_DEVICE_INFO_REQ:
			{
				WRTC_GET_DEVICE_INFO_RSP w_rsp;
				char tmp[PROPERTY_VALUE_MAX] = {'\0'};
				
				
				
				db_msg("fwk_msg MSG_ID_FWK_CONTROL_DEVICE_INFO_REQ \n");
				memset(&w_rsp, 0, sizeof(w_rsp));
				
				strncpy(w_rsp.deviceid, getImei(), WRTC_MAX_NAME_LEN);
				
				memset(tmp, 0, sizeof(tmp));
				property_get("ro.product.version",tmp,"");
				strncpy(w_rsp.version, tmp, WRTC_MAX_FILE_NAME_LEN);

				memset(tmp, 0, sizeof(tmp));
				property_get("ro.product.cpu.abi",tmp,"");
				strncpy(w_rsp.cputype, tmp, WRTC_MAX_NAME_LEN);

				strncpy(w_rsp.cpu, WRTC_DEVICE_INFO_CHIP_NAME, WRTC_MAX_NAME_LEN);

				memset(tmp, 0, sizeof(tmp));
				property_get("ro.product.manufacturer",tmp,"");
				strncpy(w_rsp.manufacturer, tmp, WRTC_MAX_NAME_LEN);
				
				fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_VIEW, MSG_ID_FWK_CONTROL_DEVICE_INFO_RSP, &w_rsp, sizeof(w_rsp));
			}
			break;
		case MSG_ID_FWK_CONTROL_AP_INFO_REQ:
			{
				WRTC_GET_AP_INFO_RSP w_rsp;
				bool ret = false;
			
				
				db_msg("fwk_msg MSG_ID_FWK_CONTROL_AP_INFO_REQ \n");
				memset(&w_rsp, 0, sizeof(w_rsp));
				
				ret = getSoftapSSID(w_rsp.ssid);
				if(ret)
				{
					w_rsp.result = 0;
				}
				else
				{
					w_rsp.result = -1;
				}
				getSoftap_SecType_PW(w_rsp.pwd);
				
				fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_VIEW, MSG_ID_FWK_CONTROL_AP_INFO_RSP, &w_rsp, sizeof(w_rsp));
			}
			break;
		case MSG_ID_FWK_CONTROL_CAM_NUMBER_INFO_REQ:
			{
				WRTC_GET_CAM_NUMBER_RSP w_rsp;
				int num = 0;
			
				
				db_msg("fwk_msg MSG_ID_FWK_CONTROL_CAM_NUMBER_INFO_REQ \n");
				memset(&w_rsp, 0, sizeof(w_rsp));

				num= 0;
				pthread_mutex_lock(&s_view_mutex);
				if(pView_cdrMain->isFrontCamExist())
				{
					num++;
				}

				if(pView_cdrMain->isBackCamExist())
				{
					num++;
				}
				pthread_mutex_unlock(&s_view_mutex);

				w_rsp.number = num;
				w_rsp.result = 0;
				
				fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_VIEW, MSG_ID_FWK_CONTROL_CAM_NUMBER_INFO_RSP, &w_rsp, sizeof(w_rsp));
			}
			break;
		case MSG_ID_FWK_CONTROL_RTSP_URL_REQ:
			{
				WRTC_GET_RTSP_URL_RSP w_rsp;
			
				
				db_msg("fwk_msg MSG_ID_FWK_CONTROL_RTSP_URL_REQ \n");
				memset(&w_rsp, 0, sizeof(w_rsp));

		
				pthread_mutex_lock(&s_view_mutex);
				if(pView_cdrMain->isFrontCamExist())
				{
					sprintf(w_rsp.camfront, "rtsp://%s/camfront", SOFTAP_GATEWAY_ADDR);
				}

				if(pView_cdrMain->isBackCamExist())
				{
					sprintf(w_rsp.camback, "rtsp://%s/camback", SOFTAP_GATEWAY_ADDR);
				}
				pthread_mutex_unlock(&s_view_mutex);


				w_rsp.result = 0;
				
				fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_VIEW, MSG_ID_FWK_CONTROL_RTSP_URL_RSP, &w_rsp, sizeof(w_rsp));
			}
			break;

		case MSG_ID_FWK_CONTROL_3G_REMOTE_VIDEO_REQ:
			{
				REMOTE_NETWORK_VIDEO_REQ_T* w_req = (REMOTE_NETWORK_VIDEO_REQ_T*)msg->para;
				
				bool is_insert = false;
				REMOTE_NETWORK_UPDATE_STATUS_IND_T status_ind_req;

				db_msg("fwk_msg MSG_ID_FWK_CONTROL_3G_REMOTE_VIDEO_REQ sid=%s timestamp=%ld \n", w_req->sid, w_req->timestamp);
				#ifdef REMOTE_3G
				pthread_mutex_lock(&s_view_mutex);
				is_insert = pView_cdrMain->isCardInsert();
				pthread_mutex_unlock(&s_view_mutex);
				if(!is_insert)
				{
					db_msg("fwk_msg MSG_ID_FWK_CONTROL_3G_REMOTE_VIDEO_REQ no sdcard \n");
					if(REMOTE_NETWORK_TYPE_REALTIME == w_req->type)
					{
						#if 1
						notify_is_rejecting();
						#else
						memset(&status_ind_req, 0x0, sizeof(status_ind_req));
						status_ind_req.type = REMOTE_STATUS_TYPE_REJECTED;
						fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_Reserverd1, MSG_ID_FWK_CONTROL_3G_REMOTE_STATUS_IND, &status_ind_req, sizeof(status_ind_req));
						#endif
					}
				}
				else
				{
					pthread_mutex_lock(&s_view_mutex);
					pView_cdrMain->setRecordWebRTC(0);
					record_for_remote(pView_cdrMain->getHerbCamera(0), w_req->timestamp, w_req->type);
					pthread_mutex_unlock(&s_view_mutex);
				}
				db_msg("fwk_msg MSG_ID_FWK_CONTROL_3G_REMOTE_VIDEO_REQ is done \n");
				#endif
				
			}
			break;
			
		case MSG_ID_FWK_CONTROL_3G_REMOTE_IMAGE_REQ:
			{
				REMOTE_NETWORK_IMG_REQ_T* w_req = (REMOTE_NETWORK_IMG_REQ_T*)msg->para;
				bool is_insert = false;
				REMOTE_NETWORK_UPDATE_STATUS_IND_T status_ind_req;
				REMOTE_NETWORK_UPLOAD_REQ_T upload_req;

				db_msg("fwk_msg MSG_ID_FWK_CONTROL_3G_REMOTE_IMAGE_REQ sid=%s timestamp=%ld type=%d\n", w_req->sid, w_req->timestamp, w_req->type);
				#ifdef REMOTE_3G
				pthread_mutex_lock(&s_view_mutex);
				is_insert = pView_cdrMain->isCardInsert();
				pthread_mutex_unlock(&s_view_mutex);
				if(!is_insert)
				{
					db_msg("fwk_msg MSG_ID_FWK_CONTROL_3G_REMOTE_IMAGE_REQ no sdcard \n");
					if(REMOTE_NETWORK_TYPE_REALTIME == w_req->type)
					{
						#if 1
						notify_is_rejecting();
						#else
						memset(&status_ind_req, 0x0, sizeof(status_ind_req));
						status_ind_req.type = REMOTE_STATUS_TYPE_REJECTED;
						fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_Reserverd1, MSG_ID_FWK_CONTROL_3G_REMOTE_STATUS_IND, &status_ind_req, sizeof(status_ind_req));
						#endif
					}
				}
				else
				{
					char* p = NULL;

					#if 1
					if(REMOTE_NETWORK_TYPE_REALTIME == w_req->type)
					{
						notify_is_recording();
					}
					#else
					memset(&status_ind_req, 0x0, sizeof(status_ind_req));
					status_ind_req.type = REMOTE_STATUS_TYPE_RECORDING;
					fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_Reserverd1, MSG_ID_FWK_CONTROL_3G_REMOTE_STATUS_IND, &status_ind_req, sizeof(status_ind_req));
					#endif
					
					pthread_mutex_lock(&s_view_mutex);
					pView_cdrMain->clearSnapshotPath();
					pView_cdrMain->take_picture();
					pthread_mutex_unlock(&s_view_mutex);

				check_take_picture_done:
					pthread_mutex_lock(&s_view_mutex);
					p = pView_cdrMain->getSnapshotPath();
					pthread_mutex_unlock(&s_view_mutex);
					while((p == NULL)  || (strlen(p) == 0))
					{
						//sleep(1);
						goto check_take_picture_done;
					}

					memset(&upload_req, 0x0, sizeof(upload_req));
					strncpy(upload_req.file_name, p, WRTC_MAX_FILE_NAME_LEN);
					upload_req.timestamp = w_req->timestamp;
					upload_req.type = (REMOTE_NETWORK_TYPE)w_req->type;
					upload_req.data_type = REMOTE_NETWORK_DATA_TYPE_IMG;

					db_msg("fwk_msg MSG_ID_FWK_CONTROL_3G_REMOTE_IMAGE_REQ snapshot done. path=%s upload_req.file_name=%s\n", p, upload_req.file_name);
					fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_Reserverd1, MSG_ID_FWK_CONTROL_3G_REMOTE_UPLOAD_REQ, &upload_req, sizeof(upload_req));
				
				}
				#endif

			}
			break;

		case MSG_ID_FWK_CONTROL_3G_REMOTE_ALARM_IND:
			{
				REMOTE_NETWORK_ALARM_T* w_req = (REMOTE_NETWORK_ALARM_T*)msg->para;
				REMOTE_NETWORK_LOCATION_REQ_T location_req;
				REMOTE_NETWORK_VIDEO_REQ_T video_req;
				REMOTE_NETWORK_IMG_REQ_T img_req;
				long sTimeStamp = 0;

				sTimeStamp = time(NULL);
				db_msg("fwk_msg MSG_ID_FWK_CONTROL_3G_REMOTE_ALARM_IND type=%d sTimeStamp=%ld\n", w_req->type, sTimeStamp);

			#ifdef REMOTE_3G
				
				if(strlen(remote_network_get_token()) > 0)
				{
					db_msg("fwk_msg MSG_ID_FWK_CONTROL_3G_REMOTE_ALARM_IND token=%s \n", remote_network_get_token());
					memset(&location_req, 0x0, sizeof(location_req));
					location_req.type = REMOTE_NETWORK_TYPE_ALARM;
					location_req.timestamp = sTimeStamp;
					fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_GET_GPS_INFO_REQ, &location_req, sizeof(location_req));

					memset(&img_req, 0x0, sizeof(img_req));
					img_req.type = REMOTE_NETWORK_TYPE_ALARM;
					img_req.timestamp = sTimeStamp;
					img_req.img_foramt = 0;
					fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_3G_REMOTE_IMAGE_REQ, &img_req, sizeof(img_req));

					
					memset(&video_req, 0x0, sizeof(video_req));
					video_req.type = REMOTE_NETWORK_TYPE_ALARM;
					video_req.timestamp = sTimeStamp;
					video_req.duration = 15;
					video_req.video_foramt = 0;
					fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_3G_REMOTE_VIDEO_REQ, &video_req, sizeof(video_req));
				}
			#endif
				
			}
			break;

    	default :
        {
			db_msg("fwk_msg Unknow message to Controller \n");
    	}
        break;
		
    
    }


	return;
}


void* controller_handler(void* p_arg)
{
    FWK_MSG *msg;
	int ret;


    while (1)
    {
        msg = fwk_get_message(fwk_controller_queue, MSG_GET_WAIT_ROREVER, &ret);
        if((ret <0)||(msg == NULL))
        {
            //fwk_debug("get message NULL\n");
			db_msg("fwk_msg get message NULL\n");
            continue;
        }
        else
        {
            deal_controller_msg(msg);
            fwk_free_message(&msg);
        }
    }

	return NULL;
}


int fwk_controller_init(void)
{
	pthread_t comtid;
    int err;
	
    fwk_controller_queue=fwk_create_queue(FWK_MOD_CONTROL);
    if (fwk_controller_queue == -1)
    {
        db_error("fwk_msg unable to create queue for controller \n");
        return -1;
    }

    

    err = pthread_create(&comtid,NULL,controller_handler,NULL);
    if(err != 0)
    {
        db_error("fwk_msg create thread for controller failed \n");
        return -1;
    }

    return 0;
}


