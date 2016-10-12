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
#include <net/if.h>
#include <cutils/log.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <sys/types.h>  
#include <sys/stat.h>

#include <network/debug.h>
#include <network/config.h>
#include <network/net.h>
#include <network/ConnectivityServer.h>

#define  BUFSZ      PIPE_BUF
#define  FIFO_NAME	"/data/misc/wifi/fifo"
static pid_t mPid = 0;
static int pipe_fd_client = -1;

static int decode_network_message(const char* msg, NetWorkType_t* type, NetWorkState_t* state) {
    int i;
    int j;

    for (i=(int)WIFI; i<(int)NetworkTypeNUM; i++) {
        if (strstr(msg, TypeStr((NetWorkType_t)i))) {
            *type = (NetWorkType_t)i;
            break;
        }
    }
    for (j=(int)ENABLED; j<(int)NetworkStateNUM; j++) {
        if (strstr(msg, StateStr((NetWorkState_t)j))) {
            *state = (NetWorkState_t)j;
            break;
        }
    }

    if (i == (int)NetworkTypeNUM || j == (int)NetworkStateNUM)
        return -1;

    return 0;
}

static int network_state_change_dispatch(int* NetworkState, NetWorkType_t type, NetWorkState_t state) {
    switch (type) {
        case WIFI:
                if (state == CONNECTED && NetworkState[MODEM] == CONNECTED) {
                    LOGD("Wifi connected, need to disconnect 3G connection");
                } else if (state == DISCONNECT && NetworkState[MODEM] == DISCONNECT) {
                    LOGD("Wifi disconnectd ,try to connect 3G");
                }
            break;
        case SOFTAP:
                if (state == ENABLED) {
                    LOGD("Softap enabled");
                } else if (state == DISABLED) {
                    LOGD("Softap disabled");
                }
            break;
        case MODEM:
                if (state == CONNECTED && NetworkState[SOFTAP] == ENABLED) {
                    LOGD("3G connected event recived, need to reconfig SOFTAP's nds & route");
                    reconfig_Softap_tether_upstream();
                }
            break;
        default:
            LOGD("Unrecognize type...");
            break;
    }

    return 0;
}

static int network_state_monitor(void) {
    int pipe_fd = -1;
    int res = 0;
    char buffer[BUFSZ];

    if(access(FIFO_NAME, F_OK) == -1) {
        LOGD("%s: Create the fifo pipe[%s].\n", __func__, FIFO_NAME);
        res = mkfifo(FIFO_NAME, 0777);
        if(res != 0)
        {
            LOGE("%s: Could not create fifo %s\n", __func__, FIFO_NAME);
            exit(EXIT_FAILURE);
        }
    }

    pipe_fd = open(FIFO_NAME, O_RDONLY);
    if(pipe_fd != -1)
    {   
        NetWorkType_t type = WIFI;
        NetWorkState_t state = DISABLED;
        int NetworkState[NetworkTypeNUM] = {0};
        do {
            memset(buffer, 0, BUFSZ);
            res = read(pipe_fd, buffer, BUFSZ);
            if (DBG) LOGE("%s: read: %s", __func__, buffer);
            if (!decode_network_message(buffer, &type, &state)) {
                NetworkState[(int)type] = (int)state;
                if (DBG) LOGE("%s: NetworkState[%s] = %s", 
                        __func__, TypeStr(type), StateStr(state));
                network_state_change_dispatch(NetworkState, type, state);
            }
        } while (res > 0);
        
        close(pipe_fd);
    } else
        exit(EXIT_FAILURE);

    exit(0);
}

int set_network_state(NetWorkType_t type, NetWorkState_t state) {
    int res = -1;
    char buffer[BUFSZ] = {0};

    LOGD("%s enter...", __func__);
    if (!is_connectivity_server_running()) {
        LOGE("%s: Connectivity server isn't running.", __func__);
        return res;
    }
    if(access(FIFO_NAME, F_OK) == -1) {
        LOGD("%s: Create the fifo pipe[%s].\n", __func__, FIFO_NAME);
        res = mkfifo(FIFO_NAME, 0777);
        if(res != 0)
        {   
            LOGE("%s: Could not create fifo %s\n", __func__, FIFO_NAME);
            exit(EXIT_FAILURE);
        }
    }
  
    if (pipe_fd_client == -1)
        pipe_fd_client = open(FIFO_NAME, O_WRONLY|O_NONBLOCK);
    if(pipe_fd_client != -1) {
        sprintf(buffer, "%s-%s", TypeStr(type), StateStr(state));
        if (DBG) LOGD("%s: Set state [%s]", __func__, buffer);
        res = write(pipe_fd_client, buffer, strlen(buffer));
        if(res == -1) {
            LOGD("%s: Write error on pipe\n", __func__);
            return res;
        }
    }

    return 0;
}

int get_network_state(enum NetWorkType) {

    return 0;
}

int start_connectivity_server(void) {
    pid_t pid = 1;

    if (mPid) {
        LOGE("Connectivity Server is already running");
        return mPid;
    }

    if ((pid = fork()) < 0) {
        LOGE("%s, fork failed (%s)", __func__, strerror(errno));
        return -1;
    }

    if (!pid) {
        prctl(PR_SET_NAME, "ConnectivityServer", NULL, NULL, NULL);
        network_state_monitor();

        LOGE("ConnectivityServer failed to start");
        mPid = 0;
        return -1;
    } else {
        mPid = pid;
        LOGE("ConnectivityServer started successfully, pid=%d", mPid);
        usleep(100000);
    }
    return 0;
}

void stop_connectivity_server(void) {
    LOGD("%s start, mPid=%d", __func__, mPid);
    if (mPid != 0) {
        kill(mPid, SIGKILL);
        waitpid(mPid, NULL, 0);
        mPid = 0;
    }
    LOGD("%s end...", __func__);
}

bool is_connectivity_server_running(void) {
    return (mPid != 0)? true:false;
}
