#ifndef _WIFIHAL_H
#define _WIFIHAL_H

#define WIFI_INTERFACE          "wlan0"

struct dhcpinfo {
	uint32_t ipaddr;
	uint32_t gateway;
	uint32_t mask;
	uint32_t dns1;
	uint32_t dns2;
	uint32_t server;
	uint32_t lease;
};

int check_wireless_ready(void);
bool is_wifi_enabled(void);
int wifi_start_supplicant(void);
int wifi_stop_supplicant(void);
int wifi_connect_to_supplicant(void);
void wifi_close_supplicant_connection(void);
int wifi_wait_for_event(char *buf, size_t len);
int wifi_command(const char *command, char *reply, size_t *reply_len);

int do_dhcp_request(uint32_t *ipaddr, uint32_t *gateway, uint32_t *mask,
                    uint32_t *dns1, uint32_t *dns2, uint32_t *server, uint32_t *lease);
const char *get_dhcp_error_string();
int ipv4_int_2_str(uint32_t addr, char* str);
#endif  // _WIFIHAL_H
