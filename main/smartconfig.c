/*
 * smartconfig.c
 *
 *  Created on: Dec 19, 2017
 *      Author: sergey
 */

#include "smartconfig.h"
#include "Functions.h"


static EventGroupHandle_t wifi_event_group;
static const int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;
static const char *TAG = "sc";



void connect_wifi(void *ptr)
{
	while(1)
	{
		if((esp_wifi_connect() == ESP_OK) || (conn_flag == 1))
		{
			vTaskDelete(NULL);
		}
		vTaskDelay(5000 / portTICK_RATE_MS);
	}
}



//---------------------------------------------------------------------------------------------------------------
//------------------------------static-------------------Handle WIFI status--------------------------------------------
//---------------------------------------------------------------------------------------------------------------
static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {

    case SYSTEM_EVENT_AP_START:
    	//conn_flag = 0;
    	wifi_start_flag = 1;
    	xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        ap_flag = 1;
        //WIFI = 1;
        first_link = 1;
        if(disconnect) // если был разрыв связи, восстанавливаем задачу (флаг выставляется только после 1го и последующик разрывов)
        {
        	vTaskResume( tcp_server_handle );
        	vTaskResume( udp_server_handle );
        	vTaskResume( mqtt_subscribe_handle );
        	disconnect = 0;
        }else          // если это подключение после ребута или включения то создаем задачу mqtt
        {
            xTaskCreate( &tcp_server, "tcp_server", 4096, NULL, 10, &tcp_server_handle );
            xTaskCreate( &udp_server, "udp_server", 4096, NULL, 10, &udp_server_handle );
            xTaskCreate( &mqtt_subscribe, "mqtt_subscribe", 8192, NULL, 10, &mqtt_subscribe_handle );
        }

    	break;
    	break;

    case SYSTEM_EVENT_AP_STOP:


    	xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
    	conditions_register = (conditions_register & (~SC_AP));
    	WIFI = 0;
    	WIFI_Level = 0x00;
    	wifi_start_flag = 0;
    	vTaskSuspend( tcp_server_handle );
    	vTaskSuspend( udp_server_handle );
    	vTaskSuspend( mqtt_subscribe_handle );
    	WIFI=0;
    	disconnect = 1;
//    	printf("TCP_UDP_SUSPEND ------ AP\n");

    	break;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    case SYSTEM_EVENT_STA_START:
    	ap_flag = 0;
    	wifi_start_flag = 1;


        break;
    case SYSTEM_EVENT_STA_GOT_IP:
    	conditions_register = (conditions_register & (~SC_AP));
    	if(WINDOW == MAIN_WIN){
    		display_dig();
			conditions_register=(conditions_register|LIGHT);
			led_Counter = 0;
    	}

    	sta_ap_flag = 0;

    	ota_count = 0;
    	conditions_register=(conditions_register|OTA);          // start ota task (5 min)

    	sc_ap_dig = 97;
    	wifi_conn_Counter = 10000;

    	conn_flag = 0;

        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        WIFI = 1;
        get_mac_buf();


        first_sub = 1;


        if(disconnect) // если был разрыв связи, восстанавливаем задачу (флаг выставляется только после 1го и последующик разрывов)
        {
        	vTaskResume( tcp_server_handle );
        	vTaskResume( udp_server_handle );
        	vTaskResume( mqtt_subscribe_handle );
        	disconnect = 0;
        }else          // если это подключение после ребута или включения то создаем задачу mqtt
        {
            xTaskCreate( &tcp_server, "tcp_server", 4096, NULL, 10, &tcp_server_handle );
            xTaskCreate( &udp_server, "udp_server", 4096, NULL, 10, &udp_server_handle );
        	xTaskCreate( &mqtt_subscribe, "mqtt_subscribe", 8192, NULL, 10, &mqtt_subscribe_handle );
        	vTaskDelay(30000 / portTICK_PERIOD_MS);
        	xTaskCreate(&ota_example_task, "ota_example_task", 4096, NULL, 20, &ota_handle);
        }
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:

    	conditions_register = (conditions_register & (~OTA));

    	if(WIFI == 1)
    	{
    		vTaskSuspend( tcp_server_handle );
    		vTaskSuspend( udp_server_handle );
    		vTaskSuspend( mqtt_subscribe_handle );
    		WIFI=0;
    		disconnect = 1;
    	}

    	xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
    	WIFI_Level = 0x00;
    	wifi_start_flag = 0;
    	printf("DISCONNECTED\n");
/*
    	if(++reconn_count==30)
    	{

			Time_relay_ON += temp_Time_relay_ON;
			printf("Time_relay_ON %d\n",Time_relay_ON);
    		nvs_open("storage", NVS_READWRITE, &my_handle);
    		nvs_set_u32(my_handle, "Time_relay", Time_relay_ON);
    		nvs_commit(my_handle);
    		nvs_close(my_handle);

    		esp_restart();
    	}
*/
    	//reconnect_wifi();

    	if(!sta_ap_flag) esp_wifi_connect();

        break;
    default:
        break;
    }
    return ESP_OK;
}



//---------------------------------------------------------------------------------------------------------------
//-------------------------------------------------WIFI init-----------------------------------------------------
//---------------------------------------------------------------------------------------------------------------

void initialise_wifi(void)
{


    // Configure dynamic frequency scaling:
    // maximum and minimum frequencies are set in sdkconfig,
    // automatic light sleep is enabled if tickless idle support is enabled.
/*    esp_pm_config_esp32_t pm_config = {
            .max_freq_mhz = 80,
            .min_freq_mhz = 10,
            .light_sleep_enable = 1,
    };
    ESP_ERROR_CHECK( esp_pm_configure(&pm_config) );*/


	tcpip_adapter_init();

	wifi_event_group = xEventGroupCreate();
	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

	nvs_open("storage", NVS_READWRITE, &my_handle);
	nvs_get_u8(my_handle, "first_link", &first_link);
	nvs_commit(my_handle);
	nvs_close(my_handle);
	//printf("first_link %d\n", first_link);
	if (first_link)
	{
		wifi_config_t wifi_config =
		{ .sta =
		{ .ssid = "",
		  .password = "",}, };
		//printf("first_link %s\n", SSID);
		//printf("first_link %s\n", PASS);
		strcpy((char*) &wifi_config.sta.ssid[0], (const char*) &SSID[0]);
		strcpy((char*) &wifi_config.sta.password[0], (const char*) &PASS[0]);

		curr_interface = ESP_IF_WIFI_STA;

		ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
		ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
		curr_interface = ESP_IF_WIFI_STA;
esp_wifi_set_ps(2);
		ESP_ERROR_CHECK(esp_wifi_start());

		ESP_ERROR_CHECK(esp_wifi_connect());
	}

}


void set_wifi_sta(void)
{
	sta_ap_flag = 1;

	if(wifi_start_flag)
	{
		printf("stop wifi\n");

		//ESP_ERROR_CHECK( esp_wifi_disconnect() );
		esp_wifi_disconnect();
		//vTaskDelay(3000 / portTICK_RATE_MS);
		//ESP_ERROR_CHECK(esp_wifi_deauth_sta(0));
		//ESP_ERROR_CHECK( esp_wifi_stop());
		esp_wifi_stop();
	}
	ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );

	curr_interface = ESP_IF_WIFI_STA;
	ESP_ERROR_CHECK( esp_wifi_start() );
	xTaskCreate(&smartconfig_example_task, "smartconfig_example_task", 4096, NULL, 15, &smartconfig_handle);
	sta_ap_flag = 1;
}

void set_wifi_ap()
{
	sta_ap_flag = 1;
	curr_interface = ESP_IF_WIFI_STA;
	get_mac_buf();
	if (wifi_start_flag)
	{
		ESP_ERROR_CHECK(esp_wifi_disconnect());
		ESP_ERROR_CHECK(esp_wifi_stop());
	}
	//conn_flag = 1;
	//WIFI = 0;
	 wifi_config_t wifi_config1 = {
			 .ap = {
	 	      //.ssid="Equation",
	 	      .ssid_len=17,
	 	      .password="1234567890",
	 	      .channel=6,
	 	      .authmode=WIFI_AUTH_WPA_WPA2_PSK,
	 	      .ssid_hidden=0,
	 	      .max_connection=4,
	 	      .beacon_interval=100
	 	   },
	 	};
	strncpy((char*)wifi_config1.ap.ssid, (char*)MAC_esp, 17);

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config1));
	//ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_AP) );
	sta_ap_flag = 1;
	ESP_ERROR_CHECK(esp_wifi_start());

}


void reconnect_wifi()
{

	//ESP_ERROR_CHECK(esp_wifi_disconnect());
    ESP_ERROR_CHECK( esp_wifi_stop());

    ESP_ERROR_CHECK( esp_wifi_restore());

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
          			.sta = {
          					.ssid = "",
      						.password = "",
          			},
          	};
	//printf("recon %s\n",SSID);
	//printf("recon %s\n",PASS);
          	strcpy((char*)&wifi_config.sta.ssid[0],(const char*)&SSID[0]);
          	strcpy((char*)&wifi_config.sta.password[0],(const char*)&PASS[0]);
          	curr_interface = ESP_IF_WIFI_STA;
        	//printf("%s\n",wifi_config.sta.ssid);
        	//printf("%s\n",wifi_config.sta.password);
              ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
              ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
              curr_interface = ESP_IF_WIFI_STA;
              ESP_ERROR_CHECK( esp_wifi_start() );
              ESP_ERROR_CHECK( esp_wifi_connect() );
}


//---------------------------------------------------------------------------------------------------------------
//-------------------------------------------------Handle smartconfig status-------------------------------------
//---------------------------------------------------------------------------------------------------------------
static void sc_callback(smartconfig_status_t status, void *pdata)
{
    switch (status) {
        case SC_STATUS_WAIT:
            ESP_LOGI(TAG, "SC_STATUS_WAIT");
            break;
        case SC_STATUS_FIND_CHANNEL:
            ESP_LOGI(TAG, "SC_STATUS_FINDING_CHANNEL");
            break;
        case SC_STATUS_GETTING_SSID_PSWD:
            ESP_LOGI(TAG, "SC_STATUS_GETTING_SSID_PSWD");
            break;
        case SC_STATUS_LINK:
            ESP_LOGI(TAG, "SC_STATUS_LINK");
            wifi_config_t *wifi_config = pdata;
            wifi_config->sta.bssid_set = 0;
            ESP_LOGI(TAG, "SSID:%s", wifi_config->sta.ssid);
            ESP_LOGI(TAG, "PASSWORD:%s", wifi_config->sta.password);

//---------------------------------------------------------------------------------------------------------------Save SSID & PASS to flash
        	strcpy((char*)&SSID[0],(const char*)&wifi_config->sta.ssid[0]);
        	strcpy((char*)&PASS[0],(const char*)&wifi_config->sta.password[0]);
        	//printf("%s\n",SSID);
        	//printf("%s\n",PASS);
            nvs_open("storage", NVS_READWRITE, &my_handle);
            nvs_set_str(my_handle, "SSID", (const char *)SSID);
            nvs_set_str(my_handle, "PASS", (const char *)PASS);
            nvs_commit(my_handle);
            nvs_close(my_handle);

//---------------------------------------------------------------------------------------------------------------
            ESP_ERROR_CHECK( esp_wifi_disconnect() );
            ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_config) );
            esp_wifi_set_ps(2);
            ESP_ERROR_CHECK( esp_wifi_connect() );

            break;
        case SC_STATUS_LINK_OVER:
            ESP_LOGI(TAG, "SC_STATUS_LINK_OVER");
            if (pdata != NULL) {
                uint8_t phone_ip[4] = { 0 };
                memcpy(phone_ip, (uint8_t* )pdata, 4);
                ESP_LOGI(TAG, "Phone ip: %d.%d.%d.%d\n", phone_ip[0], phone_ip[1], phone_ip[2], phone_ip[3]);
            }
            xEventGroupSetBits(wifi_event_group, ESPTOUCH_DONE_BIT);
            break;
        default:
            break;
    }
}



//---------------------------------------------------------------------------------------------------------------
//--------------------------------------------------Smartconfig task--------------------------------------------
//---------------------------------------------------------------------------------------------------------------
void smartconfig_example_task(void * parm)
{
    EventBits_t uxBits;


    //esp_smartconfig_fast_mode(pdTRUE);
    ESP_ERROR_CHECK( esp_smartconfig_set_type(SC_TYPE_ESPTOUCH) );
    ESP_ERROR_CHECK( esp_smartconfig_start(sc_callback) );

    while (1) {

        uxBits = xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY);
        if(uxBits & CONNECTED_BIT) {
            ESP_LOGI(TAG, "WiFi Connected to ap");
            first_link = 1;

            nvs_open("storage", NVS_READWRITE, &my_handle);
            nvs_set_u8(my_handle, "first_link", first_link);
            nvs_commit(my_handle);
            nvs_close(my_handle);
        }
        if(uxBits & ESPTOUCH_DONE_BIT) {
            ESP_LOGI(TAG, "smartconfig over");

            esp_smartconfig_stop();
            vTaskDelete(NULL);
        }
        //vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
