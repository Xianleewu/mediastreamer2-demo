#ifndef _HOTSPOT_H
#define _HOTSPOT_H

#include <linux/in.h>
#include <net/if.h>
#include <cutils/log.h>
#include <window/windows.h>
#include <event/EventManager.h>
#include <misc/resourceManager.h>

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

#define AP_BSS_START_DELAY	200000
#define AP_BSS_STOP_DELAY	500000

typedef enum WifiHotspotEncrypt_t{
	NONE_SECURT = 0,
	WPA1_PSK,
	WPA2_PSK,
} WifiHotspotEncrypt_t;

typedef struct WifiHotspot_t{
	WifiHotspotEncrypt_t wifiHotspotEncrypt;
	char ssid[32];
	char wpa_psk[64];
	int hidden;
} WifiHotspot_t;

class SoftapController {
public:
    SoftapController();
    ~SoftapController();

    int startSoftap();
    int stopSoftap();
    bool isSoftapStarted();
    int setSoftapConfig(WifiHotspot_t* wht);
    int getSoftapConfig(WifiHotspot_t* wht);
    int setSoftapUIConfig(WifiHotspot_t* wht);
    int fwReloadSoftap();
    int getSoftap();

private:
    pid_t mHostapdPid;
    pid_t mDnsmasqPid;

private:
    void generatePsk(char *ssid, char *passphrase, char *psk);
};

int enable_Softap(void);
int disable_Softap(void);
bool restart_Softap(void);
bool isSoftapEnabled(void);
bool getSoftapSSID(char *ssid);
char setSoftapSSID(char * ssid);
char getSoftap_SecType_PW(char *pw);
void setSoftap_SecType_PW(WifiHotspotEncrypt_t SecType, char *pw);
#endif
