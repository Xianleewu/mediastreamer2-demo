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
#define LOG_TAG "remote_network"
#include "debug.h"

using namespace std;
#include <string>
#include <json/json.h>

#include "resumable_io.h"

pthread_mutex_t g_curl_mutex = PTHREAD_MUTEX_INITIALIZER;
#define GPS_LOCATION_NO_SINGLE_STR			("0.00000000000000")


extern int remote_network_notify(const char* pbuf, int len, REOMOTE_NETWORK_CMD_TYPE type,void *pSToken);
extern void acqurie_remote_network_wakelock(void);
extern void release_remote_network_wakelock(void);
extern char* remote_network_get_token(void);



static int remote_network_upload_queue;
#define RETRY_TIMES_FOR_STORAGE_SERVER		(8)

int simple_upload(Qiniu_Client* client, char* uptoken, const char* key, const char* localFile)
{	
	Qiniu_Error err;
	int retry_times = 0;

	db_msg("remote_network simple_upload \n");
	retry_times = 0;

retry_simple_uploading:	
	err = Qiniu_Io_PutFile(client, NULL, uptoken, key, localFile, NULL);
	
	db_msg("remote_network simple_upload err.code=%d message=%s \n", err.code, err.message);

	if(err.code != 200)
	{
		retry_times++;
		if(retry_times < RETRY_TIMES_FOR_STORAGE_SERVER)
		{
			goto retry_simple_uploading;
		}
	}
	
	return err.code;
}


int resumable_upload(Qiniu_Client* client, char* uptoken, const char* key, const char* localFile)
{	
	Qiniu_Error err;	
	Qiniu_Rio_PutExtra extra;	
	int retry_times = 0;
	Qiniu_Zero(extra);	

	retry_times = 0;

	db_msg("remote_network resumable_upload \n");
	
retry_uploading:
	//memset(&err, 0, sizeof(err));
	err = Qiniu_Rio_PutFile(client, NULL, uptoken, key, localFile, &extra);
	db_msg("remote_network resumable_upload err.code=%d message=%s \n", err.code, err.message);
	if(err.code != 200)
	{
		retry_times++;
		if(retry_times < RETRY_TIMES_FOR_STORAGE_SERVER)
		{
			//db_msg("remote_network resumable_upload failed! try again! retry_times=%s \n", retry_times);
			goto retry_uploading;
		}
		else
		{
			//db_msg("remote_network resumable_upload failed!  \n");
		}
	}
	return err.code;
}

char* upload_to_storage_server(char* pFileName, REMOTE_NETWORK_DATA_TYPE data_type, char* storage_token, char* storage_domain, char* pStorageUrl)
{
	Qiniu_Client client;
	char local_file_name[64+1];
	char ch = 0;
	int i=0;
	char local_file_name_reverse[64+1];
	int len = 0;
	int err_code = 200;
	
	if((NULL == pFileName) || (NULL == pStorageUrl))
	{
		db_msg("remote_network upload_to_storage_server pFileName or pStorageUrl is NULL");
		return NULL;
	}

	memset(local_file_name, 0, sizeof(local_file_name));
	ch = pFileName[strlen(pFileName)-1];
	i=0;
	while(ch != '\/' && ch != '\0')
	{
		local_file_name[i] = ch;
		i++;
		ch = pFileName[strlen(pFileName)-1-i];
	}

	memset(local_file_name_reverse, 0, sizeof(local_file_name_reverse));
	len = strlen(local_file_name);
	for(i=0; i<len; i++)
	{
		local_file_name_reverse[i] = local_file_name[len-1-i];
	}

	db_msg("remote_network upload_to_storage_server pFileName=%s storage_token=%s storage_domain=%s local_file_name=%s local_file_name_reverse=%s\n", pFileName, storage_token, storage_domain, local_file_name, local_file_name_reverse);
	
	Qiniu_Global_Init(-1); 
	Qiniu_Client_InitNoAuth(&client, 1024);

	//err_code = resumable_upload(&client, storage_token, NULL, pFileName);
	err_code = simple_upload(&client, storage_token, local_file_name_reverse, pFileName);

	Qiniu_Client_Cleanup(&client); 
	Qiniu_Global_Cleanup(); 

	if(200 == err_code)
	{
		snprintf(pStorageUrl, WRTC_MAX_FILE_NAME_LEN, "http://%s/%s", storage_domain, local_file_name_reverse);
	}
	db_msg("remote_network upload_to_storage_server pStorageUrl=%s \n", pStorageUrl);
	return NULL;
}

void notify_is_uploading(void)
{
	Json::Value root;
	Json::FastWriter writer;
	Json::Value json_rsp;
	
	db_msg("remote_network notify_is_uploading  \n");

	json_rsp["token"]= remote_network_get_token();
	
	json_rsp["status"] = REMOTE_NETWORK_STATUS_UPLOADING;
	
	string path = writer.write(/*root*/json_rsp);
	pthread_mutex_lock(&g_curl_mutex);
	remote_network_notify(path.c_str(),path.length(), REOMOTE_NETWORK_CMD_UPDATE_STATUS, NULL);
	pthread_mutex_unlock(&g_curl_mutex);

	return ;
}


void notify_is_recording(void)
{
	Json::Value root;
	Json::FastWriter writer;
	Json::Value json_rsp;
	
	db_msg("remote_network notify_is_recording  \n");

	json_rsp["token"]= remote_network_get_token();
	
	json_rsp["status"] = REMOTE_NETWORK_STATUS_RECORDING;
	
	string path = writer.write(/*root*/json_rsp);
	pthread_mutex_lock(&g_curl_mutex);
	remote_network_notify(path.c_str(),path.length(), REOMOTE_NETWORK_CMD_UPDATE_STATUS, NULL);
	pthread_mutex_unlock(&g_curl_mutex);

	return ;
}

void notify_is_rejecting(void)
{
	Json::Value root;
	Json::FastWriter writer;
	Json::Value json_rsp;
	
	db_msg("remote_network notify_is_rejecting  \n");

	json_rsp["token"]= remote_network_get_token();
	
	json_rsp["status"] = REMOTE_NETWORK_STATUS_REJECTED;
	
	string path = writer.write(/*root*/json_rsp);
	pthread_mutex_lock(&g_curl_mutex);
	remote_network_notify(path.c_str(),path.length(), REOMOTE_NETWORK_CMD_UPDATE_STATUS, NULL);
	pthread_mutex_unlock(&g_curl_mutex);
	release_remote_network_wakelock();

	return ;
}


void deal_remote_network_upload_msg(FWK_MSG* msg)
{

    
    switch(msg->msg_id)
    {	
		case MSG_ID_FWK_CONTROL_3G_REMOTE_UPLOAD_REQ:
			{
				REMOTE_NETWORK_UPLOAD_REQ_T*  info_rsp = (REMOTE_NETWORK_UPLOAD_REQ_T*)msg->para;
				REMOTE_NETWORK_GET_STORAGE_TOKEN_REQ_T storage_token;
				
				
				db_msg("remote_network MSG_ID_FWK_CONTROL_3G_REMOTE_UPLOAD_REQ filename=%s timestamp=%ld type=%d data_type=%d\n", info_rsp->file_name, info_rsp->timestamp, info_rsp->type, info_rsp->data_type);
				
				memset(&storage_token, 0, sizeof(storage_token));
				strncpy(storage_token.file_name, info_rsp->file_name, WRTC_MAX_FILE_NAME_LEN);
				storage_token.timestamp = info_rsp->timestamp;
				storage_token.type = info_rsp->type;
				storage_token.data_type = info_rsp->data_type;
				fwk_send_message_ext(FWK_MOD_Reserverd1,FWK_MOD_Reserverd1, MSG_ID_FWK_CONTROL_3G_REMOTE_GET_STORAGE_TOKEN_REQ, &storage_token, sizeof(storage_token));
							
			}
			break;
			
		case MSG_ID_FWK_CONTROL_3G_REMOTE_GET_STORAGE_TOKEN_REQ:
			{
				REMOTE_NETWORK_GET_STORAGE_TOKEN_REQ_T*  storage_req = (REMOTE_NETWORK_GET_STORAGE_TOKEN_REQ_T*)msg->para;
				Json::Value root;
				Json::FastWriter writer;
				Json::Value json_rsp;
				REMOTE_NETWORK_STORAGE_TOKEN_INFO_T s_token_info;
				REMOTE_NETWORK_GET_STORAGE_TOKEN_RSP_T w_rsp;

				db_msg("remote_network MSG_ID_FWK_CONTROL_3G_REMOTE_GET_STORAGE_TOKEN_REQ filename=%s timestamp=%ld type=%d data_type=%d \n", storage_req->file_name, storage_req->timestamp, storage_req->type, storage_req->data_type);
				#ifdef REMOTE_3G
				json_rsp["token"]= remote_network_get_token();
				if(REMOTE_NETWORK_TYPE_REALTIME == storage_req->type)
				{
					json_rsp["type"] = REMOTE_NETWORK_GPS_TYPE_NORMAL;
				}
				else if(REMOTE_NETWORK_TYPE_ALARM == storage_req->type)
				{
					json_rsp["type"] = REMOTE_NETWORK_GPS_TYPE_ALARM;
				}
				else
				{
					json_rsp["type"] = "unknown";
				}
				string path = writer.write(/*root*/json_rsp);

				pthread_mutex_lock(&g_curl_mutex);
				memset(&s_token_info, 0, sizeof(s_token_info));
				remote_network_notify(path.c_str(),path.length(),REOMOTE_NETWORK_CMD_GET_STORAGE_TOKEN, &s_token_info);
				pthread_mutex_unlock(&g_curl_mutex);
				
				db_msg("remote_network MSG_ID_FWK_CONTROL_3G_REMOTE_GET_STORAGE_TOKEN_REQ storage_token=%s storage_domain=%s result=%d \n", s_token_info.storage_token, s_token_info.storage_domain, s_token_info.result);
				
				memset(&w_rsp, 0, sizeof(w_rsp));
				strncpy(w_rsp.file_name, storage_req->file_name, WRTC_MAX_FILE_NAME_LEN);
				w_rsp.timestamp = storage_req->timestamp;
				w_rsp.type = storage_req->type;
				w_rsp.data_type = storage_req->data_type;
				strncpy(w_rsp.storage_token, s_token_info.storage_token,WRTC_MAX_FILE_NAME_LEN);
				strncpy(w_rsp.storage_domain, s_token_info.storage_domain, WRTC_MAX_FILE_NAME_LEN);
				w_rsp.result = s_token_info.result;
				if(strlen(s_token_info.storage_token) == 0)
				{
					w_rsp.result = -1;
				}

				fwk_send_message_ext(FWK_MOD_Reserverd1,FWK_MOD_Reserverd1, MSG_ID_FWK_CONTROL_3G_REMOTE_GET_STORAGE_TOKEN_RSP, &w_rsp, sizeof(w_rsp));
				#endif
			}
			break;
			
		case MSG_ID_FWK_CONTROL_3G_REMOTE_GET_STORAGE_TOKEN_RSP:
			{
				REMOTE_NETWORK_GET_STORAGE_TOKEN_RSP_T*  info_rsp = (REMOTE_NETWORK_GET_STORAGE_TOKEN_RSP_T*)msg->para;
				char storage_server_url[WRTC_MAX_FILE_NAME_LEN+1]={0};
				REMOTE_NETWORK_UPDATE_STATUS_IND_T status_ind_req;
				REMOTE_NETWORK_UPLOAD_STORAGE_DONE_T upload_done_ind;

				db_msg("remote_network MSG_ID_FWK_CONTROL_3G_REMOTE_GET_STORAGE_TOKEN_RSP filename=%s timestamp=%ld type=%d data_type=%d\n", info_rsp->file_name, info_rsp->timestamp, info_rsp->type, info_rsp->data_type);
				db_msg("remote_network MSG_ID_FWK_CONTROL_3G_REMOTE_GET_STORAGE_TOKEN_RSP storage_token=%s storage_domain=%s result=%d \n", info_rsp->storage_token, info_rsp->storage_domain, info_rsp->result);

				if(0 == info_rsp->result)
				{
					if(REMOTE_NETWORK_TYPE_REALTIME == info_rsp->type)
					{
						notify_is_uploading();
					}
					
					pthread_mutex_lock(&g_curl_mutex);
					memset(storage_server_url, 0x0, sizeof(storage_server_url));
					upload_to_storage_server(info_rsp->file_name, info_rsp->data_type, info_rsp->storage_token,info_rsp->storage_domain, storage_server_url);
					pthread_mutex_unlock(&g_curl_mutex);
					db_msg("remote_network MSG_ID_FWK_CONTROL_3G_REMOTE_GET_STORAGE_TOKEN_RSP storage_server_url=%s \n", storage_server_url);

					memset(&upload_done_ind, 0, sizeof(upload_done_ind));
					
					if(strlen(storage_server_url) > 0)
					{
						upload_done_ind.result = 0;

						strncpy(upload_done_ind.storage_upload_url, storage_server_url, WRTC_MAX_FILE_NAME_LEN);
						
						strncpy(upload_done_ind.file_name, info_rsp->file_name,WRTC_MAX_FILE_NAME_LEN);
						upload_done_ind.timestamp = info_rsp->timestamp;
						upload_done_ind.type = info_rsp->type;
						upload_done_ind.data_type = info_rsp->data_type;
						strncpy(upload_done_ind.storage_token, info_rsp->storage_token, WRTC_MAX_FILE_NAME_LEN);
						strncpy(upload_done_ind.storage_domain, info_rsp->storage_domain, WRTC_MAX_FILE_NAME_LEN);
						fwk_send_message_ext(FWK_MOD_Reserverd1,FWK_MOD_Reserverd1, MSG_ID_FWK_CONTROL_3G_REMOTE_UPLOAD_DONE_IND, &upload_done_ind, sizeof(upload_done_ind));
					}
					else
					{
						upload_done_ind.result = -1;

						if(REMOTE_NETWORK_TYPE_REALTIME == info_rsp->type)
						{
							#if 1
							notify_is_rejecting();
							#else
							memset(&status_ind_req, 0x0, sizeof(status_ind_req));
							status_ind_req.type = REMOTE_STATUS_TYPE_REJECTED;
							fwk_send_message_ext(FWK_MOD_Reserverd1,FWK_MOD_Reserverd1, MSG_ID_FWK_CONTROL_3G_REMOTE_STATUS_IND, &status_ind_req, sizeof(status_ind_req));
							#endif
						}
					}
					
					
				}
				else
				{
					if(REMOTE_NETWORK_TYPE_REALTIME == info_rsp->type)
					{
						#if 1
						notify_is_rejecting();
						#else
						memset(&status_ind_req, 0x0, sizeof(status_ind_req));
						status_ind_req.type = REMOTE_STATUS_TYPE_REJECTED;
						fwk_send_message_ext(FWK_MOD_Reserverd1,FWK_MOD_Reserverd1, MSG_ID_FWK_CONTROL_3G_REMOTE_STATUS_IND, &status_ind_req, sizeof(status_ind_req));
						#endif
					}
				}
			}
			break;

		
			case MSG_ID_FWK_CONTROL_3G_REMOTE_UPLOAD_DONE_IND:
			{
				REMOTE_NETWORK_UPLOAD_STORAGE_DONE_T*  upload_done_ind = (REMOTE_NETWORK_UPLOAD_STORAGE_DONE_T*)msg->para;
				Json::Value root;
				Json::FastWriter writer;
				Json::Value json_rsp;
				char LongitudeStr[WRTC_MAX_VALUE_LEN+1]={0};
				char LatitudeStr[WRTC_MAX_VALUE_LEN+1]={0};
				char itimeBuf[WRTC_MAX_VALUE_LEN*2+1];
				
				db_msg("remote_network MSG_ID_FWK_CONTROL_3G_REMOTE_UPLOAD_DONE_IND pStorageUrl=%s type=%d timestamp=%ld  data_type=%d \n", upload_done_ind->storage_upload_url, upload_done_ind->type, upload_done_ind->timestamp, upload_done_ind->data_type);

				memset(LongitudeStr, 0, sizeof(LongitudeStr));
				snprintf(LongitudeStr, WRTC_MAX_VALUE_LEN, "%.14lf", getLongitude());

				memset(LatitudeStr, 0, sizeof(LatitudeStr));
				snprintf(LatitudeStr, WRTC_MAX_VALUE_LEN, "%.14lf", getLatitude());

				db_msg("remote_network MSG_ID_FWK_CONTROL_3G_REMOTE_UPLOAD_DONE_IND latitude=%s longitude=%s\n", LatitudeStr, LongitudeStr);

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
				
				json_rsp["qiniuUrl"] = upload_done_ind->storage_upload_url;

				
				memset(itimeBuf, 0, sizeof(itimeBuf));
				snprintf(itimeBuf, sizeof(itimeBuf), "%ld000", upload_done_ind->timestamp);
				
				if(REMOTE_NETWORK_TYPE_REALTIME == upload_done_ind->type)
				{
					json_rsp["type"] = REMOTE_NETWORK_GPS_TYPE_NORMAL;
					json_rsp["timestamp"] = itimeBuf;
				}
				else if(REMOTE_NETWORK_TYPE_ALARM == upload_done_ind->type)
				{
					json_rsp["type"] = REMOTE_NETWORK_GPS_TYPE_ALARM;
					json_rsp["timestamp"] = itimeBuf;
				}
				else
				{
					json_rsp["type"] = "unknown";
					json_rsp["timestamp"] = itimeBuf;
				}

				if(REMOTE_NETWORK_DATA_TYPE_VIDEO == upload_done_ind->data_type)
				{
					json_rsp["dataType"] = "VIDEO";
				}
				else if(REMOTE_NETWORK_DATA_TYPE_IMG== upload_done_ind->data_type)
				{
					json_rsp["dataType"] = "IMG";
				}
				else
				{
					json_rsp["dataType"] = "Unknown";
				}

				//root.append(json_rsp);
				string path = writer.write(/*root*/json_rsp);
				
				
				db_msg("remote_network MSG_ID_FWK_CONTROL_3G_REMOTE_UPLOAD_DONE_IND path=%s length=%d ",  path.c_str(), path.length());
				
				pthread_mutex_lock(&g_curl_mutex);
				remote_network_notify(path.c_str(),path.length(), REOMOTE_NETWORK_CMD_UPLOAD_DONE, NULL);
				pthread_mutex_unlock(&g_curl_mutex);
				release_remote_network_wakelock();

				
			}
			break;

		case MSG_ID_FWK_CONTROL_3G_REMOTE_STATUS_IND:
			{
				REMOTE_NETWORK_UPDATE_STATUS_IND_T*  info_rsp = (REMOTE_NETWORK_UPDATE_STATUS_IND_T*)msg->para;
				Json::Value root;
				Json::FastWriter writer;
				Json::Value json_rsp;
				
				db_msg("remote_network MSG_ID_FWK_CONTROL_3G_REMOTE_STATUS_IND type=%d \n", info_rsp->type);
				#ifdef REMOTE_3G
				json_rsp["token"]= remote_network_get_token();
				if(REMOTE_STATUS_TYPE_RECORDING == info_rsp->type)
				{
					json_rsp["status"] = REMOTE_NETWORK_STATUS_RECORDING;
				}
				else if(REMOTE_STATUS_TYPE_UPLOADING == info_rsp->type)
				{
					json_rsp["status"] = REMOTE_NETWORK_STATUS_UPLOADING;
				}
				else if(REMOTE_STATUS_TYPE_REJECTED == info_rsp->type)
				{
					json_rsp["status"] = REMOTE_NETWORK_STATUS_REJECTED;
				}
				else
				{
					json_rsp["status"] = "unknown";
				}
				string path = writer.write(/*root*/json_rsp);
				pthread_mutex_lock(&g_curl_mutex);
				remote_network_notify(path.c_str(),path.length(), REOMOTE_NETWORK_CMD_UPDATE_STATUS, NULL);
				pthread_mutex_unlock(&g_curl_mutex);
				if(REMOTE_STATUS_TYPE_REJECTED == info_rsp->type)
				{
					release_remote_network_wakelock();
				}
				#endif
			}
			break;

		case MSG_ID_FWK_CONTROL_3G_REMOTE_NOTIFY_IND:
			{
				REMOTE_NETWORK_NOTIFY_T*  info_rsp = (REMOTE_NETWORK_NOTIFY_T*)msg->para;

				db_msg("remote_network MSG_ID_FWK_CONTROL_3G_REMOTE_NOTIFY_IND  buffer=%s type=%d \n", info_rsp->buffer, info_rsp->type);
				if(strlen(info_rsp->buffer) > 0)
				{
					pthread_mutex_lock(&g_curl_mutex);
					remote_network_notify(info_rsp->buffer,strlen(info_rsp->buffer), info_rsp->type, NULL);
					pthread_mutex_unlock(&g_curl_mutex);
				}

				if((info_rsp->type == REOMOTE_NETWORK_CMD_LOCATION) || (info_rsp->type == REOMOTE_NETWORK_CMD_UPLOAD_DONE))
				{
					release_remote_network_wakelock();
				}
			}
			break;
			

    	default :
        {
			db_msg("fwk_msg Unknow message to remote_network_upload \n");
    	}
        break;
		
    
    }


	return;
}


void* remtoe_network_upload_handler(void* p_arg)
{
    FWK_MSG *msg;
	int ret;


    while (1)
    {
        msg = fwk_get_message(remote_network_upload_queue, MSG_GET_WAIT_ROREVER, &ret);
        if((ret <0)||(msg == NULL))
        {
            //fwk_debug("get message NULL\n");
			db_msg("fwk_msg get message remote_network_upload_queue NULL\n");
            continue;
        }
        else
        {
            deal_remote_network_upload_msg(msg);
            fwk_free_message(&msg);
        }
    }

	return NULL;
}


int remote_network_upload_init(void)
{
	pthread_t comtid;
    int err;
	
    remote_network_upload_queue = fwk_create_queue(FWK_MOD_Reserverd1);
    if (remote_network_upload_queue == -1)
    {
        db_error("remote_network unable to create queue for upload \n");
        return -1;
    }

    

    err = pthread_create(&comtid,NULL,remtoe_network_upload_handler,NULL);
    if(err != 0)
    {
        db_error("remote_network create thread for remote_network_upload failed \n");
        return -1;
    }

    return 0;
}


