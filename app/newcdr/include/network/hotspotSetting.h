#ifndef _HOTSPOTSETTING_H
#define _HOTSPOTSETTING_H

#define IDC_START		200
#define IDC_SWITCH		200
#define IDC_SWITCH_TITLE	201
#define IDC_SSID_TITLE		202
#define IDC_SSID		203
#define IDC_PASSWORD_TITLE	204
#define IDC_PASSWORD		205
#define IDC_NUM_0		206
#define IDC_NUM_1		207
#define IDC_NUM_2		208
#define IDC_NUM_3		209
#define IDC_NUM_4		210
#define IDC_NUM_5		211
#define IDC_NUM_6		212
#define IDC_NUM_7		213
#define IDC_NUM_8		214
#define IDC_NUM_9		215
#define IDC_NUM_OK		216

#define INDEX_SWITCH		0
#define INDEX_SWITCH_TITLE	1
#define INDEX_SSID_TITLE	2
#define INDEX_SSID		3
#define INDEX_PASSWORD_TITLE	4
#define INDEX_PASSWORD		5
#define INDEX_NUM_0		6
#define INDEX_NUM_1		7
#define INDEX_NUM_2		8
#define INDEX_NUM_3		9
#define INDEX_NUM_4		10
#define INDEX_NUM_5		11
#define INDEX_NUM_6		12
#define INDEX_NUM_7		13
#define INDEX_NUM_8		14
#define INDEX_NUM_9		15
#define INDEX_NUM_OK		16
#define INDEX_NUM_TOTAL	17
#define SELECT_BORDER   1
#define UNSELECT_BORDER 0

int apSettingDialog(HWND hParent, apData_t* configData);
int apSetSsid(char * buffer);
int apSetPasswd(char* buffer);


#endif
