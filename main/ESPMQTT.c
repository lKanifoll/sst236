/*
 * mqtt.c
 *
 *  Created on: Dec 20, 2017
 *      Author: sergey
 */

#include "ESPMQTT.h"
#include "Functions.h"

//#define DEBUG

int s; //socket pointer

bool Connect_mqttserver()
{

	struct addrinfo *res;
	const struct addrinfo hints =
	{ .ai_family = AF_INET, .ai_socktype = SOCK_STREAM, };

	//printf("MQTT_Port %s\n", MQTT_Port);
	// resolve the IP of the target website
	int result = getaddrinfo(&MQTT_IP[0], &MQTT_Port[0], &hints, &res);
	if ((result != 0) || (res == NULL))
	{
		printf("Unable to resolve IP for target website %s\n", &MQTT_IP[0]);
		return pdFALSE;
	}
	//printf("Target website's IP resolved\n");

	// create a new socket
	s = socket(res->ai_family, res->ai_socktype, 0);
	if (s < 0)
	{
		printf("Unable to allocate a new socket\n");
		freeaddrinfo(res);
		return pdFALSE;
	}
	//printf("Socket allocated, id=%d\n", s);



	// connect to the specified server
	result = connect(s, res->ai_addr, res->ai_addrlen);
	if (result != 0)
	{
		printf("Unable to connect to the target website\n");
		freeaddrinfo(res);
		return pdFALSE;
	}
	//printf("Connected to the target website\n");
	freeaddrinfo(res);
	bzero(Buffer, sizeof(Buffer));
	bzero(recv_buf, sizeof(recv_buf));

	buffer_add_string((uint8_t*) "\x10\x1D\x00\x04MQTT\x04\x02\x00\x00\x00\x11", 0, 14);
	buffer_add_string((uint8_t*)MAC_esp, 14, 17);

#ifdef DEBUG
	printf("\n");
#endif
	result = write(s, Buffer, 31);
	//printf("%d\n", result);
	if (result < 0)
	{
		printf("Unable to send the HTTP request\n");
		//close(s);
		return pdFALSE;
	}

	struct timeval receiving_timeout;
	receiving_timeout.tv_sec = 5;
	receiving_timeout.tv_usec = 0;
	if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout, sizeof(receiving_timeout)) < 0) {
		printf("... failed to set socket receiving timeout\n");
		close(s);
		return pdFALSE;
	}
	//printf("... set socket receiving timeout success\n");

	//---------------------------------------------------------------------------------------------- receive
	uint16_t num_bytes = 0;
	num_bytes = read(s, &recv_buf[0], sizeof(recv_buf) - 1);
	//printf("num_bytes %d\n", num_bytes);

	if (num_bytes == 0)
	{
		//close(s);
		return pdFALSE;
	}

#ifdef DEBUG
	printf("\n");
#endif

	if ((recv_buf[0] == 0x20) && (recv_buf[1] == 0x02))
	{
		return pdTRUE;
	}
	else
	{
		//close(s);
		return pdFALSE;
	}

	//----------------------------------------------------------------------------------------------

}

void mqtt_subscribe(void *ptr)
{

	//vTaskDelay(5000 / portTICK_PERIOD_MS);
	while (1)
	{
		uint16_t count = 0;
		uint8_t ID;
		uint16_t CRC, CRC_Load;

		if (Connect_mqttserver())
		{
			uint16_t Position = 0;

			uint16_t num_bytes;

			bzero(Buffer, sizeof(Buffer));
			bzero(BufTmp, sizeof(BufTmp));

			buffer_add_string((uint8_t*) &UID[0], Position, sizeof(UID) - 1);
			Position += sizeof(UID) - 1;
			buffer_add_char('/', Position++);
			buffer_add_string((uint8_t*) &MAC_esp[0], Position, sizeof(MAC_esp) - 1);
			Position += sizeof(MAC_esp) - 1;
			buffer_add_char('/', Position++);
			buffer_add_string((uint8_t*) "to", Position, sizeof("to") - 1);
			Position += sizeof("to") - 1;
			lenght = sizeof(UID) - 1 + sizeof(MAC_esp) - 1 + sizeof("to") - 1 + 2;

			memcpy((void*) BufTmp, (void*) Buffer, lenght);

			Position = 0;
			buffer_add_string((uint8_t*) "\x82\x00\x00\x01", 0, 4); // кладем инструкцию в буфер + длина пакета 6 БАЙТ
			Position += 4;
			buffer_add_int(lenght, Position); // topic length
			Position += 2;
			buffer_add_string(&BufTmp[0], Position, lenght); // topic
			Position += (lenght);
			buffer_add_char(0, Position++); //qos = 0
			MQTT_Encoding_size(MQTT_Encod_size, Position - 2); //func encode
			buffer_add_char(MQTT_Encod_size[0], 1); //encode size of parsel into 2 byte

			num_bytes = 0;
			//printf("\n");

			write(s, &Buffer[0], Position);
			//printf("%d\n", result);

			bzero(recv_buf, sizeof(recv_buf));

			//vTaskDelay(1000 / portTICK_RATE_MS);
			num_bytes = read(s, &recv_buf[0], sizeof(recv_buf));

			if ((recv_buf[0] == 0x90) && (recv_buf[1] == 0x03))
			{
				bzero(recv_buf, sizeof(recv_buf));

				num_bytes = read(s, &recv_buf[0], sizeof(recv_buf));
				if(num_bytes > 500)
				{
					Position = 0;
					buffer_add_string((uint8_t*) "\x31\x00", 0, 2); // кладем инструкцию в буфер + длина пакета 6 БАЙТ
					Position += 2;
					buffer_add_int(lenght, Position); // topic length
					Position += 2;
					buffer_add_string(&BufTmp[0], Position, lenght); // topic
					Position += (lenght);
					buffer_add_string((uint8_t*) "\x00", Position, sizeof("\x00") - 1);
					Position += sizeof("\x00") - 1;
					MQTT_Encoding_size(MQTT_Encod_size, Position - 2); //func encode
					buffer_add_char(MQTT_Encod_size[0], 1); //encode size of parsel into 2 byte
					write(s, &Buffer[0], Position);
				}
				else
				{
#ifdef DEBUG
					printf("%d\n", num_bytes);
					printf("SUBSCRIBE***************************************************************************************\n");
					for (uint8_t u = lenght+4; u < num_bytes; u++)
					{
						printf("%02X", recv_buf[u]);
					}
					printf("\n");
					printf("************************************************************************************************\n");

					printf("\n");
#endif
				}
				 if(num_bytes && (TCP_flag == 0))
				 {
					 while (count<num_bytes)
				      {
				        if(!strncmp((char*)&recv_buf[count],"\x02TQW",4))
				        {
			              count += 3;
				          ID = recv_buf[count++];
				          lenght_p = ((uint16_t)(recv_buf[count] << 8)&0xFF00);
				          lenght_p |=((uint16_t)recv_buf[count+1])&0x00FF;
				          count+=2;
				          CRC = CRC16_2((uint8_t*)recv_buf, count - 6, lenght_p + 6);
				          CRC_Load = (((uint16_t)recv_buf[count + lenght_p] << 8)&0xFF00);
				          CRC_Load |= ((uint16_t)recv_buf[count + lenght_p + 1])&0x00FF;
				          if( CRC_Load == CRC )
				          {
				            if(ID == 'W'){
				            	//затираем топик после получения
				                Position = 0;
				                buffer_add_string((uint8_t*) "\x31\x00", 0, 2); // кладем инструкцию в буфер + длина пакета 6 БАЙТ
				                Position += 2;
				                buffer_add_int(lenght, Position); // topic length
				                Position += 2;
				                buffer_add_string(&BufTmp[0], Position, lenght); // topic
				                Position += (lenght);
				                buffer_add_string((uint8_t*) "\x00", Position, sizeof("\x00") - 1);
				                Position += sizeof("\x00") - 1;
				                MQTT_Encoding_size(MQTT_Encod_size, Position - 2); //func encode
				                buffer_add_char(MQTT_Encod_size[0], 1); //encode size of parsel into 2 byte
				                write(s, &Buffer[0], Position);

				                //Save settings
				            	EX_Save_Config((uint8_t*)&recv_buf[0], count, lenght_p);

				                if(Flag_Reconect)
				                {

				                	reconnect_wifi();
				                	Flag_Reconect = 0;
				                }

				              break;
				            }
				          }

				        }
				        count++;
				      }
				 }else
				 {
#ifdef DEBUG
					 printf("Access denied\n");
#endif
				 }



				//printf("Subscribe!\n");
//*********************************************************************************************************************
				vTaskDelay(1000 / portTICK_PERIOD_MS);
				uint16_t Position = 0;
				uint16_t lenght = 0;

				bzero(Buffer, sizeof(Buffer));
				bzero(BufTmp, sizeof(BufTmp));

				buffer_add_string((uint8_t*) &UID[0], Position, sizeof(UID) - 1);
				Position += sizeof(UID) - 1;
				buffer_add_char('/', Position++);
				buffer_add_string((uint8_t*) &MAC_esp[0], Position, sizeof(MAC_esp) - 1);
				Position += sizeof(MAC_esp) - 1;
				buffer_add_char('/', Position++);
				buffer_add_string((uint8_t*) "from", Position, sizeof("from") - 1);
				Position += sizeof("from") - 1;
				lenght = sizeof(UID) - 1 + sizeof(MAC_esp) - 1 + sizeof("from") - 1 + 2;
				memcpy((void*) BufTmp, (void*) Buffer, lenght);

				MQTT_Encoding_size(MQTT_Encod_size, EX_Send_Config(0) + lenght + 2); //кодируем длина топика +2байта длины топика + длина конфига
				bzero(Buffer, sizeof(Buffer));
				Position = 0;
				if (MQTT_Encod_size[1] == 0x00) //если длина пакета меньше 128байт
				{
					buffer_add_string((uint8_t*) "\x31\x00", 0, 2);
					Position += 2;
					buffer_add_char(MQTT_Encod_size[0], 1);

				}
				else //если больше, то добавляется 2 байт блины посылки после первого
				{
					buffer_add_string((uint8_t*) "\x31\x00\00", 0, 3);
					Position += 3;
					buffer_add_string(&MQTT_Encod_size[0], 1, 2);
				}

				buffer_add_int(lenght, Position);					// topic length
				Position += 2;
				buffer_add_string(&BufTmp[0], Position, lenght); // topic
				Position += (lenght);
				uint16_t pos = 0;
				pos = EX_Send_Config(Position);
				Position += pos;
/*
				printf("PUBLISH***************************************************************************************\n");
				for (uint8_t u = 0; u < Position; u++)
				{
					printf("%02X", Buffer[u]);
				}
				printf("\n");
				printf("**********************************************************************************************\n");

*/
				write(s, &Buffer[0], Position);
				//printf("%d\n", result);
				close(s);
				//printf("Publish!\n");
				//vTaskDelete(NULL);
			}
			else
			{
				close(s);
				bzero(recv_buf, sizeof(recv_buf));
				//vTaskDelete(NULL);
			}

		}
		else
		{
			//printf("Connection false\n");
			close(s);
			//vTaskDelete(NULL);
		}
		//printf("MQTT\n");
		vTaskDelay(5000 / portTICK_PERIOD_MS);
		//vTaskDelete(NULL);
	}

}

void MQTT_Encoding_size(uint8_t* encoding_result, uint16_t size) // функция кодирования длины пакета
{
	uint8_t temp;
	while (size > 0)
	{
		temp = size % 128;
		size = size / 128;
		if (size)
		{
			temp |= 128;
		}
		*encoding_result = temp;
		encoding_result++;
	}
}


