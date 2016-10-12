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
#include <sys/prctl.h> 
#include <fcntl.h>
#include <dirent.h>
#include <network/wifiHal.h>
#include <network/wpaCtl.h>
#include <network/net.h>
#include <network/debug.h>
#include <network/wifiMonitor.h>
#include <network/ConnectivityServer.h>
#include <misc/resourceManager.h>
#include <misc/cdr.h>
#include "libwpa_client/wpa_ctrl.h"

#include "cutils/log.h"
#include "cutils/memory.h"
#include "cutils/misc.h"
#include "cutils/properties.h"
#include <sys/system_properties.h>

static pid_t mPid = 0;
static pid_t mSignalTid = 0;

int dispatchEvent(char* event) {
    char* point = NULL;
    if (strstr(event, STACMDPREFEX)) {
        if ((point = strstr(event, WPA_EVENT_PREFIX)) != NULL) {
            if (!memcmp(point+11, WIFI_STATE_CHANGE, strlen(WIFI_STATE_CHANGE))) {
            //WIFI_STATE_CHANGE
                int state = -1;
                LOGD("%s, %s ......", __func__, WIFI_STATE_CHANGE);
                point = strstr(event, "state=");
                if (point) {
                    state = *(point+6) - 48;
                    LOGE("%s, %s", __func__, wpaStateInt2Str(state));
                }
                switch (state) {
                    case WPA_AUTHENTICATING: case WPA_ASSOCIATING: case WPA_ASSOCIATED:
                    case WPA_4WAY_HANDSHAKE:
                        property_set(WIFI_SIGNAL_LEVEL, "3");
                        break;
                    default: break;
                }
            } else if (!memcmp(point+11, WIFI_STATE_DISCONNECT, strlen(WIFI_STATE_DISCONNECT))) {
            //WIFI_STATE_DISCONNECT
                LOGD("%s, %s ......", __func__, WIFI_STATE_DISCONNECT);
                stopWifiSignalPoll();
                clean_connect_addrs(WIFI_INTERFACE);
                set_network_state(WIFI, DISCONNECT);
                reconnect();
            } else if (!memcmp(point+11, WIFI_STATE_CONNECT, strlen(WIFI_STATE_CONNECT))) {
            //WIFI_STATE_CONNECT
                set_network_state(WIFI, CONNECTING);
                LOGD("%s, %s ......", __func__, WIFI_STATE_CONNECT);
                struct dhcpinfo info;
                LOGE("%s, start do dhcp request...", __func__);
                int ret = do_dhcp_request(&info.ipaddr, &info.gateway, &info.mask,
                                 &info.dns1, &info.dns2, &info.server, &info.lease);
                if (ret) {
                    LOGE("wifi dhcp fail...[%s]", get_dhcp_error_string());
                    return 0;
                }
                LOGE("dhcpcd:ip[%X],gateway[%X],mask[%X],dns1[%X],dns2[%X],server[%X],lease[%d]", 
                       info.ipaddr, info.gateway, info.mask, info.dns1, info.dns2, info.server, info.lease);
                if (DBG) {
                    char addr[20] = {0};
                    ipv4_int_2_str(info.ipaddr, addr);
                    LOGD("dhcpcd:ip[%s]", addr);
                    ipv4_int_2_str(info.gateway, addr);
                    LOGD("dhcpcd:gateway[%s]", addr);
                    ipv4_int_2_str(info.mask, addr);
                    LOGD("dhcpcd:mask[%s]", addr);
                    ipv4_int_2_str(info.dns1, addr);
                    LOGD("dhcpcd:dns1[%s]", addr);
                    ipv4_int_2_str(info.dns2, addr);
                    LOGD("dhcpcd:dns2[%s]", addr);
                    ipv4_int_2_str(info.server, addr);
                    LOGD("dhcpcd:server[%s]", addr);
                }

                set_route_and_dns(WIFI_INTERFACE, &info);
                startWifiSignalPoll();
                set_network_state(WIFI, CONNECTED);
            } else if (!memcmp(point+11, WPA_TERMINATING, strlen(WPA_TERMINATING))) {
            //WPA_TERMINATING
                LOGE("%s, %s ......", __func__, WPA_TERMINATING);
                LOGE("%s, try to restart wpa_supplicant...", __func__);
                int ret;
                wifi_close_supplicant_connection();
                ret = wifi_start_supplicant();
                if (ret) {
                    LOGE("%s, restart wpa_supplicant fail...", __func__);
                    return -1;
                }
                wifi_connect_to_supplicant();
                enableAllNetwork();
            }
        } else if ((strstr(event, WIFI_WPS_TIMEOUT)) != NULL) {
        //WIFI_WPS_TIMEOUT
            LOGD("%s, %s ......", __func__, WIFI_WPS_TIMEOUT);
            property_set(WIFI_SIGNAL_LEVEL, "1");
        }
    }
    return 0;
}

static int calculateSignalLevel(int rssi) {
    if (rssi <= -85) {
        return 1;
    } else if (rssi > -85 && rssi <= -70) {
        return 2;
    } else if (rssi > -70 && rssi <= -55) {
        return 3;
    } else if (rssi > -55) {
        return 4;
    }
    return 0;
}

int startWifiSignalPoll(void) {
    pthread_t tid;
    int ret;

    if (mSignalTid) {
        LOGE("wifi SignalPoll is already running");
        return mSignalTid;
    }

    pid_t pid = 1;

    if (mSignalTid) {
        LOGE("wifi SignalPoll is already running");
        return mSignalTid;
    }

    if ((pid = fork()) < 0) {
        LOGE("%s, fork failed (%s)", __func__, strerror(errno));
        return -1;
    }

    if (!pid) {
        prctl(PR_SET_NAME, (unsigned long)"WifiSignalPoll", 0, 0, 0);
        char replyStr[REPLY_BUF_SIZE];
        int rssi, level, oldLevel = -1;
        char Str[10];
        char *p1, *p2;
        for (;;) {
            memset(replyStr, 0, REPLY_BUF_SIZE);
            getSignalPoll(replyStr);
            p1 = strstr(replyStr, "RSSI=");
            if (p1 != NULL) {
                p2 = strstr(p1, "\n");
                memset(Str, 0, 10);
                memcpy(Str, p1+6, 2);
                rssi = 0 - atoi(Str);
                level = calculateSignalLevel(rssi) + 3;
                if (oldLevel != level) {
                    oldLevel = level;
                    memset(Str, 0, 10);
                    sprintf(Str, "%d", level);
                    property_set(WIFI_SIGNAL_LEVEL, Str);
                }
                LOGD("%s, SignalPoll, rssi=%d, level:%d", __func__, rssi, level);
            }
            usleep(WIFI_SIGNAL_INTERVAL*10000); // interval 3S
        }

        LOGE("wifiMonitor failed to start");
        mSignalTid = 0;
        return -1;
    } else {
        mSignalTid = pid;
        LOGE("wifi SignalPoll started successfully, pid=%d", mSignalTid);
        usleep(100000);
    }
    return 0;
}

void stopWifiSignalPoll(void) {
    LOGD("%s start, mSignalPid=%d", __func__, mSignalTid);
    property_set(WIFI_SIGNAL_LEVEL, "1");
    if (mSignalTid != 0) {
        kill(mSignalTid, SIGKILL);
        waitpid(mSignalTid, NULL, 0);
        mSignalTid = 0;
    }
    LOGD("%s end...", __func__);
}

int startWifiMonitor(void) {
    pid_t pid = 1;

    if (mPid) {
        LOGE("wifiMonitor is already running");
        return mPid;
    }

    if ((pid = fork()) < 0) {
        LOGE("%s, fork failed (%s)", __func__, strerror(errno));
        return -1;
    }

    if (!pid) {
        char eventStr[EVENT_BUF_SIZE];
        prctl(PR_SET_NAME, "wifiMonitor", NULL, NULL, NULL);
        for (;;) {
            memset(eventStr, 0, EVENT_BUF_SIZE);
            if (!waitForEvent(eventStr)) continue;

            if (DBG && strstr(eventStr, SCAN_RESULTS_STR) == NULL) {
                LOGE("%s, Event [%s]", __func__, eventStr);
            }
            if (dispatchEvent(eventStr)) {
                LOGD("Disconnecting from the supplicant, no more events");
                break;
            }
        }

        LOGE("wifiMonitor failed to start");
        mPid = 0;
        return -1;
    } else {
        mPid = pid;
        LOGE("wifiMonitor started successfully, pid=%d", mPid);
        usleep(100000);
    }
    return 0;
}

void stopWifiMonitor(void) {
    LOGD("%s start, mPid=%d", __func__, mPid);
    if (mPid != 0) {
        kill(mPid, SIGKILL);
        waitpid(mPid, NULL, 0);
        mPid = 0;
    }
    LOGD("%s end...", __func__);
}
