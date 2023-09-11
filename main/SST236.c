

#include "Functions.h"


//==========================================================================================
//======================================================================== Task for read ADC
//==========================================================================================
void read_temperature(void *pvParameters)
{
	while (1)
	{ //-----------------------------------ADC calc--------------------------------------------
		if(!(conditions_register & SC_AP))
		{
			adc_floor = 0;
			for (int adc_1 = 0; adc_1 < 10; adc_1++)
			{
				adc2_get_raw(ADC2_CHANNEL_7, ADC_WIDTH_BIT_12, &raw_out);
				adc_floor += raw_out;
			}
			adc_floor /= 10;
			uint16_t voltage = esp_adc_cal_raw_to_voltage((uint32_t) adc_floor, adc_chars);
			//printf("adc_floor %d\n",adc_floor);

			if ((adc_floor >= 3500))                // ERROR "E1" open source
			{
				Sensor_err = 1;
				uint16_t seg[1] = { 0x86F9 };
				trans_desc.length = 16;
				trans_desc.tx_buffer = seg;
				ESP_ERROR_CHECK(spi_device_transmit(handle1, &trans_desc));
				show_seg();
			}

			else if ((adc_floor <= 20))				// ERROR "E2" short circuit
			{
				Sensor_err = 1;
				uint16_t seg[1] = { 0x86A4 };
				trans_desc.length = 16;
				trans_desc.tx_buffer = seg;
				ESP_ERROR_CHECK(spi_device_transmit(handle1, &trans_desc));
				show_seg();
			}

			else
			{
				if(Sensor_err == 1)					// Protect from redraw display every 5 sec.
				{
					Sensor_err = 0;
					if ((WINDOW == MAIN_WIN))
					{
						if(!FlagBlock) display_dig();
					}
					else
					{
						send_seg(255);
						send_seg(255);
						show_seg();
					}
				}
				Themperature = Calculate_Temper(voltage, sensor_num - 1);

			}
		}

		//===================================================================================
	    if(Sensor_err)
	    {
			Relay_flag = 0;
			RELAY_OFF;
	    }

		if ((Heat_mode == 0)&&(POWER_ON_OFF))
		{
			if(delta_air_temperature>5)
				{
					if ((!Sensor_err) && (!Relay_flag) && ((Themperature+(delta_air_temperature-5)) < set_floor_temperature))
					{
						temp_time_relay = RTC_TimeStr.RTC_Seconds;
						Relay_flag = 1;
						RELAY_ON;

					}

				    if((Relay_flag) && ((Themperature+(delta_air_temperature-5))  > set_floor_temperature))
					{
						Relay_flag = 0;
						RELAY_OFF;
					}
				}
			else
				{
					if ((!Sensor_err) && (!Relay_flag) && ((Themperature-(5-delta_air_temperature))   < set_floor_temperature))
					{
						temp_time_relay = RTC_TimeStr.RTC_Seconds;
						Relay_flag = 1;
						RELAY_ON;

					}

					if((Relay_flag) && ((Themperature-(5-delta_air_temperature))  > set_floor_temperature))
					{

						Relay_flag = 0;
						RELAY_OFF;
					}
				}
		}


	    if ((Heat_mode == 1)&&(POWER_ON_OFF))
		{
			EVENT();
	/*
			set_floor_temperature_prog = Program_step[Day_Event_Now][Event_now].Temp_set;

			if((set_floor_temperature_prog!=temp_set_floor_temperature))
				{
					if(WINDOW == MAIN_WIN) LedIndicatorDisplay(set_floor_temperature_prog);
					temp_set_floor_temperature = set_floor_temperature_prog;
				}
			if ((TMP_Event_now != Event_now))
				{
					TMP_Event_now = Event_now;

					set_floor_temperature_prog = Program_step[Day_Event_Now][Event_now].Temp_set;
					if(WINDOW == MAIN_WIN) LedIndicatorDisplay(set_floor_temperature_prog);
				}
	*/

			if((TMP_Event_now != Event_now)||(upd_tmp_tbl == pdTRUE))
			{
				TMP_Event_now = Event_now;
				set_floor_temperature_prog = Program_step[Day_Event_Now][Event_now].Temp_set;
				if(WINDOW == MAIN_WIN) LedIndicatorDisplay(set_floor_temperature_prog);
				upd_tmp_tbl = pdFALSE;
			}





			if(delta_air_temperature>5)
			{
				if ((!Sensor_err) && (!Relay_flag) && ((Themperature+(delta_air_temperature-5)) < set_floor_temperature_prog))
				{
					temp_time_relay = RTC_TimeStr.RTC_Seconds;
					Relay_flag = 1;
					RELAY_ON;
				}

				if((Relay_flag) && ((Themperature+(delta_air_temperature-5))  > set_floor_temperature_prog))
				{
					Relay_flag = 0;
					RELAY_OFF;
				}
			}
			else
			{
				if ((!Sensor_err) && (!Relay_flag) && ((Themperature-(5-delta_air_temperature))  < set_floor_temperature_prog))
				{
					temp_time_relay = RTC_TimeStr.RTC_Seconds;
					Relay_flag = 1;
					RELAY_ON;
				}

				if((Relay_flag) && ((Themperature-(5-delta_air_temperature))  > set_floor_temperature_prog))
				{
					Relay_flag = 0;
					RELAY_OFF;
				}
			}
		}
	    //======================================================


		if (WIFI)
		{
			wifi_rssi();
		}

		//printf("Free heap size: %d\n",esp_get_free_heap_size());

		vTaskDelay(5000 / portTICK_PERIOD_MS);
	}
}

//==========================================================================================
//======================================================================== Task for counters
//==========================================================================================
void Counter(void *pvParameters) // Counter 10ms
{

	while (1)
	{
		//-------------------Connection counter 40s,then reset first connected IP----------------------------
		if (conditions_register & CONN)
		{
			//printf("conn_Counter Start\n");
     		if (++conn_Counter > 4000)
			{
				Access_flag = 0;
				TCP_flag = 0;
				conditions_register = (conditions_register & (~CONN));
				conn_Counter = 0;
				ip_last = 0;
				printf("conn_Counter out\n");
			}

		}

		//-------------------LED COUNTER 40s-----------------------------------
		if (conditions_register & LIGHT)
		{

			if (++led_Counter > 4000)
			{
				if(!Sensor_err){
				uint16_t seg[1] = { 0xFFFF };
				trans_desc.length = 16;
				trans_desc.tx_buffer = seg;
				ESP_ERROR_CHECK(spi_device_transmit(handle1, &trans_desc));
				show_seg();}
				WINDOW=STANDBY_WIN;
	    		nvs_open("storage", NVS_READWRITE, &my_handle);
	    		nvs_set_u8(my_handle, "WINDOW", WINDOW);
	    		nvs_commit(my_handle);
	    		nvs_close(my_handle);
				led_Counter = 0;
				conditions_register = (conditions_register & (~LIGHT));
			}
		}
		//-------------------Smartconfig & AP count---------------------------
		if (conditions_register & SC_AP)
		{

			conn_flag = 1;

			if(wifi_conn_Counter<9800)
			{
				if(++sc_ap_counter==100)
				{
					LedIndicatorDisplay(sc_ap_dig);
					sc_ap_dig--;
					sc_ap_counter = 0;
					led_Counter = 0;
				}
			}

			if(--wifi_conn_Counter == 0)
			{
				//LED_OFF;
				conn_flag = 0;
				conditions_register = (conditions_register & (~SC_AP));

				Time_relay_ON += temp_Time_relay_ON;
				printf("Time_relay_ON %d\n",Time_relay_ON);
	    		nvs_open("storage", NVS_READWRITE, &my_handle);
	    		nvs_set_u32(my_handle, "Time_relay", Time_relay_ON);
	    		nvs_commit(my_handle);
	    		nvs_close(my_handle);
				esp_restart();
			}
		}
		//-------------------ERR COUNTER---------------------------------------
		if (count_register & HEART_SET)
		{

			heart_counter++;

			if (heart_counter == 20)
			{
				uint16_t seg[1] = { 0xFFFF };
				trans_desc.length = 16;
				trans_desc.tx_buffer = seg;
				ESP_ERROR_CHECK(spi_device_transmit(handle1, &trans_desc));
				show_seg();
			}
			if (heart_counter == 40)
			{
				LedIndicatorDisplay(set_floor_temperature);
				seg_calls++;
				heart_counter = 0;
			}
			if (seg_calls == 3)
			{
				count_register = (count_register & (~HEART_SET));
				seg_calls = 0;
			}
		}
		//---------------------------------------------------------------------

		if (count_register & BLOCK_COUNT)
		{
			block_counter++;
			if(block_counter == 1)
				LedIndicatorDisplay(3);
			if(block_counter == 100)
				LedIndicatorDisplay(2);
			if(block_counter == 200)
				LedIndicatorDisplay(1);
			if(block_counter == 300)
			{
				led_Counter = 0;
				conditions_register = (conditions_register & (~LIGHT));
				LedIndicatorDisplay(0);
				POWER_ON_OFF = 0;
				nvs_open("storage", NVS_READWRITE, &my_handle);
				nvs_set_u8(my_handle, "POWER_ON_OFF", POWER_ON_OFF);
				WINDOW=POWER_OFF_WIN;
	    		nvs_set_u8(my_handle, "WINDOW", WINDOW);
				Time_relay_ON += temp_Time_relay_ON;
	    		nvs_set_u32(my_handle, "Time_relay", Time_relay_ON);
	    		temp_Time_relay_ON = 0;
	    		nvs_commit(my_handle);
	    		nvs_close(my_handle);
				Relay_flag = 0;
				RELAY_OFF;
				OFF_ALL;
			}
		}
		//---------------------------------------------------------------------
		if (++clock_count > 100)
		{
			if(conditions_register & OTA)
			{

				if((Time_min24 == temp_mac[5])&&(RTC_TimeStr.RTC_Seconds == 0)) // check updates after 00.00 + last byte of MAC
				{
					ota_count = 0;
					vTaskSuspend(mqtt_subscribe_handle);
					//vTaskSuspend(read_temperature_handle);
					xTaskCreate(&ota_example_task, "ota_example_task", 4096, NULL, 20, &ota_handle);
				}
			}

			if(Relay_flag)
			{
				if(RTC_TimeStr.RTC_Seconds == 0)
				{
					temp_Time_relay_ON++;
					printf("temp_Time_relay_ON %d\n",temp_Time_relay_ON);
				}

				if((RTC_TimeStr.RTC_Seconds == 0)&&(RTC_TimeStr.RTC_Minutes == 0)) //save time_relay every our
				{
					Time_relay_ON += temp_Time_relay_ON;
					//printf("Time_relay_ON %d\n",Time_relay_ON);
		    		nvs_open("storage", NVS_READWRITE, &my_handle);
		    		nvs_set_u32(my_handle, "Time_relay", Time_relay_ON);
		    		nvs_commit(my_handle);
		    		nvs_close(my_handle);

		    		temp_Time_relay_ON = 0;
				}
			}
			else if((!Relay_flag)&&(temp_Time_relay_ON))
			{
				Time_relay_ON += temp_Time_relay_ON;
				//printf("Time_relay_ON %d\n",Time_relay_ON);
	    		nvs_open("storage", NVS_READWRITE, &my_handle);
	    		nvs_set_u32(my_handle, "Time_relay", Time_relay_ON);
	    		nvs_commit(my_handle);
	    		nvs_close(my_handle);

				temp_Time_relay_ON = 0;
				temp_time_relay = 0;

				if(Power1 == 65535)
				{
					Time_relay_ON = 0;
		    		nvs_open("storage", NVS_READWRITE, &my_handle);
		    		nvs_set_u32(my_handle, "Time_relay", Time_relay_ON);
		    		nvs_commit(my_handle);
		    		nvs_close(my_handle);
		    		Power1 = 0;
		    		Power2 = 0;
		    		temp_Time_relay_ON = 0;
				}
			}

			clock_count = 0;
			RTC_task();

			//printf("set_floor_temperature %d\n",set_floor_temperature);
			//printf("Time_min24 %d\n",Time_min24);
			//printf("Day_Event_Now %d\nDay_Event_Next %d\n",Day_Event_Now,Day_Event_Next);
			//printf("Temp_minuts %d\n",Temp_minuts);
			//printf("Heat_mode %d\n",Heat_mode);
			//printf("set_floor_temperature %d\n",set_floor_temperature);
		 }
		Time_min24=(unsigned int)RTC_TimeStr.RTC_Minutes+(unsigned int)RTC_TimeStr.RTC_Hours*60;

/*		for (int i = TOUCH_PAD_NUM2; i< TOUCH_PAD_NUM6; i++)
		{
			touch_pad_read_filtered(i, &touch_value);
			printf("%d   ", touch_value);
		}
		printf("\n");*/

		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}

void Button(void *pvParameters) // Counter 10ms
{

	while (1)
	{
/*		for (int i = TOUCH_PAD_NUM2; i < TOUCH_PAD_NUM6; i++)
		{
			if (s_pad_activated[i] == true)
			{
				button_fl[i] = true;
			}
			else
			{
				button_fl[i] = 0;
				button_en[i] = 0;
				button_count[i] = 0;
			}

			if (counter_calib[i] == 500)
			{
				if ((!button_fl[PLUS]) && (!button_fl[MINUS]) && (!button_fl[MODE]) && (!button_fl[HEART]))
				{
					counter_calib[i] = 0;
					touch_pad_read_filtered(i, &touch_value);
					//if (((s_pad_init_val[i] + 10) > (touch_value)))
					{
						s_pad_init_val[i] = touch_value;
						ESP_ERROR_CHECK(touch_pad_set_thresh(i, touch_value * TOUCH_THRESH_PERCENT / 100));
					}
				}
			}
		}*/

		if (counter_calib == 100)
		{
			if ((!s_pad_activated[TOUCH_PAD_NUM2])
			 && (!s_pad_activated[TOUCH_PAD_NUM3])
			 && (!s_pad_activated[TOUCH_PAD_NUM4])
			 && (!s_pad_activated[TOUCH_PAD_NUM5]))
			{
				counter_calib = 0;
				touch_pad_read_filtered(TOUCH_PAD_NUM2, &touch_value);
				//printf("(%d, %d)  ", s_pad_init_val[TOUCH_PAD_NUM2], touch_value);
				//if (((touch_button_no_use[BUTTON_MINUS] + 10) > (touch_value)))
				{
					s_pad_init_val[TOUCH_PAD_NUM2] = touch_value;
					ESP_ERROR_CHECK(touch_pad_set_thresh(TOUCH_PAD_NUM2, touch_value * TOUCH_THRESH_PERCENT / 100));
				}

				touch_pad_read_filtered(TOUCH_PAD_NUM3, &touch_value);
				//printf("(%d, %d)   ", s_pad_init_val[TOUCH_PAD_NUM3], touch_value);
				//if (((touch_button_no_use[BUTTON_MODE_OK] + 10) > (touch_value)))
				{
					s_pad_init_val[TOUCH_PAD_NUM3] = touch_value;
					ESP_ERROR_CHECK(touch_pad_set_thresh(TOUCH_PAD_NUM3, touch_value * TOUCH_THRESH_PERCENT / 100));
				}

				touch_pad_read_filtered(TOUCH_PAD_NUM4, &touch_value);
				//printf("(%d, %d)   ", s_pad_init_val[TOUCH_PAD_NUM4], touch_value);
				//if (((touch_button_no_use[BUTTON_ON_OFF] + 10) > (touch_value)))
				{
					s_pad_init_val[TOUCH_PAD_NUM4] = touch_value;
					ESP_ERROR_CHECK(touch_pad_set_thresh(TOUCH_PAD_NUM4, touch_value * TOUCH_THRESH_PERCENT / 100));
				}

				touch_pad_read_filtered(TOUCH_PAD_NUM5, &touch_value);
				//printf("(%d, %d)\n", s_pad_init_val[TOUCH_PAD_NUM5], touch_value);
				//if (((touch_button_no_use[BUTTON_SET_CANCEL] + 10) > (touch_value)))
				{
					s_pad_init_val[TOUCH_PAD_NUM5] = touch_value;
					ESP_ERROR_CHECK(touch_pad_set_thresh(TOUCH_PAD_NUM5, touch_value * TOUCH_THRESH_PERCENT / 100));
				}

/* 				printf("======================== DEBUG THRESH HOLD ======================\n");
				printf("%d    %d   %d   %d   %d\n", touch_button_no_use[2],
					   								touch_button_no_use[4],
					   								touch_button_no_use[5],
					   								touch_button_no_use[7],
					   								touch_button_no_use[8]);


				uint16_t no_use_val[5] = {0,0,0,0,0};
				touch_pad_get_thresh(2, &no_use_val[0]);
				touch_pad_get_thresh(4, &no_use_val[1]);
				touch_pad_get_thresh(5, &no_use_val[2]);
				touch_pad_get_thresh(7, &no_use_val[3]);
				touch_pad_get_thresh(8, &no_use_val[4]);
				printf("%d    %d   %d   %d   %d\n", no_use_val[0],
					   								no_use_val[1],
					   								no_use_val[2],
					   								no_use_val[3],
					   								no_use_val[4]);
				printf("=================================================================\n"); */
			}
		}
		else
		{
			counter_calib++;
		}
		 //printf("%d   %d   %d   %d",counter_calib[2],counter_calib[3],counter_calib[4],counter_calib[5]);
		 //printf("          %d   %d   %d   %d\n",s_pad_init_val[2],s_pad_init_val[3],s_pad_init_val[4],s_pad_init_val[5]);

/*		if(counter_calib[PLUS]<500)  counter_calib[PLUS]++;
		if(counter_calib[MINUS]<500) counter_calib[MINUS]++;
		if(counter_calib[MODE]<500)  counter_calib[MODE]++;
		if(counter_calib[HEART]<500) counter_calib[HEART]++;*/

		if(s_pad_activated[PLUS])
		{
			if (++button_count[PLUS] > 10)
			{
				button_en[PLUS] = 1;
				button_count[PLUS] = 1;
			}
		}
		else
		{
			button_en[PLUS] = 0;
			button_count[PLUS] = 0;
		}

		if(s_pad_activated[MINUS])
		{
			if (++button_count[MINUS] > 10)
			{
				button_en[MINUS] = 1;
				button_count[MINUS] = 1;
			}
		}
		else
		{
			button_en[MINUS] = 0;
			button_count[MINUS] = 0;
		}

		if(s_pad_activated[HEART])
		{
			if (++button_count[HEART] > 10)
			{
				button_en[HEART] = 1;
				button_count[HEART] = 1;
			}
		}
		else
		{
			button_en[HEART] = 0;
			button_count[HEART] = 0;
		}

		if(s_pad_activated[MODE])
		{
			if (++button_count[MODE] > 10)
			{
				button_en[MODE] = 1;
				button_count[MODE] = 1;
			}
		}
		else
		{
			button_en[MODE] = 0;
			button_count[MODE] = 0;
		}



		if(!(sta_ap_flag)){ 		// no action while connecting to wifi

			if (button_en[HEART])
			{
/*				counter_calib[PLUS] = 0;
				counter_calib[MINUS] = 0;
				counter_calib[HEART] = 0;
				counter_calib[MODE] = 0;*/
				counter_calib = 0;

				if (BT_HEART_PAR.counter == BT_HEART_PAR.clock_dlong_click)
				{
					BT_HEART_PAR.flag_click = 1;
					BT_HEART_PAR.type_click = DLONG_CLICK;
				}
				else if (BT_HEART_PAR.counter == BT_HEART_PAR.clock_long_click)
				{
					BT_HEART_PAR.flag_click = 1;
					BT_HEART_PAR.type_click = LONG_CLICK;
				}
				else if (BT_HEART_PAR.counter == BT_HEART_PAR.clock_short_click)
				{
					BT_HEART_PAR.flag_click = 1;
					BT_HEART_PAR.type_click = SHORT_CLICK;
				}
				else if (BT_HEART_PAR.counter == BT_HEART_PAR.clock_dshort_click)
				{
					BT_HEART_PAR.flag_click = 1;
					BT_HEART_PAR.type_click = DSHORT_CLICK;
				}
				if (BT_HEART_PAR.counter < BT_HEART_PAR.clock_dlong_click)
					BT_HEART_PAR.counter++;
			}
			else
			{
				BT_HEART_PAR.flag_click = 0;
				BT_HEART_PAR.counter = 0;
			}


			if (button_en[PLUS])
			{
/*				counter_calib[PLUS] = 0;
				counter_calib[MINUS] = 0;
				counter_calib[HEART] = 0;
				counter_calib[MODE] = 0;*/

				counter_calib = 0;


				if (BT_PLUS_PAR.counter == BT_PLUS_PAR.clock_dlong_click)
				{
					BT_PLUS_PAR.flag_click = 1;
					BT_PLUS_PAR.type_click = DLONG_CLICK;
				}
				else if (BT_PLUS_PAR.counter == BT_PLUS_PAR.clock_long_click)
				{
					BT_PLUS_PAR.flag_click = 1;
					BT_PLUS_PAR.type_click = LONG_CLICK;
				}
				else if (BT_PLUS_PAR.counter == BT_PLUS_PAR.clock_short_click)
				{
					BT_PLUS_PAR.flag_click = 1;
					BT_PLUS_PAR.type_click = SHORT_CLICK;
				}
				else if (BT_PLUS_PAR.counter == BT_PLUS_PAR.clock_dshort_click)
				{
					BT_PLUS_PAR.flag_click = 1;
					BT_PLUS_PAR.type_click = DSHORT_CLICK;
				}

				if (BT_PLUS_PAR.counter < BT_PLUS_PAR.clock_dlong_click)
					BT_PLUS_PAR.counter++;
			}
			else
			{
				BT_PLUS_PAR.flag_click = 0;
				BT_PLUS_PAR.counter = 0;
			}

			if (button_en[MINUS])
			{
/*				counter_calib[PLUS] = 0;
				counter_calib[MINUS] = 0;
				counter_calib[HEART] = 0;
				counter_calib[MODE] = 0;*/

				counter_calib = 0;


				if (BT_MINUS_PAR.counter == BT_MINUS_PAR.clock_dlong_click)
				{
					BT_MINUS_PAR.flag_click = 1;
					BT_MINUS_PAR.type_click = DLONG_CLICK;
				}
				else if (BT_MINUS_PAR.counter == BT_MINUS_PAR.clock_long_click)
				{
					BT_MINUS_PAR.flag_click = 1;
					BT_MINUS_PAR.type_click = LONG_CLICK;
				}
				else if (BT_MINUS_PAR.counter == BT_MINUS_PAR.clock_short_click)
				{
					BT_MINUS_PAR.flag_click = 1;
					BT_MINUS_PAR.type_click = SHORT_CLICK;
				}
				else if (BT_MINUS_PAR.counter == BT_MINUS_PAR.clock_dshort_click)
				{
					BT_MINUS_PAR.flag_click = 1;
					BT_MINUS_PAR.type_click = DSHORT_CLICK;
				}

				if (BT_MINUS_PAR.counter < BT_MINUS_PAR.clock_dlong_click)
					BT_MINUS_PAR.counter++;
			}
			else
			{
				BT_MINUS_PAR.flag_click = 0;
				BT_MINUS_PAR.counter = 0;

			}

			if (button_en[MODE])
			{
/*				counter_calib[PLUS] = 0;
				counter_calib[MINUS] = 0;
				counter_calib[HEART] = 0;
				counter_calib[MODE] = 0;*/

				counter_calib = 0;


				if (BT_MODE_PAR.counter == BT_MODE_PAR.clock_dlong_click)
				{
					BT_MODE_PAR.flag_click = 1;
					BT_MODE_PAR.type_click = DLONG_CLICK;
				}
				else if (BT_MODE_PAR.counter == BT_MODE_PAR.clock_long_click)
				{
					BT_MODE_PAR.flag_click = 1;
					BT_MODE_PAR.type_click = LONG_CLICK;
				}
				else if (BT_MODE_PAR.counter == BT_MODE_PAR.clock_short_click)
				{
					BT_MODE_PAR.flag_click = 1;
					BT_MODE_PAR.type_click = SHORT_CLICK;
				}
				else if (BT_MODE_PAR.counter == BT_MODE_PAR.clock_dshort_click)
				{
					BT_MODE_PAR.flag_click = 1;
					BT_MODE_PAR.type_click = DSHORT_CLICK;
				}

				if (BT_MODE_PAR.counter < BT_MODE_PAR.clock_dlong_click)
					BT_MODE_PAR.counter++;
			}
			else
			{
				BT_MODE_PAR.flag_click = 0;
				BT_MODE_PAR.counter = 0;
			}
		}

/*		for (int i = TOUCH_PAD_NUM2; i < TOUCH_PAD_NUM6; i++)  //reset interrupt register
		{
			s_pad_activated[i] = false;
		}*/

		s_pad_activated[TOUCH_PAD_NUM2] 		= 0;
		s_pad_activated[TOUCH_PAD_NUM3] 		= 0;
		s_pad_activated[TOUCH_PAD_NUM4] 	    = 0;
		s_pad_activated[TOUCH_PAD_NUM5] 		= 0;

		LedButtonDisplay();
		Menu();

		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}



void heat_task(void *pvParameters)
{
	while(1){
	//printf("while\r");
    if(Sensor_err)
    {
		Relay_flag = 0;
		RELAY_OFF;
    }

	if ((Heat_mode == 0)&&(POWER_ON_OFF))
	{
		if(delta_air_temperature>5)
			{
				if ((!Sensor_err) && (!Relay_flag) && ((Themperature+(delta_air_temperature-5)) < set_floor_temperature))
				{
					temp_time_relay = RTC_TimeStr.RTC_Seconds;
					Relay_flag = 1;
					RELAY_ON;

				}

			    if((Relay_flag) && ((Themperature+(delta_air_temperature-5))  > set_floor_temperature))
				{
					Relay_flag = 0;
					RELAY_OFF;
				}
			}
		else
			{
				if ((!Sensor_err) && (!Relay_flag) && ((Themperature-(5-delta_air_temperature))   < set_floor_temperature))
				{
					temp_time_relay = RTC_TimeStr.RTC_Seconds;
					Relay_flag = 1;
					RELAY_ON;

				}

				if((Relay_flag) && ((Themperature-(5-delta_air_temperature))  > set_floor_temperature))
				{

					Relay_flag = 0;
					RELAY_OFF;
				}
			}
	}


    if ((Heat_mode == 1)&&(POWER_ON_OFF))
	{
		EVENT();
/*
		set_floor_temperature_prog = Program_step[Day_Event_Now][Event_now].Temp_set;

		if((set_floor_temperature_prog!=temp_set_floor_temperature))
			{
				if(WINDOW == MAIN_WIN) LedIndicatorDisplay(set_floor_temperature_prog);
				temp_set_floor_temperature = set_floor_temperature_prog;
			}
		if ((TMP_Event_now != Event_now))
			{
				TMP_Event_now = Event_now;

				set_floor_temperature_prog = Program_step[Day_Event_Now][Event_now].Temp_set;
				if(WINDOW == MAIN_WIN) LedIndicatorDisplay(set_floor_temperature_prog);
			}
*/

		if((TMP_Event_now != Event_now)||(upd_tmp_tbl == pdTRUE))
		{
			TMP_Event_now = Event_now;
			set_floor_temperature_prog = Program_step[Day_Event_Now][Event_now].Temp_set;
			if(WINDOW == MAIN_WIN) LedIndicatorDisplay(set_floor_temperature_prog);
			upd_tmp_tbl = pdFALSE;
		}





		if(delta_air_temperature>5)
		{
			if ((!Sensor_err) && (!Relay_flag) && ((Themperature+(delta_air_temperature-5)) < set_floor_temperature_prog))
			{
				temp_time_relay = RTC_TimeStr.RTC_Seconds;
				Relay_flag = 1;
				RELAY_ON;
			}

			if((Relay_flag) && ((Themperature+(delta_air_temperature-5))  > set_floor_temperature_prog))
			{
				Relay_flag = 0;
				RELAY_OFF;
			}
		}
		else
		{
			if ((!Sensor_err) && (!Relay_flag) && ((Themperature-(5-delta_air_temperature))  < set_floor_temperature_prog))
			{
				temp_time_relay = RTC_TimeStr.RTC_Seconds;
				Relay_flag = 1;
				RELAY_ON;
			}

			if((Relay_flag) && ((Themperature-(5-delta_air_temperature))  > set_floor_temperature_prog))
			{
				Relay_flag = 0;
				RELAY_OFF;
			}
		}
	}
    vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}




//==========================================================================================
//===================================================================================== MAIN
//==========================================================================================
void app_main()
{

//----------------------Buttons init--------------------------------------------

	BT_PLUS_PAR.flag_click = 0;
	BT_PLUS_PAR.counter = 0;
	BT_PLUS_PAR.type_click = NO_CLICK;
	BT_PLUS_PAR.clock_short_click = COUNT_SHORT_CLICK;
	BT_PLUS_PAR.clock_dshort_click = COUNT_DSHORT_CLICK;
	BT_PLUS_PAR.clock_long_click = COUNT_LONG_CLICK;
	BT_PLUS_PAR.clock_dlong_click = COUNT_DLONG_CLICK;

	BT_MINUS_PAR.flag_click = 0;
	BT_MINUS_PAR.counter = 0;
	BT_MINUS_PAR.type_click = NO_CLICK;
	BT_MINUS_PAR.clock_short_click = COUNT_SHORT_CLICK;
	BT_MINUS_PAR.clock_dshort_click = COUNT_DSHORT_CLICK;
	BT_MINUS_PAR.clock_long_click = COUNT_LONG_CLICK;
	BT_MINUS_PAR.clock_dlong_click = COUNT_DLONG_CLICK;

	BT_MODE_PAR.flag_click = 0;
	BT_MODE_PAR.counter = 0;
	BT_MODE_PAR.type_click = NO_CLICK;
	BT_MODE_PAR.clock_short_click = COUNT_SHORT_CLICK;
	BT_MODE_PAR.clock_dshort_click = COUNT_DSHORT_CLICK;
	BT_MODE_PAR.clock_long_click = COUNT_LONG_CLICK;
	BT_MODE_PAR.clock_dlong_click = COUNT_DLONG_CLICK;

	BT_HEART_PAR.flag_click = 0;
	BT_HEART_PAR.counter = 0;
	BT_HEART_PAR.type_click = NO_CLICK;
	BT_HEART_PAR.clock_short_click = COUNT_SHORT_CLICK;
	BT_HEART_PAR.clock_dshort_click = COUNT_DSHORT_CLICK;
	BT_HEART_PAR.clock_long_click = COUNT_LONG_CLICK;
	BT_HEART_PAR.clock_dlong_click = COUNT_DLONG_CLICK;

//----------------------------Beeper init pwm mode-------------------------------

	ledc_timer_config_t timer_conf;
	timer_conf.duty_resolution = LEDC_TIMER_12_BIT;
	timer_conf.freq_hz = 4000;
	timer_conf.speed_mode = LEDC_HIGH_SPEED_MODE;
	timer_conf.timer_num = LEDC_TIMER_0;
	ledc_timer_config(&timer_conf);

	ledc_channel_config_t ledc_conf;
	ledc_conf.channel = LEDC_CHANNEL_1;
	ledc_conf.duty = 512;
	ledc_conf.gpio_num = 19;
	ledc_conf.intr_type = LEDC_INTR_DISABLE;
	ledc_conf.speed_mode = LEDC_HIGH_SPEED_MODE;
	ledc_conf.timer_sel = LEDC_TIMER_0;
	ledc_channel_config(&ledc_conf);
	ledc_timer_pause(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0);

//--------------------------------------------------------------------------------
//----------------OTA partitions init---------------------------------------------
	esp_err_t err = nvs_flash_init();

	if (err == ESP_ERR_NVS_NO_FREE_PAGES)
	{
		// OTA app partition table has a smaller NVS partition size than the non-OTA
		// partition table. This size mismatch may cause NVS initialization to fail.
		// If this happens, we erase NVS partition and initialize NVS again.
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();

	}
//----------------------------------------------------------------------------------

	adc_init();
	GPIO_Conf();
	SPI_Init();
	init_i2c_rtc();

//--------------------------------------------------First start management---------------------------
	RELAY_OFF;
	first_start = 1;

	nvs_open("storage", NVS_READWRITE, &my_handle);
	nvs_get_u8(my_handle, "first_start", &first_start);
	nvs_get_u8(my_handle, "tempSensorset", &temp_Sensor_set);
	nvs_get_u8(my_handle, "no_floor_flag", &no_floor_flag);
	nvs_get_u32(my_handle, "Time_relay", &Time_relay_ON);
	nvs_commit(my_handle);
	nvs_close(my_handle);

	if (first_start)
	{
		RELAY_OFF;
		Factory_SAVE();
		first_start = 0;
		nvs_open("storage", NVS_READWRITE, &my_handle);
		nvs_set_u8(my_handle, "first_start", first_start);
		nvs_commit(my_handle);
		nvs_close(my_handle);
		upd_tmp_tbl = pdTRUE;
		for(uint8_t sg = 99; sg > 0; sg-=11)
		{
			vTaskDelay(1000 / portTICK_PERIOD_MS);
			LedIndicatorDisplay(sg);
			if(sg==99) send_led(0xB61D);
			if(sg==77) send_led(0xDB46);
			if(sg==55) send_led(0x6D2B);
			if(sg==33) send_led(0x00F0);
		}
	}
//----------------------------------------------------------------------------------------------------
	Load_from_flash();
//----------------------------------------------------------------------------------------------------
	wifi_start_flag = 0;
	conn_flag = 0;
	ap_flag = 0;
	sta_ap_flag = 0;

	Flag_OFF = 1;

	set_temperature = 15;


	force_update = 0;

	TMP_Event_now = 5;
	task_flag = 1;
	conn_Counter = 0;
	reconn_count = 0;
	led_Counter = 0;
	ota_count = 0;
	wifi_conn_Counter = 10000;
	Relay_flag = 0;
	TCP_flag = 0;
	temp_set_floor_temperature = 0;

	WIFI = 0;

	Flag_Reconect = 0;

	sc_ap_dig=97;


	touchinit();
	initialise_wifi();

	if(POWER_ON_OFF)
	{
		if(WINDOW == MAIN_WIN){
		display_dig();
		conditions_register=(conditions_register|LIGHT);
		led_Counter = 0;}
		else
		{
			send_seg(255);
			send_seg(255);
			show_seg();
		}
	}
	else
	{
		Relay_flag = 0;
		RELAY_OFF;
		OFF_ALL;
		WINDOW = POWER_OFF_WIN;
	}



	xTaskCreatePinnedToCore(&read_temperature, "read_temperature", 8096, NULL, 10, &read_temperature_handle, 0);
	xTaskCreatePinnedToCore(&Counter, "Counter", 8096, NULL, 10, &Counter_handle, 1);
	xTaskCreatePinnedToCore(&Button, "Button", 8096, NULL, 10, &Button_handle, 1);
	//xTaskCreate( &heat_task, "heat_task", 8096, NULL, 10, &heat_task_handle );


}


