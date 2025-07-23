#ifndef __NETWORK_H__
#define __NETWORK_H__

#include "model/model.h"


int       network_init(void);
int       network_get_ip_address(char *ifname, char *ipaddr);
int       network_read_wifi_scan(wifi_network_t **networks);
void      network_save_config(void);
int       network_ntp_update(time_t *time);
pthread_t launch_network_time_sync_task(void (*cb)(time_t));

wifi_status_t network_status(char *ssid);
void          network_connect(char *ssid, char *psk);
void          network_scan(void);

#endif
