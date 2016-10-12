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
#include <sys/ipc.h>
#include <sys/msg.h>
	
#include <netinet/in.h>
#include <pthread.h>
#include <termio.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>



#include "fwk_gl_api.h"
#include "fwk_gl_def.h"
#include "fwk_gl_msg.h"

#include "windows.h"

#undef LOG_TAG
#define LOG_TAG "fwk_view"
#include "debug.h"

using namespace std;
#include <string>
#include <vector>
#include <json/json.h>

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
#define GPS_LOCATION_NO_SINGLE_STR			("0.00000000000000")



static int fwk_view_queue;


/*for MINIGUI*/
extern CdrMain *pView_cdrMain;

#ifdef REMOTE_3G
extern int remote_network_init(void);
extern char* remote_network_get_token(void);
extern int remote_network_notify(const char* pbuf, int len, REOMOTE_NETWORK_CMD_TYPE type,void *pSToken);
extern void release_remote_network_wakelock(void);
extern pthread_mutex_t g_curl_mutex;
#endif

pthread_mutex_t s_view_mutex = PTHREAD_MUTEX_INITIALIZER;


static void  getFiles(std::string path, std::vector<std::string>& files)
{
	DIR *dir;
	struct dirent *ptr;
	char base[1000];

	if ((dir = opendir(path.c_str())) == NULL)
	{
		db_msg("getFiles: Open dir error... \n");
		return;
	}
	
	while ((ptr = readdir(dir)) != NULL)
	{
		
		if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0)    ///current dir OR parrent dir
			continue;
		else if (ptr->d_type == 8) {    ///file
			std::string file=path+"/"+ ptr->d_name;
			files.push_back(file);
		}
		else if (ptr->d_type == 10) {    ///link file
			db_msg("getFiles: link d_name \n");
		}
		else if (ptr->d_type == 4)    ///dir
		{
			memset(base, '\0', sizeof(base));
			strcpy(base, path.c_str());
			strcat(base, "/");
			strcat(base, ptr->d_name);
			getFiles(base,files);
		}
	}
	closedir(dir);
	
}

namespace {
template<typename T>
Json::Value VectorToJsonArray(const std::vector<T>& vec) {
  Json::Value result(Json::arrayValue);
  for (size_t i = 0; i < vec.size(); ++i) {
    result.append(Json::Value(vec[i]));
  }
  return result;
}

template<typename T>
Json::Value VectorToJsonArray2(const std::vector<T>& vec, const std::vector<T>& vec2) {
  Json::Value result(Json::arrayValue);
  for (size_t i = 0; i < vec.size(); ++i) {
    result.append(Json::Value(vec[i]));
  }
  for (size_t i = 0; i < vec2.size(); ++i) {
    result.append(Json::Value(vec2[i]));
  }
  return result;
}

template<typename T>
Json::Value VectorToJsonArray3(const std::vector<T>& vec, const std::vector<T>& vec2, const std::vector<T>& vec3) {
  Json::Value result(Json::arrayValue);
  for (size_t i = 0; i < vec.size(); ++i) {
    result.append(Json::Value(vec[i]));
  }
  for (size_t i = 0; i < vec2.size(); ++i) {
    result.append(Json::Value(vec2[i]));
  }
  for (size_t i = 0; i < vec3.size(); ++i) {
    result.append(Json::Value(vec3[i]));
  }
  return result;
}


} 

Json::Value StringVectorToJsonArray(const std::vector<std::string>& in) {
  return VectorToJsonArray(in);
}

Json::Value StringVectorToJsonArray2(const std::vector<std::string>& in, const std::vector<std::string>& in2) {
  return VectorToJsonArray2(in, in2);
}

Json::Value StringVectorToJsonArray3(const std::vector<std::string>& in, const std::vector<std::string>& in2, const std::vector<std::string>& in3) {
  return VectorToJsonArray3(in, in2, in3);
}



void deal_view_msg(FWK_MSG* msg)
{

    
    switch(msg->msg_id)
    {
    	case MSG_ID_FWK_CONTROL_COMMU_SWITCH_RSP:
		{
			CommuRsp*  communication_rsp = (CommuRsp*)msg->para;
			Json::Value root;
			Json::FastWriter writer;
			Json::Value wrtc_rsp;
			bool report2wrtc = false;
			string wrtc_path;
			REOMOTE_NETWORK_CMD_TYPE type = REOMOTE_NETWORK_CMD_INVALID;

			report2wrtc = false;
			db_msg("fwk_msg MSG_ID_FWK_CONTROL_COMMU_SWITCH_RSP %s line=%d cmdType=%d operType=%d result=%d\n", __FUNCTION__, __LINE__,communication_rsp->cmdType, communication_rsp->operType, communication_rsp->result);

#ifndef USE_NEWUI

			if(pView_cdrMain != NULL)
			{
				if(COMMU_TYPE_GPS == communication_rsp->cmdType)
				{
				}
				else if(COMMU_TYPE_WIFI == communication_rsp->cmdType)
				{
					db_msg("fwk_msg Old UI wifi settings \n");
					if(0 == communication_rsp->result)
					{
						pthread_mutex_lock(&s_view_mutex);
						if(communication_rsp->operType)
						{
							pView_cdrMain->notify(EVENT_WIFI, 1);
						}
						else
						{
							pView_cdrMain->notify(EVENT_WIFI, 0);
						}
						pthread_mutex_unlock(&s_view_mutex);
					}
				}
				else if(COMMU_TYPE_CELLULAR== communication_rsp->cmdType)
				{
				}
			}
#endif

			if(COMMU_TYPE_GPS == communication_rsp->cmdType)
			{
				wrtc_rsp["name"] = WRTC_CMD_SET_GPS;
				report2wrtc = true;
				type = REOMOTE_NETWORK_CMD_GPS;
			}
			else if(COMMU_TYPE_CELLULAR== communication_rsp->cmdType)
			{
				wrtc_rsp["name"] = WRTC_CMD_SET_MOBILE;
				report2wrtc = true;
				type = REOMOTE_NETWORK_CMD_CELLULAR;
			}
			else if(COMMU_TYPE_WATERMARK== communication_rsp->cmdType)
			{
				wrtc_rsp["name"] = WRTC_CMD_SET_WATERMARK;
				report2wrtc = true;
				type = REOMOTE_NETWORK_CMD_WATERMARK;
			}
			else if(COMMU_TYPE_MOVING == communication_rsp->cmdType)
			{
				wrtc_rsp["name"] = WRTC_CMD_SET_MOVING;
				report2wrtc = true;
				type = REOMOTE_NETWORK_CMD_MOVING;
				
			}
			else if(COMMU_TYPE_FORMAT == communication_rsp->cmdType)
			{
				wrtc_rsp["name"] = WRTC_CMD_SET_FORMAT;
				report2wrtc = true;
				type = REOMOTE_NETWORK_CMD_FORMAT;
			}
			else if(COMMU_TYPE_FACTORY_RESET == communication_rsp->cmdType)
			{
				wrtc_rsp["name"] = WRTC_CMD_SET_RESET_FACTORY;
				report2wrtc = true;
				type = REOMOTE_NETWORK_CMD_FACTORY_RESET;
			}
			else if(COMMU_TYPE_VOICE== communication_rsp->cmdType)
			{
				wrtc_rsp["name"] = WRTC_CMD_SET_SOUND;
				report2wrtc = true;
				type = REOMOTE_NETWORK_CMD_VOICE;
				
			}
			else if(COMMU_TYPE_RECORD== communication_rsp->cmdType)
			{
				wrtc_rsp["name"] = WRTC_CMD_SET_RECORD;
				report2wrtc = true;
				type = REOMOTE_NETWORK_CMD_RECORD;
				
			}
			
			if(report2wrtc)
			{
				if(communication_rsp->operType)
				{
					wrtc_rsp["value"]=true;
				}
				else
				{
					wrtc_rsp["value"]=false;
				}
				
				if(0 == communication_rsp->result)
				{
					wrtc_rsp["result"]=true;
				}
				else
				{
					wrtc_rsp["result"]=false;
				}

				if((COMMU_TYPE_FORMAT == communication_rsp->cmdType) || (COMMU_TYPE_FACTORY_RESET == communication_rsp->cmdType))
				{
					wrtc_rsp["type"]= "Action";
				}
				else
				{
					wrtc_rsp["type"]= WRTC_CMD_SETTINGS;
				}
				
				root.append(wrtc_rsp);
				wrtc_path = writer.write(root);

				db_msg("fwk_msg MSG_ID_FWK_CONTROL_COMMU_SWITCH_RSP length=%d ",wrtc_path.length());
				pthread_mutex_lock(&s_view_mutex);
				pView_cdrMain->notifyWebRtc((unsigned char*)wrtc_path.c_str(),wrtc_path.length());
				pthread_mutex_unlock(&s_view_mutex);
			#ifdef REMOTE_3G
				{
					REMOTE_NETWORK_NOTIFY_T notify;
					
					memset(&notify, 0, sizeof(notify));
					strncpy(notify.buffer, wrtc_path.c_str(), REMOTE_NETWORK_BUFFER_LEN);
					notify.type = type;
					fwk_send_message_ext(FWK_MOD_VIEW,FWK_MOD_Reserverd1,MSG_ID_FWK_CONTROL_3G_REMOTE_NOTIFY_IND,&notify,sizeof(notify));
					
				}
			#endif
			}
			
    	}
		break;

		case MSG_ID_FWK_CONTROL_TAKE_PICTURE_RSP:
			{
				WRTC_TAKE_PICTURE_NOTIFY*  take_picture_rsp = (WRTC_TAKE_PICTURE_NOTIFY*)msg->para;
				Json::Value root;
				Json::FastWriter writer;
				Json::Value picture_rsp;

				db_msg("fwk_msg MSG_ID_FWK_CONTROL_TAKE_PICTURE_RSP buf=%s size=%d \n", (char*)take_picture_rsp->path, take_picture_rsp->size);

				picture_rsp["name"]=WRTC_TAKE_PICTURE_RSP;
				picture_rsp["value"]= (char*)take_picture_rsp->path;
				if(0 == take_picture_rsp->result)
				{
					picture_rsp["result"]=true;
				}
				else
				{
					picture_rsp["result"]=false;
				}
				root.append(picture_rsp);
				string path = writer.write(root);
				
				//pView_cdrMain->notifyWebRtc(take_picture_rsp->path,take_picture_rsp->size);
				db_msg("fwk_msg MSG_ID_FWK_CONTROL_TAKE_PICTURE_RSP path=%s length=%d ", path.c_str(), path.length());
				pthread_mutex_lock(&s_view_mutex);
				pView_cdrMain->notifyWebRtc((unsigned char*)path.c_str(),path.length());
				pthread_mutex_unlock(&s_view_mutex);
				#ifdef REMOTE_3G
				{
					REMOTE_NETWORK_NOTIFY_T notify;
					
					memset(&notify, 0, sizeof(notify));
					strncpy(notify.buffer, path.c_str(), REMOTE_NETWORK_BUFFER_LEN);
					notify.type = REOMOTE_NETWORK_CMD_TAKE_PICTURE;
					fwk_send_message_ext(FWK_MOD_VIEW,FWK_MOD_Reserverd1,MSG_ID_FWK_CONTROL_3G_REMOTE_NOTIFY_IND,&notify,sizeof(notify));
					
				}
				#endif
			}
			break;

		case MSG_ID_FWK_CONTROL_SET_DATE_TIME_RSP:
			{
				WRTC_DT_RSP*  dt_rsp = (WRTC_DT_RSP*)msg->para;
				Json::Value root;
				Json::FastWriter writer;
				Json::Value json_rsp;
				char pdate_buf[WRTC_MAX_NAME_LEN+1];

				db_msg("fwk_msg MSG_ID_FWK_CONTROL_SET_DATE_TIME_RSP year=%d hour=%d \n", dt_rsp->dt.year, dt_rsp->dt.hour);

				json_rsp["name"]= WRTC_CMD_SET_DATE_TIME;
				json_rsp["year"]= dt_rsp->dt.year;
				json_rsp["month"]= dt_rsp->dt.month;
				json_rsp["day"]= dt_rsp->dt.day;
				json_rsp["hour"]= dt_rsp->dt.hour;
				json_rsp["minute"]= dt_rsp->dt.minute;
				json_rsp["second"]= dt_rsp->dt.second;
				
				memset(pdate_buf, 0, sizeof(pdate_buf));
				sprintf(pdate_buf, "%4d%2d%2d%2d%2d", dt_rsp->dt.year, dt_rsp->dt.month, dt_rsp->dt.day, dt_rsp->dt.hour, dt_rsp->dt.minute);
				//json_rsp["date"]= pdate_buf;
				if(dt_rsp->result== 0)
				{
					json_rsp["result"]=true;
				}
				else
				{
					json_rsp["result"]=false;
				}
				root.append(json_rsp);
				string path = writer.write(root);
				
				
				db_msg("fwk_msg MSG_ID_FWK_CONTROL_SET_DATE_TIME_RSP length=%d ",  path.length());
				pthread_mutex_lock(&s_view_mutex);
				pView_cdrMain->notifyWebRtc((unsigned char*)path.c_str(),path.length());
				pthread_mutex_unlock(&s_view_mutex);
				#ifdef REMOTE_3G
				{
					REMOTE_NETWORK_NOTIFY_T notify;
					
					memset(&notify, 0, sizeof(notify));
					strncpy(notify.buffer, path.c_str(), REMOTE_NETWORK_BUFFER_LEN);
					notify.type = REOMOTE_NETWORK_CMD_DATE_TIME;
					fwk_send_message_ext(FWK_MOD_VIEW,FWK_MOD_Reserverd1,MSG_ID_FWK_CONTROL_3G_REMOTE_NOTIFY_IND,&notify,sizeof(notify));
					
				}
				
				#endif
			}
			break;

		case MSG_ID_FWK_CONTROL_SET_WIFIPASSWD_RSP:
			{
				WRTC_WIFI_PASSWD_RSP*  w_rsp = (WRTC_WIFI_PASSWD_RSP*)msg->para;
				Json::Value root;
				Json::FastWriter writer;
				Json::Value json_rsp;

				db_msg("fwk_msg MSG_ID_FWK_CONTROL_SET_WIFIPASSWD_RSP resultr=%d \n", w_rsp->result);

				json_rsp["name"]= WRTC_CMD_SET_WIFI_PASSWD;
				if(w_rsp->result == 0)
				{
					json_rsp["result"]=true;
				}
				else
				{
					json_rsp["result"]=false;
				}
				root.append(json_rsp);
				string path = writer.write(root);
				
				
				db_msg("fwk_msg MSG_ID_FWK_CONTROL_SET_WIFIPASSWD_RSP length=%d ",  path.length());
				pthread_mutex_lock(&s_view_mutex);
				pView_cdrMain->notifyWebRtc((unsigned char*)path.c_str(),path.length());
				pthread_mutex_unlock(&s_view_mutex);
				#ifdef REMOTE_3G
				{
					REMOTE_NETWORK_NOTIFY_T notify;
					
					memset(&notify, 0, sizeof(notify));
					strncpy(notify.buffer, path.c_str(), REMOTE_NETWORK_BUFFER_LEN);
					notify.type = REOMOTE_NETWORK_CMD_HOTSPOT_PASSWD;
					fwk_send_message_ext(FWK_MOD_VIEW,FWK_MOD_Reserverd1,MSG_ID_FWK_CONTROL_3G_REMOTE_NOTIFY_IND,&notify,sizeof(notify));
					
				}
				#endif
			}
			break;

		case MSG_ID_FWK_CONTROL_GET_PARAS_RSP:
			{
				WRTC_SETTINGS*  get_params_rsp = (WRTC_SETTINGS*)msg->para;
				Json::Value root;
				Json::Value arrayObj;
				Json::Value item;
				Json::FastWriter writer;

				db_msg("MSG_ID_FWK_CONTROL_GET_PARAS_RSP");

				root["name"] = get_params_rsp->szName;
				root["count"] = get_params_rsp->nCount;

				for(int i=0; i < get_params_rsp->nCount; i++)
				{
					item["item_name"] = get_params_rsp->items[i].itemName;
					if(1 == get_params_rsp->items[i].valueType)
					{
						item["item_value"] = get_params_rsp->items[i].valueStr;
					}
					else if(2 == get_params_rsp->items[i].valueType)
					{
						item["item_value"] = get_params_rsp->items[i].bValue;
					}
					else if(3 == get_params_rsp->items[i].valueType)
					{
						item["year"] = get_params_rsp->items[i].dt.year;
						item["month"] = get_params_rsp->items[i].dt.month;
						item["day"] = get_params_rsp->items[i].dt.day;
						item["hour"] = get_params_rsp->items[i].dt.hour;
						item["minute"] = get_params_rsp->items[i].dt.minute;
						item["second"] = get_params_rsp->items[i].dt.second;
					}
					else
					{
						item["item_value"] = get_params_rsp->items[i].value;
					}

					arrayObj.append(item);
				}

				root["item_list"] = arrayObj;
				root["result"]=true;

				string strOut = writer.write(root);

				db_msg("fwk_msg MSG_ID_FWK_CONTROL_GET_PARAS_RSP length=%d ", strOut.length());
				pthread_mutex_lock(&s_view_mutex);
				pView_cdrMain->notifyWebRtc((unsigned char*)strOut.c_str(),strOut.length());
				pthread_mutex_unlock(&s_view_mutex);
				#ifdef REMOTE_3G
				{
					REMOTE_NETWORK_NOTIFY_T notify;
					
					memset(&notify, 0, sizeof(notify));
					strncpy(notify.buffer, strOut.c_str(), REMOTE_NETWORK_BUFFER_LEN);
					notify.type = REOMOTE_NETWORK_CMD_GET_PARAMS;
					fwk_send_message_ext(FWK_MOD_VIEW,FWK_MOD_Reserverd1,MSG_ID_FWK_CONTROL_3G_REMOTE_NOTIFY_IND,&notify,sizeof(notify));
					
				}
				#endif
			}
			break;

		case MSG_ID_FWK_CONTROL_SET_VALUE_RSP:
			{
				ValueRsp*  value_rsp = (ValueRsp*)msg->para;
				Json::Value root;
				Json::FastWriter writer;
				Json::Value wrtc_rsp;
				bool report2wrtc = false;
				string wrtc_path;
				REOMOTE_NETWORK_CMD_TYPE type = REOMOTE_NETWORK_CMD_INVALID;

				db_msg("MSG_ID_FWK_CONTROL_SET_VALUE_RSP");

				report2wrtc = false;
				if(value_rsp->cmdType == VALUE_TYPE_RESOLUTION)
				{
					wrtc_rsp["name"] = WRTC_CMD_SET_RESOLUTION;
					report2wrtc = true;
					type = REOMOTE_NETWORK_CMD_RESOLUTION;
					
					if(value_rsp->value == 1080)
					{
						wrtc_rsp["value"] = WRTC_VALUE_RESOLUTION_1080P;
					}
					else if(value_rsp->value == 720)
					{
						wrtc_rsp["value"] = WRTC_VALUE_RESOLUTION_720P;
					}
					else
					{
						report2wrtc = false;
					}
				}
				else if(value_rsp->cmdType == VALUE_TYPE_RECORD_TIME)
				{
					wrtc_rsp["name"] = WRTC_CMD_SET_RECORD_TIME;
					report2wrtc = true;
					wrtc_rsp["value"] = value_rsp->value;
					type = REOMOTE_NETWORK_CMD_RECORD_TIME;
					
				}
				else if(value_rsp->cmdType == VALUE_TYPE_EXPOSURE)
				{
					wrtc_rsp["name"] = WRTC_CMD_SET_EXPOSURE;
					report2wrtc = true;
					wrtc_rsp["value"] = value_rsp->value;
					type = REOMOTE_NETWORK_CMDEXPOSURE;
					
				}
				else if(value_rsp->cmdType == VALUE_TYPE_WB)
				{
					wrtc_rsp["name"] = WRTC_CMD_SET_WB;
					report2wrtc = true;
					type = REOMOTE_NETWORK_CMD_WB;
					
					if(value_rsp->value == 0)
					{
						wrtc_rsp["value"] = WRTC_VALUE_WB_AUTO;
					}
					else if(value_rsp->value == 1)
					{
						wrtc_rsp["value"] = WRTC_VALUE_WB_DAYLIGHT;
					}
					else if(value_rsp->value == 2)
					{
						wrtc_rsp["value"] = WRTC_VALUE_WB_CLOUDY;
					}
					else if(value_rsp->value == 3)
					{
						wrtc_rsp["value"] = WRTC_VALUE_WB_INCANDESCENT;
					}
					else if(value_rsp->value == 4)
					{
						wrtc_rsp["value"] = WRTC_VALUE_WB_FLUORESCENT;
					}
					else
					{
						report2wrtc = false;
					}
					
					
				}
				else if(value_rsp->cmdType == VALUE_TYPE_CONTRAST)
				{
					wrtc_rsp["name"] = WRTC_CMD_SET_CONTRAST;
					report2wrtc = true;
					wrtc_rsp["value"] = value_rsp->value;
					type = REOMOTE_NETWORK_CMD_CONTRAST;
					
				}

				if(report2wrtc)
				{
					
					if(0 == value_rsp->result)
					{
						wrtc_rsp["result"]=true;
					}
					else
					{
						wrtc_rsp["result"]=false;
					}
					wrtc_rsp["type"]= WRTC_CMD_SETTINGS;
					
					root.append(wrtc_rsp);
					wrtc_path = writer.write(root);

					db_msg("fwk_msg MSG_ID_FWK_CONTROL_SET_VALUE_RSP wrtc_path_length=%d ",wrtc_path.length());
					pthread_mutex_lock(&s_view_mutex);
					pView_cdrMain->notifyWebRtc((unsigned char*)wrtc_path.c_str(),wrtc_path.length());
					pthread_mutex_unlock(&s_view_mutex);
					#ifdef REMOTE_3G
					{
						REMOTE_NETWORK_NOTIFY_T notify;
					
						memset(&notify, 0, sizeof(notify));
						strncpy(notify.buffer, wrtc_path.c_str(), REMOTE_NETWORK_BUFFER_LEN);
						notify.type = type;
						fwk_send_message_ext(FWK_MOD_VIEW,FWK_MOD_Reserverd1,MSG_ID_FWK_CONTROL_3G_REMOTE_NOTIFY_IND,&notify,sizeof(notify));
					
					}
					#endif
				}
					
			}
			break;

		case MSG_ID_FWK_CONTROL_GET_FILES_RSP:
			{
				WRTC_GET_FILES_RSP*  files_rsp = (WRTC_GET_FILES_RSP*)msg->para;
				std::vector<std::string> files;
				std::vector<std::string> v_files_r;
				std::vector<std::string> v_files_f_rlt;
				REOMOTE_NETWORK_CMD_TYPE type = REOMOTE_NETWORK_CMD_INVALID;


				db_msg("fwk_msg MSG_ID_FWK_CONTROL_GET_FILES_RSP reqType=%d  result=%d \n", files_rsp->reqType, files_rsp->result);

				if(WRTC_GET_FILES_PICTURE == files_rsp->reqType)
				{
					getFiles(WRTC_SDCARD_PICTURE_PATH, files);
					type = REOMOTE_NETWORK_CMD_GET_PICTURE_FILES_LIST;
				}
				else if(WRTC_GET_FILES_VIDEO == files_rsp->reqType)
				{
					getFiles(WRTC_SDCARD_VIDEO_PATH, files);
					getFiles(WRTC_SDCARD_VIDEO_R_PATH, v_files_r);
					getFiles(WRTC_SDCARD_VIDEO_F_ALT_PATH, v_files_f_rlt);
					type = REOMOTE_NETWORK_CMD_GET_VIDEO_FILES_LIST;
				}
				
				int size = files.size();
				Json::FastWriter writer;		
				Json::Value out = StringVectorToJsonArray3(files, v_files_r, v_files_f_rlt);
				Json::Value message;

				if(WRTC_GET_FILES_PICTURE == files_rsp->reqType)
				{
					message["name"] = WRTC_CMD_GET_PICTURE_FILES;
				}
				else if(WRTC_GET_FILES_VIDEO == files_rsp->reqType)
				{
					message["name"] = WRTC_CMD_GET_VIDEO_FILES;
				}
				else
				{
					message["name"] = "Unknow";
				}
				
				message["type"] = "data";
				message["result"] = true;
				message["files"] = out;
				string path = writer.write(message);

				pthread_mutex_lock(&s_view_mutex);
				pView_cdrMain->notifyWebRtc((unsigned char*)path.c_str(),path.length());
				pthread_mutex_unlock(&s_view_mutex);
				#ifdef REMOTE_3G
				{
					REMOTE_NETWORK_NOTIFY_T notify;
					
					memset(&notify, 0, sizeof(notify));
					strncpy(notify.buffer, path.c_str(), REMOTE_NETWORK_BUFFER_LEN);
					notify.type = type;
					fwk_send_message_ext(FWK_MOD_VIEW,FWK_MOD_Reserverd1,MSG_ID_FWK_CONTROL_3G_REMOTE_NOTIFY_IND,&notify,sizeof(notify));
					
				}
				#endif
			}
			break;

		case MSG_ID_FWK_CONTROL_GET_FILE_INFO_RSP:
			{
				WRTC_GET_FILE_INFO_RSP_T*  info_rsp = (WRTC_GET_FILE_INFO_RSP_T*)msg->para;
				Json::Value root;
				Json::FastWriter writer;
				Json::Value json_rsp;
				struct stat info;

				db_msg("fwk_msg MSG_ID_FWK_CONTROL_GET_FILE_INFO_RSP file_name=%s \n", info_rsp->file_name);

				memset(&info, 0x0, sizeof(info));
				info_rsp->result = stat(info_rsp->file_name, &info);
				
				json_rsp["name"]= WRTC_CMD_GET_FILE_INFO;
				json_rsp["value"]= info_rsp->file_name;
				if(0 == info_rsp->result)
				{
					json_rsp["result"]=true;
				}
				else
				{
					json_rsp["result"]=false;
				}
				
				json_rsp["size"]= info.st_size;
				json_rsp["atime"]= (unsigned long long int)info.st_atime;
				json_rsp["mtime"]= (unsigned long long int)info.st_mtime;

				root.append(json_rsp);
				string path = writer.write(root);
				
				
				db_msg("fwk_msg MSG_ID_FWK_CONTROL_GET_FILE_INFO_RSP length=%d ",  path.length());
				pthread_mutex_lock(&s_view_mutex);
				pView_cdrMain->notifyWebRtc((unsigned char*)path.c_str(),path.length());
				pthread_mutex_unlock(&s_view_mutex);
				#ifdef REMOTE_3G
				{
					REMOTE_NETWORK_NOTIFY_T notify;
					
					memset(&notify, 0, sizeof(notify));
					strncpy(notify.buffer, path.c_str(), REMOTE_NETWORK_BUFFER_LEN);
					notify.type = REOMOTE_NETWORK_CMD_GET_FILE_INFO;
					fwk_send_message_ext(FWK_MOD_VIEW,FWK_MOD_Reserverd1,MSG_ID_FWK_CONTROL_3G_REMOTE_NOTIFY_IND,&notify,sizeof(notify));
					
				}
				#endif
			}
			break;

		case MSG_ID_FWK_CONTROL_GET_GPS_INFO_RSP:
			{
				REMOTE_NETWORK_LOCATION_RSP_T*  info_rsp = (REMOTE_NETWORK_LOCATION_RSP_T*)msg->para;
				Json::Value root;
				Json::FastWriter writer;
				Json::Value json_rsp;
				char LongitudeStr[WRTC_MAX_VALUE_LEN+1]={0};
				char LatitudeStr[WRTC_MAX_VALUE_LEN+1]={0};
				double latitude = getLatitude();
				double longtitude = getLongitude();
				char itimeBuf[WRTC_MAX_VALUE_LEN*2+1];
				
				db_msg("fwk_msg MSG_ID_FWK_CONTROL_GET_GPS_INFO_RSP sid=%s type=%d timestamp=%ld\n", info_rsp->sid, info_rsp->type, info_rsp->timestamp);

				if(REMOTE_NETWORK_TYPE_ALARM == info_rsp->type)
				{
					latitude = getValidLatitude();
					longtitude = getValidLongtitude();
				}
				else
				{
					latitude = getLatitude();
					longtitude = getLongitude();
				}
				
				memset(LongitudeStr, 0, sizeof(LongitudeStr));
				snprintf(LongitudeStr, WRTC_MAX_VALUE_LEN, "%.14lf", longtitude);

				memset(LatitudeStr, 0, sizeof(LatitudeStr));
				snprintf(LatitudeStr, WRTC_MAX_VALUE_LEN, "%.14lf", latitude);

				db_msg("fwk_msg MSG_ID_FWK_CONTROL_GET_GPS_INFO_RSP latitude=%s longitude=%s\n", LatitudeStr, LongitudeStr);

				#ifdef REMOTE_3G
				json_rsp["token"]= remote_network_get_token();
				#endif
				if(strcasecmp(LongitudeStr, GPS_LOCATION_NO_SINGLE_STR) == 0)
				{
					json_rsp["longitude"]= "119.275335";
				}
				else
				{
					json_rsp["longitude"]= LongitudeStr;
				}
				
				if(strcasecmp(LatitudeStr, GPS_LOCATION_NO_SINGLE_STR) == 0)
				{
					json_rsp["latitude"]= "26.115858";
				}
				else
				{
					json_rsp["latitude"]= LatitudeStr;
				}

				
				memset(itimeBuf, 0, sizeof(itimeBuf));
				snprintf(itimeBuf, sizeof(itimeBuf), "%ld000", info_rsp->timestamp);
				
				if(REMOTE_NETWORK_TYPE_REALTIME == info_rsp->type)
				{
					json_rsp["type"] = REMOTE_NETWORK_GPS_TYPE_NORMAL;
					json_rsp["timestamp"] = itimeBuf;
				}
				else if(REMOTE_NETWORK_TYPE_ALARM == info_rsp->type)
				{
					json_rsp["type"] = REMOTE_NETWORK_GPS_TYPE_ALARM;
					json_rsp["timestamp"] = itimeBuf;
				}
				else
				{
					json_rsp["type"] = "unknown";
					json_rsp["timestamp"] = itimeBuf;
				}

				//root.append(json_rsp);
				string path = writer.write(/*root*/json_rsp);
				
				
				db_msg("fwk_msg MSG_ID_FWK_CONTROL_GET_GPS_INFO_RSP path=%s length=%d ",  path.c_str(), path.length());
				//pView_cdrMain->notifyWebRtc((unsigned char*)path.c_str(),path.length());
				#ifdef REMOTE_3G
				{
					#if 1
					pthread_mutex_lock(&g_curl_mutex);
					remote_network_notify(path.c_str(),path.length(), REOMOTE_NETWORK_CMD_LOCATION, NULL);
					pthread_mutex_unlock(&g_curl_mutex);
					release_remote_network_wakelock();
					#else
					REMOTE_NETWORK_NOTIFY_T notify;
					
					memset(&notify, 0, sizeof(notify));
					strncpy(notify.buffer, path.c_str(), REMOTE_NETWORK_BUFFER_LEN);
					notify.type = REOMOTE_NETWORK_CMD_LOCATION;
					fwk_send_message_ext(FWK_MOD_VIEW,FWK_MOD_Reserverd1,MSG_ID_FWK_CONTROL_3G_REMOTE_NOTIFY_IND,&notify,sizeof(notify));
					#endif
					
				}
				
				#endif
			}
			break;

		case MSG_ID_FWK_CONTROL_GET_FILE_LIST_D_RSP:
			{
				WRTC_GET_FILE_LIST_D_RSP*  files_rsp = (WRTC_GET_FILE_LIST_D_RSP*)msg->para;
				Json::Value root;
				Json::Value arrayObj;
				Json::Value item;
				Json::FastWriter writer;
				bool report2wrtc = false;
				std::vector<std::string> files;
				std::vector<std::string> v_files_r;
				std::vector<std::string> v_files_f_rlt;
				REOMOTE_NETWORK_CMD_TYPE r_type = REOMOTE_NETWORK_CMD_INVALID;
				

				db_msg("MSG_ID_FWK_CONTROL_GET_FILE_LIST_D_RSP type=%d \n", files_rsp->reqType);
				

				root["name"] = WRTC_CMD_GET_FILE_LIST_D;
				if(files_rsp->reqType == WRTC_GET_FILES_D_VIDEO)
				{
					root["type"] = WRTC_CMD_GET_FILE_LIST_TYPE_V;
					r_type = REOMOTE_NETWORK_CMD_GET_FILE_LIST_D_VIDEO;
					report2wrtc = true;
				}
				else if(files_rsp->reqType == WRTC_GET_FILES_D_PICTURE)
				{
					root["type"] = WRTC_CMD_GET_FILE_LIST_TYPE_P;
					r_type = REOMOTE_NETWORK_CMD_GET_FILE_LIST_D_PICTURE;
					report2wrtc = true;
				}
				else if(files_rsp->reqType == WRTC_GET_FILES_D_EVENT_VIDEO)
				{
					root["type"] = WRTC_CMD_GET_FILE_LIST_TYPE_EV;
					r_type = REOMOTE_NETWORK_CMD_GET_FILE_LIST_D_EVENT_VIDEO;
					report2wrtc = true;
				}
				
				if(report2wrtc)
				{
					struct stat info;
					char* p = NULL;
					const char* pChar = NULL;
					
					if(files_rsp->reqType == WRTC_GET_FILES_D_VIDEO)
					{
						getFiles(WRTC_SDCARD_VIDEO_PATH, files);
						getFiles(WRTC_SDCARD_VIDEO_R_PATH, v_files_r);
						getFiles(WRTC_SDCARD_VIDEO_F_ALT_PATH, v_files_f_rlt);

						for (size_t i = 0; i < files.size(); ++i) {
							memset(&info, 0x0, sizeof(info));
							pChar = files[i].c_str();
							if(NULL == pChar)
							{
								db_msg("pChar is NULL, careful! ");
								continue;
							}
							
							p = strstr(pChar, "SOS");
							if(p != NULL)
							{
								db_msg("SOS files, skip \n");
								continue;
							}

							stat(pChar, &info);

							item["name"] = pChar;
							item["size"]= info.st_size;
							pthread_mutex_lock(&s_view_mutex);
							if(info.st_size > 0)
							{
								item["duration"]= pView_cdrMain->getVideoDuration(pChar);
							}
							else
							{
								item["duration"] = 0;
							}
							pthread_mutex_unlock(&s_view_mutex);
							item["atime"]= (unsigned long long int)info.st_atime;
							item["mtime"]= (unsigned long long int)info.st_mtime;
    						arrayObj.append(item);
  						}

						for (size_t i = 0; i < v_files_r.size(); ++i) {
							memset(&info, 0x0, sizeof(info));
							pChar = v_files_r[i].c_str();
							if(NULL == pChar)
							{
								db_msg("pChar is NULL, careful! ");
								continue;
							}
							
							p = strstr(pChar, "SOS");
							if(p != NULL)
							{
								db_msg("SOS files, skip \n");
								continue;
							}

							stat(pChar, &info);

							item["name"] = pChar;
							item["size"]= info.st_size;
							pthread_mutex_lock(&s_view_mutex);
							if(info.st_size > 0)
							{
								item["duration"]= pView_cdrMain->getVideoDuration(pChar);
							}
							else
							{
								item["duration"] = 0;
							}
							pthread_mutex_unlock(&s_view_mutex);
							item["atime"]= (unsigned long long int)info.st_atime;
							item["mtime"]= (unsigned long long int)info.st_mtime;
    						arrayObj.append(item);
  						}

						for (size_t i = 0; i < v_files_f_rlt.size(); ++i) {
							memset(&info, 0x0, sizeof(info));
							pChar = v_files_f_rlt[i].c_str();
							if(NULL == pChar)
							{
								db_msg("pChar is NULL, careful! ");
								continue;
							}
							
							p = strstr(pChar, "SOS");
							if(p != NULL)
							{
								db_msg("SOS files, skip \n");
								continue;
							}

							stat(pChar, &info);

							item["name"] = pChar;
							item["size"]= info.st_size;
							pthread_mutex_lock(&s_view_mutex);
							if(info.st_size > 0)
							{
								item["duration"]= pView_cdrMain->getVideoDuration(pChar);
							}
							else
							{
								item["duration"] = 0;
							}
							pthread_mutex_unlock(&s_view_mutex);
							item["atime"]= (unsigned long long int)info.st_atime;
							item["mtime"]= (unsigned long long int)info.st_mtime;
    						arrayObj.append(item);
  						}
						
					}
					else if(files_rsp->reqType == WRTC_GET_FILES_D_PICTURE)
					{
						getFiles(WRTC_SDCARD_PICTURE_PATH, files);
						for (size_t i = 0; i < files.size(); ++i) {
							memset(&info, 0x0, sizeof(info));
							pChar = files[i].c_str();
							if(NULL == pChar)
							{
								db_msg("pChar is NULL, careful! ");
								continue;
							}
							

							stat(pChar, &info);

							item["name"] = pChar;
							item["size"]= info.st_size;
							item["atime"]= (unsigned long long int)info.st_atime;
							item["mtime"]= (unsigned long long int)info.st_mtime;
    						arrayObj.append(item);
  						}
					}
					else if(files_rsp->reqType == WRTC_GET_FILES_D_EVENT_VIDEO)
					{
						getFiles(WRTC_SDCARD_VIDEO_PATH, files);
						getFiles(WRTC_SDCARD_VIDEO_R_PATH, v_files_r);
						getFiles(WRTC_SDCARD_VIDEO_F_ALT_PATH, v_files_f_rlt);

						for (size_t i = 0; i < files.size(); ++i) {
							memset(&info, 0x0, sizeof(info));
							pChar = files[i].c_str();
							if(NULL == pChar)
							{
								db_msg("pChar is NULL, careful! ");
								continue;
							}
							
							p = strstr(pChar, "SOS");
							if(NULL == p)
							{
								//db_msg("SOS files, skip \n");
								continue;
							}

							stat(pChar, &info);

							item["name"] = pChar;
							item["size"]= info.st_size;
							pthread_mutex_lock(&s_view_mutex);
							if(info.st_size > 0)
							{
								item["duration"]= pView_cdrMain->getVideoDuration(pChar);
							}
							else
							{
								item["duration"] = 0;
							}
							pthread_mutex_unlock(&s_view_mutex);
							item["atime"]= (unsigned long long int)info.st_atime;
							item["mtime"]= (unsigned long long int)info.st_mtime;
    						arrayObj.append(item);
  						}

						for (size_t i = 0; i < v_files_r.size(); ++i) {
							memset(&info, 0x0, sizeof(info));
							pChar = v_files_r[i].c_str();
							if(NULL == pChar)
							{
								db_msg("pChar is NULL, careful! ");
								continue;
							}
							
							p = strstr(pChar, "SOS");
							if(NULL == p)
							{
								//db_msg("SOS files, skip \n");
								continue;
							}

							stat(pChar, &info);

							item["name"] = pChar;
							item["size"]= info.st_size;
							pthread_mutex_lock(&s_view_mutex);
							if(info.st_size > 0)
							{
								item["duration"]= pView_cdrMain->getVideoDuration(pChar);
							}
							else
							{
								item["duration"] = 0;
							}
							pthread_mutex_unlock(&s_view_mutex);
							item["atime"]= (unsigned long long int)info.st_atime;
							item["mtime"]= (unsigned long long int)info.st_mtime;
    						arrayObj.append(item);
  						}

						for (size_t i = 0; i < v_files_f_rlt.size(); ++i) {
							memset(&info, 0x0, sizeof(info));
							pChar = v_files_f_rlt[i].c_str();
							if(NULL == pChar)
							{
								db_msg("pChar is NULL, careful! ");
								continue;
							}
							
							p = strstr(pChar, "SOS");
							if(NULL == p)
							{
								//db_msg("SOS files, skip \n");
								continue;
							}

							stat(pChar, &info);

							item["name"] = pChar;
							item["size"]= info.st_size;
							pthread_mutex_lock(&s_view_mutex);
							if(info.st_size > 0)
							{
								item["duration"]= pView_cdrMain->getVideoDuration(pChar);
							}
							else
							{
								item["duration"] = 0;
							}
							pthread_mutex_unlock(&s_view_mutex);
							item["atime"]= (unsigned long long int)info.st_atime;
							item["mtime"]= (unsigned long long int)info.st_mtime;
    						arrayObj.append(item);
  						}
					}
				}
				
				
				

				root["FileList"] = arrayObj;
				root["result"]=true;

				string strOut = writer.write(root);

				db_msg("fwk_msg MSG_ID_FWK_CONTROL_GET_FILE_LIST_D_RSP length=%d ", strOut.length());
				pthread_mutex_lock(&s_view_mutex);
				pView_cdrMain->notifyWebRtc((unsigned char*)strOut.c_str(),strOut.length());
				pthread_mutex_unlock(&s_view_mutex);
				#ifdef REMOTE_3G
				pthread_mutex_lock(&g_curl_mutex);
				remote_network_notify(strOut.c_str(),strOut.length(), r_type, NULL);
				pthread_mutex_unlock(&g_curl_mutex);
				#endif
			}
			break;

		case MSG_ID_FWK_CONTROL_DEL_FILE_RSP:
			{
				WRTC_DEL_FILE_RSP*  info_rsp = (WRTC_DEL_FILE_RSP*)msg->para;
				Json::Value root;
				Json::FastWriter writer;
				Json::Value json_rsp;

				db_msg("fwk_msg MSG_ID_FWK_CONTROL_DEL_FILE_RSP file_name=%s result=%d \n", info_rsp->path, info_rsp->result);

				
				json_rsp["name"]= WRTC_CMD_DEL_FILE;
				json_rsp["value"]= info_rsp->path;
				if(0 == info_rsp->result)
				{
					json_rsp["result"]=true;
				}
				else
				{
					json_rsp["result"]=false;
				}
				

				root.append(json_rsp);
				string path = writer.write(root);
				
				
				db_msg("fwk_msg MSG_ID_FWK_CONTROL_DEL_FILE_RSP length=%d ",  path.length());
				pthread_mutex_lock(&s_view_mutex);
				pView_cdrMain->notifyWebRtc((unsigned char*)path.c_str(),path.length());
				pthread_mutex_unlock(&s_view_mutex);
				#ifdef REMOTE_3G
				{
					REMOTE_NETWORK_NOTIFY_T notify;
					
					memset(&notify, 0, sizeof(notify));
					strncpy(notify.buffer, path.c_str(), REMOTE_NETWORK_BUFFER_LEN);
					notify.type = REOMOTE_NETWORK_CMD_DEL_FILE;
					fwk_send_message_ext(FWK_MOD_VIEW,FWK_MOD_Reserverd1,MSG_ID_FWK_CONTROL_3G_REMOTE_NOTIFY_IND,&notify,sizeof(notify));
					
				}
				#endif
			}
			break;

		case MSG_ID_FWK_CONTROL_DEVICE_INFO_RSP:
			{
				WRTC_GET_DEVICE_INFO_RSP*  info_rsp = (WRTC_GET_DEVICE_INFO_RSP*)msg->para;
				Json::Value root;
				Json::FastWriter writer;
				Json::Value json_rsp;

				db_msg("fwk_msg WRTC_GET_DEVICE_INFO_RSP  deviceID=%s\n", info_rsp->deviceid);

				
				json_rsp["name"]= WRTC_CMD_GET_DEVICE_INFO;
				json_rsp["deviceid"]= info_rsp->deviceid;
				json_rsp["version"]= info_rsp->version;
				json_rsp["cputype"]= info_rsp->cputype;
				json_rsp["cpu"]=info_rsp->cpu;
				json_rsp["manufacturer"] = info_rsp->manufacturer;
				json_rsp["result"]=true;
				

				root.append(json_rsp);
				string path = writer.write(root);
				
				
				db_msg("fwk_msg WRTC_GET_DEVICE_INFO_RSP length=%d ",  path.length());
				pthread_mutex_lock(&s_view_mutex);
				pView_cdrMain->notifyWebRtc((unsigned char*)path.c_str(),path.length());
				pthread_mutex_unlock(&s_view_mutex);
				#ifdef REMOTE_3G
				{
					REMOTE_NETWORK_NOTIFY_T notify;
					
					memset(&notify, 0, sizeof(notify));
					strncpy(notify.buffer, path.c_str(), REMOTE_NETWORK_BUFFER_LEN);
					notify.type = REOMOTE_NETWORK_CMD_DEVICE_INFO;
					fwk_send_message_ext(FWK_MOD_VIEW,FWK_MOD_Reserverd1,MSG_ID_FWK_CONTROL_3G_REMOTE_NOTIFY_IND,&notify,sizeof(notify));
					
				}
				#endif
			}
			break;
		case MSG_ID_FWK_CONTROL_AP_INFO_RSP:
			{
				WRTC_GET_AP_INFO_RSP*  info_rsp = (WRTC_GET_AP_INFO_RSP*)msg->para;
				Json::Value root;
				Json::FastWriter writer;
				Json::Value json_rsp;

				db_msg("fwk_msg MSG_ID_FWK_CONTROL_AP_INFO_RSP  ssid=%s\n", info_rsp->ssid);

				
				json_rsp["name"]= WRTC_CMD_GET_AP_INFO;
				json_rsp["ssid"]= info_rsp->ssid;
				json_rsp["password"]= info_rsp->pwd;
				if(0 == info_rsp->result)
				{
					json_rsp["result"]=true;
				}
				else
				{
					json_rsp["result"]=false;
				}
				json_rsp["type"]= "Settings";
				

				root.append(json_rsp);
				string path = writer.write(root);
				
				
				db_msg("fwk_msg MSG_ID_FWK_CONTROL_AP_INFO_RSP length=%d ",  path.length());
				pthread_mutex_lock(&s_view_mutex);
				pView_cdrMain->notifyWebRtc((unsigned char*)path.c_str(),path.length());
				pthread_mutex_unlock(&s_view_mutex);
				#ifdef REMOTE_3G
				{
					REMOTE_NETWORK_NOTIFY_T notify;
					
					memset(&notify, 0, sizeof(notify));
					strncpy(notify.buffer, path.c_str(), REMOTE_NETWORK_BUFFER_LEN);
					notify.type = REOMOTE_NETWORK_CMD_AP_INFO;
					fwk_send_message_ext(FWK_MOD_VIEW,FWK_MOD_Reserverd1,MSG_ID_FWK_CONTROL_3G_REMOTE_NOTIFY_IND,&notify,sizeof(notify));
					
				}
				#endif
			}
			break;

		case MSG_ID_FWK_CONTROL_CAM_NUMBER_INFO_RSP:
			{
				WRTC_GET_CAM_NUMBER_RSP*  info_rsp = (WRTC_GET_CAM_NUMBER_RSP*)msg->para;
				Json::Value root;
				Json::FastWriter writer;
				Json::Value json_rsp;

				db_msg("fwk_msg MSG_ID_FWK_CONTROL_CAM_NUMBER_INFO_RSP  num=%d\n", info_rsp->number);

				
				json_rsp["name"]= WRTC_CMD_GET_CAM_NUMBER;
				json_rsp["number"]= info_rsp->number;
				if(0 == info_rsp->result)
				{
					json_rsp["result"]=true;
				}
				else
				{
					json_rsp["result"]=false;
				}
				json_rsp["type"]= "Settings";
				

				root.append(json_rsp);
				string path = writer.write(root);
				
				
				db_msg("fwk_msg MSG_ID_FWK_CONTROL_CAM_NUMBER_INFO_RSP length=%d ",  path.length());
				pthread_mutex_lock(&s_view_mutex);
				pView_cdrMain->notifyWebRtc((unsigned char*)path.c_str(),path.length());
				pthread_mutex_unlock(&s_view_mutex);
				#ifdef REMOTE_3G
				{
					REMOTE_NETWORK_NOTIFY_T notify;
					
					memset(&notify, 0, sizeof(notify));
					strncpy(notify.buffer, path.c_str(), REMOTE_NETWORK_BUFFER_LEN);
					notify.type = REOMOTE_NETWORK_CMD_CAM_NUMBER;
					fwk_send_message_ext(FWK_MOD_VIEW,FWK_MOD_Reserverd1,MSG_ID_FWK_CONTROL_3G_REMOTE_NOTIFY_IND,&notify,sizeof(notify));
					
				}
				#endif
			}
			break;

		case MSG_ID_FWK_CONTROL_RTSP_URL_RSP:
			{
				WRTC_GET_RTSP_URL_RSP*  info_rsp = (WRTC_GET_RTSP_URL_RSP*)msg->para;
				Json::Value root;
				Json::FastWriter writer;
				Json::Value json_rsp;

				db_msg("fwk_msg MSG_ID_FWK_CONTROL_RTSP_URL_RSP  camfront=%s \n", info_rsp->camfront);

				
				json_rsp["name"]= WRTC_CMD_GET_RTSP_URL;
				if(strlen(info_rsp->camfront) > 0)
				{
					json_rsp["camfront_url"]= info_rsp->camfront;
				}

				if(strlen(info_rsp->camback) > 0)
				{
					json_rsp["camback_url"]= info_rsp->camback;
				}
				if(0 == info_rsp->result)
				{
					json_rsp["result"]=true;
				}
				else
				{
					json_rsp["result"]=false;
				}
				json_rsp["type"]= "Settings";
				

				root.append(json_rsp);
				string path = writer.write(root);
				
				
				db_msg("fwk_msg MSG_ID_FWK_CONTROL_RTSP_URL_RSP length=%d ",  path.length());
				pthread_mutex_lock(&s_view_mutex);
				pView_cdrMain->notifyWebRtc((unsigned char*)path.c_str(),path.length());
				pthread_mutex_unlock(&s_view_mutex);
				#ifdef REMOTE_3G
				{
					REMOTE_NETWORK_NOTIFY_T notify;
					
					memset(&notify, 0, sizeof(notify));
					strncpy(notify.buffer, path.c_str(), REMOTE_NETWORK_BUFFER_LEN);
					notify.type = REOMOTE_NETWORK_CMD_RTSP_URL;
					fwk_send_message_ext(FWK_MOD_VIEW,FWK_MOD_Reserverd1,MSG_ID_FWK_CONTROL_3G_REMOTE_NOTIFY_IND,&notify,sizeof(notify));
					
				}
				#endif
			}
			break;
	
    	default :
        {
			db_msg("wk_msg Unknow message to View \n");
    	}
        break;
		
    
    }


	return;
}


void* view_handler(void* p_arg)
{
    FWK_MSG *msg;
	int ret;


    while (1)
    {
        msg = fwk_get_message(fwk_view_queue, MSG_GET_WAIT_ROREVER, &ret);
        if((ret <0)||(msg == NULL))
        {
            db_msg("fwk_msg get message NULL\n");
            continue;
        }
        else
        {
            deal_view_msg(msg);
            fwk_free_message(&msg);
        }
    }
	return NULL;
}


int fwk_view_init(void)
{
	pthread_t viewid;
    int err;
	
#ifdef REMOTE_3G
	remote_network_init();
#endif
	
    fwk_view_queue=fwk_create_queue(FWK_MOD_VIEW);
    if (fwk_view_queue == -1)
    {
        db_error("fwk_msg unable to create queue for view \n");
        return -1;
    }

    

    err = pthread_create(&viewid,NULL,view_handler,NULL);
    if(err != 0)
    {
        db_error("fwk_msg create thread for view failed \n");
        return -1;
    }

    return 0;
}



String8 handle_remote_message_sync(unsigned char *pbuffer, int size)
{
	String8 result((char *)"");

	const char *pbuf = (const char*)pbuffer;
	Json::Reader reader;
	Json::Value root;
	
	db_msg("fwk_msg handle_remote_message_sync pbuf=%s \n", pbuf);	
	if(reader.parse(pbuf,root))
	{
		const char* pname=root["name"].asCString();
		const char* pOper=root["operation"].asCString();
		
		db_msg("fwk_msg handle_remote_message_sync name=%s pOper=%s\n", pname, pOper);
		if(pname != NULL)
		{
			if(strcasecmp(pname,WRTC_CMD_TAKE_PICTURE) == 0)
			{
				char* p = NULL;
				Json::Value lroot;
				Json::FastWriter writer;
				Json::Value picture_rsp;
				bool is_insert = false;

				pthread_mutex_lock(&s_view_mutex);
				is_insert = pView_cdrMain->isCardInsert();
				pthread_mutex_unlock(&s_view_mutex);
				if(!is_insert)
				{
					db_msg("fwk_msg handle_remote_message_sync snapshot no sdcard \n");
					picture_rsp["name"]=WRTC_TAKE_PICTURE_RSP;
					picture_rsp["value"]= "";
					picture_rsp["result"]=false;
					lroot.append(picture_rsp);
					string path = writer.write(lroot);
					
					result += path.c_str();
					return result;
				}
				
				pthread_mutex_lock(&s_view_mutex);
				pView_cdrMain->clearSnapshotPath();
				pView_cdrMain->take_picture();
				pthread_mutex_unlock(&s_view_mutex);

			check_snap_done:
				pthread_mutex_lock(&s_view_mutex);
				p = pView_cdrMain->getSnapshotPath();
				pthread_mutex_unlock(&s_view_mutex);
				while((p == NULL)  || (strlen(p) == 0))
				{
					//sleep(1);
					goto check_snap_done;
				}

				db_msg("fwk_msg snapshot done. path=%s \n", p);
				picture_rsp["name"]=WRTC_TAKE_PICTURE_RSP;
				picture_rsp["value"]= p;
				picture_rsp["result"]=true;
				lroot.append(picture_rsp);
				string path = writer.write(lroot);
				
				result += path.c_str();
				return result;
			}
			else if(strcasecmp(pname,WRTC_CMD_SET_RESOLUTION) == 0)
			{
				const char* pvalue=root["value"].asCString();
				int ret = -1;
				Json::Value lroot;
				Json::FastWriter writer;
				Json::Value wrtc_rsp;
				string wrtc_path;

				db_msg("fwk_msg handle_remote_message_sync name=%s pvalue=%s\n", pname, pvalue);

				wrtc_rsp["name"] = WRTC_CMD_SET_RESOLUTION;
				if(pvalue != NULL)
				{
					wrtc_rsp["value"] = pvalue;
					if(strcasecmp(pvalue, WRTC_VALUE_RESOLUTION_720P) == 0)
					{
						pthread_mutex_lock(&s_view_mutex);
						ret = pView_cdrMain->setVideoModeWebRTC(1);
						pthread_mutex_unlock(&s_view_mutex);
					}
					else if(strcasecmp(pvalue, WRTC_VALUE_RESOLUTION_1080P) == 0)
					{
						pthread_mutex_lock(&s_view_mutex);
						ret = pView_cdrMain->setVideoModeWebRTC(0);
						pthread_mutex_unlock(&s_view_mutex);
					}
				}
				if(0 == ret)
				{
					wrtc_rsp["result"]=true;
				}
				else
				{
					wrtc_rsp["result"]=false;
				}
				wrtc_rsp["type"]= WRTC_CMD_SETTINGS;
				lroot.append(wrtc_rsp);
				wrtc_path = writer.write(lroot);

				result += wrtc_path.c_str();
				return result;
			}
			else if(strcasecmp(pname,WRTC_CMD_SET_PICQUALITY) == 0)
			{
				const char* pvalue=root["value"].asCString();
				int ret = -1;
				Json::Value lroot;
				Json::FastWriter writer;
				Json::Value wrtc_rsp;
				string wrtc_path;

				db_msg("fwk_msg handle_remote_message_sync name=%s pvalue=%s\n", pname, pvalue);

				wrtc_rsp["name"] = WRTC_CMD_SET_PICQUALITY;
				if(pvalue != NULL)
				{
					wrtc_rsp["value"] = pvalue;
					if(strcasecmp(pvalue, WRTC_VALUE_PICQUALITY_QQVGA) == 0)
					{
						ret = pView_cdrMain->setPicModeWebRTC(ImageQualityQQVGA);
					}
					else if(strcasecmp(pvalue, WRTC_VALUE_PICQUALITY_QVGA) == 0)
					{
						ret = pView_cdrMain->setPicModeWebRTC(ImageQualityQVGA);
					}
					else if(strcasecmp(pvalue, WRTC_VALUE_PICQUALITY_720P) == 0)
					{
						ret = pView_cdrMain->setPicModeWebRTC(ImageQuality720P);
					}
					else if(strcasecmp(pvalue, WRTC_VALUE_PICQUALITY_1080P) == 0)
					{
						ret = pView_cdrMain->setPicModeWebRTC(ImageQuality1080P);
					}
					else
					{
						ret = -1;
					}

				}
				if(0 == ret)
				{
					wrtc_rsp["result"]=true;
				}
				else
				{
					wrtc_rsp["result"]=false;
				}
				wrtc_rsp["type"]= WRTC_CMD_SETTINGS;
				lroot.append(wrtc_rsp);
				wrtc_path = writer.write(lroot);

				result += wrtc_path.c_str();
				return result;
			}
			else if(strcasecmp(pname,WRTC_CMD_SET_RECORD_TIME) == 0)
			{
				int pvalue=root["value"].asInt();
				int index=0;
				int ret = -1;
				Json::Value lroot;
				Json::FastWriter writer;
				Json::Value wrtc_rsp;
				string wrtc_path;

				wrtc_rsp["name"] = WRTC_CMD_SET_RECORD_TIME;
				wrtc_rsp["value"] = pvalue;
				db_msg("fwk_msg handle_remote_message_sync name=%s pvalue=%d\n", pname, pvalue);
				index = (pvalue/60)-1;
				pthread_mutex_lock(&s_view_mutex);
				ret = pView_cdrMain->setRecordTimeWebRTC(index);
				pthread_mutex_unlock(&s_view_mutex);

				if(0 == ret)
				{
					wrtc_rsp["result"]=true;
				}
				else
				{
					wrtc_rsp["result"]=false;
				}
				wrtc_rsp["type"]= WRTC_CMD_SETTINGS;
				lroot.append(wrtc_rsp);
				wrtc_path = writer.write(lroot);

				result += wrtc_path.c_str();
				return result;
				
			}
			else if(strcasecmp(pname,WRTC_CMD_SET_EXPOSURE) == 0)
			{
				int pvalue=root["value"].asInt();
				int index=0;
				int ret = -1;
				Json::Value lroot;
				Json::FastWriter writer;
				Json::Value wrtc_rsp;
				string wrtc_path;

				wrtc_rsp["name"] = WRTC_CMD_SET_EXPOSURE;
				wrtc_rsp["value"] = pvalue;
				db_msg("fwk_msg handle_remote_message_sync name=%s pvalue=%d\n", pname, pvalue);
				index = pvalue + 3;
				pthread_mutex_lock(&s_view_mutex);
				ret = pView_cdrMain->setExposureWebRTC(index);
				pthread_mutex_unlock(&s_view_mutex);

				if(0 == ret)
				{
					wrtc_rsp["result"]=true;
				}
				else
				{
					wrtc_rsp["result"]=false;
				}
				wrtc_rsp["type"]= WRTC_CMD_SETTINGS;
				lroot.append(wrtc_rsp);
				wrtc_path = writer.write(lroot);

				result += wrtc_path.c_str();
				return result;	
			}
			else if(strcasecmp(pname,WRTC_CMD_SET_WB) == 0)
			{
				const char* pvalue=root["value"].asCString();
				int index = -1;
				int ret = -1;
				Json::Value lroot;
				Json::FastWriter writer;
				Json::Value wrtc_rsp;
				string wrtc_path;

				db_msg("fwk_msg handle_remote_message_sync name=%s pvalue=%s\n", pname, pvalue);

				wrtc_rsp["name"] = WRTC_CMD_SET_WB;
				if(pvalue != NULL)
				{
					wrtc_rsp["value"] = pvalue;
					
					if(strcasecmp(pvalue, WRTC_VALUE_WB_AUTO) == 0)
					{
						db_msg("fwk_msg handle_remote_message_sync auto \n");
						index = WB_VALUE_AUTO;
					}
					else if(strcasecmp(pvalue, WRTC_VALUE_WB_DAYLIGHT) == 0)
					{
						index = WB_VALUE_DAYLIGHT;
					}
					else if(strcasecmp(pvalue, WRTC_VALUE_WB_CLOUDY) == 0)
					{
						index = WB_VALUE_CLOUDY;
					}
					else if(strcasecmp(pvalue, WRTC_VALUE_WB_INCANDESCENT) == 0)
					{
						index = WB_VALUE_INCANDESCENT;
					}
					else if(strcasecmp(pvalue, WRTC_VALUE_WB_FLUORESCENT) == 0)
					{
						index = WB_VALUE_FLUORESCENT;
					}
				}

				if(index != -1)
				{
					pthread_mutex_lock(&s_view_mutex);
					ret = pView_cdrMain->setWhiteBalanceWebRTC(index);
					pthread_mutex_unlock(&s_view_mutex);
				}

				if(0 == ret)
				{
					wrtc_rsp["result"]=true;
				}
				else
				{
					wrtc_rsp["result"]=false;
				}
				wrtc_rsp["type"]= WRTC_CMD_SETTINGS;
				lroot.append(wrtc_rsp);
				wrtc_path = writer.write(lroot);

				result += wrtc_path.c_str();
				return result;	
				
			}
			else if(strcasecmp(pname,WRTC_CMD_SET_CONTRAST) == 0)
			{
				int pvalue=root["value"].asInt();
				int index = 0;
				int ret = -1;
				Json::Value lroot;
				Json::FastWriter writer;
				Json::Value wrtc_rsp;
				string wrtc_path;

				db_msg("fwk_msg handle_remote_message_sync name=%s pvalue=%d\n", pname, pvalue);
				wrtc_rsp["name"] = WRTC_CMD_SET_CONTRAST;
				wrtc_rsp["value"] = pvalue;
				
				index = pvalue;
				pthread_mutex_lock(&s_view_mutex);
				ret = pView_cdrMain->setContrastWebRTC(index);
				pthread_mutex_unlock(&s_view_mutex);
				
				if(0 == ret)
				{
					wrtc_rsp["result"]=true;
				}
				else
				{
					wrtc_rsp["result"]=false;
				}
				wrtc_rsp["type"]= WRTC_CMD_SETTINGS;
				lroot.append(wrtc_rsp);
				wrtc_path = writer.write(lroot);

				result += wrtc_path.c_str();
				return result;	
			}
			else if(strcasecmp(pname,WRTC_CMD_SET_WATERMARK) == 0)
			{
				int pvalue=root["value"].asBool();
				int ret = -1;
				Json::Value lroot;
				Json::FastWriter writer;
				Json::Value wrtc_rsp;
				string wrtc_path;

				wrtc_rsp["name"] = WRTC_CMD_SET_WATERMARK;
				db_msg("fwk_msg handle_remote_message_sync name=%s pvalue=%d\n", pname, pvalue);
				pthread_mutex_lock(&s_view_mutex);
				ret = pView_cdrMain->setTimeWaterMarkWebRTC(pvalue);
				pthread_mutex_unlock(&s_view_mutex);

				if(pvalue)
				{
					wrtc_rsp["value"]=true;	
				}
				else
				{
					wrtc_rsp["value"]=false;
				}

				if(0 == ret)
				{
					wrtc_rsp["result"]=true;
				}
				else
				{
					wrtc_rsp["result"]=false;
				}
				wrtc_rsp["type"]= WRTC_CMD_SETTINGS;
				lroot.append(wrtc_rsp);
				wrtc_path = writer.write(lroot);

				result += wrtc_path.c_str();
				return result;	
				
			}
			else if(strcasecmp(pname,WRTC_CMD_SET_MOVING) == 0)
			{
				int pvalue=root["value"].asBool();
				int ret = -1;
				Json::Value lroot;
				Json::FastWriter writer;
				Json::Value wrtc_rsp;
				string wrtc_path;

				wrtc_rsp["name"] = WRTC_CMD_SET_MOVING;
				db_msg("fwk_msg handle_remote_message_sync name=%s pvalue=%d\n", pname, pvalue);
				pthread_mutex_lock(&s_view_mutex);
				ret = pView_cdrMain->setAWMDWebRTC(pvalue);
				pthread_mutex_unlock(&s_view_mutex);

				if(pvalue)
				{
					wrtc_rsp["value"]=true;	
				}
				else
				{
					wrtc_rsp["value"]=false;
				}

				if(0 == ret)
				{
					wrtc_rsp["result"]=true;
				}
				else
				{
					wrtc_rsp["result"]=false;
				}
				wrtc_rsp["type"]= WRTC_CMD_SETTINGS;
				lroot.append(wrtc_rsp);
				wrtc_path = writer.write(lroot);

				result += wrtc_path.c_str();
				return result;
			}
			else if(strcasecmp(pname,WRTC_CMD_SET_GPS) == 0)
			{
				int pvalue=root["value"].asBool();
				int ret = -1;
				Json::Value lroot;
				Json::FastWriter writer;
				Json::Value wrtc_rsp;
				string wrtc_path;

				db_msg("fwk_msg handle_remote_message_sync name=%s pvalue=%d\n", pname, pvalue);
				wrtc_rsp["name"] = WRTC_CMD_SET_GPS;
				pthread_mutex_lock(&s_view_mutex);
				pView_cdrMain->setGpsWebRTC(pvalue);
				pthread_mutex_unlock(&s_view_mutex);
				if(pvalue)
				{
					if (enableGps())
					{
						ret = 0;
					}
					else
					{
						ret = -1;
					}
				}
				else
				{
					/*oFF*/						
					if (disableGps())
					{
						ret = 0;
					}
					else
					{
						ret = -1;
					}
				}

				if(pvalue)
				{
					wrtc_rsp["value"]=true;	
				}
				else
				{
					wrtc_rsp["value"]=false;
				}

				if(0 == ret)
				{
					wrtc_rsp["result"]=true;
				}
				else
				{
					wrtc_rsp["result"]=false;
				}
				wrtc_rsp["type"]= WRTC_CMD_SETTINGS;
				lroot.append(wrtc_rsp);
				wrtc_path = writer.write(lroot);

				result += wrtc_path.c_str();
				return result;
			}
			else if(strcasecmp(pname,WRTC_CMD_SET_MOBILE) == 0)
			{
				int pvalue=root["value"].asBool();
				int ret = -1;
				Json::Value lroot;
				Json::FastWriter writer;
				Json::Value wrtc_rsp;
				string wrtc_path;

				wrtc_rsp["name"] = WRTC_CMD_SET_MOBILE;
				db_msg("fwk_msg handle_remote_message_sync name=%s pvalue=%d\n", pname, pvalue);
				pthread_mutex_lock(&s_view_mutex);
				pView_cdrMain->setCellularWebRTC(pvalue);
				pthread_mutex_unlock(&s_view_mutex);
				if(pvalue)
				{
					/*ON*/
					if (enableCellular())
					{
						ret = 0;
					}
					else
					{
						ret = -1;
					}
				}
				else
				{
					/*oFF*/
					if (disableCellular())
					{
						ret = 0;	
					}
					else
					{
						ret = -1;
					}
				}

				if(pvalue)
				{
					wrtc_rsp["value"]=true;	
				}
				else
				{
					wrtc_rsp["value"]=false;
				}

				if(0 == ret)
				{
					wrtc_rsp["result"]=true;
				}
				else
				{
					wrtc_rsp["result"]=false;
				}
				wrtc_rsp["type"]= WRTC_CMD_SETTINGS;
				lroot.append(wrtc_rsp);
				wrtc_path = writer.write(lroot);

				result += wrtc_path.c_str();
				return result;
			}
			else if(strcasecmp(pname,WRTC_CMD_SET_FORMAT) == 0)
			{
				int pvalue=root["value"].asBool();
				int ret = -1;
				Json::Value lroot;
				Json::FastWriter writer;
				Json::Value wrtc_rsp;
				string wrtc_path;

				wrtc_rsp["name"] = WRTC_CMD_SET_FORMAT;
				db_msg("fwk_msg handle_remote_message_sync name=%s pvalue=%d\n", pname, pvalue);
				pthread_mutex_lock(&s_view_mutex);
				ret = pView_cdrMain->setFormatWebRTC();
				pthread_mutex_unlock(&s_view_mutex);
				
				if(pvalue)
				{
					wrtc_rsp["value"]=true;	
				}
				else
				{
					wrtc_rsp["value"]=false;
				}

				if(0 == ret)
				{
					wrtc_rsp["result"]=true;
				}
				else
				{
					wrtc_rsp["result"]=false;
				}
				wrtc_rsp["type"]= "Action";
				lroot.append(wrtc_rsp);
				wrtc_path = writer.write(lroot);

				result += wrtc_path.c_str();
				return result;		
			}
			else if(strcasecmp(pname,WRTC_CMD_SET_RESET_FACTORY) == 0)
			{
				CommuReq req;
				int pvalue=root["value"].asBool();
				Json::Value lroot;
				Json::FastWriter writer;
				Json::Value wrtc_rsp;
				string wrtc_path;

				db_msg("fwk_msg handle_remote_message_sync name=%s \n", pname);
				wrtc_rsp["name"] = WRTC_CMD_SET_RESET_FACTORY;
				wrtc_rsp["value"]=true;
				wrtc_rsp["result"]=true;
				wrtc_rsp["type"]= "Action";
				lroot.append(wrtc_rsp);
				wrtc_path = writer.write(lroot);

				
				
				
				memset(&req, 0x0, sizeof(req));
				req.cmdType = COMMU_TYPE_FACTORY_RESET;
				req.operType = pvalue;
				fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_COMMU_SWITCH_REQ, &req, sizeof(req));

				result += wrtc_path.c_str();
				return result;
			}
			else if(strcasecmp(pname,WRTC_CMD_GET_ALL_PARAMS) == 0)
			{
				WRTC_SETTINGS rsp;
				Json::Value lroot;
				Json::Value arrayObj;
				Json::Value item;
				Json::FastWriter writer;

				db_msg("fwk_msg handle_remote_message_sync name=%s \n", pname);
				
				memset(&rsp, 0x0, sizeof(rsp));
				pthread_mutex_lock(&s_view_mutex);
				pView_cdrMain->getSettingsWebRTC(&rsp);
				pthread_mutex_unlock(&s_view_mutex);

				lroot["name"] = rsp.szName;
				lroot["count"] = rsp.nCount;

				for(int i=0; i < rsp.nCount; i++)
				{
					item["item_name"] = rsp.items[i].itemName;
					if(1 == rsp.items[i].valueType)
					{
						item["item_value"] = rsp.items[i].valueStr;
					}
					else if(2 == rsp.items[i].valueType)
					{
						item["item_value"] = rsp.items[i].bValue;
					}
					else if(3 == rsp.items[i].valueType)
					{
						item["year"] = rsp.items[i].dt.year;
						item["month"] = rsp.items[i].dt.month;
						item["day"] = rsp.items[i].dt.day;
						item["hour"] = rsp.items[i].dt.hour;
						item["minute"] = rsp.items[i].dt.minute;
						item["second"] = rsp.items[i].dt.second;
					}
					else
					{
						item["item_value"] = rsp.items[i].value;
					}

					arrayObj.append(item);
				}

				lroot["item_list"] = arrayObj;
				lroot["result"]=true;

				string strOut = writer.write(lroot);
				
				result += strOut.c_str();
				return result;
			}
			else if(strcasecmp(pname,WRTC_CMD_SET_RECORD) == 0)
			{
				int pvalue=root["value"].asBool();
				int ret = -1;
				Json::Value lroot;
				Json::FastWriter writer;
				Json::Value wrtc_rsp;
				string wrtc_path;

				wrtc_rsp["name"] = WRTC_CMD_SET_RECORD;
				db_msg("fwk_msg handle_remote_message_sync name=%s pvalue=%d\n", pname, pvalue);
				pthread_mutex_lock(&s_view_mutex);
				ret = pView_cdrMain->setRecordWebRTC(pvalue);
				pthread_mutex_unlock(&s_view_mutex);

				if(pvalue)
				{
					wrtc_rsp["value"]=true;	
				}
				else
				{
					wrtc_rsp["value"]=false;
				}

				if(0 == ret)
				{
					wrtc_rsp["result"]=true;
				}
				else
				{
					wrtc_rsp["result"]=false;
				}
				wrtc_rsp["type"]= WRTC_CMD_SETTINGS;
				lroot.append(wrtc_rsp);
				wrtc_path = writer.write(lroot);

				result += wrtc_path.c_str();
				return result;	
			}
			else if(strcasecmp(pname,WRTC_CMD_SET_SOUND) == 0)
			{
				int pvalue=root["value"].asBool();
				int ret = -1;
				Json::Value lroot;
				Json::FastWriter writer;
				Json::Value wrtc_rsp;
				string wrtc_path;

				wrtc_rsp["name"] = WRTC_CMD_SET_SOUND;

				db_msg("fwk_msg handle_remote_message_sync name=%s pvalue=%d\n", pname, pvalue);
				pthread_mutex_lock(&s_view_mutex);
				ret = pView_cdrMain->setSoundWebRTC(pvalue);
				pthread_mutex_unlock(&s_view_mutex);

				if(pvalue)
				{
					wrtc_rsp["value"]=true;	
				}
				else
				{
					wrtc_rsp["value"]=false;
				}

				if(0 == ret)
				{
					wrtc_rsp["result"]=true;
				}
				else
				{
					wrtc_rsp["result"]=false;
				}
				wrtc_rsp["type"]= WRTC_CMD_SETTINGS;
				lroot.append(wrtc_rsp);
				wrtc_path = writer.write(lroot);

				result += wrtc_path.c_str();
				return result;	
			}
			else if(strcasecmp(pname,WRTC_CMD_SET_DATE_TIME) == 0)
			{
				WRTC_DATE_TIME dt;
				date_t date;
				Json::Value lroot;
				Json::FastWriter writer;
				Json::Value json_rsp;
				char pdate_buf[WRTC_MAX_NAME_LEN+1];

				db_msg("fwk_msg handle_remote_message_sync name=%s \n", pname);
				
				memset(&dt, 0x0, sizeof(dt));
				dt.year = root["year"].asUInt();
				dt.month = root["month"].asUInt();
				dt.day = root["day"].asUInt();
				dt.hour = root["hour"].asUInt();
				dt.minute = root["minute"].asUInt();
				dt.second = root["second"].asUInt();

				memset(&date ,0, sizeof(date));
				date.year = dt.year;
				date.month = dt.month;
				date.day = dt.day;
				date.hour = dt.hour;
				date.minute = dt.minute;
				date.second = dt.second;
				
				setSystemDate(&date);

				json_rsp["name"]= WRTC_CMD_SET_DATE_TIME;
				json_rsp["year"]= dt.year;
				json_rsp["month"]= dt.month;
				json_rsp["day"]= dt.day;
				json_rsp["hour"]= dt.hour;
				json_rsp["minute"]= dt.minute;
				json_rsp["second"]= dt.second;
				memset(pdate_buf, 0, sizeof(pdate_buf));
				sprintf(pdate_buf, "%4d%2d%2d%2d%2d", dt.year, dt.month, dt.day, dt.hour, dt.minute);
				//json_rsp["date"]= pdate_buf;
				
				json_rsp["result"]=true;
				
				lroot.append(json_rsp);
				string path = writer.write(lroot);
				
				result += path.c_str();
				return result;			
			}
			else if(strcasecmp(pname,WRTC_CMD_SET_WIFI_PASSWD) == 0)
			{
				const char* pvalue=root["value"].asCString();
				int ret = -1;
				Json::Value lroot;
				Json::FastWriter writer;
				Json::Value json_rsp;
				WRTC_WIFI_PASSWD_REQ req;

				db_msg("fwk_msg handle_remote_message_sync name=%s \n", pname);

				memset(&req, 0x0, sizeof(req));
				if(pvalue != NULL)
				{
					ret = 0;
					strncpy(req.pwd, pvalue,WRTC_MAX_VALUE_LEN);
					fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_SET_WIFIPASSWD_REQ, &req, sizeof(req));
				}
				else
				{
					ret = -1;
				}

				json_rsp["name"]= WRTC_CMD_SET_WIFI_PASSWD;
				if(ret == 0)
				{
					json_rsp["result"]=true;
				}
				else
				{
					json_rsp["result"]=false;
				}
				lroot.append(json_rsp);
				string path = writer.write(lroot);
				
				result += path.c_str();
				return result;					
			}
			else if(strcasecmp(pname,WRTC_CMD_SET_WIFI_SSID) == 0)
			{
				const char* pvalue=root["value"].asCString();
				int ret = -1;
				Json::Value lroot;
				Json::FastWriter writer;
				Json::Value json_rsp;
				WRTC_WIFI_SSID_REQ req;

				db_msg("fwk_msg handle_remote_message_sync name=%s \n", pname);

				memset(&req, 0x0, sizeof(req));
				if(pvalue != NULL)
				{
					ret = 0;
					strncpy(req.ssid, pvalue,WRTC_MAX_VALUE_LEN);
					fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_SET_WIFISSID_REQ, &req, sizeof(req));
				}
				else
				{
					ret = -1;
				}

				json_rsp["name"]= WRTC_CMD_SET_WIFI_SSID;
				if(ret == 0)
				{
					json_rsp["result"]=true;
				}
				else
				{
					json_rsp["result"]=false;
				}
				lroot.append(json_rsp);
				string path = writer.write(lroot);
				
				result += path.c_str();
				return result;					
			}
			else if(strcasecmp(pname,WRTC_CMD_GET_PICTURE_FILES) == 0)
			{
				std::vector<std::string> files;

				db_msg("fwk_msg handle_remote_message_sync name=%s \n", pname);
				
				getFiles(WRTC_SDCARD_PICTURE_PATH, files);

				int size = files.size();
				Json::FastWriter writer;		
				Json::Value out = StringVectorToJsonArray(files);
				Json::Value message;

				message["name"] = WRTC_CMD_GET_PICTURE_FILES;
				message["type"] = "data";
				message["result"]=true;
				message["files"] = out;
				string path = writer.write(message);

				result += path.c_str();
				return result;				
			}
			else if(strcasecmp(pname,WRTC_CMD_GET_VIDEO_FILES) == 0)
			{
				std::vector<std::string> files;
				std::vector<std::string> v_files_r;
				std::vector<std::string> v_files_f_rlt;

				db_msg("fwk_msg handle_remote_message_sync name=%s \n", pname);

				getFiles(WRTC_SDCARD_VIDEO_PATH, files);
				getFiles(WRTC_SDCARD_VIDEO_R_PATH, v_files_r);
				getFiles(WRTC_SDCARD_VIDEO_F_ALT_PATH, v_files_f_rlt);

				int size = files.size();
				Json::FastWriter writer;		
				Json::Value out = StringVectorToJsonArray3(files, v_files_r, v_files_f_rlt);
				Json::Value message;

				message["name"] = WRTC_CMD_GET_VIDEO_FILES;
				message["type"] = "data";
				message["result"]=true;
				message["files"] = out;
				string path = writer.write(message);

				result += path.c_str();
				return result;						
			}
			else if(strcasecmp(pname,WRTC_CMD_GET_FILE_INFO) == 0)
			{
				const char* pvalue=root["value"].asCString();
				int ret = -1;
				Json::Value lroot;
				Json::FastWriter writer;
				Json::Value json_rsp;
				struct stat info;

				db_msg("fwk_msg handle_remote_message_sync name=%s \n", pname);

				json_rsp["name"]= WRTC_CMD_GET_FILE_INFO;
				
				if(pvalue != NULL)
				{
					json_rsp["value"]= pvalue;
					ret = stat(pvalue, &info);
				}

				
				
				if(0 == ret)
				{
					json_rsp["result"]=true;
				}
				else
				{
					json_rsp["result"]=false;
				}
				
				json_rsp["size"]= info.st_size;
				json_rsp["atime"]= (unsigned long long int)info.st_atime;
				json_rsp["mtime"]= (unsigned long long int)info.st_mtime;

				lroot.append(json_rsp);
				string path = writer.write(lroot);

				result += path.c_str();
				return result;					
			}
			else if(strcasecmp(pname,WRTC_CMD_GET_FILE_LIST_D) == 0)
			{
				const char* pvalue=root["type"].asCString();
				Json::Value lroot;
				Json::Value arrayObj;
				Json::Value item;
				Json::FastWriter writer;
				std::vector<std::string> files;
				std::vector<std::string> v_files_r;
				std::vector<std::string> v_files_f_rlt;

				lroot["name"] = WRTC_CMD_GET_FILE_LIST_D;
				if(pvalue != NULL)
				{
					struct stat info;
					char* p = NULL;
					const char* pChar = NULL;
					
					lroot["type"] = pvalue;
					db_msg("fwk_msg handle_remote_message_sync name=%s pvalue=%s\n", pname, pvalue);
					if(strcasecmp(pvalue, WRTC_CMD_GET_FILE_LIST_TYPE_V) == 0)
					{
						db_msg("fwk_msg handle_remote_message_sync video");
						getFiles(WRTC_SDCARD_VIDEO_PATH, files);
						getFiles(WRTC_SDCARD_VIDEO_R_PATH, v_files_r);
						getFiles(WRTC_SDCARD_VIDEO_F_ALT_PATH, v_files_f_rlt);

						for (size_t i = 0; i < files.size(); ++i) {
							memset(&info, 0x0, sizeof(info));
							pChar = files[i].c_str();
							if(NULL == pChar)
							{
								db_msg("pChar is NULL, careful! ");
								continue;
							}
							
							p = strstr(pChar, "SOS");
							if(p != NULL)
							{
								db_msg("SOS files, skip \n");
								continue;
							}

							stat(pChar, &info);

							item["name"] = pChar;
							item["size"]= info.st_size;
							pthread_mutex_lock(&s_view_mutex);
							if(info.st_size > 0)
							{
								item["duration"]= pView_cdrMain->getVideoDuration(pChar);
							}
							else
							{
								item["duration"] = 0;
							}
							pthread_mutex_unlock(&s_view_mutex);
							item["atime"]= (unsigned long long int)info.st_atime;
							item["mtime"]= (unsigned long long int)info.st_mtime;
    						arrayObj.append(item);
  						}

						for (size_t i = 0; i < v_files_r.size(); ++i) {
							memset(&info, 0x0, sizeof(info));
							pChar = v_files_r[i].c_str();
							if(NULL == pChar)
							{
								db_msg("pChar is NULL, careful! ");
								continue;
							}
							
							p = strstr(pChar, "SOS");
							if(p != NULL)
							{
								db_msg("SOS files, skip \n");
								continue;
							}

							stat(pChar, &info);

							item["name"] = pChar;
							item["size"]= info.st_size;
							pthread_mutex_lock(&s_view_mutex);
							if(info.st_size > 0)
							{
								item["duration"]= pView_cdrMain->getVideoDuration(pChar);
							}
							else
							{
								item["duration"] = 0;
							}
							pthread_mutex_unlock(&s_view_mutex);
							item["atime"]= (unsigned long long int)info.st_atime;
							item["mtime"]= (unsigned long long int)info.st_mtime;
    						arrayObj.append(item);
  						}

						for (size_t i = 0; i < v_files_f_rlt.size(); ++i) {
							memset(&info, 0x0, sizeof(info));
							pChar = v_files_f_rlt[i].c_str();
							if(NULL == pChar)
							{
								db_msg("pChar is NULL, careful! ");
								continue;
							}
							
							p = strstr(pChar, "SOS");
							if(p != NULL)
							{
								db_msg("SOS files, skip \n");
								continue;
							}

							stat(pChar, &info);

							item["name"] = pChar;
							item["size"]= info.st_size;
							pthread_mutex_lock(&s_view_mutex);
							if(info.st_size > 0)
							{
								item["duration"]= pView_cdrMain->getVideoDuration(pChar);
							}
							else
							{
								item["duration"] = 0;
							}
							pthread_mutex_unlock(&s_view_mutex);
							item["atime"]= (unsigned long long int)info.st_atime;
							item["mtime"]= (unsigned long long int)info.st_mtime;
    						arrayObj.append(item);
  						}
						
					}
					else if(strcasecmp(pvalue, WRTC_CMD_GET_FILE_LIST_TYPE_P) == 0)
					{
						db_msg("fwk_msg handle_remote_message_sync picture");
						getFiles(WRTC_SDCARD_PICTURE_PATH, files);
						for (size_t i = 0; i < files.size(); ++i) {
							memset(&info, 0x0, sizeof(info));
							pChar = files[i].c_str();
							if(NULL == pChar)
							{
								db_msg("pChar is NULL, careful! ");
								continue;
							}
							

							stat(pChar, &info);

							item["name"] = pChar;
							item["size"]= info.st_size;
							item["atime"]= (unsigned long long int)info.st_atime;
							item["mtime"]= (unsigned long long int)info.st_mtime;
    						arrayObj.append(item);
  						}
					}
					else if(strcasecmp(pvalue, WRTC_CMD_GET_FILE_LIST_TYPE_EV) == 0)
					{
						db_msg("fwk_msg handle_remote_message_sync EventVideo");
						getFiles(WRTC_SDCARD_VIDEO_PATH, files);
						getFiles(WRTC_SDCARD_VIDEO_R_PATH, v_files_r);
						getFiles(WRTC_SDCARD_VIDEO_F_ALT_PATH, v_files_f_rlt);

						for (size_t i = 0; i < files.size(); ++i) {
							memset(&info, 0x0, sizeof(info));
							pChar = files[i].c_str();
							if(NULL == pChar)
							{
								db_msg("pChar is NULL, careful! ");
								continue;
							}
							
							p = strstr(pChar, "SOS");
							if(NULL == p)
							{
								//db_msg("SOS files, skip \n");
								continue;
							}

							stat(pChar, &info);

							item["name"] = pChar;
							item["size"]= info.st_size;
							pthread_mutex_lock(&s_view_mutex);
							if(info.st_size > 0)
							{
								item["duration"]= pView_cdrMain->getVideoDuration(pChar);
							}
							else
							{
								item["duration"] = 0;
							}
							pthread_mutex_unlock(&s_view_mutex);
							item["atime"]= (unsigned long long int)info.st_atime;
							item["mtime"]= (unsigned long long int)info.st_mtime;
    						arrayObj.append(item);
  						}

						for (size_t i = 0; i < v_files_r.size(); ++i) {
							memset(&info, 0x0, sizeof(info));
							pChar = v_files_r[i].c_str();
							if(NULL == pChar)
							{
								db_msg("pChar is NULL, careful! ");
								continue;
							}
							
							p = strstr(pChar, "SOS");
							if(NULL == p)
							{
								//db_msg("SOS files, skip \n");
								continue;
							}

							stat(pChar, &info);

							item["name"] = pChar;
							item["size"]= info.st_size;
							pthread_mutex_lock(&s_view_mutex);
							if(info.st_size > 0)
							{
								item["duration"]= pView_cdrMain->getVideoDuration(pChar);
							}
							else
							{
								item["duration"] = 0;
							}
							pthread_mutex_unlock(&s_view_mutex);
							item["atime"]= (unsigned long long int)info.st_atime;
							item["mtime"]= (unsigned long long int)info.st_mtime;
    						arrayObj.append(item);
  						}

						for (size_t i = 0; i < v_files_f_rlt.size(); ++i) {
							memset(&info, 0x0, sizeof(info));
							pChar = v_files_f_rlt[i].c_str();
							if(NULL == pChar)
							{
								db_msg("pChar is NULL, careful! ");
								continue;
							}
							
							p = strstr(pChar, "SOS");
							if(NULL == p)
							{
								//db_msg("SOS files, skip \n");
								continue;
							}

							stat(pChar, &info);

							item["name"] = pChar;
							item["size"]= info.st_size;
							pthread_mutex_lock(&s_view_mutex);
							if(info.st_size > 0)
							{
								item["duration"]= pView_cdrMain->getVideoDuration(pChar);
							}
							else
							{
								item["duration"] = 0;
							}
							pthread_mutex_unlock(&s_view_mutex);
							item["atime"]= (unsigned long long int)info.st_atime;
							item["mtime"]= (unsigned long long int)info.st_mtime;
    						arrayObj.append(item);
  						}
					}
					
				}

				lroot["FileList"] = arrayObj;
				lroot["result"]=true;

				string strOut = writer.write(lroot);

				result += strOut.c_str();
				return result;					
			}
			else if(strcasecmp(pname,WRTC_CMD_DEL_FILE) == 0)
			{
				const char* pvalue=root["value"].asCString();
				int ret = -1;
				Json::Value lroot;
				Json::FastWriter writer;
				Json::Value json_rsp;

				json_rsp["name"]= WRTC_CMD_DEL_FILE;
				if(pvalue != NULL)
				{
					db_msg("fwk_msg handle_remote_message_sync name=%s pvalue=%s\n", pname, pvalue);
					json_rsp["value"]= pvalue;
					
					ret = unlink(pvalue);
				}

				
				
				if(0 == ret)
				{
					json_rsp["result"]=true;
				}
				else
				{
					json_rsp["result"]=false;
				}
				

				lroot.append(json_rsp);
				string path = writer.write(lroot);
				
				result += path.c_str();
				return result;				
			}
			else if(strcasecmp(pname,WRTC_CMD_GET_DEVICE_INFO) == 0)
			{
				WRTC_GET_DEVICE_INFO_RSP w_rsp;
				char tmp[PROPERTY_VALUE_MAX] = {'\0'};
				Json::Value lroot;
				Json::FastWriter writer;
				Json::Value json_rsp;
				
				
				
				db_msg("fwk_msg handle_remote_message_sync name=%s \n", pname);
				
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
				
				
				json_rsp["name"]= WRTC_CMD_GET_DEVICE_INFO;
				json_rsp["deviceid"]= w_rsp.deviceid;
				json_rsp["version"]= w_rsp.version;
				json_rsp["cputype"]= w_rsp.cputype;
				json_rsp["cpu"]= w_rsp.cpu;
				json_rsp["manufacturer"] = w_rsp.manufacturer;
				json_rsp["result"]=true;
				

				lroot.append(json_rsp);
				string path = writer.write(lroot);

				result += path.c_str();
				return result;					
			}
			else if(strcasecmp(pname,WRTC_CMD_GET_AP_INFO) == 0)
			{
				WRTC_GET_AP_INFO_RSP w_rsp;
				bool ret = false;
				Json::Value lroot;
				Json::FastWriter writer;
				Json::Value json_rsp;
				
				db_msg("fwk_msg handle_remote_message_sync name=%s \n", pname);
							
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

				json_rsp["name"]= WRTC_CMD_GET_AP_INFO;
				json_rsp["ssid"]= w_rsp.ssid;
				json_rsp["password"]= w_rsp.pwd;
				if(0 == w_rsp.result)
				{
					json_rsp["result"]=true;
				}
				else
				{
					json_rsp["result"]=false;
				}
				json_rsp["type"]= "Settings";
				

				lroot.append(json_rsp);
				string path = writer.write(lroot);
				
				result += path.c_str();
				return result;	
			}
			else if(strcasecmp(pname,WRTC_CMD_GET_CAM_NUMBER) == 0)
			{
				WRTC_GET_CAM_NUMBER_RSP w_rsp;
				int num = 0;
				Json::Value lroot;
				Json::FastWriter writer;
				Json::Value json_rsp;
				
				db_msg("fwk_msg handle_remote_message_sync	name=%s \n", pname);
				
				
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

				json_rsp["name"]= WRTC_CMD_GET_CAM_NUMBER;
				json_rsp["number"]= num;

				if(0 == w_rsp.result)
				{
					json_rsp["result"]=true;
				}
				else
				{
					json_rsp["result"]=false;
				}
				json_rsp["type"]= "Settings";

				lroot.append(json_rsp);
				string path = writer.write(lroot);
				
				result += path.c_str();
				return result;	
				
			}
			else if(strcasecmp(pname,WRTC_CMD_GET_RTSP_URL) == 0)
			{
				WRTC_GET_RTSP_URL_RSP w_rsp;
				Json::Value lroot;
				Json::FastWriter writer;
				Json::Value json_rsp;
				
				db_msg("fwk_msg handle_remote_message_sync	 name=%s \n", pname);
				
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

				json_rsp["name"]= WRTC_CMD_GET_RTSP_URL;
				if(strlen(w_rsp.camfront) > 0)
				{
					json_rsp["camfront_url"]= w_rsp.camfront;
				}

				if(strlen(w_rsp.camback) > 0)
				{
					json_rsp["camback_url"]= w_rsp.camback;
				}

				if(0 == w_rsp.result)
				{
					json_rsp["result"]=true;
				}
				else
				{
					json_rsp["result"]=false;
				}
				json_rsp["type"]= "Settings";

				lroot.append(json_rsp);
				string path = writer.write(lroot);
				
				result += path.c_str();
				return result;	
				
			}
			else
			{
				db_msg("fwk_msg handle_remote_message_sync Unknown wrtc cmd name=%s \n", pname);
				
			}
		}


	}
	
			
    return result;
}


