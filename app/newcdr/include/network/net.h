#ifndef _NET_H
#define _NET_H

typedef struct {
	char	ifname[16];
	char	addresses[16];
	char	dnses[32];
	char	gateways[16];
}netinfo;

int softap_start_net_config(void);
int softap_stop_net_config(void);
int set_route_and_dns(const char* interface, struct dhcpinfo* info);
int clean_connect_addrs(const char* interface);
int reconfig_Softap_tether_upstream(void);
int get_wlan_mac_addr_str(char *mac_str);
int set_cellular_route_and_dns(const char* interface, netinfo* info);
#endif
