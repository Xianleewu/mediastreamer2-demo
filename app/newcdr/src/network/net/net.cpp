/*
 * Copyright (c) 2016 Fuzhou Rockchip Electronics Co.Ltd All rights reserved.
 * (wilen.gao@rock-chips.com)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <linux/in.h>
#include <net/if.h>
#include <cutils/log.h>

#include <cutils/sockets.h>
#include <private/android_filesystem_config.h>
#include <network/wifiHal.h>
#include <network/debug.h>
#include <network/config.h>
#include <network/net.h>
#include <arpa/inet.h>
#include "cutils/properties.h"

static int do_monitor(int sock, int stop_after_cmd);
static int do_cmd(int sock, char *cmd);
static int Softap_config_tether_upstream(int sock);
static int Softap_config_untether_upstream(int sock);

static char *get_first_three_byte_from_ip(const char *ip_addr, char *buf) {
    unsigned int ipInt = 0;
    
    ipInt = inet_addr(ip_addr);
    sprintf(buf, "%d.%d.%d", (int)(ipInt & 0xff), (int)(ipInt>>8 & 0xff),
        (int)(ipInt>>16 & 0xff));

    LOGD("%s: %s", __func__, buf);

    return buf;
}

static int init_socket(void) {
    int sock;
    int retry = 10;

    while(retry--) {
        if ((sock = socket_local_client("netd",
                                     ANDROID_SOCKET_NAMESPACE_RESERVED,
                                     SOCK_STREAM)) >= 0) {
            return sock;
        }
        usleep(1000*1000);
    }

    LOGE("Error connecting (%s)\n", strerror(errno));
    return -1;
}

static int do_cmd(int sock, char *cmd) {

    if (write(sock, cmd, strlen(cmd) + 1) < 0) {
        return -1;
    }
    LOGD("do_cmd:cmd:%s", cmd);

    return do_monitor(sock, 1);
}

static int do_monitor(int sock, int stop_after_cmd) {
    char *buffer = (char *)malloc(4096);

    if (!stop_after_cmd)
        LOGE("[Connected to Netd]\n");

    while(1) {
        fd_set read_fds;
        struct timeval to;
        int rc = 0;

        to.tv_sec = 10;
        to.tv_usec = 0;

        FD_ZERO(&read_fds);
        FD_SET(sock, &read_fds);

        if ((rc = select(sock +1, &read_fds, NULL, NULL, &to)) < 0) {
            int res = errno;
            LOGE("Error in select (%s)\n", strerror(errno));
            free(buffer);
            return res;
        } else if (!rc) {
            continue;
            LOGE("[TIMEOUT]\n");
            return ETIMEDOUT;
        } else if (FD_ISSET(sock, &read_fds)) {
            memset(buffer, 0, 4096);
            if ((rc = read(sock, buffer, 4096)) <= 0) {
                int res = errno;
                if (rc == 0)
                    LOGE("Lost connection to Netd - did it crash?\n");
                else
                    LOGE("Error reading data (%s)\n", strerror(errno));
                free(buffer);
                if (rc == 0)
                    return ECONNRESET;
                return res;
            }

            int offset = 0;
            int i = 0;

            LOGD("do_monitor: %s",buffer);
            for (i = 0; i < rc; i++) {
                if (buffer[i] == '\0') {
                    int code;
                    char tmp[4];

                    strncpy(tmp, buffer + offset, 3);
                    tmp[3] = '\0';
                    code = atoi(tmp);

                    ALOGE("%s\n", buffer + offset);
                    if (stop_after_cmd) {
                        if (code >= 200 && code < 600)
                            return 0;
                    }
                    offset = i + 1;
                }
            }
        }
    }
    free(buffer);
    return 0;
}

int net_config_cmd(int sock, const char *cmd_string) {
    char cmd_str[1024] = {0}; 

    memcpy(cmd_str, "0 ", 2);
    memcpy(cmd_str+2, cmd_string, strlen(cmd_string));

    if (do_cmd(sock, cmd_str) == 0) {
        LOGE("%s: %s sucess...", __func__, cmd_str);
    } else {
        LOGE("%s: %s fail...", __func__, cmd_str);
        return -1;
    }

    return 0;
}

int softap_start_net_config(void) {
    int sock;
    char cmd[1024] = {0};
    char buf[20] = {0};

    sock = init_socket();
    if (sock < 0) {
        LOGE("%s: socket init fail...", __func__);
        return -1;
    }

    get_first_three_byte_from_ip(SOFTAP_GATEWAY_ADDR, buf);

    sprintf(cmd, "interface setcfg wlan0 %s 24 up multicast running broadcast", SOFTAP_GATEWAY_ADDR);
    net_config_cmd(sock, cmd);
    net_config_cmd(sock, "tether interface add wlan0");
    net_config_cmd(sock, "network interface add local wlan0");
    memset(cmd, 0, 1024);
    sprintf(cmd, "network route add local wlan0 %s.0/24", buf);
    net_config_cmd(sock, cmd);
    net_config_cmd(sock, "ipfwd enable");
    memset(cmd, 0, 1024);
    sprintf(cmd, "tether start %s.100 %s.%d", buf, buf, 100+SOFTAP_CLIENT_NUM);
    net_config_cmd(sock, cmd);

    usleep(100*1000);
    Softap_config_tether_upstream(sock);

    return 0;
}

int softap_stop_net_config(void) {
    int sock;

    sock = init_socket();
    if (sock < 0) {
        LOGE("%s: socket init fail...", __func__);
        return -1;
    }

    Softap_config_untether_upstream(sock);

    net_config_cmd(sock, "interface setcfg wlan0 0.0.0.0 0 up multicast running broadcast");
    net_config_cmd(sock, "tether interface remove wlan0");
    net_config_cmd(sock, "tether stop");
    net_config_cmd(sock, "network interface remove local wlan0");
    net_config_cmd(sock, "ipfwd disable");
    net_config_cmd(sock, "interface setcfg wlan0 down");

    return 0;
}

int set_route_and_dns(const char* interface, struct dhcpinfo* info) {
    int sock;
    unsigned int gateway;
    char str[20] = {0};
    char cmd[1024] = {0};
    int NETID = 0;

    sock = init_socket();
    if (sock < 0) {
        LOGE("%s: socket init fail...", __func__);
        return -1;
    }

    if (!memcmp(interface, "wlan", 4)) {
        NETID = WIFI_NETID;
    } else if (!memcmp(interface, "veth", 4)) {
        NETID = VETH_NETID;
    }

    sprintf(cmd, "network create %d", NETID);
    net_config_cmd(sock, cmd);
    memset(cmd, 0, 1024);
    sprintf(cmd, "network interface add %d %s", NETID, interface);
    net_config_cmd(sock, cmd);
    memset(cmd, 0, 1024);
    gateway = info->gateway;
    sprintf(str, "%d.%d.%d.0/%d", (gateway&0xff), (gateway>>8)&0xff, (gateway>>16)&0xff, info->mask);
    sprintf(cmd, "network route add %d %s %s", NETID, interface, str);
    net_config_cmd(sock, cmd);
    memset(cmd, 0, 1024);
    memset(str, 0, 20);
    ipv4_int_2_str(info->gateway, str);
    sprintf(cmd, "network route add %d %s 0.0.0.0/0 %s", NETID, interface, str);
    net_config_cmd(sock, cmd);
    memset(cmd, 0, 1024);
    memset(str, 0, 20);
    ipv4_int_2_str(info->dns1, str);
    sprintf(cmd, "resolver setnetdns %d 0 %s", NETID, str);
    net_config_cmd(sock, cmd);
    memset(cmd, 0, 1024);
    sprintf(cmd, "network default set %d", NETID);
    net_config_cmd(sock, cmd);

    return 0;
}

int set_cellular_route_and_dns(const char* interface, netinfo* info) {
    int sock;
    unsigned int gateway;
    char str[20] = {0};
    char cmd[1024] = {0};

    sock = init_socket();
    if (sock < 0) {
        ALOGE("%s: socket init fail...", __FUNCTION__);
        return -1;
    }

    sprintf(cmd, "network create %d", VETH_NETID);
    net_config_cmd(sock, cmd);
    memset(cmd, 0, 1024);
    sprintf(cmd, "network interface add %d %s", VETH_NETID, info->ifname);
    net_config_cmd(sock, cmd);
    memset(cmd, 0, 1024);
    sprintf(cmd, "network route add %d %s %s", VETH_NETID, info->ifname, info->gateways);
    net_config_cmd(sock, cmd);
    memset(cmd, 0, 1024);
    sprintf(cmd, "resolver setnetdns %d 0 %s", VETH_NETID, info->dnses);
    net_config_cmd(sock, cmd);
    memset(cmd, 0, 1024);
    sprintf(cmd, "network default set %d", VETH_NETID);
    net_config_cmd(sock, cmd);

    return 0;
}

int clean_connect_addrs(const char* interface) {
    int sock;
    char cmd[1024] = {0};
    int NETID = 0;

    sock = init_socket();
    if (sock < 0) {
        LOGE("%s: socket init fail...", __func__);
        return -1;
    }

    if (!memcmp(interface, "wlan", 4)){
        NETID = WIFI_NETID;
        LOGE("%s: NETID = WIFI_NETID", __func__);
    }else if (!memcmp(interface, "veth", 4)){
        NETID = VETH_NETID;
	LOGE("%s: NETID = VETH_NETID", __func__);
    }

    sprintf(cmd, "interface clearaddrs %s", interface);
    net_config_cmd(sock, cmd);
    memset(cmd, 0, 1024);
    sprintf(cmd, "network destroy %d", NETID);
    net_config_cmd(sock, cmd);

    return 0;
}

static int Softap_config_tether_upstream(int sock) {
    char cmd[1024] = {0};
    char buf[20] = {0};
    char upSDns[PROPERTY_VALUE_MAX] = {'\0'};

#ifdef SOFTAP_UPSTREAM_INTERFACE
    if (sock < 0) {
        LOGE("%s: socket init fail...", __func__);
        return -1;
    }

    if (property_get("net.vth0.dns", upSDns, NULL)) {
        if (strlen(upSDns) < 7)
            return -1;
    }
    LOGD("%s: get %s dns[%s]", __func__, SOFTAP_UPSTREAM_INTERFACE, upSDns);

    get_first_three_byte_from_ip(SOFTAP_GATEWAY_ADDR, buf);

    sprintf(cmd, "tether dns set %d %s", VETH_NETID, upSDns);
    net_config_cmd(sock, cmd);
    memset(cmd, 0, 1024);
    sprintf(cmd, "nat enable wlan0 %s 1 %s.0/24", SOFTAP_UPSTREAM_INTERFACE, buf);
    net_config_cmd(sock, cmd);
#endif
    return 0;
}

static int Softap_config_untether_upstream(int sock) {
    char cmd[1024] = {0};
    char upSDns[PROPERTY_VALUE_MAX] = {'\0'};
    
#ifdef SOFTAP_UPSTREAM_INTERFACE
    if (sock < 0) {
        LOGE("%s: socket init fail...", __func__);
        return -1;
    }

    sprintf(cmd, "ipfwd remove wlan0 %s", SOFTAP_UPSTREAM_INTERFACE);
    net_config_cmd(sock, cmd);
    memset(cmd, 0, 1024);
    sprintf(cmd, "nat disable wlan0 %s 0", SOFTAP_UPSTREAM_INTERFACE);
    net_config_cmd(sock, cmd);
#endif
    return 0;
}

int reconfig_Softap_tether_upstream(void) {
    int sock;

    sock = init_socket();
    if (sock < 0) {
        LOGE("%s: socket init fail...", __func__);
        return -1;
    }

    Softap_config_untether_upstream(sock);
    Softap_config_tether_upstream(sock);

    return 0;
}

#define WLAN_MAC_ADDR	"/sys/class/net/wlan0/address"
int get_wlan_mac_addr_str(char *mac_str) {
    char line[1024] = {0};
    FILE *fp = NULL;
    
    if (access(WLAN_MAC_ADDR, F_OK) == -1) {
        ALOGD("get wlan mac fail. %s not exists!\n", WLAN_MAC_ADDR);
        return -ENOENT;
    }
    fp = fopen(WLAN_MAC_ADDR, "r");
    if (fp == NULL) {
        ALOGE("Couldn't open %s\n", WLAN_MAC_ADDR);
        return -1;
    }
    
    if (fgets(line, 1024, fp) && mac_str != NULL) {
            memset(mac_str, 0, strlen(mac_str));
            memcpy(mac_str, line, strlen(line));
            ALOGD("get wlan mac addr: %s\n", mac_str);
    }
    
    fclose(fp);
    return 0;
}
