/*
 * smartconfig.h
 *
 *  Created on: Dec 19, 2017
 *      Author: sergey
 */

#ifndef MAIN_SMARTCONFIG_H_
#define MAIN_SMARTCONFIG_H_

#include "Functions.h"

struct T_instruction{
  uint8_t ID;
  uint16_t length;
  uint16_t CRC;
};
xTaskHandle smartconfig_handle;
xTaskHandle mqtt_publish_handle;
xTaskHandle mqtt_subscribe_handle;
xTaskHandle tcp_server_handle;
xTaskHandle udp_server_handle;
xTaskHandle connect_wifi_handle;
uint8_t conn_flag;
uint8_t ap_flag;
uint8_t first_link;
uint8_t task_flag;
uint8_t WIFI;
uint8_t wifi_start_flag;
uint8_t disconnect;

void initialise_wifi(void);
void smartconfig_example_task(void * parm);
void set_wifi_sta(void);
void set_wifi_ap(void);
void reconnect_wifi(void);

#endif /* MAIN_SMARTCONFIG_H_ */
