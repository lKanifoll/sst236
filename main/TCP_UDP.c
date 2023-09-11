/*
 * TCP_UDP.c
 *
 *  Created on: Jan 10, 2018
 *      Author: sergey
 */
#include "TCP_UDP.h"
#include "Functions.h"


void get_mac_buf()
{
	while(ESP_OK != esp_wifi_get_mac(curr_interface, temp_mac));
	sprintf(&MAC_esp[0], "%02X:%02X:%02X:%02X:%02X:%02X",temp_mac[0],temp_mac[1],temp_mac[2],temp_mac[3],temp_mac[4],temp_mac[5]);
	//printf("%s\n",MAC_esp);
}

void buffer_add_char(uint8_t new_char, uint16_t position)
{
  Buffer[position] = new_char;
}


void buffer_add_string(uint8_t *new_part, uint16_t position, uint16_t count)
{
  uint8_t sym;
  while(count--)
  {
    sym = *new_part++;
    Buffer[position++] = sym;
  }
}

void buffer_add_int(uint16_t new_int, uint16_t position)
{
  Buffer[position + 1] = new_int;
  Buffer[position] = (new_int >> 8);
}




//---------------------------------------------------------------------------------------------------------------
//------------------------------------------------Creating full data buff----------------------------------------
//---------------------------------------------------------------------------------------------------------------
uint16_t EX_Send_Config(uint16_t pos)
{
	uint8_t j, i;
	uint8_t access_status = 0;
	volatile uint16_t pos_temp = pos;
	if (Access_flag == 1) //||(!POWER_ON_OFF))
		access_status = 0;
	buffer_add_string((uint8_t *) "\x02TAR\x00\x00", pos_temp, 6); // instruction set + 6 bytes length
	pos_temp += 6;
	//--------------------------------------
	buffer_add_string((uint8_t *) "T\x00\x03", pos_temp, 3); // Current temperature
	pos_temp += 3;
	if (Sensor_err)
		buffer_add_char(0x81, pos_temp++);
	else
		buffer_add_char(Themperature, pos_temp++);
	//buffer_add_char(Themperature, pos_temp++);
	buffer_add_char(Day_Event_Now, pos_temp++);      // need to add day of event
	buffer_add_char(Event_now, pos_temp++);				// need to add event now

	//--------------------------------------
	buffer_add_string((uint8_t *) "S\x00\x06", pos_temp, 3);         // Settings
	pos_temp += 3;
	buffer_add_char(POWER_ON_OFF, pos_temp++);
	buffer_add_char(Heat_mode, pos_temp++);						//heating mode
	buffer_add_char(light_mode, pos_temp++);
	buffer_add_char(set_floor_temperature, pos_temp++);

	//buffer_add_char(12, pos_temp++);
	buffer_add_char(love_temperature, pos_temp++); // Поправить этот параметр, он ограничивает температуру по воздуху
	buffer_add_char((int8_t) (delta_air_temperature - 5), pos_temp++);
	//--------------------------------------
	buffer_add_string((uint8_t *) "C\x00\x54", pos_temp, 3);    // Heating table
	pos_temp += 3;
	for (i = 0; i < 7; i++)
		for (j = 0; j < 4; j++)
		{
			buffer_add_int(Program_step[i][j].Time_prog, pos_temp);
			pos_temp += 2;
			buffer_add_char(Program_step[i][j].Temp_set, pos_temp++);
		}
	//--------------------------------------
	//buffer_add_char('N', pos_temp++);                    // Reg name //not use/
	//buffer_add_string(Name_room, pos, Name_room[1]+2);
	//pos_temp += Name_room[1]+2;
	//--------------------------------------
	buffer_add_string((uint8_t *) "I\x00\x05", pos_temp, 3);   // Identification
	pos_temp += 3;
	buffer_add_string((uint8_t *) "O1", pos_temp, 2);       // Identification
	pos_temp += 2;
	buffer_add_string((uint8_t *) VERSION, pos_temp, 3);
	pos_temp += 3;
	//--------------------------------------
	buffer_add_string((uint8_t *) "M\x00\x11", pos_temp, 3);         // MAC
	pos_temp += 3;
	buffer_add_string((uint8_t *) MAC_esp, pos_temp, 17);
	pos_temp += 17;
	//--------------------------------------
	buffer_add_string((uint8_t *) "A\x00\x01", pos_temp, 3);    // Access status
	pos_temp += 3;
	buffer_add_char(access_status, pos_temp++);
	//--------------------------------------
	buffer_add_string((uint8_t *) "Z\x00\x01", pos_temp, 3);   // Type of sensor
	pos_temp += 3;
	buffer_add_char(sensor_num, pos_temp++);
	//--------------------------------------
	buffer_add_string((uint8_t *)"L\x00\x03", pos_temp, 3);         // Time when relay was ON
	 pos_temp += 3;
	 Power1=(uint16_t)((Time_relay_ON+temp_Time_relay_ON)/60);
	 Power2=(uint8_t)((Time_relay_ON+temp_Time_relay_ON)%60);

	 //printf("Power1 %d  Power2 %d\n",Power1,Power2);

	 buffer_add_int(Power1, pos_temp);
	 pos_temp+=2;
	 buffer_add_char(Power2, pos_temp++);
	//--------------------------------------
	buffer_add_string((uint8_t *) "D\x00\x0A", pos_temp, 3); // Time stamp of thermostat
	pos_temp += 3;
	//RTC_time_get();
	buffer_add_string(timestamp_ascii, pos_temp, 10);
	pos_temp += 10;
	//--------------------------------------
	buffer_add_string((uint8_t *) "W\x00\x01", pos_temp, 3);      // RSSI volume
	pos_temp += 3;
	buffer_add_char(WIFI_Level, pos_temp++);
	//--------------------------------------
	buffer_add_string((uint8_t *) "H\x00\x01", pos_temp, 3);     // Relay status
	pos_temp += 3;
	buffer_add_char(Relay_flag, pos_temp++);

	//--------------------------------------
	buffer_add_int(pos_temp - 6 - pos, pos + 4);             // Parsel length
	//--------------------------------------

	uint16_t lenght_temp = pos_temp - pos;

	buffer_add_int(CRC16_2(Buffer, pos, lenght_temp), pos_temp);    // Write CRC
	pos_temp += 2;
	//for(uint8_t y=0;y<pos_temp;y++)
	// printf("%02X",Buffer[y]);

	return lenght_temp + 2; // Length
}




uint8_t EX_Save_Config(uint8_t *buf, uint16_t count, uint16_t size)
{
  uint8_t EE_error_flag = 1;
  uint16_t length, i, j;
  uint16_t finish;
  finish = count + size;
  //printf("%d %d\n",count,finish);
  nvs_open("storage", NVS_READWRITE, &my_handle);

  while(count < (finish))
  {


	  //----------------------------------------------------------------------------------------- SERVICE COMMANDS

		if(buf[count] == 'R')  // RESTART ESP
		{
			Time_relay_ON += temp_Time_relay_ON;
			printf("Time_relay_ON %d\n",Time_relay_ON);
			nvs_open("storage", NVS_READWRITE, &my_handle);
			nvs_set_u32(my_handle, "Time_relay", Time_relay_ON);
			nvs_set_u8(my_handle, "Relay_flag", Relay_flag);
			nvs_commit(my_handle);
			nvs_close(my_handle);

            uint8_t Position = 0;
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
            close(s);
			esp_restart();
		}

		if(buf[count] == 'F')
		{
			force_update = pdTRUE;
			xTaskCreate(&ota_example_task, "ota_example_task", 4096, NULL, 20, &ota_handle);
		}


	  //-----------------------------------------------------------------------------------------



    if(buf[count] == 'U')  // UID Домовладения
    {
      //printf("UID\n");
      count++;
      length =(buf[count]<<8);
      length |= (buf[count+1]);
      count+=2;
      //FLASH_Unlock(FLASH_MemType_Data);
      if((strncmp((char*)&UID[0] ,(char*)&buf[count], length))) { // Сравниваем. Если разное, то пишем в память
    	  bzero(UID, sizeof(UID));
    	  memcpy((void*)&UID[0] ,(void*)&buf[count], length);
        //printf("UID: ");
        nvs_set_str(my_handle, "UID", (const char *)UID);

        conn_Counter = 4001;
        TCP_flag = 0;
        first_sub = 1;
        //Flag_Reconect=1;
      }

      //FLASH_Lock(FLASH_MemType_Data);
      count+=length;

    }

    else if(buf[count] == 'S')  // настройки
    {
      count += 3;
      //FLASH_Unlock(FLASH_MemType_Data);
      if(POWER_ON_OFF !=(buf[count])){
        if((buf[count]==0))
        {
			led_Counter = 0;
			conditions_register = (conditions_register & (~LIGHT));
            POWER_ON_OFF = buf[count];
            nvs_set_u8(my_handle, "POWER_ON_OFF", POWER_ON_OFF);
			WINDOW=POWER_OFF_WIN;
    		nvs_set_u8(my_handle, "WINDOW", WINDOW);
			Time_relay_ON += temp_Time_relay_ON;
    		nvs_set_u32(my_handle, "Time_relay", Time_relay_ON);

    		temp_Time_relay_ON = 0;
			Relay_flag = 0;
			RELAY_OFF;
			OFF_ALL;
        }else if(buf[count]==1)
        {

            POWER_ON_OFF = buf[count];
            nvs_set_u8(my_handle, "POWER_ON_OFF", POWER_ON_OFF);
    		display_dig();
    		ON_ALL;
    		WINDOW = MAIN_WIN;
    		nvs_set_u8(my_handle, "WINDOW", WINDOW);
    		conditions_register=(conditions_register|LIGHT);
    		led_Counter = 0;
        }
        else
        {
          EE_error_flag=pdFALSE;
        }
      }
      count++;
      if( Heat_mode != buf[count]){
        if(buf[count]<3){
        	Heat_mode = buf[count];
        	nvs_set_u8(my_handle, "Heat_mode", Heat_mode);
        	upd_tmp_tbl = pdTRUE;
        	if(WINDOW == MAIN_WIN)
        	{
        		display_dig();
        	}
        } else {
          EE_error_flag=pdFALSE;
        }
      }
      count++;
      if( light_mode != buf[count]){
        if((buf[count]<6)&&(buf[count]>0)){
        	light_mode = buf[count];

        	nvs_set_u8(my_handle, "light_mode", light_mode);
        } else {
          EE_error_flag=pdFALSE;
        }
      }
      count++;

      if(set_floor_temperature != buf[count]){
        if((buf[count]>4)&&(buf[count]<46)){
        	set_floor_temperature = buf[count];
        	if((WINDOW == MAIN_WIN)) LedIndicatorDisplay(set_floor_temperature);
        	nvs_set_u8(my_handle, "set_floor_temp", set_floor_temperature);
        }else {
          EE_error_flag=pdFALSE;
        }
      }
      count++;
      if(love_temperature != buf[count]){
        if((buf[count]>4)&&(buf[count]<46)){
        	love_temperature=buf[count];
    		nvs_set_u8(my_handle, "love_temp", love_temperature);
        } else {
          EE_error_flag=pdFALSE;
        }
      }
      count++;
      if(((-5)<=((int8_t)buf[count]))&&(((int8_t)buf[count])<=5)) {

        if((delta_air_temperature) != (buf[count])+5){
        	delta_air_temperature=((int8_t)buf[count]+5);
        	nvs_set_u8(my_handle, "delta_air_temp", delta_air_temperature);
        }
      } else {
          EE_error_flag=pdFALSE;
        }
      count++;
      //FLASH_Lock(FLASH_MemType_Data);
    }

    else if(buf[count] == 'C')  // график обогрева
    {
      program Temp_prog[7][4];
      count += 3;
      for(i=0;i<7;i++)
        for(j=0;j<4;j++)
        {
          Temp_prog[i][j].Time_prog=((uint16_t)buf[count++]<<8)&0xFF00;
          Temp_prog[i][j].Time_prog|=((uint16_t)buf[count++]&0x00FF);
          if(Temp_prog[i][j].Time_prog>1440) {
            EE_error_flag=pdFALSE;
          }
          Temp_prog[i][j].Temp_set=buf[count++];
          if((Temp_prog[i][j].Temp_set>46)||(Temp_prog[i][j].Temp_set<4)) {
            EE_error_flag=pdFALSE;
          }
        }

      if (EE_error_flag==pdTRUE) {
        //FLASH_Unlock(FLASH_MemType_Data);
        for(i=0;i<7;i++)
          for(j=0;j<4;j++) {
            if((Program_step[i][j].Temp_set != Temp_prog[i][j].Temp_set)||
               (Program_step[i][j].Time_prog != Temp_prog[i][j].Time_prog)){
                 Program_step[i][j]=Temp_prog[i][j];
                 //printf("Time_prog %d Temp_set %d // ",Program_step[i][j].Time_prog,Program_step[i][j].Temp_set);
               }
            //printf("\n");
          }

        size_t Program_step_len = 112;
        nvs_set_blob(my_handle, "Program_step", (const void*)Program_step, Program_step_len);
        upd_tmp_tbl = pdTRUE;
      }

    }

    else if(buf[count] == 'D')  // дата и время
    {
      uint8_t time_ascii[19];
      count += 3;
      for(i = 0; i < 19; ++i)
        time_ascii[i] = buf[count++];
      count += 2;                    // пропускаем резерв для часового пояса
      RTC_time_set_ascii(time_ascii);
    }

    else if(buf[count] == 'W')  // Имя сети
    {
      count++;
      length =(buf[count]<<8);
      length |= (buf[count+1]);
      count += 2;
      if(strncmp((char*)&buf[count], (char*)SSID, length)){
    	bzero(SSID, sizeof(uint8_t)*32);

        for(i=0; i<length; i++)
        {
          SSID[i]= buf[count+i];
        }
        nvs_set_str(my_handle, "SSID", (const char *)SSID);

        first_link = 1;

        nvs_set_u8(my_handle, "first_link", first_link);

      }
      Flag_Reconect=1;
      count +=length;
    }
    else if(buf[count] == 'P')  // Пароль к сети
    {
      count++;
      length =(buf[count]<<8);
      length |= (buf[count+1]);
      count += 2;
      if(strncmp((char*)&buf[count], (char*)PASS, length)){
    	bzero(PASS, sizeof(PASS));

        for(i=0; i<length; i++)
        {
          PASS[i]= buf[count+i];
        }
        nvs_set_str(my_handle, "PASS", (const char *)PASS);

        first_link = 1;
        nvs_set_u8(my_handle, "first_link", first_link);

      }
      Flag_Reconect=1;
      count +=length;
    }
    /*
    else if(buf[count] == 'R')  // Законектиться к указанной сети
    {
      count += 3;
      FLASH_Unlock(FLASH_MemType_Data);
      First_link=1;
      AP_STA_MODE = 1;
      FLASH_Lock(FLASH_MemType_Data);
      Flag_Reconect=0;
    }
*/
    else if(buf[count] == 'Z')  //Тип датчика 02545157000C5300050155001F055A00020267E6
    {
      count += 3;
      if(sensor_num !=(buf[count])){
        sensor_num =(buf[count]);
        nvs_set_u8(my_handle, "sensor_num", sensor_num);
      }
      count++;
    }

    else if(buf[count] == 'M')		// Адрес и порт сервера MQTT
    {
      count++;
      length =(buf[count]<<8);
      length |= (buf[count+1]);
      count += 2;
      uint8_t length_m=0;
      for(uint8_t k=0; k<length; k++){
        if(buf[count+k]==':') break;
        length_m++;
      }
      if(strncmp((char*)&buf[count], (char*)MQTT_IP, length_m)){
    	bzero(MQTT_IP, sizeof(MQTT_IP));
        for(i=0; i<length_m; i++){
          MQTT_IP[i]= buf[count+i];
        }
        nvs_set_str(my_handle, "MQTT_IP", MQTT_IP);

      }
      count += (length_m+1);
      length = length-length_m-1;

      memcpy((char *)&MQTT_Port[0],(char *)&buf[count],4);

      nvs_set_str(my_handle, "MQTT_Port", MQTT_Port);
      first_sub = 1;
      count += length;
    }
    else                              // неизвестная команда
    {
      count++; // Переходим к разбору следующего байта
    }
  }
  nvs_commit(my_handle);
  nvs_close(my_handle);


  return EE_error_flag;
}

//---------------------------------------------------------------------------------------------------------------
//---------------------------------------------------Function send from ESP32 full buff--------------------------
//---------------------------------------------------------------------------------------------------------------
void EX_Instructions_Read(struct netconn *conn)
{

  uint16_t pos = 0;
  pos = EX_Send_Config(0);
  netconn_write(conn, (const void *)Buffer, pos, NETCONN_NOFLAG);
}

//---------------------------------------------------------------------------------------------------------------
//---------------------------------------------------Function save full buff from server-------------------------
//---------------------------------------------------------------------------------------------------------------
void EX_Instructions_Write(struct netconn *conn, uint8_t *buf, uint16_t count, uint16_t size)
{

  uint8_t EE_error_flag;
  EE_error_flag = EX_Save_Config(buf, count, size);
  printf("EE_error_flag %d\n",EE_error_flag);
  if(EE_error_flag == 1){
    EX_response(OKEY,conn);

    //printf("Flag_Reconect %d\n",Flag_Reconect);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    if(Flag_Reconect)
    {
    	reconnect_wifi();
    	Flag_Reconect = 0;
    }
  }
  else
    EX_response(ERR,conn);
}


//---------------------------------------------------------------------------------------------------------------
//---------------------------------------------------Function for ERR sending------------------------------------
//---------------------------------------------------------------------------------------------------------------
void EX_response(EX_Response_TypeDef response,struct netconn *conn)
{
  switch(response)
  {
  case OKEY:
    buffer_add_string((uint8_t *)"\x02\x54\x41\x00\x00\x00\x01\x4C", 0, 8);
    break;
  case ERR:
    buffer_add_string((uint8_t *)"\x02\x54\x41\xFF\x00\x00\xCE\x2F", 0, 8);
    break;
  case ECRC:
    buffer_add_string((uint8_t *)"\x02\x54\x41\xFE\x00\x00\xF9\x1F", 0, 8);
    break;
  case TIMEOUT:
    buffer_add_string((uint8_t *)"\x02\x54\x41\xFD\x00\x00\xA0\x4F", 0, 8);
    break;
  case NOACCESS:
    buffer_add_string((uint8_t *)"\x02\x54\x41\xFC\x00\x00\x97\x7F", 0, 8);
    break;
  case UNKNOW:
    buffer_add_string((uint8_t *)"\x02\x54\x41\xFF\x00\x00\xCE\x2F", 0, 8);
    break;
  }
  netconn_write(conn, (const void *)Buffer, 8, NETCONN_NOFLAG);
}



//---------------------------------------------------------------------------------------------------------------
//-----------------------------------------------Calculating CRC16-----------------------------------------------
//---------------------------------------------------------------------------------------------------------------
uint16_t CRC16(uint8_t *DATA, uint16_t length)
{
  uint16_t crc = 0xFFFF;
  uint8_t i;
  while (length--)
  {
    crc ^= *DATA++ << 8;
    for (i = 0; i < 8; i++)
    crc = crc & 0x8000 ? (crc << 1) ^ 0x1021 : crc << 1;
  }
  return crc;
}

// Расчет контрольной суммы не с начала пакета
uint16_t CRC16_2(uint8_t *DATA, uint16_t start, uint16_t length)
{
  uint16_t crc = 0xFFFF;
  uint8_t i;
  uint16_t v_ind = start;
  while ( v_ind < start+length )
  {
    crc ^= DATA[v_ind] << 8;
    for (i = 0; i < 8; i++)
      crc = crc & 0x8000 ? (crc << 1) ^ 0x1021 : crc << 1;
      v_ind++;
  }
  return crc;
}

//---------------------------------------------------------------------------------------------------------------
//----------------------------------------------TCP Server task--------------------------------------------------
//---------------------------------------------------------------------------------------------------------------

void tcp_server(void *ptr)
{
	struct netconn *tcp_conn = netconn_new(NETCONN_TCP);
	if (tcp_conn)
	{
		netconn_bind(tcp_conn, NULL, 6350);
		netconn_listen(tcp_conn);
		printf("TCP Server listening...\n");
	}
	Access_flag = 0;

	uint16_t des_Port;
	struct netconn *newconn;

	while (1)
	{
		//tcp_conn->recv_timeout = 1000;
		//printf("accepting\n");
		netconn_accept(tcp_conn, &newconn);

		netconn_getaddr(newconn, (ip_addr_t *)&ip_first, &des_Port, 0);

		if(ip_last == 0)
		{
			TCP_flag = 1;
			ip_last = (uint32_t)ip_first;
			conditions_register=(conditions_register|CONN);
		}
		if(ip_last==ip_first)
		{
			conn_Counter = 0;
		}
		else
		{
			Access_flag = 1;
		}

		//---------------------------------------------------------------------------------------------------

		tcp_server_service(newconn);
		//vTaskDelay(100 / portTICK_PERIOD_MS);
	}
}



//---------------------------------------------------------------------------------------------------------------
//----------------------------------------------TCP management---------------------------------------------------
//---------------------------------------------------------------------------------------------------------------
void tcp_server_service(struct netconn *conn) {

		struct T_instruction instruction;

		struct netbuf *inbuf;

		uint8_t *buf;

		uint16_t CRC;
		uint16_t buflen=0;
		uint16_t count=0;
		err_t err;

		err = netconn_recv(conn, &inbuf);

		if ((err == ERR_OK)) {

			netbuf_data(inbuf, (void**)&buf, &buflen);
			if(buflen)
			{
/*
			for(int i=0;i<buflen;i++)
			{
				printf("%02X",buf[i]);
			}
			printf("\n");
*/
			if((buf[count] == 0x02)&&(buf[count + 1] == 'T')&&(buf[count + 2] == 'Q'))
			     {

			          count += 3;
			          instruction.ID = buf[count++];

			          instruction.length  = ((uint16_t)(buf[count] << 8) & 0xFF00);
			          instruction.length |= ((uint16_t)buf[count+1]) & 0x00FF;
			          count += 2;
			          instruction.CRC = (((uint16_t)buf[count + instruction.length] << 8)&0xFF00);
			          instruction.CRC |= ((uint16_t)buf[count + instruction.length + 1])&0x00FF;
			          CRC = CRC16(buf,buflen - 2);
			          if(instruction.CRC == CRC)
			          {
			        	  if(instruction.ID == 'R')
			        	  {
			        		  EX_Instructions_Read(conn);
			        	  }
			        	  else if((instruction.ID == 'W'))
			        	  {
			        	      EX_Instructions_Write(conn, buf, count, instruction.length); // - 2B crc
			        	  }else printf("Access\n");

			          }else
			          {
		        	  	  EX_response(ECRC,conn);
		        	  	  printf("false crc\n");
		        	  }
			     }
			else EX_response(UNKNOW,conn);

			}
			netbuf_free(inbuf);
			netconn_close(conn);
		}else printf("Resv err %d\n", err);
		//}

}


//---------------------------------------------------------------------------------------------------------------
//----------------------------------------------UDP server task--------------------------------------------------
//---------------------------------------------------------------------------------------------------------------
void udp_server(void *ptr)
{

	struct netconn *udp_conn;
	udp_conn = netconn_new(NETCONN_UDP);
	netconn_bind(udp_conn, IP_ADDR_ANY, 6350);
	printf("UDP Server listening...\n");
	while (1)
	{
		//printf("UDP\n");
		udp_server_service(udp_conn);
		vTaskDelay(100 / portTICK_PERIOD_MS);
	}
}



//---------------------------------------------------------------------------------------------------------------
//----------------------------------------------UDP management---------------------------------------------------
//---------------------------------------------------------------------------------------------------------------
void udp_server_service(struct netconn *conn) {

	struct netbuf *inbuf;



	char *buf;
	uint16_t buflen;
	err_t err;

	err = netconn_recv(conn, &inbuf);

	if (err == ERR_OK) {

		netbuf_data(inbuf, (void**)&buf, &buflen);


/*
		for(int i=0;i<buflen;i++)
		{
			printf("%02X",buf[i]);
		}
		printf("\n");
*/
		if(!strncmp((char*)&buf[0],"\x02TQI\x00\x00\x99\xD7",8))
		{
			buffer_add_string((uint8_t *)"\x02TAI\x00\x16", 0, 6);
		    buffer_add_string((uint8_t *)"O1", 6, 2);
		    buffer_add_string((uint8_t *)VERSION, 8, 3);
		    buffer_add_string((uint8_t *)MAC_esp, 11, 17);
		    buffer_add_int(CRC16(Buffer,28),28);
		    /*
			for(int i=0;i<30;i++)
			{
				printf("%02X",Buffer[i]);
			}
			printf("\n");
			*/
		    netbuf_ref(inbuf, (void*)&Buffer, 30);
		    netconn_send(conn, inbuf);
		 }
	}
	netbuf_delete(inbuf);
}


/*

static char tag[] = "socket_server";


void tcp_server(void *ptr) {
	struct sockaddr_in clientAddress;
	struct sockaddr_in serverAddress;

	// Create a socket that we will listen upon.
	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0) {
		ESP_LOGE(tag, "socket: %d %s", sock, strerror(errno));
	}

	// Bind our server socket to a port.
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddress.sin_port = htons(6350);
	int rc  = bind(sock, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
	if (rc < 0) {
		ESP_LOGE(tag, "bind: %d %s", rc, strerror(errno));
	}

	// Flag the socket as listening for new connections.
	rc = listen(sock, 1);
	if (rc < 0) {
		ESP_LOGE(tag, "listen: %d %s", rc, strerror(errno));
	}

	while (1) {
		// Listen for a new client connection.
		printf("accept wait\n");
		socklen_t clientAddressLength = sizeof(clientAddress);
		int clientSock = accept(sock, (struct sockaddr *)&clientAddress, &clientAddressLength);
		ip_first = (uint32_t)clientAddress.sin_addr.s_addr;
		printf("%d\n",ip_first);
		if (clientSock < 0) {
			ESP_LOGE(tag, "accept: %d %s", clientSock, strerror(errno));
		}
		printf("accepted\n");

		if(ip_last == 0)
		{
			TCP_flag = 1;
			ip_last = ip_first;
			//memcpy((in_addr_t *)&ipv4_last,(in_addr_t *)&ipv4_first,sizeof(ipv4_first));
			//printf("IP LAST %d.%d.%d.%d:",(ipv4_last)&0xFF, (ipv4_last>>8)&0xFF, (ipv4_last>>16)&0xFF, (ipv4_last>>24)&0xFF);
			conditions_register=(conditions_register|CONN);
		}
		if(ip_last==ip_first)
		{
			conn_Counter = 0;
		}
		else
		{
			Access_flag = 1;
		}

		uint8_t buf[20];
		struct T_instruction instruction;
		uint16_t CRC;
		uint16_t buflen=0;
		uint16_t count=0;

		ssize_t sizeRead = recv(clientSock, buf, 20, 0);
		for(uint8_t jkl = 0;jkl<sizeRead;jkl++){
		printf("%02X",buf[jkl]);}
		printf("\n");

		if((buf[count] == 0x02)&&(buf[count + 1] == 'T')&&(buf[count + 2] == 'Q'))
		     {

		          count += 3;
		          instruction.ID = buf[count++];

		          instruction.length  = ((uint16_t)(buf[count] << 8) & 0xFF00);
		          instruction.length |= ((uint16_t)buf[count+1]) & 0x00FF;
		          count += 2;
		          instruction.CRC = (((uint16_t)buf[count + instruction.length] << 8)&0xFF00);
		          instruction.CRC |= ((uint16_t)buf[count + instruction.length + 1])&0x00FF;
		          CRC = CRC16(buf,buflen - 2);
		          if(instruction.CRC == CRC)
		          {
		        	  if(instruction.ID == 'R')
		        	  {
		        		  EX_Instructions_Read(conn);
		        	  }
		        	  else if((instruction.ID == 'W'))
		        	  {
		        	      EX_Instructions_Write(conn, buf, count, instruction.length); // - 2B crc
		        	  }else printf("Access\n");

		          }else printf("false crc\n");

		     }


		close(clientSock);
		vTaskDelay(500 / portTICK_PERIOD_MS);
	}
}

*/













