/*
 * Copyright (c) 2016 Fuzhou Rockchip Electronics Co.Ltd All rights reserved.
 * (wilen.gao@rock-chips.com)
 */
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <linux/wireless.h>

#include <openssl/evp.h>
#include <openssl/sha.h>
#include <netutils/ifc.h>
#include <private/android_filesystem_config.h>
#include <network/hotspot.h>
#include <network/net.h>
#include <network/debug.h>
#include <network/config.h>
#include <network/ConnectivityServer.h>

static const char SOFTAPD_CONF_FILE[]    = "/data/misc/wifi/softap.conf";
static const char HOSTAPD_CONF_FILE[]	 = "/data/misc/wifi/hostapd.conf";
static const char HOSTAPD_BIN_FILE[]     = "/system/bin/hostapd";

SoftapController::SoftapController()
    : mHostapdPid(0){}

SoftapController::~SoftapController() {
    stopSoftap();
}

int SoftapController::startSoftap() {
    pid_t pid = 1;

    if (mHostapdPid) {
        LOGE("SoftAP is already running");
        return 0;
    } else if(mHostapdPid) {
    	stopSoftap();
    }

    if (softap_start_net_config()) {
        LOGE("softap_start_net_config failed (%s)", strerror(errno));
        return -1;
    }

    //hostapd pid
    if ((pid = fork()) < 0) {
        LOGE("fork hostapd pid failed (%s)", strerror(errno));
        return -1;
    }

    if (!pid) {
        if (execl(HOSTAPD_BIN_FILE, HOSTAPD_BIN_FILE,
                  HOSTAPD_CONF_FILE, (char *) NULL)) {
            LOGE("execl \"hostapd -d /data/misc/wifi/hostapd.conf\" failed (%s)", strerror(errno));
        }
        LOGE("SoftAP failed to start");
        return -1;
    }
    mHostapdPid = pid;
    LOGE("SoftAP started hostapd successfully");
    set_network_state(SOFTAP, ENABLED);
    usleep(AP_BSS_START_DELAY);

    return 0;
}

int SoftapController::stopSoftap() {

    if (mHostapdPid == 0) {
        LOGE("SoftAP is not running");
        return 0;
    }

    if (softap_stop_net_config()) {
        LOGE("softap_stop_net_config failed (%s)", strerror(errno));
    }

    LOGE("Stopping the SoftAP service...");

    usleep(AP_BSS_STOP_DELAY);
    if(mHostapdPid != 0) {
        kill(mHostapdPid, SIGTERM);
        waitpid(mHostapdPid, NULL, 0);
    }

    mHostapdPid = 0;
    LOGE("SoftAP stopped successfully");
    set_network_state(SOFTAP, DISABLED);
    usleep(AP_BSS_STOP_DELAY);
    return 0;
}

bool SoftapController::isSoftapStarted() {
    return (mHostapdPid != 0);
}

int SoftapController::setSoftapConfig(WifiHotspot_t* wht) {
    char psk_str[2*SHA256_DIGEST_LENGTH+1];
    int ret = 0;
    int i = 0;
    int fd;
    int hidden = 0;
    char *wbuf = NULL;
    char *fbuf = NULL;
    
    //set UI config file
    if(setSoftapUIConfig(wht)) {
    	LOGE("Cannot update \"%s\": %s", SOFTAPD_CONF_FILE, strerror(errno));
    	return -1;
    }

    if(wht->hidden == 1)
        hidden = 1;

    asprintf(&wbuf, "interface=wlan0\ndriver=nl80211\nctrl_interface="
            "/data/misc/wifi/hostapd\nssid=%s\nchannel=6\nieee80211n=1\n"
            "hw_mode=g\nignore_broadcast_ssid=%d\n",
            wht->ssid, hidden);

    if (wht->wifiHotspotEncrypt == WPA1_PSK) {
        generatePsk(wht->ssid, wht->wpa_psk, psk_str);
        asprintf(&fbuf, "%swpa=1\nwpa_pairwise=TKIP CCMP\nwpa_psk=%s\n", wbuf, psk_str);
    } else if (wht->wifiHotspotEncrypt == WPA2_PSK) {
        generatePsk(wht->ssid, wht->wpa_psk, psk_str);
        asprintf(&fbuf, "%swpa=2\nrsn_pairwise=CCMP\nwpa_psk=%s\n", wbuf, psk_str);
    } else if (wht->wifiHotspotEncrypt == NONE_SECURT) {
        asprintf(&fbuf, "%s", wbuf);
    }

    LOGE("fbuf=%s", fbuf);

    fd = open(HOSTAPD_CONF_FILE, O_CREAT | O_TRUNC | O_WRONLY | O_NOFOLLOW, 0660);
    if (fd < 0) {
        LOGE("Cannot update \"%s\": %s", HOSTAPD_CONF_FILE, strerror(errno));
        free(wbuf);
        free(fbuf);
        return -1;
    }
    
    if (write(fd, fbuf, strlen(fbuf)) < 0) {
        LOGE("Cannot write to \"%s\": %s", HOSTAPD_CONF_FILE, strerror(errno));
        ret = -1;
    }
    free(wbuf);
    free(fbuf);

    /* Note: apparently open can fail to set permissions correctly at times */
    if (fchmod(fd, 0660) < 0) {
        LOGE("Error changing permissions of %s to 0660: %s",
                HOSTAPD_CONF_FILE, strerror(errno));
        close(fd);
        unlink(HOSTAPD_CONF_FILE);
        return -1;
    }

    if (fchown(fd, AID_SYSTEM, AID_WIFI) < 0) {
        LOGE("Error changing group ownership of %s to %d: %s",
                HOSTAPD_CONF_FILE, AID_WIFI, strerror(errno));
        close(fd);
        unlink(HOSTAPD_CONF_FILE);
        return -1;
    }

    close(fd);
    return ret;
}

void SoftapController::generatePsk(char *ssid, char *passphrase, char *psk_str) {
    unsigned char psk[SHA256_DIGEST_LENGTH];
    int j;
    // Use the PKCS#5 PBKDF2 with 4096 iterations
    PKCS5_PBKDF2_HMAC_SHA1(passphrase, strlen(passphrase),
            reinterpret_cast<const unsigned char *>(ssid), strlen(ssid),
            4096, SHA256_DIGEST_LENGTH, psk);
    for (j=0; j < SHA256_DIGEST_LENGTH; j++) {
        sprintf(&psk_str[j*2], "%02x", psk[j]);
    }
}

int SoftapController::getSoftapConfig(WifiHotspot_t* wht) {
    FILE*fp=NULL;
    size_t len = 0;
    ssize_t read;
    char *line = NULL;

    wht->wifiHotspotEncrypt = WPA2_PSK;
    wht->hidden = 0;
    snprintf(wht->ssid, 32-1, "%s", DEFAULT_NAME);
    snprintf(wht->wpa_psk, 64-1, "%s", DEFAULT_PASSWORD);

    fp=fopen(SOFTAPD_CONF_FILE,"r");
    if(NULL==fp) {
    	LOGE("getSoftapConfig no softap.conf file.");
        return 0;
    }

    wht->wifiHotspotEncrypt = NONE_SECURT;

    while ((read = getline(&line, &len, fp)) != -1) { 
        char *p = NULL;
        char *pstr = NULL;

        pstr = strstr(line, "ssid=");
        if(pstr && !strstr(line, "nignore_broadcast_ssid"))
        {
           p = strtok(line, "=");
           p = strtok(NULL, "=");
		   p = strtok(p, "\n");
           snprintf(wht->ssid, 32-1, "%s", p);
        }

        pstr = NULL;
        pstr = strstr(line, "wpa_psk=");
        if(pstr)
        {
           p = strtok(line, "=");
           p = strtok(NULL, "=");
		   p = strtok(p, "\n");
           snprintf(wht->wpa_psk, 64-1, "%s", p);
        }

        pstr = NULL;
        pstr = strstr(line, "ignore_broadcast_ssid=0");
        if(pstr)
        {
           wht->hidden = 0;
        }

        pstr = NULL;
        pstr = strstr(line, "ignore_broadcast_ssid=1");
        if(pstr)
        {
           wht->hidden = 1;
        }

        pstr = NULL;
        pstr = strstr(line, "wpa=2");
        if(pstr)
        {
           wht->wifiHotspotEncrypt = WPA2_PSK;
        }

        pstr = NULL;
        pstr = strstr(line, "wpa=1");
        if(pstr)
        {
           wht->wifiHotspotEncrypt = WPA1_PSK;
        }
    }
		
    fclose(fp);
    return 0;
}

int SoftapController::setSoftapUIConfig(WifiHotspot_t* wht) {
    FILE*fp=NULL;
	char *wbuf = NULL;
	char *fbuf = NULL;
	
	asprintf(&wbuf, "ssid=%s\nnignore_broadcast_ssid=%d\n",
        wht->ssid, wht->hidden);
            
    if (wht->wifiHotspotEncrypt == WPA1_PSK) {
        asprintf(&fbuf, "%swpa=1\nwpa_psk=%s\n", wbuf, wht->wpa_psk);
    } else if (wht->wifiHotspotEncrypt == WPA2_PSK) {
        asprintf(&fbuf, "%swpa=2\nwpa_psk=%s\n", wbuf, wht->wpa_psk);
    } else if (wht->wifiHotspotEncrypt == NONE_SECURT) {
        asprintf(&fbuf, "%s", wbuf);
    }

    fp=fopen(SOFTAPD_CONF_FILE,"w");
    if(NULL==fp)
    {
        LOGE("setSoftapUIConfig::Cannot update \"%s\": %s", SOFTAPD_CONF_FILE, strerror(errno));
        free(wbuf);
        free(fbuf);
        return -1;
    }

    fwrite(fbuf, strlen(fbuf), 1, fp);
    fclose(fp);
    free(wbuf);
    free(fbuf);
    return 0;
}
