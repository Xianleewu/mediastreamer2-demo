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
#include <network/wifiMonitor.h>
#include <network/hotspot.h>
#include <network/debug.h>
#include <network/ConnectivityServer.h>

#include "cutils/log.h"
#include "cutils/memory.h"
#include "cutils/misc.h"
#include "cutils/properties.h"
#include <sys/system_properties.h>

static bool hotspot_state_saved = false;

int enable_wifi(void) {
    int ret;

    ret = check_wireless_ready();
    if (ret) {
        LOGE("%s, wifi driver is not ready....", __func__);
        return ret;
    }

    if (isSoftapEnabled()) {
        LOGE("%s, hotspot is running, save state and disable it.", __func__);
        hotspot_state_saved = true;
        ret = TEMP_FAILURE_RETRY(disable_Softap());
        if (ret) {
            LOGE("%s, hotspot disable fail...", __func__);
            return ret;
        }
    }

    ret = wifi_start_supplicant();
    if (ret) {
        LOGE("%s, wpa_supplicant start fail...", __func__);
        if (hotspot_state_saved) {
            TEMP_FAILURE_RETRY(enable_Softap());
            hotspot_state_saved = false;
        }
        return ret;
    }

    if ((ret = wifi_connect_to_supplicant()) != 0) {
	LOGE("%s, connect to supplicant fail.", __func__);
        goto wpa_connect_error;
    }

    set_network_state(WIFI, ENABLED);

    if ((ret = startWifiMonitor()) != 0) {
        LOGE("%s, start Wifi Monitor fail.", __func__);
        goto wifiMonitor_error;
    }

    if (!enableAllNetwork()) {
        LOGE("%s, enableAllNetwork fail...", __func__);
    }
    enableAutoConnect(true);

    return 0;

wifiMonitor_error:
    wifi_close_supplicant_connection();
wpa_connect_error:
    wifi_stop_supplicant();
    return ret;
}

int disable_wifi(void) {
    int ret;

    LOGD("%s ...", __func__);
    disconnect();
    usleep(100000);
    stopWifiMonitor();
    wifi_close_supplicant_connection();
    ret = wifi_stop_supplicant();
    if (ret) {
        LOGE("%s, wpa_supplicant stop fail...", __func__);
        return ret;
    }
    property_set(WIFI_SIGNAL_LEVEL, "-1");
    set_network_state(WIFI, DISABLED);

    return 0;
}

bool check_if_wifi_enabled(void) {
    return is_wifi_enabled();
}

bool startWifiWpsPBC(void) {
    return startWpsPbc();
}
