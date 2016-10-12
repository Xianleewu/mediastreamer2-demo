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
#include <network/wifiHal.h>
#include <network/wpaCtl.h>
#include <network/debug.h>
#include <network/wifiMonitor.h>
#include "libwpa_client/wpa_ctrl.h"

#include "cutils/log.h"
#include "cutils/memory.h"
#include "cutils/misc.h"
#include "cutils/properties.h"
#include <sys/system_properties.h>

static bool doCmd(const char* cmd, char* reply) {
    size_t reply_len = REPLY_BUF_SIZE - 1;

    if (cmd == NULL)
        LOGE("%s, cmd = NULL.", __func__);

    LOGD("cmd: %s", cmd);
    if (wifi_command(cmd, reply, &reply_len) != 0) {
        return false;
    }

    if (reply_len > 0 && reply[reply_len-1] == '\n') {
        reply[reply_len-1] = '\0';
    } else {
        reply[reply_len] = '\0';
    }

    return true;
}

static bool doBoolCmd(const char* cmd) {
    char reply[REPLY_BUF_SIZE];
    char Ncmd[1024] = {0};
 
    strcat(Ncmd, STACMDPREFEX);
    strcat(Ncmd, cmd);
    if (!doCmd(Ncmd, reply)) {
        return false;
    }

    return (strcmp(reply, "OK") == 0);;
}

static int doIntCmd(const char* cmd) {
    char reply[REPLY_BUF_SIZE];
    char Ncmd[1024] = {0};
    
    strcat(Ncmd, STACMDPREFEX);
    strcat(Ncmd, cmd);
    if (!doCmd(Ncmd, reply)) {
        return -1;
    }
    return atoi(reply);
}

static bool doStringCmd(const char* cmd, char* reply) {
    char Ncmd[1024] = {0};

    strcat(Ncmd, STACMDPREFEX);
    strcat(Ncmd, cmd);
    if (!doCmd(Ncmd, reply)) {
        return false;
    }

    return true;
}

bool scan(void) {
    return doBoolCmd("SCAN");
}

bool getScanResult(char* result) {
    return doStringCmd("SCAN_RESULT", result);
}

int addNetwork(void) {
    return doIntCmd("ADD_NETWORK");
}

bool listNetworks(char *result) {
    return doStringCmd("LIST_NETWORKS", result);
}

bool removeNetwork(int ID) {
    char cmd[1024] = {0};

    sprintf(cmd, "REMOVE_NETWORK %d", ID);
    return doBoolCmd(cmd);
}

bool setNetwork(int ID, const char* name, char* value) {
    char cmd[1024] = {0};

    if (name == NULL || value == NULL)
        return false;

    sprintf(cmd, "SET_NETWORK %d %s \"%s\"", ID, name, value);
    return doBoolCmd(cmd);
}

bool enableNetwork(int ID, bool disableOthers) {
    char cmd[1024] = {0};

    LOGD("enableNetwork nid=%d disableOthers=%d", ID, disableOthers);
    if (disableOthers) {
        sprintf(cmd, "SELECT_NETWORK %d", ID);
    } else {
        sprintf(cmd, "ENABLE_NETWORK %d", ID);
    }
    return doBoolCmd(cmd);
}

bool enableAllNetwork(void) {

    LOGD("enableAllNetwork ");
    return doBoolCmd("ENABLE_NETWORK ALL");
}

bool disableNetwork(int ID) {
    char cmd[1024] = {0};

    LOGD("disableNetwork nid=%d", ID);
    sprintf(cmd, "DISABLE_NETWORK %d", ID);
    return doBoolCmd(cmd);
}

bool disconnect(void) {
    LOGD("DISCONNECT ");
    return doBoolCmd("DISCONNECT");
}

bool reconnect(void) {
    LOGD("RECONNECT ");
    return doBoolCmd("RECONNECT");
}

int addNewAPConfig(char* ssid, char* password) {
    int id = addNetwork();

    if (!setNetwork(id, "ssid", ssid)) {
        LOGE("%s, setNetwork %d ssid %s fail...", __func__, id, ssid);
        return false;
    }
    if (!setNetwork(id, "psk", password)) {
        LOGE("%s, setNetwork %d psk %s fail...", __func__, id, password);
        return false;
    }

    doBoolCmd("SAVE_CONFIG");

    return id;
}

bool status(char* result) {
    return doStringCmd("STATUS", result);
}

bool getSignalPoll(char * result) {
    return doStringCmd("SIGNAL_POLL", result);
}

bool startDriver(void) {
    return doBoolCmd("DRIVER START");
}

bool stopDriver(void) {
    return doBoolCmd("DRIVER STOP");
}

void enableAutoConnect(bool enable) {
    if (enable) {
        doBoolCmd("STA_AUTOCONNECT 1");
    } else {
        doBoolCmd("STA_AUTOCONNECT 0");
    }
}

bool startWpsPbc(void) {
    property_set(WIFI_SIGNAL_LEVEL, "2");
    return doBoolCmd("WPS_PBC");
}

static bool wifiWaitForEvent(char* buf)
{
    size_t length = EVENT_BUF_SIZE;
    int nread = wifi_wait_for_event(buf, length);
    if (nread > 0) {
        return true;
    } else {
        return false;
    }
}

bool waitForEvent(char* buf) {
    return wifiWaitForEvent(buf);
}

const char* wpaStateInt2Str(int state) {
    switch (state) {
        case WPA_DISCONNECTED:
            return "WPA_DISCONNECTED"; break;
        case WPA_INTERFACE_DISABLED:
            return "WPA_INTERFACE_DISABLED"; break;
        case WPA_INACTIVE:
            return "WPA_INACTIVE"; break;
        case WPA_SCANNING:
            return "WPA_SCANNING"; break;
        case WPA_AUTHENTICATING:
            return "WPA_AUTHENTICATING"; break;
        case WPA_ASSOCIATING:
            return "WPA_ASSOCIATING"; break;
        case WPA_4WAY_HANDSHAKE:
            return "WPA_4WAY_HANDSHAKE"; break;
        case WPA_GROUP_HANDSHAKE:
            return "WPA_GROUP_HANDSHAKE"; break;
        case WPA_COMPLETED:
            return "WPA_COMPLETED"; break;
        default: break;
    }
    return "nonrecognition state";
}
