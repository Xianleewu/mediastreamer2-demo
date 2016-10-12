#ifndef _RK_CELLULAR_CONTROLLER_H
#define _RK_CELLULAR_CONTROLLER_H

#include "hardware/hardware.h"
#include "hardware/radio.h"
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <linux/in.h>
#include <linux/in6.h>
#include <cutils/log.h>
#include <stdint.h>
#include <telephony/ril.h>
#include <CedarMediaPlayer.h>
using namespace android;

typedef struct {
	char number[30];
	char data[255];
}sms_info;

typedef struct {
	int count;
	RIL_CallState   state;
	char number[30];
}call_info;

typedef struct {
	char apn[16];		//the APN to connect to
	char userName[16];	//the username for APN, or NULL
	char password[16];	//the password for APN, or NULL
	char authType[16];	/** authentication protocol used for this PDP context
				* (None: 0, PAP: 1, CHAP: 2, PAP&CHAP: 3)*/
	char protocol[16];	/** one of the PDP_type values in TS 27.007 section 10.1.1.
				 * For example, "IP", "IPV6", "IPV4V6", or "PPP".default is "IP"*/
}apn_info;

typedef struct {
	int mcc;
	int mnc;
	int lac;
	int cell;
}cell_info;

typedef enum{
	OUT_OF_SERVICE = 0,
	IN_SERVICE = 1,
}CS_STATE;

typedef enum{
	SIMSTATE_ABSENT = 0,
	SIMSTATE_PRESENT = 1,
}SIM_STATE;

typedef enum{
	RESPONSE_UNSOLICITED = 0,
	RESPONSE_SOLICITED = 1,
}RESPONSE_TYPE;

bool enableCellular();
bool disableCellular();
bool creatRilSocket();
int setScreenState(bool state);
int sendSMS(sms_info* info);
int receiveSMS(sms_info* info);
int setSmsSendResult(char *data);
int setRadioPower(bool onOff);
int setNetworkSelectionAutomatic();
int setPreferredNetworkType(int networkType);
int initRadio();
int getSignalStrength();
int reconnectNetwork();
int callAnswer();
int callHangup();
int setApnInfo(char *apn, char *userName, char *password, char *authType, char *protocol);
char *getImei();
char *getIccid();
char *getImsi();
cell_info getCellInfo();
CS_STATE getCsState();
SIM_STATE getSimState();

#endif 
