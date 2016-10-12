#ifndef _HOTSPOTCLI_H
#define _HOTSPOTCLI_H

#define CMD_RETURN_BUF_LEN	4096
int hostapd_cmd_ping(void);
int hostapd_cmd_status(char *buf);
int hostapd_cmd_get_config(char *buf);
int hostapd_cmd_get_all_stations(char *buf);
int hostapd_ctrl_recv(char *reply, size_t *reply_len);

#endif
