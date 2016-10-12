#ifndef _CONNECTIVITYSERVER_H
#define _CONNECTIVITYSERVER_H

#define DBG	true

typedef enum NetWorkType {
	WIFI = 0,
	SOFTAP,
	MODEM,
	NetworkTypeNUM,
}NetWorkType_t;

typedef enum NetWorkState {
	ENABLED = 1,
	DISABLED,
	CONNECTING,
	CONNECTED,
	DISCONNECT,
	NetworkStateNUM,
}NetWorkState_t;

inline const char* TypeStr(NetWorkType_t type) {
    switch (type) {
        case WIFI: return "WIFI"; break;
        case SOFTAP: return "SOFTAP"; break;
        case MODEM: return "MODEM"; break;
        default: return "NONE"; break;
    }
}

inline const char* StateStr(NetWorkState_t state) {
    switch (state) {
        case ENABLED: return "ENABLED"; break;
        case DISABLED: return "DISABLED"; break;
        case CONNECTING: return "CONNECTING"; break;
        case CONNECTED: return "CONNECTED"; break;
        case DISCONNECT: return "DISCONNECT"; break;
        default: return "NONE"; break;
    }
}

int set_network_state(NetWorkType_t type, NetWorkState_t state);
int get_network_state(enum NetWorkType);
int start_connectivity_server(void);
void stop_connectivity_server(void);
bool is_connectivity_server_running(void);
#endif
