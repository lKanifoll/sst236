/*
 * TCP_UDP.h
 *
 *  Created on: Jan 10, 2018
 *      Author: sergey
 */

#ifndef MAIN_TCP_UDP_H_
#define MAIN_TCP_UDP_H_
#include "Functions.h"
#include "lwip/api.h"



uint8_t Flag_Reconect;
uint8_t SSID[32];
uint8_t PASS[32];
uint8_t Buffer[1000],BufTmp[300];
uint16_t lenght, lenght_p;
uint8_t UID[11];
uint8_t Access_flag;
uint8_t TCP_flag;



char MQTT_Port[4];
char MQTT_IP[64];
char MAC_esp[18];

typedef enum {OKEY = 0, UNKNOW = 1, ERR = 3, ECRC = 4, TIMEOUT = 5, NOACCESS = 6} EX_Response_TypeDef;

xTaskHandle tcp_server_service1;
xTaskHandle tcp_server_service2;
xTaskHandle tcp_server_service3;

void tcp_server(void *ptr);
void udp_server(void *ptr);

void get_mac_buf();
void EX_Instructions_Read(struct netconn *conn);
void EX_Instructions_Write(struct netconn *conn, uint8_t *buf, uint16_t count, uint16_t size);
void EX_response(EX_Response_TypeDef response,struct netconn *conn);
void tcp_server_service();
void udp_server_service();
uint16_t CRC16(uint8_t *DATA, uint16_t length);
uint16_t CRC16_2(uint8_t *DATA, uint16_t start, uint16_t length);
uint8_t EX_Save_Config(uint8_t *buf, uint16_t count, uint16_t size);
#endif /* MAIN_TCP_UDP_H_ */
