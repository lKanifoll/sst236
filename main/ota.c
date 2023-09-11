/*
 * ota.c
 *
 *  Created on: Dec 18, 2017
 *      Author: sergey
 */

#include "ota.h"
#include "Functions.h"


static const char *TAG = "ota";
/*an ota data write buffer ready to write to the flash*/
static char ota_write_data[BUFFSIZE + 1] = { 0 };
/*an packet receive buffer*/
static char text_fw[BUFFSIZE + 1] = { 0 };
/* an image total length*/
static int binary_file_length = 0;
/*socket id*/
static int socket_id = -1;


/*read buffer by byte still delim ,return read bytes counts*/
static int read_until(char *buffer, char delim, int len)
{
//  /*TODO: delim check,buffer check,further: do an buffer length limited*/
    int i = 0;
    while (buffer[i] != delim && i < len) {
        ++i;
    }
    return i + 1;
}


static bool read_past_http_header(char text_fw[], int total_len, esp_ota_handle_t update_handle)
{
    /* i means current position */
    int i = 0, i_read_len = 0;
    while (text_fw[i] != 0 && i < total_len) {
        i_read_len = read_until(&text_fw[i], '\n', total_len);
        // if we resolve \r\n line,we think packet header is finished
        if (i_read_len == 2) {
            int i_write_len = total_len - (i + 2);
            memset(ota_write_data, 0, BUFFSIZE);
            /*copy first http packet body to write buffer*/
            memcpy(ota_write_data, &(text_fw[i + 2]), i_write_len);

            esp_err_t err = esp_ota_write( update_handle, (const void *)ota_write_data, i_write_len);
            if (err != ESP_OK) {
				#ifdef DEBUG
					ESP_LOGE(TAG, "Error: esp_ota_write failed! err=0x%x", err);
				#endif
                return pdFALSE;
            } else {
				#ifdef DEBUG
					ESP_LOGI(TAG, "esp_ota_write header OK");
				#endif
                binary_file_length += i_write_len;
            }
            return pdTRUE;
        }
        i += i_read_len;
    }
    return pdFALSE;
}

static bool connect_to_http_server()
{

    struct addrinfo *res;
	const struct addrinfo hints =
	{ .ai_family = AF_INET, .ai_socktype = SOCK_STREAM, };

	//printf("MQTT_Port %s\n", MQTT_Port);
	// resolve the IP of the target website
	int result = getaddrinfo(SERVER_IP, SERVER_PORT, &hints, &res);
	if ((result != 0) || (res == NULL))
	{
		#ifdef DEBUG
			printf("Unable to resolve IP for target website %s\n", SERVER_IP);
		#endif
		return pdFALSE;
	}
	//printf("Target website's IP resolved\n");

	// create a new socket
	socket_id = socket(res->ai_family, res->ai_socktype, 0);
	if (s < 0)
	{
		#ifdef DEBUG
			printf("Unable to allocate a new socket\n");
		#endif
		freeaddrinfo(res);
		return pdFALSE;
	}
	//printf("Socket allocated, id=%d\n", s);

	// connect to the specified server
	result = connect(socket_id, res->ai_addr, res->ai_addrlen);
	if (result != 0)
	{
		#ifdef DEBUG
			printf("Unable to connect to the target website\n");
		#endif
		freeaddrinfo(res);
		return pdFALSE;
	}
	freeaddrinfo(res);
	return pdTRUE;
}

void ota_example_task(void *ptr)
{
	while(1){

		//vTaskDelay(5000 / portTICK_PERIOD_MS);

    esp_err_t err;
    /* update handlclose(socket_id);e : set by esp_ota_begin(), must be freed via esp_ota_end() */
    esp_ota_handle_t update_handle = 0 ;
    const esp_partition_t *update_partition = NULL;
    ESP_LOGI(TAG, "Starting OTA example...");
	#ifdef DEBUG
		ESP_LOGI(TAG, "Starting OTA example...");
	#endif

    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();

    if (configured != running) {
		#ifdef DEBUG
			ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x",
					 configured->address, running->address);
			ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
		#endif
    }
	#ifdef DEBUG
		ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)",
				 running->type, running->subtype, running->address);
	#endif

    /* Wait for the callback to set the CONNECTED_BIT in the
       event group.
    */
    //xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
    //                    false, true, portMAX_DELAY);
    //ESP_LOGI(TAG, "Connect to Wifi ! Start to Connect to Server....");

    /*connect to http server*/
    if (connect_to_http_server()) {
		#ifdef DEBUG
			printf("Connected to http server\n");
		#endif
    } else
    {
		#ifdef DEBUG
			printf("Connect to http server failed!\n");
		#endif

        close(socket_id);
        vTaskResume(mqtt_subscribe_handle);
        vTaskDelete(NULL);
    }


//---------------------------------------------------------------------------------check version----------------------------------------------------


    static const char *REQUEST = "GET ""/static/firmware/firmware_oke_wifi_http.xml"" HTTP/1.1\n"
    	"Host: "SERVER_IP"\n"
    	"User-Agent: ESP32\n"
    	"\n";
    char recv_buf[1000];
    int result = write(socket_id, REQUEST, strlen(REQUEST));
    		if(result < 0) {
			#ifdef DEBUG
				printf("Unable to send the HTTP request\n");
			#endif
    		close(socket_id);
            vTaskResume(mqtt_subscribe_handle);
            vTaskDelete(NULL);
    	}
		#ifdef DEBUG
			printf("HTTP request version sent\n");
		#endif

	#ifdef DEBUG
		printf("--------------------------------------------------------------------------------\n");
	#endif
    	int num_bytes,i=0;
    		char version[]="version";
    		char url[]="url";
    		char temp_buf_ver[15];
    		char ver[3];
    		char http[200];
    		bzero(http, sizeof(http));
    		bzero(temp_buf_ver, sizeof(temp_buf_ver));

    			bzero(recv_buf, sizeof(recv_buf));
    			num_bytes =read(socket_id, recv_buf, sizeof(recv_buf) - 1);

    			while(num_bytes)
    			{

    					for(uint8_t g=0;g<10;g++)
    						temp_buf_ver[g]=recv_buf[i+g];

    					if(strncmp(temp_buf_ver,version,strlen(version))==0)  	// Если нашли version, отрабатываем значение по этому ключу
    					{
    						i+=7;												// Шагаем на 7 вперед (длина слова version)
    						bzero(temp_buf_ver, sizeof(temp_buf_ver));
    						while(1)											// Цикл поиска слова <string>
    						{
    							for(uint8_t g=0;g<8;g++)
    								temp_buf_ver[g]=recv_buf[i+g];				// Перебираем далее массив: записываем во временный массив по 8 символов (длина слова <string>)
    							if((strcmp(temp_buf_ver,"<string>"))==0)		// и сравниваем
    							{
    								i+=8;

    								for(uint8_t g=0;g<3;g++)
    								{
    									ver[g]=recv_buf[i+g];					// Если нашли, пишем в массив ver значение после слова <string>

    									}
									#ifdef DEBUG
										printf("version %s\n",ver);
									#endif
    								break;
    							}
    							i++;
    						}
    					}


    					if(strncmp(temp_buf_ver,url,strlen(url))==0)  			// Если нашли url, отрабатываем значение по этому ключу
    					{
    						i+=3;												// Шагаем на 3 вперед (длина слова url)
    						bzero(temp_buf_ver, sizeof(temp_buf_ver));
    						while(1)											// Цикл поиска слова <string>
    						{
    							for(uint8_t g=0;g<8;g++)
    								temp_buf_ver[g]=recv_buf[i+g];				// Перебираем далее массив: записываем во временный массив по 8 символов (длина слова <string>)
    							if((strcmp(temp_buf_ver,"<string>"))==0)		// и сравниваем
    							{
    								i+=8;
    								uint8_t l=0;
    								while(1)									// Если нашли, пишем в массив http значение после слова <string>
    								{
    									if(recv_buf[i+l]=='<')					// пишем пока не встретим знак '<' - конец значения <string>
    										{break;}
    									http[l]=recv_buf[i+l];
    									l++;

    								}

									#ifdef DEBUG
										printf("URL %s\n",http);
									#endif
    								break;
    							}
    							i++;
    						}
    					}
    				num_bytes--;												// декремент длины приняных данных по http
    				i++;														// инкремент буффера с данными для обхода
    				bzero(temp_buf_ver, sizeof(temp_buf_ver));					// обнуляем временный буффер
    			}
	#ifdef DEBUG
		printf("--------------------------------------------------------------------------------\n");
	#endif

//-------------------------------------------------------------------------------------------------------------------------------------------

		if(!force_update)
		{
			if(!(strncmp(&ver[0], VERSION, 3)))
			{
				//#ifdef DEBUG
					printf("NO NEED TO UPDATE\n");
				//#endif
				close(socket_id);
				vTaskResume(mqtt_subscribe_handle);
				vTaskDelete(NULL);
			}
			else
			{
				//#ifdef DEBUG
					printf("NEED TO UPDATE\n");
				//#endif
			}
		}

	Time_relay_ON += temp_Time_relay_ON;
	printf("Time_relay_ON %d\n",Time_relay_ON);
	nvs_open("storage", NVS_READWRITE, &my_handle);
	nvs_set_u32(my_handle, "Time_relay", Time_relay_ON);
	nvs_set_u8(my_handle, "Relay_flag", Relay_flag);
	nvs_commit(my_handle);
	nvs_close(my_handle);




//--------------------------------------------------------------------------------------start ota---------------------------------------------
    /*send GET request to http server*/
    const char *GET_FORMAT =
        "GET %s HTTP/1.0\r\n"
        "Host: %s:%s\r\n"
        "User-Agent: esp-idf/1.0 esp32\r\n\r\n";

    char *http_request = NULL;
    int get_len = asprintf(&http_request, GET_FORMAT, (const char*)&http[0], SERVER_IP, SERVER_PORT);
    if (get_len < 0) {
		#ifdef DEBUG
			ESP_LOGE(TAG, "Failed to allocate memory for GET request buffer");
		#endif
        close(socket_id);
        vTaskResume(mqtt_subscribe_handle);
        vTaskDelete(NULL);
        //task_fatal_error();
    }
    int res = send(socket_id, http_request, get_len, 0);
    free(http_request);

    if (res < 0) {
		#ifdef DEBUG
			ESP_LOGE(TAG, "Send GET request to server failed");
		#endif
        close(socket_id);
        vTaskResume(mqtt_subscribe_handle);
        vTaskDelete(NULL);
        //task_fatal_error();
    } else {
		#ifdef DEBUG
			ESP_LOGI(TAG, "Send GET request to server succeeded");
		#endif
    }

    update_partition = esp_ota_get_next_update_partition(NULL);
	#ifdef DEBUG
		ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x",
				 update_partition->subtype, update_partition->address);
	#endif
    assert(update_partition != NULL);

    err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
    if (err != ESP_OK) {
		#ifdef DEBUG
			ESP_LOGE(TAG, "esp_ota_begin failed, error=%d", err);
		#endif
        close(socket_id);
        vTaskResume(mqtt_subscribe_handle);
        vTaskDelete(NULL);
        //task_fatal_error();
    }
	#ifdef DEBUG
		ESP_LOGI(TAG, "esp_ota_begin succeeded");
	#endif

    bool resp_body_start = pdFALSE, flag = pdTRUE;
    /*deal with all receive packet*/
    while (flag) {
        memset(text_fw, 0, TEXT_BUFFSIZE);
        memset(ota_write_data, 0, BUFFSIZE);
        int buff_len = recv(socket_id, text_fw, TEXT_BUFFSIZE, 0);
        //printf("buff_len %d\n",buff_len);

        if (buff_len < 0) { /*receive error*/
			#ifdef DEBUG
				ESP_LOGE(TAG, "Error: receive data error! errno=%d", errno);
			#endif
            close(socket_id);
            vTaskResume(mqtt_subscribe_handle);
            vTaskDelete(NULL);
            //task_fatal_error();
        } else if (buff_len > 0 && !resp_body_start) { /*deal with response header*/
            memcpy(ota_write_data, text_fw, buff_len);
            resp_body_start = read_past_http_header(text_fw, buff_len, update_handle);
        } else if (buff_len > 0 && resp_body_start) { /*deal with response body*/
            memcpy(ota_write_data, text_fw, buff_len);
            err = esp_ota_write( update_handle, (const void *)ota_write_data, buff_len);
            if (err != ESP_OK) {
				#ifdef DEBUG
					ESP_LOGE(TAG, "Error: esp_ota_write failed! err=0x%x", err);
				#endif
                close(socket_id);
                vTaskResume(mqtt_subscribe_handle);
                vTaskDelete(NULL);
                //task_fatal_error();
            }
            binary_file_length += buff_len;
            //ESP_LOGI(TAG, "Have written image length %d", binary_file_length);
        } else if (buff_len == 0) {  /*packet over*/
            flag = pdFALSE;
			#ifdef DEBUG
				ESP_LOGI(TAG, "Connection closed, all packets received");
			#endif
            close(socket_id);
        } else {
			#ifdef DEBUG
				ESP_LOGE(TAG, "Unexpected recv result");
			#endif
            close(socket_id);
        }
    }

	#ifdef DEBUG
		ESP_LOGI(TAG, "Total Write binary data length : %d", binary_file_length);
	#endif

    if (esp_ota_end(update_handle) != ESP_OK) {
		#ifdef DEBUG
			ESP_LOGE(TAG, "esp_ota_end failed!");
		#endif
        close(socket_id);
        vTaskResume(mqtt_subscribe_handle);
        vTaskDelete(NULL);
        //task_fatal_error();
    }
    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
		#ifdef DEBUG
			ESP_LOGE(TAG, "esp_ota_set_boot_partition failed! err=0x%x", err);
		#endif
        close(socket_id);
        vTaskResume(mqtt_subscribe_handle);
        vTaskDelete(NULL);
        //task_fatal_error();
    }
	#ifdef DEBUG
		ESP_LOGI(TAG, "Prepare to restart system!");
	#endif
    esp_restart();
    //return ;
	}
}

