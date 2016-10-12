#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <dirent.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <net/if_arp.h>
#include <cutils/properties.h>
#include <network/net.h>
#include <network/ConnectivityServer.h>
#include <cellular/util.h>
#include <cellular/simIo.h>
#include <cellular/CellularController.h>
#include <hardware_legacy/power.h>
#include <gps/GpsController.h>

#define LOG_TAG "CELLULAR"

bool DEBUG = property_get_bool("persist.cellular.log.enable", false);

#define logd(...) do { if (DEBUG) \
                        __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__); \
		} while(0)

#define loge(...) do { __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__); \
		} while(0)

//#define SMS_TEST
//#define HANGUP_TEST
#define AUTO_ANSWER true

pthread_t s_tid_handle_read;
pthread_attr_t thread_attr;
pthread_t s_tid_handle_radio_init;
pthread_attr_t thread_attr_radio_init;
pthread_t s_tid_handle_ril_timeout;
pthread_attr_t thread_attr_ril_timeout;

bool moblieDataEnable = false;
bool socketConnected = false;
bool radioInit = false;
bool iccidReady = false;
bool imsiReady = false;

CS_STATE cs_state = OUT_OF_SERVICE;
SIM_STATE sim_state = SIMSTATE_ABSENT;
RESPONSE_TYPE response_type;

int rssi = 0;//value:0-31

#ifdef SMS_TEST
int sendSmsTest();
#endif

T_SYS_SEM s_cellular_wait_sem = NULL;

typedef struct {
    int msg;
    char data[1000];
} RIL_Msg;

int socket_id = -1;

#define PHONE_LIST_LENGTH 3

static char *phone_list[PHONE_LIST_LENGTH] = {"10086","10010","059183991906"};

char iccid[21] = {'\0'};
char imsi[16] = {'\0'};
char imei[PROPERTY_VALUE_MAX] = {'\0'};
cell_info cell;

#define CELLULAR_INTERFACE "veth0"

#define RIL_REQUEST_SET_APN_INFO 2000

#define RIL_UNSOL_DATA_CONNECTED 3000
#define RIL_UNSOL_DATA_DISCONNECTED 3001
#define RIL_UNSOL_SMS_SEND_RESULTS 3002
#define RIL_UNSOL_CS_STATE 3003
#define RIL_UNSOL_IMEI 3004
#define RIL_UNSOL_SIM_STATE 3005
#define RIL_UNSOL_APN_CHANGE 3006
#define RIL_UNSOL_CELL_INFO 3007

#define RIL_REQUEST_WAKE_LOCK "ril-request"
#define TIMEOUT_TIME 30000

int getRilImei();
int getRilSimState();
void setCsState(char *data);
void setSimState(char *data);
void setSignalStrength(char *data);
bool checkNetworkState();
void acquireWakelock();
void decrementWakeLock();
void clearWakeLock();
int setTimeout();
int getCurrentCalls();
int currentCallState(call_info *data);
int setCallWaiting();
int requestIccID(int type);
int iccIo(int type, int command, int fileid, int p1, int p2, int p3);
int setSimIoData(char *data);
int getRilImsi();
int updateCellInfo(char *data);
static int wakeLockCount = 0;

pthread_mutex_t ril_wait_sem_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t s_ril_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t s_ril_cond = PTHREAD_COND_INITIALIZER;


static void *handleRilTimeout( void *p_arg){
    int err = 0;

    pthread_cond_signal(&s_ril_cond);
    logd("%s: enter\n",__FUNCTION__);
    pthread_mutex_lock(&s_ril_mutex);
    if(wakeLockCount > 0)
    err = pthread_cond_timeout_np(&s_ril_cond, &s_ril_mutex, TIMEOUT_TIME);
    pthread_mutex_unlock(&s_ril_mutex);
    logd("%s: err = %d\n",__FUNCTION__,err);
    if (err == ETIMEDOUT) {
         logd("%s: TIMEOUT\n",__FUNCTION__);
         clearWakeLock();
    }
    return NULL;
}

int setTimeout() {
    int ret;

    pthread_attr_init (&thread_attr_ril_timeout);
    pthread_attr_setdetachstate(&thread_attr_ril_timeout, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&s_tid_handle_ril_timeout, &thread_attr_ril_timeout, handleRilTimeout, NULL);

    if(ret = -1){
	pthread_attr_destroy(&thread_attr_ril_timeout);
    }
    return 0;
}

void acquireWakelock() {
	pthread_mutex_lock(&ril_wait_sem_lock);
	acquire_wake_lock(PARTIAL_WAKE_LOCK, RIL_REQUEST_WAKE_LOCK);
	wakeLockCount++;
	setTimeout();
	pthread_mutex_unlock(&ril_wait_sem_lock);
	logd("%s: wakeLockCount = %d\n",__FUNCTION__,wakeLockCount);
}

void decrementWakeLock() {
	pthread_mutex_lock(&ril_wait_sem_lock);
	if(wakeLockCount > 1){
		wakeLockCount--;
	}else{
		wakeLockCount = 0;
		pthread_cond_signal(&s_ril_cond);
		release_wake_lock(RIL_REQUEST_WAKE_LOCK);
	}
	pthread_mutex_unlock(&ril_wait_sem_lock);
	logd("%s: wakeLockCount = %d\n",__FUNCTION__,wakeLockCount);
}

void clearWakeLock() {
	pthread_mutex_lock(&ril_wait_sem_lock);
	wakeLockCount = 0;
	release_wake_lock(RIL_REQUEST_WAKE_LOCK);
	pthread_mutex_unlock(&ril_wait_sem_lock);
	logd("%s: wakeLockCount = %d\n",__FUNCTION__,wakeLockCount);
}

const char *
requestToString(int request) {
    switch(request) {
	case RIL_REQUEST_SEND_SMS: return "REQUEST_SEND_SMS";
	case RIL_REQUEST_SMS_ACKNOWLEDGE: return "REQUEST_SMS_ACKNOWLEDGE";
	case RIL_REQUEST_SETUP_DATA_CALL: return "REQUEST_SETUP_DATA_CALL";
	case RIL_REQUEST_DEACTIVATE_DATA_CALL: return "REQUEST_DEACTIVATE_DATA_CALL";
	case RIL_REQUEST_RADIO_POWER: return "REQUEST_RADIO_POWER";
	case RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC: return "REQUEST_SET_NETWORK_SELECTION_AUTOMATIC";
	case RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE: return "REQUEST_SET_PREFERRED_NETWORK_TYPE";
	case RIL_REQUEST_DATA_CALL_LIST: return "REQUEST_DATA_CALL_LIST";
	case RIL_REQUEST_SCREEN_STATE: return "REQUEST_SCREEN_STATE";
	case RIL_REQUEST_GET_SIM_STATUS: return "REQUEST_GET_SIM_STATUS";
	case RIL_REQUEST_GET_IMEI: return "REQUEST_GET_IMEI";
	case RIL_REQUEST_GET_CURRENT_CALLS: return "REQUEST_GET_CURRENT_CALLS";
	case RIL_REQUEST_ANSWER: return "REQUEST_ANSWER";
	case RIL_REQUEST_HANGUP: return "REQUEST_HANGUP";
	case RIL_REQUEST_SET_CALL_WAITING: return "REQUEST_SET_CALL_WAITING";
	case RIL_REQUEST_SIM_IO: return "REQUEST_SIM_IO";
	case RIL_REQUEST_GET_IMSI: return "REQUEST_GET_IMSI";
	case RIL_REQUEST_SET_APN_INFO: return "REQUEST_SET_APN_INFO";
	case RIL_UNSOL_RESPONSE_NEW_SMS: return "UNSOL_NEW_SMS";
	case RIL_UNSOL_SIGNAL_STRENGTH: return "UNSOL_SIGNAL_STRENGTH";
	case RIL_UNSOL_DATA_CONNECTED: return "UNSOL_DATA_CONNECTED";
	case RIL_UNSOL_DATA_DISCONNECTED: return "UNSOL_DATA_DISCONNECTED";
	case RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED: return "UNSOL_VOICE_NETWORK_STATE_CHANGED";
	case RIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED: return "UNSOL_SIM_STATUS_CHANGED";
	case RIL_UNSOL_DATA_CALL_LIST_CHANGED: return "UNSOL_DATA_CALL_LIST_CHANGED";
	case RIL_UNSOL_SMS_SEND_RESULTS: return "UNSOL_SMS_SEND_RESULTS";
	case RIL_UNSOL_CS_STATE: return "UNSOL_CS_STATE";
	case RIL_UNSOL_IMEI: return "UNSOL_IMEI";
	case RIL_UNSOL_SIM_STATE: return "UNSOL_SIM_STATE";
	case RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED: return "UNSOL_CALL_STATE_CHANGED";
	case RIL_UNSOL_APN_CHANGE: return "UNSOL_APN_CHANGE";
	case RIL_UNSOL_CELL_INFO: return "UNSOL_CELL_INFO";
	default: return "<unknown request>";
    }
}

RESPONSE_TYPE getResponseType(int request) {
    switch(request) {
	case RIL_REQUEST_SEND_SMS:
	case RIL_REQUEST_SMS_ACKNOWLEDGE:
	case RIL_REQUEST_SETUP_DATA_CALL:
	case RIL_REQUEST_DEACTIVATE_DATA_CALL:
	case RIL_REQUEST_RADIO_POWER:
	case RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC:
	case RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE:
	case RIL_REQUEST_DATA_CALL_LIST:
	case RIL_REQUEST_SCREEN_STATE:
	case RIL_REQUEST_GET_SIM_STATUS:
	case RIL_REQUEST_GET_IMEI:
	case RIL_REQUEST_GET_CURRENT_CALLS:
	case RIL_REQUEST_ANSWER:
	case RIL_REQUEST_HANGUP:
	case RIL_REQUEST_SET_CALL_WAITING:
	case RIL_REQUEST_SIM_IO:
	case RIL_REQUEST_GET_IMSI:
	case RIL_REQUEST_SET_APN_INFO:
		return RESPONSE_SOLICITED;
	case RIL_UNSOL_RESPONSE_NEW_SMS:
	case RIL_UNSOL_SIGNAL_STRENGTH:
	case RIL_UNSOL_DATA_CONNECTED:
	case RIL_UNSOL_DATA_DISCONNECTED:
	case RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED:
	case RIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED:
	case RIL_UNSOL_DATA_CALL_LIST_CHANGED:
	case RIL_UNSOL_SMS_SEND_RESULTS:
	case RIL_UNSOL_CS_STATE:
	case RIL_UNSOL_IMEI:
	case RIL_UNSOL_SIM_STATE:
	case RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED:
	case RIL_UNSOL_APN_CHANGE:
	case RIL_UNSOL_CELL_INFO:
		return RESPONSE_UNSOLICITED;
	default: return RESPONSE_UNSOLICITED;
    }
}

int sendData(RIL_Msg * Data){
    int len;
    char sendBuf[1024] = {0};

    logd("%s: socket_id = %d\n",__FUNCTION__,socket_id);
    if(socket_id < 0 || socketConnected == false)
	    return -1;
    acquireWakelock();
    logd("%s: >> %s",__FUNCTION__,requestToString(Data->msg));
    memcpy(sendBuf,Data,sizeof(RIL_Msg));
    len = send(socket_id,sendBuf,sizeof(sendBuf),0);
    if(len > 0)
        logd("%s: len = %d\n",__FUNCTION__,len);
    else
	clearWakeLock();

    return 0;
}
extern void agpsCellRequestSetId();

int recvData(RIL_Msg *data){
	netinfo info;
	sms_info smsinfo;
	call_info callinfo;
	logd("%s: << %s",__FUNCTION__,requestToString(data->msg));
	response_type = getResponseType(data->msg);
	switch(data->msg){
		case RIL_UNSOL_RESPONSE_NEW_SMS:
			memset(&smsinfo,0,sizeof(sms_info));
			memcpy(&smsinfo,data->data,sizeof(sms_info));
			receiveSMS(&smsinfo);
			break;
		case RIL_UNSOL_SIGNAL_STRENGTH:
			setSignalStrength(data->data);
			break;
		case RIL_UNSOL_SIM_STATE:
			setSimState(data->data);
			break;
		case RIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED:
		case RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED:
		case RIL_UNSOL_DATA_CALL_LIST_CHANGED:
			if(moblieDataEnable)
				checkNetworkState();
			break;
		case RIL_UNSOL_DATA_CONNECTED:
			memset(&info,0,sizeof(netinfo));
			memcpy(&info,data->data,sizeof(netinfo));
			logd("%s: info->ifname = %s\ninfo->addresses = %s\ninfo->dnses = %s\ninfo->gateways = %s\n",__FUNCTION__,info.ifname,info.addresses,info.dnses,info.gateways);
			set_cellular_route_and_dns(CELLULAR_INTERFACE,&info);
			set_network_state(MODEM, CONNECTED);
			gpsUpdateNetworkState(1, 3, 0, NULL);
			break;
		case RIL_UNSOL_DATA_DISCONNECTED:
			clean_connect_addrs(CELLULAR_INTERFACE);
			set_network_state(MODEM, DISCONNECT);

			gpsUpdateNetworkState(0, 3, 0, NULL);
			break;
		case RIL_UNSOL_SMS_SEND_RESULTS:
			setSmsSendResult(data->data);
			break;
		case RIL_UNSOL_CS_STATE:
			setCsState(data->data);
			break;
		case RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED:
			getCurrentCalls();
			break;
		case RIL_UNSOL_APN_CHANGE:
			reconnectNetwork();
			break;
		case RIL_UNSOL_CELL_INFO:
			updateCellInfo(data->data);
			break;
		case RIL_REQUEST_GET_CURRENT_CALLS:
			memset(&callinfo,0,sizeof(call_info));
			memcpy(&callinfo,data->data,sizeof(call_info));
			currentCallState(&callinfo);
			break;
		case RIL_REQUEST_SIM_IO:
			setSimIoData(data->data);
			break;
		case RIL_REQUEST_GET_IMSI:
			logd("%s: imsi = %s",__FUNCTION__,data->data);
			strcpy(imsi,data->data);
			imsiReady = true;
			agpsCellRequestSetId();
			break;
		default:
			//logd("%s: invalid parameter",__FUNCTION__);
			break;
	}
	if(response_type == RESPONSE_SOLICITED) {
		decrementWakeLock();
	}
	return 0;
}

static void *handleMessage( void *p_arg){
    RIL_Msg data;
    int len;
    int result;
    char buf[BUFSIZ];
    struct sockaddr_in remote_addr;
    memset(&remote_addr,0,sizeof(remote_addr));
    remote_addr.sin_family=AF_INET;
    remote_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    remote_addr.sin_port=htons(7788);
    while(1){
     if((socket_id = socket(PF_INET,SOCK_STREAM,0))<0){
         loge("socket error");
	 sleep(3);
	 continue;
     }

     result = connect(socket_id,(struct sockaddr *)&remote_addr,sizeof(struct sockaddr));
     logd("connect result = %d\n",result);
     if(result < 0){
         loge("Couldn't find rild socket after 3s, continuing to retry ");
	 close(socket_id);
	 socket_id = -1;
	 sleep(3);
	 continue;
     }

     logd("socket_id = %d\n",socket_id);
     socketConnected = true;
     property_set("net.cellular.socket", "on");
     sys_sem_signal(s_cellular_wait_sem);
     while(socket_id > 0){
         while((len=recv(socket_id,buf,BUFSIZ,0))>0)
         {
             memcpy(&data,buf,sizeof(RIL_Msg));
             recvData(&data);
         }
     }
}
       return NULL;
}

bool creatRilSocket(){
    int ret;
    logd("%s\n",__FUNCTION__);
    socketConnected = false;
    property_set("net.cellular.socket", "off");
    s_cellular_wait_sem = sys_sem_new(0);

    pthread_attr_init (&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&s_tid_handle_read, &thread_attr, handleMessage, NULL);
    if(ret = -1){
         pthread_attr_destroy(&thread_attr);
    }
    initRadio();
    return true;
}

int checkSocketState() {
	if (socketConnected == true)
		return 1;
	if (s_cellular_wait_sem != NULL) {
		logd("The socket is not ready, wait.. ");
		sys_sem_wait(s_cellular_wait_sem);
		logd("received s_cellular_wait_sem semaphore");
		sys_sem_free(s_cellular_wait_sem);
		s_cellular_wait_sem = NULL;
	}
	return 0;
}
bool enableCellular(){
	logd("%s\n",__FUNCTION__);
	RIL_Msg data;
	int ret;
	int count = 0;

	moblieDataEnable = true;
	if(radioInit != true)
		return false;
	checkSocketState();
	return checkNetworkState();
}

bool disableCellular(){
	logd("%s\n",__FUNCTION__);
	RIL_Msg data;
	int ret;
	int count = 0;

#ifdef HANGUP_TEST
	callHangup();
	sleep(3);
#endif
#ifdef SMS_TEST
	sendSmsTest();
	sleep(3);
#endif
	checkSocketState();
	moblieDataEnable = false;
	data.msg = RIL_REQUEST_DEACTIVATE_DATA_CALL;
	data.data[0] = 0;

	ret = sendData(&data);
	if(ret < 0)
	    return false;
	else
	    return true;
}

bool checkNetworkState(){
	logd("%s\n",__FUNCTION__);
	RIL_Msg data;
	int ret;

	data.msg = RIL_REQUEST_DATA_CALL_LIST;
	data.data[0] = 0;

	ret = sendData(&data);
	if(ret < 0)
		return false;
	else
		return true;
}

bool setCellinfoUnRate(){
	loge("%s\n",__FUNCTION__);
	RIL_Msg data;
	int ret;

	data.msg = RIL_REQUEST_SET_UNSOL_CELL_INFO_LIST_RATE;
	data.data[0] = 0;

	ret = sendData(&data);
	if(ret < 0)
		return false;
	else
		return true;
}

int sendSMS(sms_info* info){
	logd("%s\n",__FUNCTION__);
	RIL_Msg data;
	int len;
	int ret;

	logd("%s: info->number = %s\ninfo->data = %s\n",__FUNCTION__,info->number,info->data);
	checkSocketState();
	data.msg = RIL_REQUEST_SEND_SMS;

	memcpy(data.data,info,sizeof(sms_info));

	ret = sendData(&data);
	if (ret < 0) {
		loge("%s: fail",__FUNCTION__);
		return -1;
	} else {
		logd("%s: success",__FUNCTION__);
		return 0;
	}
}
int setSmsAck() {
	logd("%s\n",__FUNCTION__);
	RIL_Msg data;
	int ret;

	checkSocketState();
	data.msg = RIL_REQUEST_SMS_ACKNOWLEDGE;

	data.data[0] = 1;
	data.data[1] = 0;

	ret = sendData(&data);
	if (ret < 0) {
		loge("%s: fail",__FUNCTION__);
		return -1;
	} else {
		logd("%s: success",__FUNCTION__);
		return 0;
	}
}
int receiveSMS(sms_info* info) {
	logd("%s: number = %s\ndata = %s\n",__FUNCTION__,info->number,info->data);
	setSmsAck();
	return 0;
}
int setSmsSendResult(char *data) {
	logd("%s: data[0] = %d ,data[1] = %d\n",__FUNCTION__,data[0],data[1]);
	if(data[0] == RIL_E_SUCCESS) {
		logd("%s: RIL_E_SUCCESS\n",__FUNCTION__);
	}else{
		logd("%s: RIL_E_GENERIC_FAILURE,errorCode = %d\n",__FUNCTION__,data[1]);
	}
	return 0;
}

void setCsState(char *data) {
	logd("%s: state = %s\n",__FUNCTION__,(data[0] == 0)? "out of service" : "in service");
	if(data[0] == 0) {
		cs_state = OUT_OF_SERVICE;
	} else {
		cs_state = IN_SERVICE;
	}
}

CS_STATE getCsState() {
	return cs_state;
}

void setSimState(char *data) {
	logd("%s: state = %s\n",__FUNCTION__,(data[0] == 0)? "sim absent" : "sim present");
	if(data[0] == 0) {
		sim_state = SIMSTATE_ABSENT;
		setRadioPower(false);
	} else {
		sim_state = SIMSTATE_PRESENT;
		getRilImsi();
		setRadioPower(true);
		logd("%s: sim type = %s\n",__FUNCTION__,(data[1] == 0)? "sim" : "usim");
		if(data[1] == 1)
			requestIccID(1);
		else
			requestIccID(0);
	}
}

SIM_STATE getSimState() {
	return sim_state;
}

void setSignalStrength(char *data) {
	//logd("%s: rssi = %d,ber = %d\n",__FUNCTION__,data[0],data[1]);
	rssi = data[0];
}

int getSignalStrength() {
	return rssi;
}

char *getImei() {
	property_get("persist.device.imei",imei,"000000000000000");
	return imei;
}

char *getIccid() {
	if(getSimState() == SIMSTATE_ABSENT || iccidReady == false){
		logd("%s: failed to read iccid",__FUNCTION__);
		return "unknown";
	}
	return iccid;
}

char *getImsi() {
	if(getSimState() == SIMSTATE_ABSENT || imsiReady == false){
		logd("%s: failed to read imsi",__FUNCTION__);
		return "unknown";
	}
	return imsi;
}
int setRadioPower(bool onOff) {
	logd("%s: state = %s\n",__FUNCTION__,(onOff == false)? "off" : "on");
	RIL_Msg data;
	int ret;

	property_set("ril.radio.power", (onOff == false)? "off" : "on");
	memset(&data,0,sizeof(RIL_Msg));
	data.msg = RIL_REQUEST_RADIO_POWER;
	data.data[0] = onOff;
	data.data[1] = 0;

	ret = sendData(&data);
	if (ret < 0) {
		loge("%s: fail",__FUNCTION__);
		return -1;
	} else {
		logd("%s: success",__FUNCTION__);
	}
	return 0;
}

int setNetworkSelectionAutomatic() {
	logd("%s\n",__FUNCTION__);
	RIL_Msg data;
	int ret;

	data.msg = RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC;
	data.data[0] = 0;

	ret = sendData(&data);
	if (ret < 0) {
		loge("%s: fail",__FUNCTION__);
		return -1;
	} else {
		logd("%s: success",__FUNCTION__);
	}
	return 0;
}

int setPreferredNetworkType(RIL_PreferredNetworkType networkType) {
	logd("%s: networkType = %d\n",__FUNCTION__,networkType);
	RIL_Msg data;
	int ret;

	data.msg = RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE;
	data.data[0] = networkType;

	ret = sendData(&data);
	if (ret < 0) {
		loge("%s: fail",__FUNCTION__);
		return -1;
	} else {
		logd("%s: success",__FUNCTION__);
	}
	return 0;
}

int setScreenState(bool state) {
	logd("%s: state = %s\n",__FUNCTION__,(state == false)? "off" : "on");
	RIL_Msg data;

	memset(&data,0,sizeof(RIL_Msg));
	data.msg =  RIL_REQUEST_SCREEN_STATE;
	data.data[0] = state;

	sendData(&data);
	return 0;
}

int getRilImei() {
	logd("%s\n",__FUNCTION__);
	RIL_Msg data;

	memset(&data,0,sizeof(RIL_Msg));
	data.msg = RIL_REQUEST_GET_IMEI;

	sendData(&data);
	return 0 ;
}

int getRilSimState() {
	logd("%s\n",__FUNCTION__);
	RIL_Msg data;

	memset(&data,0,sizeof(RIL_Msg));
	data.msg = RIL_REQUEST_GET_SIM_STATUS;

	sendData(&data);
	return 0;
}

const char *callStateToString (RIL_CallState state){
	switch(state){
		case RIL_CALL_ACTIVE: return "ACTIVE";
		case RIL_CALL_HOLDING: return "HOLDING";
		case RIL_CALL_DIALING: return "DIALING";
		case RIL_CALL_ALERTING: return "ALERTING";
		case RIL_CALL_INCOMING: return "INCOMING";
		case RIL_CALL_WAITING: return "WAITING";
		default: return "UNKWON_STATE";
	}
}

int getCurrentCalls() {
	logd("%s\n",__FUNCTION__);
	RIL_Msg data;
	memset(&data,0,sizeof(RIL_Msg));
	data.msg = RIL_REQUEST_GET_CURRENT_CALLS;
	sendData(&data);
	return 0;
}

bool isAllowPhoneNumber(char *number) {
	int count;
	for(count = 0;count < PHONE_LIST_LENGTH;count++) {
		if(0 == strcmp(number,phone_list[count])) {
			logd("%s: Number found in the phone list. It's an allow number.",__FUNCTION__);
			return true;
		}
	}
	logd("%s: This number is not allowed",__FUNCTION__);
	return false;
}

int currentCallState(call_info *data) {
	logd("%s: count = %d\n",__FUNCTION__,data->count);
	CedarMediaPlayer *CMP = new CedarMediaPlayer;
	if(data->count > 0) {
		logd("%s: current call state is %s\n",__FUNCTION__,callStateToString(data->state));
		logd("%s: state = %d,number = %s\n",__FUNCTION__,data->state,data->number);
#if AUTO_ANSWER
		if(data->state == RIL_CALL_INCOMING) {
			if(isAllowPhoneNumber(data->number) == true)
				callAnswer();
		}
#endif
		/*enable audio voice path*/
	        CMP->setMode(AUDIO_MODE_IN_CALL);
	        CMP->setStreamVolumeIndex(AUDIO_STREAM_VOICE_CALL, 13, AUDIO_DEVICE_OUT_SPEAKER);
	} else
		/*disable audio voice path*/
		CMP->setMode(AUDIO_MODE_NORMAL);

	return 0;
}

int callAnswer() {
	RIL_Msg data;
	memset(&data,0,sizeof(RIL_Msg));
	data.msg = RIL_REQUEST_ANSWER;
	sendData(&data);
	return 0;
}

int callHangup() {
	RIL_Msg data;
	memset(&data,0,sizeof(RIL_Msg));
	data.msg = RIL_REQUEST_HANGUP;
	sendData(&data);
	return 0;
}

int setCallWaiting() {
	RIL_Msg data;
	memset(&data,0,sizeof(RIL_Msg));
	data.msg = RIL_REQUEST_SET_CALL_WAITING;
	sendData(&data);
	return 0;
}

int requestIccID(int type) {
	iccIo(type, COMMAND_READ_BINARY, EF_ICCID, 0, 0, GET_RESPONSE_EF_IMG_SIZE_BYTES);
	return 0;
}

int iccIo(int type, int command, int fileid, int p1, int p2, int p3) {
	RIL_Msg ril_msg;
	SIM_IO p_data;
	memset(&ril_msg,0,sizeof(RIL_Msg));
	memset(&p_data,0,sizeof(SIM_IO));
	ril_msg.msg = RIL_REQUEST_SIM_IO;

	p_data.command = command;
	p_data.fileid = fileid;
	if(type == 1) //1:usim  0:sim
		memcpy(p_data.path,getUsimEFPath(fileid),strlen(getUsimEFPath(fileid)));
	else
		memcpy(p_data.path,getSimEFPath(fileid),strlen(getSimEFPath(fileid)));
	p_data.p1 = p1;
	p_data.p2 = p2;
	p_data.p3 = p3;

	memcpy(ril_msg.data,&p_data,sizeof(SIM_IO));
	sendData(&ril_msg);
	return 0;
}

int setSimIoData(char *data) {
	logd("%s\n",__FUNCTION__);
	sim_data p_data;
	memcpy(&p_data,data,sizeof(sim_data));
	logd("%s: p_data.ef_id = %x\n",__FUNCTION__,p_data.ef_id);
	switch(p_data.ef_id) {
		case EF_ICCID:
			strcpy(iccid,p_data.data);
			iccidReady = true;
			logd("%s: p_data.data = %s\n",__FUNCTION__,p_data.data);
			break;
		default:
			logd("%s: unknown ef_id %x\n",__FUNCTION__);
	}
	return 0;
}

int getRilImsi() {
	logd("%s\n",__FUNCTION__);
	RIL_Msg data;
	memset(&data,0,sizeof(RIL_Msg));
	data.msg = RIL_REQUEST_GET_IMSI;

	sendData(&data);
	return 0;
}

int setApnInfo(char *apn, char *userName, char *password, char *authType, char *protocol) {
	logd("%s\n",__FUNCTION__);
	RIL_Msg data;
	apn_info info;
	memset(&data,0,sizeof(RIL_Msg));
	memset(&info,0,sizeof(apn_info));

	data.msg = RIL_REQUEST_SET_APN_INFO;

	if(apn != NULL) {
		logd("%s: apn = %s\n",__FUNCTION__,apn);
		strcpy(info.apn,apn);
	}
	if(userName != NULL) {
		logd("%s: userName = %s\n",__FUNCTION__,userName);
		strcpy(info.userName,userName);
	}
	if(password != NULL) {
		logd("%s: passwoed = %s\n",__FUNCTION__,password);
		strcpy(info.password,password);
	}
	if(authType != NULL) {
		logd("%s: authType = %s\n",__FUNCTION__,authType);
		strcpy(info.authType,authType);
	} else {
		strcpy(info.authType,"0");
	}
	if(protocol != NULL) {
		logd("%s: protocol = %s\n",__FUNCTION__,protocol);
		strcpy(info.protocol,protocol);
	} else {
		strcpy(info.protocol,"IP");
	}

	memcpy(data.data,&info,sizeof(apn_info));
	sendData(&data);
	return 0;
}

int reconnectNetwork() {
	logd("%s\n",__FUNCTION__);
	disableCellular();
	sleep(3);
	enableCellular();
	return 0;
}


int updateCellInfo(char *data) {
	memcpy(&cell,data,sizeof(cell_info));
	loge("%s: cell.mcc = %d,cell.mnc = %d,cell.lac = %4x,cell.cell = %x\n",__FUNCTION__,cell.mcc,cell.mnc,cell.lac,cell.cell);
	
	return 0;
}

cell_info getCellInfo() {
	return cell;
}
extern void opengps();
static void *handleRadioInit( void *p_arg){
	checkSocketState();
	clearWakeLock();
	setRadioPower(true);
	sleep(2);
	opengps();
	getRilImei();
	getRilSimState();
	setNetworkSelectionAutomatic();
	setPreferredNetworkType(PREF_NET_TYPE_GSM_WCDMA);
	setCallWaiting();
	setCellinfoUnRate();
	set_network_state(MODEM, DISCONNECT);
	radioInit = true;
	if(moblieDataEnable)
	    checkNetworkState();

	return NULL;
}

int initRadio() {
	int ret;
	pthread_attr_init (&thread_attr_radio_init);
	pthread_attr_setdetachstate(&thread_attr_radio_init, PTHREAD_CREATE_DETACHED);
	ret = pthread_create(&s_tid_handle_radio_init, &thread_attr_radio_init, handleRadioInit, NULL);
	if(ret = -1){
		pthread_attr_destroy(&thread_attr_radio_init);
	}
	return 0;
}

#ifdef SMS_TEST
int sendSmsTest() {
	sms_info info;
	memset(&info, 0, sizeof(sms_info));
	int i;

	strcpy(info.number,"18859102871");
	strcpy(info.data,"sms test 测试短信 123456");
	for(i=0; i<strlen(info.data); i++){
		logd("info.data[%d] = %x",i,info.data[i]);
	}
	sendSMS(&info);
	return 0;
}
#endif
