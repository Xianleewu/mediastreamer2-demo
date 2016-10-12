/*
 * Copyright (c) 2016 Fuzhou Rockchip Electronics Co.Ltd All rights reserved.
 * (wilen.gao@rock-chips.com)
 */

#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <poll.h>

#include <network/wifiHal.h>
#include "libwpa_client/wpa_ctrl.h"

//#define LOG_TAG "WifiHW"
#include "cutils/log.h"
#include "cutils/memory.h"
#include "cutils/misc.h"
#include "cutils/properties.h"
#include <sys/system_properties.h>
#include "private/android_filesystem_config.h"
#include <network/debug.h>

__BEGIN_DECLS
extern int do_dhcp(char *iname);
extern void get_dhcp_info(uint32_t *ipaddr, uint32_t *gateway, uint32_t *prefixLength,
                   uint32_t *dns1, uint32_t *dns2, uint32_t *server,
                   uint32_t *lease);
extern char *dhcp_lasterror();
extern int ifc_init();
extern void ifc_close();
__END_DECLS

void wifi_close_sockets();

static const char WPA_EVENT_IGNORE[]    = "CTRL-EVENT-IGNORE ";
static const char DRIVER_PROP_NAME[]    = "wlan.driver.status";
static const char P2P_SUPPLICANT_NAME[] = "p2p_supplicant";
static const char P2P_PROP_NAME[]       = "init.svc.p2p_supplicant";
static const char SUPP_CONFIG_TEMPLATE[]= "/system/etc/wifi/wpa_supplicant.conf";
static const char SUPP_CONFIG_FILE[]    = "/data/misc/wifi/wpa_supplicant.conf";
static const char P2P_CONFIG_FILE[]     = "/data/misc/wifi/p2p_supplicant.conf";

static const char IFNAME[]              = "IFNAME=";
#define IFNAMELEN                       (sizeof(IFNAME) - 1)
static char supplicant_name[PROPERTY_VALUE_MAX];
static char supplicant_prop_name[PROPERTY_KEY_MAX];

static struct wpa_ctrl *ctrl_conn;
static struct wpa_ctrl *monitor_conn;

static int exit_sockets[2];
static char primary_iface[PROPERTY_VALUE_MAX];

int do_dhcp_request(uint32_t *ipaddr, uint32_t *gateway, uint32_t *mask,
                    uint32_t *dns1, uint32_t *dns2, uint32_t *server, uint32_t *lease) {
    if (ifc_init() < 0)
        return -1;

    if (do_dhcp(primary_iface) < 0) {
        ifc_close();
        return -1;
    }
    ifc_close();
    get_dhcp_info(ipaddr, gateway, mask, dns1, dns2, server, lease);
    return 0;
}

const char *get_dhcp_error_string() {
    return dhcp_lasterror();
}

int ipv4_int_2_str(uint32_t addr, char* str) {
    unsigned char ip[4] = {0};

    ip[0] = (unsigned char)(addr & 0xff);
    ip[1] = (unsigned char)((addr >> 8) & 0xff);
    ip[2] = (unsigned char)((addr >> 16) & 0xff);
    ip[3] = (unsigned char)((addr >> 24) & 0xff);
    memset(str, 0, strlen(str));
    sprintf(str, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

    return 0;
}

/* 0 - not ready; 1 - ready. */
int check_wireless_ready(void)
{
    char line[1024], *ptr = NULL;
    FILE *fp = NULL;
    char count = 10;

retry:
    fp = fopen("/proc/net/dev", "r");
    if (fp == NULL) {
        LOGE("Couldn't open /proc/net/wireless\n");
        return 0;
    }

    while(fgets(line, 1024, fp)) {
        if ((strstr(line, "wlan0:") != NULL) || (strstr(line, "p2p0:") != NULL)) {
            LOGD("Wifi driver is ready for now...");
            fclose(fp);
            return 0;
        }
    }

    fclose(fp);
    while(count--) {
        usleep(5000000);
        LOGE("Wifi driver is not ready. retry! count:%d\n", count);
        goto retry;
    }

    LOGE("Wifi driver is not ready.\n");
    return -1;
}

bool is_wifi_enabled(void) {
    char supp_status[PROPERTY_VALUE_MAX] = {'\0'};

    if (property_get(supplicant_prop_name, supp_status, NULL)
            && strcmp(supp_status, "running") == 0) {
        LOGE("Supplicant is running, wifi is enabled.");
        return true;
    }

    return false;
}

int ensure_config_file_exists(const char *config_file)
{
    char buf[2048];
    int srcfd, destfd;
    int nread;
    int ret;

    ret = access(config_file, R_OK|W_OK);
    if ((ret == 0) || (errno == EACCES)) {
        if ((ret != 0) &&
            (chmod(config_file, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP) != 0)) {
            LOGE("Cannot set RW to \"%s\": %s", config_file, strerror(errno));
            return -1;
        }
        return 0;
    } else if (errno != ENOENT) {
        LOGE("Cannot access \"%s\": %s", config_file, strerror(errno));
        return -1;
    }

    srcfd = TEMP_FAILURE_RETRY(open(SUPP_CONFIG_TEMPLATE, O_RDONLY));
    if (srcfd < 0) {
        LOGE("Cannot open \"%s\": %s", SUPP_CONFIG_TEMPLATE, strerror(errno));
        return -1;
    }

    destfd = TEMP_FAILURE_RETRY(open(config_file, O_CREAT|O_RDWR, 0660));
    if (destfd < 0) {
        close(srcfd);
        LOGE("Cannot create \"%s\": %s", config_file, strerror(errno));
        return -1;
    }

    while ((nread = TEMP_FAILURE_RETRY(read(srcfd, buf, sizeof(buf)))) != 0) {
        if (nread < 0) {
            LOGE("Error reading \"%s\": %s", SUPP_CONFIG_TEMPLATE, strerror(errno));
            close(srcfd);
            close(destfd);
            unlink(config_file);
            return -1;
        }
        TEMP_FAILURE_RETRY(write(destfd, buf, nread));
    }

    close(destfd);
    close(srcfd);

    /* chmod is needed because open() didn't set permisions properly */
    if (chmod(config_file, 0660) < 0) {
        LOGE("Error changing permissions of %s to 0660: %s",
             config_file, strerror(errno));
        unlink(config_file);
        return -1;
    }

    if (chown(config_file, AID_SYSTEM, AID_WIFI) < 0) {
        LOGE("Error changing group ownership of %s to %d: %s",
             config_file, AID_WIFI, strerror(errno));
        unlink(config_file);
        return -1;
    }
    return 0;
}

int wifi_start_supplicant(void)
{
    char supp_status[PROPERTY_VALUE_MAX] = {'\0'};
    int count = 10; /* wait at most 20 seconds for completion */

    LOGD("%s ...", __func__);
    strcpy(supplicant_name, P2P_SUPPLICANT_NAME);
    strcpy(supplicant_prop_name, P2P_PROP_NAME);

    /* Ensure p2p config file is created */
    if (ensure_config_file_exists(P2P_CONFIG_FILE) < 0) {
        LOGE("Failed to create a p2p config file");
        return -1;
    }

    /* Check whether already running */
    if (property_get(supplicant_prop_name, supp_status, NULL)
            && strcmp(supp_status, "running") == 0) {
        return 0;
    }

    /* Before starting the daemon, make sure its config file exists */
    if (ensure_config_file_exists(SUPP_CONFIG_FILE) < 0) {
        LOGE("Wi-Fi will not be enabled");
        return -1;
    }

    /* Clear out any stale socket files that might be left over. */
    wpa_ctrl_cleanup();

    /* Reset sockets used for exiting from hung state */
    exit_sockets[0] = exit_sockets[1] = -1;

    property_get("wifi.interface", primary_iface, WIFI_INTERFACE);

    property_set("ctl.start", supplicant_name);
    sched_yield();

    usleep(1000000);
    while (count-- > 0) {
        if (property_get(supplicant_prop_name, supp_status, NULL)) {
            if (strcmp(supp_status, "running") == 0) {
                LOGD("%s, SUCCESS...", __func__);
                return 0;
            }
        }
        usleep(1000000);
    }
    return -1;
}

int wifi_stop_supplicant(void)
{
    char supp_status[PROPERTY_VALUE_MAX] = {'\0'};
    int count = 50; /* wait at most 5 seconds for completion */

    strcpy(supplicant_name, P2P_SUPPLICANT_NAME);
    strcpy(supplicant_prop_name, P2P_PROP_NAME);

    /* Check whether supplicant already stopped */
    if (property_get(supplicant_prop_name, supp_status, NULL)
        && strcmp(supp_status, "stopped") == 0) {
        return 0;
    }

    property_set("ctl.stop", supplicant_name);
    sched_yield();

    while (count-- > 0) {
        if (property_get(supplicant_prop_name, supp_status, NULL)) {
            if (strcmp(supp_status, "stopped") == 0)
                return 0;
        }
        usleep(100000);
    }
    LOGE("Failed to stop supplicant");
    return -1;
}

int wifi_connect_on_socket_path(const char *path)
{
    char supp_status[PROPERTY_VALUE_MAX] = {'\0'};

    /* Make sure supplicant is running */
    if (!property_get(supplicant_prop_name, supp_status, NULL)
            || strcmp(supp_status, "running") != 0) {
        LOGE("Supplicant not running, cannot connect");
        return -1;
    }

    ctrl_conn = wpa_ctrl_open(path);
    if (ctrl_conn == NULL) {
        LOGE("Unable to open connection to supplicant on \"%s\": %s",
             path, strerror(errno));
        return -1;
    }
    monitor_conn = wpa_ctrl_open(path);
    if (monitor_conn == NULL) {
        wpa_ctrl_close(ctrl_conn);
        ctrl_conn = NULL;
        return -1;
    }
    if (wpa_ctrl_attach(monitor_conn) != 0) {
        wpa_ctrl_close(monitor_conn);
        wpa_ctrl_close(ctrl_conn);
        ctrl_conn = monitor_conn = NULL;
        return -1;
    }

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, exit_sockets) == -1) {
        wpa_ctrl_close(monitor_conn);
        wpa_ctrl_close(ctrl_conn);
        ctrl_conn = monitor_conn = NULL;
        return -1;
    }

    return 0;
}

/* Establishes the control and monitor socket connections on the interface */
int wifi_connect_to_supplicant()
{
    static char path[PATH_MAX];

    snprintf(path, sizeof(path), "@android:wpa_%s", primary_iface);
    return wifi_connect_on_socket_path(path);
}

int wifi_send_command(const char *cmd, char *reply, size_t *reply_len)
{
    int ret;
    if (ctrl_conn == NULL) {
        LOGE("Not connected to wpa_supplicant - \"%s\" command dropped.\n", cmd);
        return -1;
    }
    ret = wpa_ctrl_request(ctrl_conn, cmd, strlen(cmd), reply, reply_len, NULL);
    if (ret == -2) {
        LOGE("'%s' command timed out.\n", cmd);
        /* unblocks the monitor receive socket for termination */
        TEMP_FAILURE_RETRY(write(exit_sockets[0], "T", 1));
        return -2;
    } else if (ret < 0 || strncmp(reply, "FAIL", 4) == 0) {
        return -1;
    }
    if (strncmp(cmd, "PING", 4) == 0) {
        reply[*reply_len] = '\0';
    }
    return 0;
}

int wifi_supplicant_connection_active()
{
    char supp_status[PROPERTY_VALUE_MAX] = {'\0'};

    if (property_get(supplicant_prop_name, supp_status, NULL)) {
        if (strcmp(supp_status, "stopped") == 0)
            return -1;
    }

    return 0;
}

int wifi_ctrl_recv(char *reply, size_t *reply_len)
{
    int res;
    int ctrlfd = wpa_ctrl_get_fd(monitor_conn);
    struct pollfd rfds[2];

    memset(rfds, 0, 2 * sizeof(struct pollfd));
    rfds[0].fd = ctrlfd;
    rfds[0].events |= POLLIN;
    rfds[1].fd = exit_sockets[1];
    rfds[1].events |= POLLIN;
    do {
        res = TEMP_FAILURE_RETRY(poll(rfds, 2, 30000));
        if (res < 0) {
            LOGE("Error poll = %d", res);
            return res;
        } else if (res == 0) {
            /* timed out, check if supplicant is active
             * or not ..
             */
            res = wifi_supplicant_connection_active();
            if (res < 0)
                return -2;
        }
    } while (res == 0);

    if (rfds[0].revents & POLLIN) {
        return wpa_ctrl_recv(monitor_conn, reply, reply_len);
    }

    /* it is not rfds[0], then it must be rfts[1] (i.e. the exit socket)
     * or we timed out. In either case, this call has failed ..
     */
    return -2;
}

int wifi_wait_on_socket(char *buf, size_t buflen)
{
    size_t nread = buflen - 1;
    int result;
    char *match, *match2;

    if (monitor_conn == NULL) {
        return snprintf(buf, buflen, "IFNAME=%s %s - connection closed",
                        primary_iface, WPA_EVENT_TERMINATING);
    }

    result = wifi_ctrl_recv(buf, &nread);

    /* Terminate reception on exit socket */
    if (result == -2) {
        return snprintf(buf, buflen, "IFNAME=%s %s - connection closed",
                        primary_iface, WPA_EVENT_TERMINATING);
    }

    if (result < 0) {
        LOGE("wifi_ctrl_recv failed: %s\n", strerror(errno));
        return snprintf(buf, buflen, "IFNAME=%s %s - recv error",
                        primary_iface, WPA_EVENT_TERMINATING);
    }
    buf[nread] = '\0';
    /* Check for EOF on the socket */
    if (result == 0 && nread == 0) {
        /* Fabricate an event to pass up */
        LOGE("Received EOF on supplicant socket\n");
        return snprintf(buf, buflen, "IFNAME=%s %s - signal 0 received",
                        primary_iface, WPA_EVENT_TERMINATING);
    }
    /*
     * Events strings are in the format
     *
     *     IFNAME=iface <N>CTRL-EVENT-XXX 
     *        or
     *     <N>CTRL-EVENT-XXX 
     *
     * where N is the message level in numerical form (0=VERBOSE, 1=DEBUG,
     * etc.) and XXX is the event name. The level information is not useful
     * to us, so strip it off.
     */

    if (strncmp(buf, IFNAME, IFNAMELEN) == 0) {
        match = strchr(buf, ' ');
        if (match != NULL) {
            if (match[1] == '<') {
                match2 = strchr(match + 2, '>');
                if (match2 != NULL) {
                    nread -= (match2 - match);
                    memmove(match + 1, match2 + 1, nread - (match - buf) + 1);
                }
            }
        } else {
            return snprintf(buf, buflen, "%s", WPA_EVENT_IGNORE);
        }
    } else if (buf[0] == '<') {
        match = strchr(buf, '>');
        if (match != NULL) {
            nread -= (match + 1 - buf);
            memmove(buf, match + 1, nread + 1);
            LOGE("supplicant generated event without interface - %s\n", buf);
        }
    } else {
        /* let the event go as is! */
        LOGE("supplicant generated event without interface and without message level - %s\n", buf);
    }

    return nread;
}

int wifi_wait_for_event(char *buf, size_t buflen)
{
    return wifi_wait_on_socket(buf, buflen);
}

void wifi_close_sockets()
{
    if (ctrl_conn != NULL) {
        wpa_ctrl_close(ctrl_conn);
        ctrl_conn = NULL;
    }

    if (monitor_conn != NULL) {
        wpa_ctrl_close(monitor_conn);
        monitor_conn = NULL;
    }

    if (exit_sockets[0] >= 0) {
        close(exit_sockets[0]);
        exit_sockets[0] = -1;
    }

    if (exit_sockets[1] >= 0) {
        close(exit_sockets[1]);
        exit_sockets[1] = -1;
    }
}

void wifi_close_supplicant_connection()
{
    char supp_status[PROPERTY_VALUE_MAX] = {'\0'};
    //int count = 50; /* wait at most 5 seconds to ensure init has stopped stupplicant */

    LOGD("%s ...", __func__);
    wifi_close_sockets();

    //while (count-- > 0) {
    //    if (property_get(supplicant_prop_name, supp_status, NULL)) {
    //        if (strcmp(supp_status, "stopped") == 0)
    //            return;
    //    }
    //    usleep(100000);
    //}
}

int wifi_command(const char *command, char *reply, size_t *reply_len)
{
    return wifi_send_command(command, reply, reply_len);
}

