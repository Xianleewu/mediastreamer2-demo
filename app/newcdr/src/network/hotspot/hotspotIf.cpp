/*
 * Copyright (c) 2016 Fuzhou Rockchip Electronics Co.Ltd All rights reserved.
 * (wilen.gao@rock-chips.com)
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>  
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>
#include <network/hotspot.h>
#include <network/wifiIf.h>
#include <network/debug.h>
#include <misc/keyEvent.h>
#include <network/config.h>

SoftapController* softAp = NULL;
int enable_Softap(bool wifiCall);
int disable_Softap(bool wifiCall);
bool restart_Softap(void);
bool isSoftapEnabled(void);
bool getSoftapSSID(char *ssid);
char getSoftap_SecType_PW(char *pw);
void setSoftap_SecType_PW(WifiHotspotEncrypt_t SecType, char *pw);

int enable_Softap(void) {
	LOGD("%s, start ...\n", __func__);

	if (check_if_wifi_enabled()) {
		LOGE("%s, wifi is running, try to disable it.", __func__);
		if (TEMP_FAILURE_RETRY(disable_wifi())) {
			LOGE("%s, disable wifi fail.", __func__);
			return -1;
		}
	}

	if (softAp != NULL) {
		LOGE("enable_Softap: softAp != NULL...");
	} else {
		softAp = new SoftapController();
		LOGE("enable_Softap: softAp == NULL, new one.%p", softAp);
		if (softAp == NULL) {
			LOGE("new softap controller fail.\n");
			return -1;
		}
	}
	//update UI
	WifiHotspot_t *wifiHotspot = new WifiHotspot_t;
	softAp->getSoftapConfig(wifiHotspot);
	LOGD("main::ssid=%s, wpa_psk=%s", wifiHotspot->ssid, wifiHotspot->wpa_psk);
	
	if(softAp->setSoftapConfig(wifiHotspot) < 0) {
		LOGE("set Softap fail.\n");
		exit(0);
	}
	
	if(softAp->startSoftap()) {
		LOGE("start Softap fail.\n");
		exit(0);
	}
	usleep(1000 * 1000);
	delete(wifiHotspot);
	return 0;
}

int disable_Softap(void) {
        LOGD("%s, stop ...\n", __func__);
	if (softAp == NULL) {
		LOGE("disable_Softap: softAp == NULL, exit.");
		return -1;
	}
	if(softAp->stopSoftap()) {
		LOGE("stop Softap fail.\n");
		return -1;
	}
	free(softAp);
	softAp = NULL;
	usleep(500 * 1000);

	return 0;
}

bool restart_Softap(void) {
	if (isSoftapEnabled()) {
		if (!disable_Softap() && !enable_Softap()) {
			LOGE("restart_Softap: SUCCESS.");
			return true;
		}
	}
	return false;
}

bool isSoftapEnabled(void) {
	LOGE("isSoftapEnabled...");
	if (softAp == NULL) {
                LOGE("isSoftapEnabled: softAp == NULL, exit.");
                return false;
        }
	return softAp->isSoftapStarted();
}
bool getSoftapSSID(char *ssid) {
	LOGD("getSoftapSSID...");
	if (softAp == NULL) {
                LOGE("getSoftapSSID: softAp == NULL.");
		softAp = new SoftapController();
                LOGE("softAp == NULL, try to new one.%p", softAp);
                if (softAp == NULL) {
                        LOGE("new softap controller fail.\n");
                        return false;
                }
	}
	WifiHotspot_t *wifiHotspot = new WifiHotspot_t;
        softAp->getSoftapConfig(wifiHotspot);
        LOGD("getSoftapSSID::ssid=%s, len=%d", wifiHotspot->ssid, strlen(wifiHotspot->ssid));
	memcpy(ssid, wifiHotspot->ssid, strlen(wifiHotspot->ssid));
	delete(wifiHotspot);
        return true;
}

char setSoftapSSID(char *ssid) {
	LOGD("setSoftapSSID...");
	if (softAp == NULL) {
                LOGE("setSoftapSSID: softAp == NULL.");
		softAp = new SoftapController();
                LOGE("softAp == NULL, try to new one.%p", softAp);
                if (softAp == NULL) {
                        LOGE("new softap controller fail.\n");
                        return -1;
                }
	}
	WifiHotspot_t *wifiHotspot = new WifiHotspot_t;
        softAp->getSoftapConfig(wifiHotspot);
        LOGD("setSoftapSSID::ssid=%s, len=%d", ssid, strlen(ssid));
	memset(wifiHotspot->ssid, 0, 32);
	memcpy(wifiHotspot->ssid, ssid, strlen(ssid));
	softAp->setSoftapConfig(wifiHotspot);
	delete(wifiHotspot);
        return 0;
}


char getSoftap_SecType_PW(char *pw) {
	char SecType = 0;
	LOGD("getSoftapPW...");
        if (softAp == NULL) {
                LOGE("getSoftapPW: softAp == NULL.");
		softAp = new SoftapController();
                LOGE("softAp == NULL, try to new one.%p", softAp);
                if (softAp == NULL) {
                        LOGE("new softap controller fail.\n");
                        return -1;
                }
        }
	WifiHotspot_t *wifiHotspot = new WifiHotspot_t;
        softAp->getSoftapConfig(wifiHotspot);
        LOGD("getSoftap_SecType_PW::Security=%d, wpa_psk=%s", 
		wifiHotspot->wifiHotspotEncrypt, wifiHotspot->wpa_psk);
        memcpy(pw, wifiHotspot->wpa_psk, strlen(wifiHotspot->wpa_psk));
	SecType = wifiHotspot->wifiHotspotEncrypt;
	delete(wifiHotspot);
	return SecType;
}

void setSoftap_SecType_PW(WifiHotspotEncrypt_t SecType, char *pw) {
	LOGD("setSoftap_SecType_PW...");
	if (softAp == NULL) {
                LOGE("getSoftapPW: softAp == NULL.");
		softAp = new SoftapController();
                LOGE("softAp == NULL, try to new one.%p", softAp);
                if (softAp == NULL) {
                        LOGE("new softap controller fail.\n");
                        return;
                }
        }
	WifiHotspot_t *wifiHotspot = new WifiHotspot_t;
        softAp->getSoftapConfig(wifiHotspot);
	wifiHotspot->wifiHotspotEncrypt = SecType;
	memset(wifiHotspot->wpa_psk, 0, 64);
	memcpy(wifiHotspot->wpa_psk, pw, strlen(pw));
	softAp->setSoftapConfig(wifiHotspot);
	delete(wifiHotspot);
	return;
}

