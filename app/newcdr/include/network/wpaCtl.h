#ifndef __WPACTL_H
#define __WPACTL_H

#define REPLY_BUF_SIZE  3*4096
#define EVENT_BUF_SIZE  2048
#define STACMDPREFEX    "IFNAME=wlan0 "
#define WPA_EVENT_PREFIX	"CTRL-EVENT-"
#define SCAN_RESULTS_STR	"SCAN-RESULTS"
#define WIFI_STATE_CHANGE	"STATE-CHANGE"
#define WIFI_STATE_DISCONNECT	"DISCONNECTED"
#define WIFI_STATE_CONNECT	"CONNECTED"
#define WPA_TERMINATING		"TERMINATING"
#define WIFI_WPS_PBC		"WPS-AP-AVAILABLE-PBC"
#define WIFI_WPS_TIMEOUT	"WPS-TIMEOUT"

enum wpa_states {
	WPA_DISCONNECTED,
	WPA_INTERFACE_DISABLED,
	WPA_INACTIVE,
	WPA_SCANNING,
	WPA_AUTHENTICATING,
	WPA_ASSOCIATING,
	WPA_ASSOCIATED,
	WPA_4WAY_HANDSHAKE,
	WPA_GROUP_HANDSHAKE,
	WPA_COMPLETED
};
const char* wpaStateInt2Str(int state);

bool scan(void);
bool getScanResult(char* result);
int addNetwork(void);
bool listNetworks(char *result);
bool removeNetwork(int ID);
bool setNetwork(int ID, char* name, char* value);
bool enableNetwork(int ID, bool disableOthers);
bool enableAllNetwork(void);
bool disableNetwork(int ID);
bool disconnect(void);
bool reconnect(void);
int addNewAPConfig(char* ssid, char* password);
bool status(char* result);
bool getSignalPoll(char * result);
bool startDriver(void);
bool stopDriver(void);
void enableAutoConnect(bool enable);
bool startWpsPbc(void);
bool waitForEvent(char* buf);
#endif
