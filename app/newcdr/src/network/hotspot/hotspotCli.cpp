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
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
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
#include <network/hotspotCli.h>
#include "libwpa_client/wpa_ctrl.h"

static int exit_sockets[2] = {-1};
static struct wpa_ctrl *ctrl_conn = NULL;
static struct wpa_ctrl *monitor_conn = NULL;
#define CONFIG_CTRL_IFACE_DIR "/data/misc/wifi/hostapd/wlan0"

static int hostapd_open_connection(void) {
	if (ctrl_conn == NULL) {
		ctrl_conn = wpa_ctrl_open(CONFIG_CTRL_IFACE_DIR);
		if (ctrl_conn == NULL) {
			LOGE("Unable to open connection to \"%s\": %s",
			CONFIG_CTRL_IFACE_DIR, strerror(errno));
			return -1;
		}
	}
	if (monitor_conn == NULL) {
		monitor_conn = wpa_ctrl_open(CONFIG_CTRL_IFACE_DIR);
		if (monitor_conn == NULL) {
			wpa_ctrl_close(ctrl_conn);
			ctrl_conn = NULL;
			return -1;
		}
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

static void hostapd_close_connection(void) {
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

static void hostapd_cli_msg_cb(char *msg, size_t len) {
        LOGD("%s, len=%d\n", msg, len);
}

static int _hostapd_ctrl_command(struct wpa_ctrl *ctrl, const char *cmd, char *addr) {
        char buf[CMD_RETURN_BUF_LEN], *pos;
        size_t len;
        int ret;

        if (ctrl == NULL) {
                LOGE("Not connected to hostapd - command dropped.\n");
                return -1;
        }
        len = sizeof(buf) - 1;
        ret = wpa_ctrl_request(ctrl, cmd, strlen(cmd), buf, &len,
                               hostapd_cli_msg_cb);
        if (ret == -2) {
                LOGE("'%s' command timed out.\n", cmd);
                return -2;
        } else if (ret < 0) {
                LOGE("'%s' command failed.\n", cmd);
                return -1;
        }

	buf[len] = '\0';
	if (memcmp(buf, "FAIL", 4) == 0)
		return -1;
	//LOGE("%s", buf);

	pos = buf;
	while (*pos != '\0' && *pos != '\n')
		pos++;
	*pos = '\0';
	if (addr != NULL)
		memcpy(addr, buf, CMD_RETURN_BUF_LEN);

        return 0;
}


static int hostapd_ctrl_command(const char *cmd, char *buf) {
        if (ctrl_conn == NULL) {
                if(hostapd_open_connection()) {
			LOGE("%s: hostapd open connect fail", __func__);
			return -1;
		}
		LOGD("%s: hostapd open connect success.", __func__);
	}

        return _hostapd_ctrl_command(ctrl_conn, cmd, buf);
}

int hostapd_cmd_ping(void) {
        return hostapd_ctrl_command("PING", NULL);
}

int hostapd_cmd_status(char *buf) {
        return hostapd_ctrl_command("STATUS", buf);
}

int hostapd_cmd_get_config(char *buf) {
        return hostapd_ctrl_command("GET_CONFIG", buf);
}

int hostapd_cmd_get_all_stations(char *buf) {
	char addr[CMD_RETURN_BUF_LEN] = {0}, cmd[64] = {0};

	if (hostapd_ctrl_command("STA-FIRST", addr))
		return -1;
	do {
		if (strlen(addr) != 0) {
			strncat(buf, "[", 1);
			strncat(buf, addr, strlen(addr));
			strncat(buf, "] ", 1);
		}
		memset(cmd, 0, 64);
		snprintf(cmd, sizeof(cmd), "STA-NEXT %s", addr);
		memset(addr, 0, CMD_RETURN_BUF_LEN);
	} while (hostapd_ctrl_command(cmd, addr) == 0);

	return 0;
}

int hostapd_ctrl_recv(char *reply, size_t *reply_len) {
	int res;
	int ctrlfd;
	struct pollfd rfds[2];

	if (monitor_conn == NULL) {
		if (hostapd_open_connection()) {
			LOGE("%s: hostapd open connect fail", __func__);
			return -1;
		}
		LOGD("%s: hostapd open connect success.", __func__);
	}

        ctrlfd = wpa_ctrl_get_fd(monitor_conn);
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
		}
	} while (res == 0);

	if (rfds[0].revents & POLLIN) {
		if(!wpa_ctrl_recv(monitor_conn, reply, reply_len)) {
			LOGD(">%s", reply);
			return 0;
		}
	}

	return -2;
}
