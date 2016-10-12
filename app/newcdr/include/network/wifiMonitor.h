#ifndef __WIFIMONITOR_H
#define __WIFIMONITOR_H

#define	WIFI_SIGNAL_LEVEL	"wifi.signal.level"
#define WIFI_SIGNAL_UPDATE	200
#define WIFI_SIGNAL_INTERVAL	300

int startWifiSignalPoll(void);
void stopWifiSignalPoll(void);
int startWifiMonitor(void);
void stopWifiMonitor(void);
#endif
