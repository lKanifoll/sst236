/*
 * ESPMQTT.h
 *
 *  Created on: Jan 25, 2018
 *      Author: sergey
 */

#ifndef MAIN_ESPMQTT_H_
#define MAIN_ESPMQTT_H_

#include "Functions.h"

uint8_t MQTT_Encod_size[2];
char recv_buf[600];
extern int s;


//xTaskHandle mqtt_task_handle;
void MQTT_Encoding_size(uint8_t* encoding_result, uint16_t size);
bool Connect_mqttserver();
void mqtt_subscribe(void *ptr);
void mqtt_publish(void *ptr);
#endif /* MAIN_ESPMQTT_H_ */
