#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <strings.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <pthread.h>
#include <termio.h>
#include <errno.h>
#include <dirent.h>
#include <arpa/inet.h>
//#include <system.h>

#include <gps/GpsController.h>
#include <cellular/CellularController.h>
#include <curl.h>
#include <string>
#include <exception>
#include <linux/tcp.h>



#undef LOG_TAG
#define LOG_TAG "remote_network"
#include "debug.h"

#include "fwk_gl_api.h"
#include "fwk_gl_def.h"
#include "fwk_gl_msg.h"

#include <cutils/properties.h>

using namespace std;
#include <string>
#include <json/json.h>


#define   HOSTLEN  (256)
#define	  BACKLOG  (1)
#define 	REMOTE_BUFSIZ		(1024)
#define		RETRY_TIMES			(20)
#define REMOTE_TOKEN_LEN		(512)
#define REMOTE_RECEIVE_BUF		(2048*3)

static FILE	*sock_fpi = NULL;
static FILE	*sock_fpo = NULL;
static int sock_fd = -1;

#define  REMOTE_SERVER_IP				("60.195.250.225")
#define  REMOTE_SERVER_PORT				(8888)
#define  REMOTE_SERVER_DOMAIN_NAME		("www.example.com")

#define REMOTE_NETWORK_SOLUTION_PROVIDER		("Rockchip")


#define  REMOTE_SERVER_POST_URL_CMD_LOCATION		("http://testcarnet.timanetwork.com/tcn-tg/m/telematics/tg/uploadGPS")
#define  REMOTE_SERVER_POST_URL_CMD_PROFILE			("http://testcarnet.timanetwork.com/tcn-tg/m/telematics/tg/getProfile")
#define  REMOTE_SERVER_POST_URL_CMD_UPDATE_STATUS	("http://testcarnet.timanetwork.com/tcn-tg/m/telematics/tg/updateTaskStatus")
#define  REMOTE_SERVER_POST_URL_CMD_GET_STORAGE_TOKEN	("http://testcarnet.timanetwork.com/tcn-tg/m/telematics/tg/getQiNiuToken")
#define  REMOTE_SERVER_POST_URL_CMD_SEND_UPLOAD_DONE	("http://testcarnet.timanetwork.com/tcn-tg/m/telematics/tg/sendQiNiuInfo")




static char gConnectID[PROPERTY_VALUE_MAX] = {0};

static char gToken[REMOTE_TOKEN_LEN+1]={0};
static int gHeartBeatFrequence = 0;
static int gDrivingCurbsUpdatedTime = 0;
static char recvBuf[REMOTE_RECEIVE_BUF+1]={0};
static char gImsi[REMOTE_TOKEN_LEN+1]={0};
#define REMOTE_IMSI_RETRY		(6)

#define REMOTE_WAKEUP				("wakeup")
#define REMOTE_WAKE_LOCK_NAME		("remote_network")
pthread_mutex_t wake_lock_flag_mutex = PTHREAD_MUTEX_INITIALIZER;
bool wake_lock_flag = false;


extern int getProfileFromServer(void);
extern int remote_network_upload_init(void);
extern pthread_mutex_t g_curl_mutex;

char* remote_network_get_token(void)
{
	return gToken;
}

void acqurie_remote_network_wakelock(void)
{
	pthread_mutex_lock(&wake_lock_flag_mutex);
	if(!wake_lock_flag)
	{
		acquire_wake_lock(PARTIAL_WAKE_LOCK, REMOTE_WAKE_LOCK_NAME);
		wake_lock_flag = 1;
	}
	pthread_mutex_unlock(&wake_lock_flag_mutex);
	db_msg("acqurie_remote_network_wakelock wake_lock_flag=%d \n", wake_lock_flag);
	
}

void release_remote_network_wakelock(void)
{
	pthread_mutex_lock(&wake_lock_flag_mutex);
	if(wake_lock_flag)
	{
		release_wake_lock(REMOTE_WAKE_LOCK_NAME);
		wake_lock_flag = 0;
	}
	pthread_mutex_unlock(&wake_lock_flag_mutex);
	db_msg("release_remote_network_wakelock wake_lock_flag=%d \n", wake_lock_flag);
}


char* get_id_for_remote(void)
{
	char* pImei = NULL;
	
	if(strlen(gConnectID) > 0)
	{
		return gConnectID;
	}
retry_imei:
	pImei = getImei();
	if(strlen(pImei) == 0 || strcmp(pImei,"000000000000000") == 0)
	{
		db_msg("remote_network don't get imei, sleep some time and try again! \n");
		sleep(5);
		goto retry_imei;
	}
	else
	{
		strcpy(gConnectID, pImei);
	}
	
	db_msg("remote_network gConnectID=%s \n", gConnectID);
	return gConnectID;
	
	//return "123123123";
}



char* get_imsi_for_remote(void)
{
	char* pImsi = NULL;
	static int retry_times = 0;
	
	if(strlen(gImsi) > 0)
	{
		return gImsi;
	}

	retry_times = 0;
retry_imsi:
	pImsi = getImsi();
	if(strlen(pImsi) == 0 || strcmp(pImsi,"unknown") == 0)
	{
		
		retry_times++;
		if(retry_times > REMOTE_IMSI_RETRY)
		{
			db_msg("remote_network can not get IMSI, make sure you have SIM card insert! \n");
		}
		else
		{
			db_msg("remote_network don't get imsi, sleep some time and try again! \n");
			sleep(5);
			goto retry_imsi;
		}
	}
	else
	{
		strcpy(gImsi, pImsi);
	}
	
	db_msg("remote_network gImsi=%s \n", gImsi);
	return gImsi;
	
}


static int init_remote_network_param(void)
{
	db_msg("remote_network init_remote_network_param \n");
	
	memset(gToken, 0x0, sizeof(gToken));
	gHeartBeatFrequence = 0;
	gDrivingCurbsUpdatedTime = 0;

	memset(gConnectID, 0x0, PROPERTY_VALUE_MAX);
	get_id_for_remote();

	memset(gImsi, 0x0, sizeof(gImsi));
	get_imsi_for_remote();

	wake_lock_flag = false;
	return 0;
}


static int connect_to_server(char *host, int portnum, bool is_domain)
{
	int sock;
	struct sockaddr_in  servadd;        /* the number to call */
	struct hostent      *hp;            /* used to get number */
	static int retry = 0;
	int one = 1;

	
	db_msg("remote_network host=%s portnum=%d \n", host, portnum);

	sock = socket( AF_INET, SOCK_STREAM, 0 );    /* get a line   */
	if ( sock == -1 ) 
	{
		db_msg("remote_network create socket failed! \n");
		return -1;
	}

	retry = 0;
	
retry_get_host:
	bzero( &servadd, sizeof(servadd) );     /* zero the address     */
	if(is_domain)
	{
		hp = gethostbyname( host );             /* lookup host's ip #   */
		if (hp == NULL) 
		{
			db_msg("remote_network gethostbyname return NULL! \n");
			if(retry < RETRY_TIMES)
			{
				retry++;
				db_msg("remote_network gethostbyname failed, sleep some time and retry again\n");
				sleep(5);
				goto retry_get_host;
			}
			db_msg("remote_network gethostbyname failed!, return! \n");
	    	return -1;
		}
		bcopy(hp->h_addr, (struct sockaddr *)&servadd.sin_addr, hp->h_length);
	}
	else
	{
		if(!inet_aton(host, &servadd.sin_addr))
		{
			db_msg("remote_network inet_aton failed! \n");
			return -1;
		}
	}

	
	servadd.sin_port = htons(portnum);      /* fill in port number  */
	servadd.sin_family = AF_INET ;          /* fill in socket type  */
	db_msg("remote_network server_addr=%s \n", inet_ntoa(servadd.sin_addr));

	
	if(setsockopt(sock, SOL_TCP, TCP_NODELAY, &one, sizeof(one)) != 0) {
		db_msg("remote_network set TCP_NODELAY Failed!\n");
	}

	retry = 0;
retry_connect:
	if ( connect(sock,(struct sockaddr *)&servadd, sizeof(servadd)) !=0)
	{
		if(retry < RETRY_TIMES)
		{
			retry++;
			db_msg("remote_network connect failed, sleep some time and retry again\n");
			sleep(5);
			goto retry_connect;
		}
		db_msg("remote_network connect to server failed! \n");
	    return -1;
	}

	return sock;
}


static void* remote_network_handler(void* p_arg)
{
	int ret;
	int portnum = 0;
	//char host[HOSTLEN+1];
	char    buf[REMOTE_BUFSIZ+1];       /* from server            */

	init_remote_network_param();
	getProfileFromServer();

	//memset(host, 0, sizeof(host));
	/*init host and port*/
	portnum = REMOTE_SERVER_PORT; 
	//strncpy(host,"www.baidu.com", HOSTLEN); 

retry_connect_to_server:	
	sock_fd = connect_to_server(REMOTE_SERVER_IP, portnum, false);
	if(-1 == sock_fd)
	{
		db_msg("remote_network: connection failed! \n");
		return NULL;
	}
	if( (sock_fpi = fdopen(sock_fd,"r")) == NULL )
	{
		db_msg("remote_network fdopen reading failed! \n");
		close(sock_fd);
		sock_fd = -1;
		return NULL;
	}

	if ( (sock_fpo = fdopen(sock_fd,"w")) == NULL )	
	{
		db_msg("remote_network fdopen writing failed! \n");
		close(sock_fd);
		sock_fd = -1;
		
		fclose(sock_fpi);
		sock_fpi = NULL;
		return NULL;
	}

	memset(buf, 0x0, sizeof(buf));
	snprintf(buf, sizeof(buf),"{\"%s\":\"%s\",\"%s\":\"%s\",\"%s\":\"%s\"}\n", REMOTE_NETWORK_S_ID, get_id_for_remote(),REMOTE_NETWORK_S_TYPE, REMOTE_NETWORK_S_TYPE_VALUE,"icChip","ROCK");
	db_msg("remote_network say hello to server, buf=%s len=%d", buf, strlen(buf));
	write(sock_fd, buf, strlen(buf));
	//fputs(buf, sock_fpo);
	//fflush(sock_fpo);

    while (1)
    {
    	if ( fgets(buf, REMOTE_BUFSIZ, sock_fpi) == NULL )
		{
			db_msg("remote_network reading from server is NULLL, retry_connect_to_server \n");
			sleep(5);
			
			
			fclose(sock_fpo);
			sock_fpo = NULL;

			fclose(sock_fpi);
			sock_fpi = NULL;

			close(sock_fd);
			sock_fd = -1;
			
			goto retry_connect_to_server;
		}
		if(strstr(buf,REMOTE_WAKEUP) != NULL)
		{
			acqurie_remote_network_wakelock();
			db_msg("remote_network wakeup by server! \n");
		}
		db_msg("remote_network buf=%s len=%d\n", buf, strlen(buf));
		fwk_send_message_ext(FWK_MOD_VIEW, FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_WRTC_CALL_REQ, buf, strlen(buf));
       
    }

	close(sock_fd);
	sock_fd = -1;
		
	fclose(sock_fpi);
	sock_fpi = NULL;

	fclose(sock_fpo);
	sock_fpo = NULL;
	
	return NULL;
}

size_t write_recv_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
	db_msg("remote_network write_recv_data stream=%d ptr=%d REMOTE_RECEIVE_BUF=%d ", strlen((char *)stream), strlen((char *)ptr), REMOTE_RECEIVE_BUF);
	if((strlen((char *)stream) + strlen((char *)ptr)) > REMOTE_RECEIVE_BUF)
	{
		db_msg("remote_network write_recv_data exceed the buffer! \n");
		return 0;
	}
	strcat((char *)stream, (char *)ptr);
	return size*nmemb;
}

int parse_get_profile_response(char* pbuffer, int len)
{
	const char *pbuf = (const char*)pbuffer;
	Json::Reader reader;
	Json::Value root;
	
	db_msg("remote_netowrk parse_get_profile_response pbuf=%s \n", pbuf);	
	if(reader.parse(pbuf,root))
	{
		const char* pToken=root["token"].asCString();
		db_msg("remote_netowrk parse_get_profile_response token=%s \n", pToken);
		
		gHeartBeatFrequence = root["heartBeatFrequence"].asInt();
		db_msg("remote_netowrk parse_get_profile_response gHeartBeatFrequence=%d \n", gHeartBeatFrequence);
		
		//gDrivingCurbsUpdatedTime = root["drivingCurbsUpdatedTime"].asInt();
		//db_msg("remote_netowrk parse_get_profile_response gDrivingCurbsUpdatedTime=%d \n", gDrivingCurbsUpdatedTime);
		
		db_msg("remote_network parse_get_profile_response pToken=%s gHeartBeatFrequence=%d gDrivingCurbsUpdatedTime=%d \n", pToken, gHeartBeatFrequence, gDrivingCurbsUpdatedTime);
		if(pToken != NULL)
		{
			memset(gToken, 0x0, sizeof(gToken));
			strncpy(gToken, pToken, REMOTE_TOKEN_LEN);
		}
	}

	return 0;
}

int parse_get_storage_token_info(char* pbuffer, int len, REMOTE_NETWORK_STORAGE_TOKEN_INFO_T *pSToken)
{
	const char *pbuf = (const char*)pbuffer;
	Json::Reader reader;
	Json::Value root;
	
	db_msg("remote_netowrk parse_get_storage_token_info pbuf=%s \n", pbuf);
	if(NULL == pSToken)
	{
		return -1;
	}
	
	if(reader.parse(pbuf,root))
	{
		const char* pQiNiuToken=root["qiniuToken"].asCString();
		const char* pQiNiuDomain = root["domain"].asCString();
		const char* pResult = root["result"].asCString();
		
		
		db_msg("remote_network parse_get_storage_token_info pQiNiuToken=%s pQiNiuDomain=%s \n", pQiNiuToken, pQiNiuDomain);
		if(pQiNiuToken != NULL)
		{
			strncpy(pSToken->storage_token, pQiNiuToken, WRTC_MAX_FILE_NAME_LEN);
		}

		if(pQiNiuDomain != NULL)
		{
			strncpy(pSToken->storage_domain, pQiNiuDomain, WRTC_MAX_FILE_NAME_LEN);
		}

		if(pResult != NULL)
		{
			if(strcasecmp(pResult, "F") == 0)
			{
				pSToken->result = -1;
			}
		}
	}

	return 0;
}




int remote_network_notify(const char* pbuf, int len, REOMOTE_NETWORK_CMD_TYPE type, void *pSToken)
{
	//char buffer[REMOTE_BUFSIZ+1];
	char* pPostUrl = NULL;
	CURL *pCurl = NULL;
	CURLcode res;
	int ret = 0;
	
	if(NULL == pbuf)
	{
		return -1;
	}

#if 0
	if(type != REOMOTE_NETWORK_CMD_GET_PROFILE)
	{
		if(NULL == sock_fpo)
		{
			db_msg("remote_network server is not connected!  \n");
			return -1;
		}
	}
#endif

#if 0
	memset(buffer, 0, sizeof(buffer));
	if(len > REMOTE_BUFSIZ)
	{
		db_error("remote_network local buffer is too small! \n");
	}
	strncpy(buffer, pbuf, REMOTE_BUFSIZ);
	buffer[len] = '\n';
#endif

	db_msg("remote_network notify server buffer=%s type=%d \n", pbuf, type);
	switch(type)
	{
		case REOMOTE_NETWORK_CMD_LOCATION:
			{
				db_msg("remote_netowrk REOMOTE_NETWORK_CMD_LOCATION \n");
				pPostUrl = REMOTE_SERVER_POST_URL_CMD_LOCATION;
			}
			break;
		case REOMOTE_NETWORK_CMD_GET_PROFILE:
			{
				db_msg("remote_netowrk REOMOTE_NETWORK_CMD_GET_PROFILE \n");
				pPostUrl = REMOTE_SERVER_POST_URL_CMD_PROFILE;
			}
			break;
		case REOMOTE_NETWORK_CMD_UPDATE_STATUS:
			{
				db_msg("remote_netowrk REOMOTE_NETWORK_CMD_UPDATE_STATUS \n");
				pPostUrl = REMOTE_SERVER_POST_URL_CMD_UPDATE_STATUS;
			}
			break;
		case REOMOTE_NETWORK_CMD_GET_STORAGE_TOKEN:
			{
				db_msg("remote_netowrk REOMOTE_NETWORK_CMD_GET_STORAGE_TOKEN \n");
				pPostUrl = REMOTE_SERVER_POST_URL_CMD_GET_STORAGE_TOKEN;
			}
			break;
		case REOMOTE_NETWORK_CMD_UPLOAD_DONE:
			{
				db_msg("remote_netowrk REOMOTE_NETWORK_CMD_UPLOAD_DONE \n");
				pPostUrl = REMOTE_SERVER_POST_URL_CMD_SEND_UPLOAD_DONE;
			}
			break;
		

		default:
			pPostUrl = NULL;
			break;
	}


	ret = 0;
	if(pPostUrl != NULL)
	{
		curl_global_init(CURL_GLOBAL_ALL);

		memset(recvBuf, 0x0, sizeof(recvBuf));
		
		pCurl = curl_easy_init();
		if (NULL != pCurl) 
		{
			curl_easy_setopt(pCurl, CURLOPT_TIMEOUT, 30);


			curl_easy_setopt(pCurl, CURLOPT_URL, pPostUrl);
			

			curl_slist *plist = curl_slist_append(NULL, 
				"Content-Type:application/json;charset=UTF-8");
			curl_easy_setopt(pCurl, CURLOPT_HTTPHEADER, plist);

			curl_easy_setopt(pCurl, CURLOPT_POSTFIELDS, pbuf);

			if((REOMOTE_NETWORK_CMD_GET_PROFILE == type) || (REOMOTE_NETWORK_CMD_GET_STORAGE_TOKEN == type))
			{
				curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, write_recv_data);
				curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, recvBuf);
			}

			res = curl_easy_perform(pCurl);
			// Check for errors
			if (res != CURLE_OK) 
			{
				ret = -1;
				db_msg("remote_netowrk curl_easy_perform() failed:%s\n", curl_easy_strerror(res));
				if(REOMOTE_NETWORK_CMD_GET_STORAGE_TOKEN == type)
				{
					if(pSToken != NULL)
					{
						REMOTE_NETWORK_STORAGE_TOKEN_INFO_T* pStorageInfo = (REMOTE_NETWORK_STORAGE_TOKEN_INFO_T*)pSToken;
						pStorageInfo->result = -1;
					}
				}
			}
			else
			{
				ret = 0;
				db_msg("remote_netowrk curl_easy_perform() is Okay! \n");
				if(REOMOTE_NETWORK_CMD_GET_PROFILE == type)
				{
					parse_get_profile_response(recvBuf, strlen(recvBuf));
				}
				else if(REOMOTE_NETWORK_CMD_GET_STORAGE_TOKEN == type)
				{
					if(pSToken != NULL)
					{
						REMOTE_NETWORK_STORAGE_TOKEN_INFO_T* pStorageInfo = (REMOTE_NETWORK_STORAGE_TOKEN_INFO_T*)pSToken;
						parse_get_storage_token_info(recvBuf, strlen(recvBuf), pStorageInfo);
					}
				}
				
			}
			// always cleanup
			curl_easy_cleanup(pCurl);
		}
		curl_global_cleanup();
	}
	
	
#if 1
	//fputs(pbuf, sock_fpo);
	//fputs("\n", sock_fpo);
	//fflush(sock_fpo);
#else
	if(write(sock_fd, pbuf, len) == -1)
	{
		db_msg("remote_network write pbuf error! \n");
	}
	if(write(sock_fd, "\n", 1) == -1)
	{
		db_msg("remote_network write  error! \n");
	}
#endif
	

	return ret;
}

char* getSolutionProvider(void)
{
	return REMOTE_NETWORK_SOLUTION_PROVIDER;
}

int getProfileFromServer(void)
{
	char    buf[REMOTE_BUFSIZ+1]; 
	char firmwareVersion[PROPERTY_VALUE_MAX] = {'\0'};
	char manufacturer[PROPERTY_VALUE_MAX] = {'\0'};
	int ret = 0;

	db_msg("remote_network getProfileFromServer \n");
	
	memset(firmwareVersion, 0, sizeof(firmwareVersion));
	property_get("ro.product.version",firmwareVersion,"");

	memset(manufacturer, 0, sizeof(manufacturer));
	property_get("ro.product.manufacturer",manufacturer,"");
				
	
	memset(buf, 0x0, sizeof(buf));
	snprintf(buf, sizeof(buf),"{\"%s\":\"%s\",\"%s\":\"%s\",\"%s\":\"%s\",\"%s\":\"%s\",\"%s\":\"%s\",\"%s\":\"%s\",\"%s\":\"%s\",\"%s\":\"%s\"}", "tachographId", get_id_for_remote(), \
								REMOTE_NETWORK_S_TYPE, REMOTE_NETWORK_S_TYPE_VALUE,\
								"tachographOsType","LINUX",\
								"tachographIcChip","ROCK",\
								"firmwareVersion",firmwareVersion,\
								"imsi", get_imsi_for_remote(),
								"tachographManufacturer",/*manufacturer*/"M000",
								"tachographSolutionProvider",/*getSolutionProvider()*/"D000"
								);

retry_get_profile_again:
	db_msg("remote_network getProfileFromServer, buf=%s len=%d", buf, strlen(buf));
	pthread_mutex_lock(&g_curl_mutex);
	ret = remote_network_notify(buf, strlen(buf),REOMOTE_NETWORK_CMD_GET_PROFILE, NULL);
	pthread_mutex_unlock(&g_curl_mutex);

	if(ret != 0)
	{
		db_msg("remote_network sleep some time and get profile again! \n");
		sleep(2);
		goto retry_get_profile_again;
	}
	return 0;
}



int remote_network_init(void)
{
	pthread_t remote_network_id;
    int err;

	db_msg("remote_network id=%s imsi=%s \n", getImei(),  getImsi());
	

    err = pthread_create(&remote_network_id,NULL,remote_network_handler,NULL);
    if(err != 0)
    {
        db_error("remote_network_init create thread for remote network failed \n");
        return -1;
    }

	remote_network_upload_init();

    return 0;
}










