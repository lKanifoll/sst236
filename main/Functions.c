/*
 * Functions.c
 *
 *  Created on: Dec 3, 2017
 *      Author: sergey
 */

#include "Functions.h"
void reset_clock()
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (0x51 << 1) | I2C_MASTER_WRITE, 1);
	i2c_master_write_byte(cmd, 0x00, 1); //start adress of date

	i2c_master_write_byte(cmd, 0x01, 1);
	i2c_master_write_byte(cmd, 0x00, 1);
	i2c_master_write_byte(cmd, 0x00, 1);
	i2c_master_write_byte(cmd, 0x00, 1);
	i2c_master_write_byte(cmd, 0x01, 1); //seconds
	i2c_master_write_byte(cmd, 0x00, 1); //minutes
	i2c_master_write_byte(cmd, 0x00, 1); //ours
	i2c_master_write_byte(cmd, 0x01, 1); //date
	i2c_master_write_byte(cmd, 0x01, 1); //weekday
	i2c_master_write_byte(cmd, 0x01, 1); //month
	i2c_master_write_byte(cmd, 0x18, 1); //year
	i2c_master_stop(cmd);
	i2c_master_cmd_begin(I2C_NUM_0, cmd, 10);

	i2c_cmd_link_delete(cmd);
}

void send_seg(uint8_t data) //send 8bit to shift register
{
	uint8_t seg[1] = { data };
	trans_desc.length = 8;
	trans_desc.tx_buffer = seg;
	ESP_ERROR_CHECK(spi_device_transmit(handle1, &trans_desc));
}

void send_led(uint16_t data) //send 8bit to shift register
{
	uint16_t seg[1] = { data };
	trans_desc1.length = 16;
	trans_desc1.tx_buffer = seg;
	ESP_ERROR_CHECK(spi_device_transmit(handle, &trans_desc1));
	gpio_set_level(17, 1);
	gpio_set_level(17, 0);
}

void show_seg()            // switch shift register to show info
{
	gpio_set_level(21, 1);
	gpio_set_level(21, 0);
}

void display_dig()
{
	if(Heat_mode)
	LedIndicatorDisplay(set_floor_temperature_prog);
	else
	LedIndicatorDisplay(set_floor_temperature);
}
//==========================================================================================
//===================================================================== RSSI calc and output
//==========================================================================================

void wifi_rssi()
{
	esp_wifi_sta_get_ap_info(&ap_info);
	if(ap_info.rssi > (-60))  							  {WIFI_Level = 0x04;}
	if((ap_info.rssi <= (-60))&&(ap_info.rssi >= (-70)))  {WIFI_Level = 0x03;}
	if((ap_info.rssi <= (-70))&&(ap_info.rssi >= (-80)))  {WIFI_Level = 0x02;}
	if(ap_info.rssi < (-80))  							  {WIFI_Level = 0x01;}
	//printf("RSSI %d\n",ap_info.rssi);
}


//==========================================================================================
//================================================================================= ADC init
//==========================================================================================
void adc_init()
{
	//adc2_vref_to_gpio(25);
	adc2_config_channel_atten(ADC2_CHANNEL_7,ADC_ATTEN_DB_11);
	//adc2_config_channel_atten(ADC2_CHANNEL_8,ADC_ATTEN_DB_11);
/*
	adc2_config_channel_atten(ADC2_CHANNEL_8,ADC_ATTEN_DB_11);
	adc2_vref_to_gpio(25);
	int adccalib=0;
	adc2_get_raw(ADC2_CHANNEL_8, ADC_WIDTH_BIT_12, &adccalib);
	calib_ref = (uint32_t)adccalib;
	printf("calib %d\n",calib_ref);
*/
	adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
	esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_2, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1200, adc_chars);
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        //printf("Characterized using Two Point Value\n");
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        //printf("Characterized using eFuse Vref\n");
    } else {
        //printf("Characterized using Default Vref\n");
    }
    printf("===============\n");
    printf("= VERSION %s =\n",VERSION);
    printf("===============\n");
}
//==========================================================================================
//===================================================== Save default to flash (first start)
//==========================================================================================
void Factory_SAVE(void)
{
  nvs_open("storage", NVS_READWRITE, &my_handle);
  POWER_ON_OFF = 0;
  //WINDOW = MAIN_WIN;
  WINDOW = POWER_OFF_WIN;
  nvs_set_u8(my_handle, "POWER_ON_OFF", POWER_ON_OFF);
  nvs_set_u8(my_handle, "WINDOW", WINDOW);
  //---------------------------------------------------------------------------------------- Heat table
  for(uint8_t k=0;k<7;k++)						// 7 days counter
  {
	Program_step[k][0].Time_prog= 420;  // 7.00
    Program_step[k][0].Temp_set= 28;	// 28C
    Program_step[k][1].Time_prog= 480;	// 8.00
    Program_step[k][1].Temp_set= 12;	// 12C
    Program_step[k][2].Time_prog= 1140;	// 19.00
    Program_step[k][2].Temp_set= 28;	// 28C
    Program_step[k][3].Time_prog= 1320;	// 22.00
    Program_step[k][3].Temp_set= 12;	// 12C
  }
  Time_relay_ON = 0;
  nvs_set_u32(my_handle, "Time_relay", Time_relay_ON);

  no_sound = 0;
  nvs_set_u8(my_handle, "no_sound", no_sound);

  light_mode = 1;
  nvs_set_u8(my_handle, "light_mode", light_mode);

  love_temperature = 28;
  nvs_set_u8(my_handle, "love_temp", love_temperature);

  size_t Program_step_len = 112; //sizeof(program)*4*7
  //Program_step_len = sizeof(program)*4*7;
  nvs_set_blob(my_handle, "Program_step", (const void*)Program_step, Program_step_len);
  //---------------------------------------------------------------------------------------- Preheat table
  for(uint8_t k=0;k<40;k++) { Table_HEAT_TIME[k]=10; }
  size_t Table_HEAT_TIME_len = 0;
  Table_HEAT_TIME_len = sizeof(Table_HEAT_TIME);
  nvs_set_blob(my_handle, "Table_HEAT_TIME", (const void*)Table_HEAT_TIME, Table_HEAT_TIME_len);
  //---------------------------------------------------------------------------------------- Heat mode
  Heat_mode = 0;
  nvs_set_u8(my_handle, "Heat_mode", Heat_mode);
  //---------------------------------------------------------------------------------------- Set floor temp
  set_floor_temperature = 20;
  nvs_set_u8(my_handle, "set_floor_temp", set_floor_temperature);
  //---------------------------------------------------------------------------------------- Set air temp
  set_air_temperature = 20;
  nvs_set_u8(my_handle, "set_air_temp", set_air_temperature);
  //---------------------------------------------------------------------------------------- Set delta air temp
  delta_air_temperature=5;
  nvs_set_u8(my_handle, "delta_air_temp", delta_air_temperature);
  //---------------------------------------------------------------------------------------- Set sensor set 0x07 - all sensors + selftraining
  Sensor_set=0x02;
  nvs_set_u8(my_handle, "Sensor_set", Sensor_set);
  //---------------------------------------------------------------------------------------- Sensor type 0x01 - 6.8K
  sensor_num=0x01;
  nvs_set_u8(my_handle, "sensor_num", sensor_num);
  //---------------------------------------------------------------------------------------- MQTT server IP (default 185.76.147.189)
  strcpy((char*)&MQTT_IP[0],"");
  nvs_set_str(my_handle, "MQTT_IP", MQTT_IP);
  //---------------------------------------------------------------------------------------- MQTT port (default 1883)
  strcpy((char*)&MQTT_Port[0],"");
  nvs_set_str(my_handle, "MQTT_Port", MQTT_Port);
  //---------------------------------------------------------------------------------------- MQTT UID
  strcpy((char*)&UID[0],"");
  nvs_set_str(my_handle, "UID", (const char *)UID);
  //---------------------------------------------------------------------------------------- WIFI SSID
  strcpy((char*)&SSID[0],"");
  nvs_set_str(my_handle, "SSID", (const char *)SSID);
  //---------------------------------------------------------------------------------------- WIFI PASS
  strcpy((char*)&PASS[0],"");
  nvs_set_str(my_handle, "PASS", (const char *)PASS);
  //---------------------------------------------------------------------------------------- First link flag (def 0)
  first_link = 0;
  nvs_set_u8(my_handle, "first_link", first_link);
  //----------------------------------------------------------------------------------------
  nvs_commit(my_handle);
  nvs_close(my_handle);
}

//==========================================================================================
//================================================================= Load settings from flash
//==========================================================================================
void Load_from_flash()
{
  nvs_open("storage", NVS_READWRITE, &my_handle);
  nvs_get_u8(my_handle, "no_sound", &no_sound);
  nvs_get_u8(my_handle, "light_mode", &light_mode);
  nvs_get_u8(my_handle, "love_temp", &love_temperature);
  nvs_get_u8(my_handle, "POWER_ON_OFF", &POWER_ON_OFF);
  nvs_get_u8(my_handle, "WINDOW", &WINDOW);
  //---------------------------------------------------------------------------------------- Heat table
  size_t required_size_p = 0;
  nvs_get_blob(my_handle, "Program_step", NULL, &required_size_p);
  nvs_get_blob(my_handle, "Program_step",&Program_step, &required_size_p);
  //---------------------------------------------------------------------------------------- Preheat table
  size_t required_size_h = 0;
  nvs_get_blob(my_handle, "Table_HEAT_TIME", NULL, &required_size_h);
  nvs_get_blob(my_handle, "Table_HEAT_TIME",&Table_HEAT_TIME, &required_size_h);
  //---------------------------------------------------------------------------------------- Heat mode
  nvs_get_u8(my_handle, "Heat_mode", &Heat_mode);
  //---------------------------------------------------------------------------------------- Set floor temp
  nvs_get_u8(my_handle, "set_floor_temp", &set_floor_temperature);
  //---------------------------------------------------------------------------------------- Set air temp
  nvs_get_u8(my_handle, "set_air_temp", &set_air_temperature);
  //---------------------------------------------------------------------------------------- Set delta air temp
  nvs_get_u8(my_handle, "delta_air_temp", &delta_air_temperature);
  //---------------------------------------------------------------------------------------- Set sensor set 0x07 - all sensors + selftraining
  nvs_get_u8(my_handle, "Sensor_set", &Sensor_set);
  //---------------------------------------------------------------------------------------- Sensor type 0x01 - 6.8K
  nvs_get_u8(my_handle, "sensor_num", &sensor_num);
  //---------------------------------------------------------------------------------------- MQTT server IP (default 185.76.147.189)
  size_t mqtt_ip_len = 0;
  nvs_get_str(my_handle, "MQTT_IP", NULL, &mqtt_ip_len);
  nvs_get_str(my_handle, "MQTT_IP", (char *)&MQTT_IP[0], &mqtt_ip_len);
  //---------------------------------------------------------------------------------------- MQTT port (default 1883)
  size_t mqtt_port_len = 0;
  nvs_get_str(my_handle, "MQTT_Port", NULL, &mqtt_port_len);
  nvs_get_str(my_handle, "MQTT_Port", (char *)&MQTT_Port[0], &mqtt_port_len);
  //---------------------------------------------------------------------------------------- MQTT UID
  size_t mqtt_uid_len = 0;
  nvs_get_str(my_handle, "UID", NULL, &mqtt_uid_len);
  nvs_get_str(my_handle, "UID", (char *)&UID[0], &mqtt_uid_len);
  //printf("uid %s\n",UID);
  //---------------------------------------------------------------------------------------- WIFI SSID
  size_t mqtt_ssid_len = 0;
  nvs_get_str(my_handle, "SSID", NULL, &mqtt_ssid_len);
  nvs_get_str(my_handle, "SSID", (char *)&SSID[0], &mqtt_ssid_len);
  //---------------------------------------------------------------------------------------- WIFI PASS
  size_t mqtt_pass_len = 0;
  nvs_get_str(my_handle, "PASS", NULL, &mqtt_pass_len);
  nvs_get_str(my_handle, "PASS", (char *)&PASS[0], &mqtt_pass_len);
  //----------------------------------------------------------------------------------------

  nvs_commit(my_handle);
  nvs_close(my_handle);
}

//==========================================================================================
//================================================================ Function calc temperature
//==========================================================================================
int8_t Calculate_Temper(uint16_t Code, uint8_t Sens_tipe)
 {
	  int8_t Temper[37]={       -55,     -50,     -45,     -40,     -35,     -30,     -25,     -20,     -15,     -10,      -5,       0,       5,      10,      15,      20,       25,      30,      35,      40,      45,      50,     55,     60,     65,     70,     75,     80,     85,     90,     95,    100,    105,    110,    115,    120,      125 };
	  float Resistor[6][37]={
							  {3143.87, 3079.56, 2995.17, 2887.57, 2754.51, 2596.14, 2413.56, 2212.25, 1998.05, 1780.00, 1565.53, 1362.58, 1175.78, 1008.96,  862.77,  737.34,  630.88,  541.57,  467.16,  405.57,  354.60,  312.50, 277.85, 249.15, 225.46, 205.81, 189.54, 175.91, 164.55, 155.02, 147.01, 140.25, 134.56, 129.73, 125.58, 122.02, 118.93},
							  {3180.70, 3132.87, 3069.93, 2989.01, 2887.59, 2763.94, 2617.45, 2450.16, 2264.55, 2066.81, 1857.71, 1650.34, 1454.58, 1271.72, 1105.23,  956.75,     825,  712.07,  615.68,  534.07,  465.79,  408.41, 359.87, 319.36, 285.74, 257.60, 233.97, 214.14, 197.45, 183.38, 171.49, 161.42, 152.96, 145.75, 139.56, 134.26, 129.74},
							  {3164.15, 3114.84, 3051.85, 2973.09, 2876.81, 2761.94, 2628.39, 2477.34, 2307.72, 2126.72, 1936.22, 1745.00, 1559.30, 1382.74, 1218.11, 1068.25,  932.60,  813.78,  710.83,  621.90,  545.22,  479.85, 423.77, 376.36, 336.64, 302.98, 274.39, 250.13, 229.46, 211.86, 196.97, 184.24, 173.27, 163.84, 155.74, 148.75, 142.72},
							  {3228.69, 3197.52, 3155.46, 3099.92, 3028.23, 2937.97, 2827.02, 2694.58, 2542.38, 2371.74, 2186.46, 1991.95, 1793.10, 1597.36, 1409.60, 1234.84, 1077.55,  933.22,  808.57,  700.59,  610.41,  532.96, 464.86, 407.66, 360.07, 320.07, 286.59, 258.45, 234.93, 215.13, 198.05, 183.63, 171.96, 162.04, 153.38, 146.00, 139.77},
							  {3273.05, 3261.38, 3245.24, 3223.19, 3193.47, 3153.96, 3102.05, 3035.92, 2951.94, 2849.36, 2724.88, 2580.51, 2418.16, 2241.05, 2055.04, 1864.34, 1674.62, 1491.28, 1315.61, 1153.94, 1009.22,  880.52, 766.86, 668.42, 583.71, 511.31, 449.89, 397.81, 354.17, 317.27, 285.82, 259.20, 236.80, 217.79, 201.54, 187.70, 175.91},
							  {3280.10, 3271.33, 3259.20, 3242.64, 3220.37, 3190.80, 3152.15, 3102.42, 3039.52, 2961.46, 2866.53, 2753.66, 2622.64, 2474.49, 2311.46, 2137.01, 1955.55, 1772.04, 1591.36, 1417.85, 1254.99, 1105.21, 969.84, 849.32, 743.35, 651.08, 571.34, 502.89, 444.34, 394.45, 352.02, 315.96, 285.32, 259.29, 237.15, 218.31, 202.24},
	  };
  //float code_temp= (float)Code/1000.0;   // mV -> V
  float Code_ADC[37];
  //float factory_ref_voltage=3.3;
  int8_t Res;
  uint8_t i, kr;
  float k, m;


  kr=0;
  for (i=0;i<37;i++)
   {
	  Code_ADC[i]=Resistor[Sens_tipe][i];
    if (Code>=Code_ADC[i])
     {
      kr=i;
      break;
     }
   }
  k=(float)(Temper[kr]-Temper[kr-1])/(Code_ADC[kr]-Code_ADC[kr-1]);
  m=(float)((float)Temper[kr]-k*(float)Code_ADC[kr]);
  if(((k*(float)Code+m)-((int8_t)(k*(float)Code+m)))>=0.5){
    Res=(int8_t)(k*(float)Code+m)+1;
  } else {
    Res=(int8_t)(k*(float)Code+m);
  }



  if(Res<=0)
   {

     Res=0;
   }
  if(Res>70)
   {

     Res=70;
   }

  return Res;
 }

//==========================================================================================
//================================================================================ Reset LCD
//==========================================================================================
/*void reset_LCD() // PIN RESET ON LCD
{
	RESET_OFF;
	vTaskDelay(3000 / portTICK_PERIOD_MS);
	RESET_ON;
}*/

//==========================================================================================
//=============================================================================== HEX to BCD
//==========================================================================================


//==========================================================================================
//=============================================================================== Beeper set
//==========================================================================================
void Sound (uint8_t time_ms)
{
	if(!no_sound)
	{
		ledc_timer_resume(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0);
		vTaskDelay(time_ms / portTICK_PERIOD_MS);
		ledc_timer_pause(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0);
	}
}

//==========================================================================================
//================================================================================= SPI init
//==========================================================================================
void SPI_Init()
{
	spi_bus_config_t bus_config;
	bus_config.sclk_io_num = 23;   // CLK
	bus_config.mosi_io_num = 22;   // MOSI
	bus_config.miso_io_num = -1;   // Not used
	bus_config.quadwp_io_num = -1; // Not used
	bus_config.quadhd_io_num = -1; // Not used
	bus_config.max_transfer_sz = 0;
	bus_config.flags = 1;
	printf("... Initializing bus.\n");
	ESP_ERROR_CHECK(spi_bus_initialize(VSPI_HOST, &bus_config, 0));

	spi_bus_config_t bus_config1;
	bus_config1.sclk_io_num = 16;   // CLK
	bus_config1.mosi_io_num = 4;   // MOSI
	bus_config1.miso_io_num = -1;   // Not used
	bus_config1.quadwp_io_num = -1; // Not used
	bus_config1.quadhd_io_num = -1; // Not used
	bus_config1.max_transfer_sz = 0;
	bus_config1.flags = 1;
	printf("... Initializing bus.\n");
	ESP_ERROR_CHECK(spi_bus_initialize(HSPI_HOST, &bus_config1, 0));

	//-------Transaction stucture----------
	trans_desc.addr = 0;
	trans_desc.cmd = 0;
	trans_desc.flags = 0;
	trans_desc.rxlength = 0;

	trans_desc1.addr = 0;
	trans_desc1.cmd = 0;
	trans_desc1.flags = 0;
	trans_desc1.rxlength = 0;

	//-------Device config-----------------
	dev_config.address_bits = 0;
	dev_config.command_bits = 0;
	dev_config.dummy_bits = 0;
	dev_config.mode = 0;
	dev_config.duty_cycle_pos = 0;
	dev_config.cs_ena_posttrans = 0;
	dev_config.cs_ena_pretrans = 0;
	dev_config.clock_speed_hz = 10000000;
	dev_config.spics_io_num = -1;
	dev_config.flags = SPI_DEVICE_HALFDUPLEX;
	dev_config.queue_size = 1;
	dev_config.pre_cb = NULL;
	dev_config.post_cb = NULL;
	printf("... Adding device bus.\n");
	ESP_ERROR_CHECK(spi_bus_add_device(HSPI_HOST, &dev_config, &handle));

	dev_config1.address_bits = 0;
	dev_config1.command_bits = 0;
	dev_config1.dummy_bits = 0;
	dev_config1.mode = 0;
	dev_config1.duty_cycle_pos = 0;
	dev_config1.cs_ena_posttrans = 0;
	dev_config1.cs_ena_pretrans = 0;
	dev_config1.clock_speed_hz = 10000000;
	dev_config1.spics_io_num = -1;
	dev_config1.flags = SPI_DEVICE_HALFDUPLEX;
	dev_config1.queue_size = 1;
	dev_config1.pre_cb = NULL;
	dev_config1.post_cb = NULL;
	printf("... Adding device bus.\n");
	ESP_ERROR_CHECK(spi_bus_add_device(VSPI_HOST, &dev_config1, &handle1));


}

//==========================================================================================
//================================================================================ GPIO init
//==========================================================================================
void GPIO_Conf()
{

	gpio_config_t gpioConfig1;
	gpioConfig1.pin_bit_mask = (1 << 26);
	gpioConfig1.mode = GPIO_MODE_OUTPUT;
	gpioConfig1.pull_up_en = GPIO_PULLUP_ENABLE;
	gpioConfig1.pull_down_en = GPIO_PULLDOWN_DISABLE;
	gpioConfig1.intr_type = GPIO_INTR_DISABLE;
	gpio_config(&gpioConfig1);

	gpio_config_t gpioConfig;
	gpioConfig.pin_bit_mask = (1 << 21) | (1 << 17) | (1 << 14);
	gpioConfig.mode = GPIO_MODE_OUTPUT;
	gpioConfig.pull_up_en = GPIO_PULLUP_DISABLE;
	gpioConfig.pull_down_en = GPIO_PULLDOWN_ENABLE;
	gpioConfig.intr_type = GPIO_INTR_DISABLE;
	gpio_config(&gpioConfig);

}



void LedButtonDisplay()
{
	switch(light_mode)
	{
		case 1:
			switch(WINDOW)
			{
				case MAIN_WIN:

					if(WIFI)
					{
						if(Relay_flag){
							if(BT_HEART_PAR.flag_click) 	 send_led(0xD812);
							else if(BT_MINUS_PAR.flag_click) send_led(0xC312);
							else if(BT_PLUS_PAR.flag_click)  send_led(0x1B12);
							else if(BT_MODE_PAR.flag_click)  send_led(0xDB10);
							else send_led(0xDB12);
						}
						else
						{
							if(BT_HEART_PAR.flag_click) 	 send_led(0xD842);
							else if(BT_MINUS_PAR.flag_click) send_led(0xC342);
							else if(BT_PLUS_PAR.flag_click)  send_led(0x1B42);
							else if(BT_MODE_PAR.flag_click)  send_led(0xDB40);
							else send_led(0xDB42);
						}
					}
					else
					{
						if(Relay_flag){
							if(BT_HEART_PAR.flag_click) 	 send_led(0xD816);
							else if(BT_MINUS_PAR.flag_click) send_led(0xC316);
							else if(BT_PLUS_PAR.flag_click)  send_led(0x1B16);
							else if(BT_MODE_PAR.flag_click)  send_led(0xDB10);
							else send_led(0xDB16);
						}
						else
						{
							if(BT_HEART_PAR.flag_click) 	 send_led(0xD846);
							else if(BT_MINUS_PAR.flag_click) send_led(0xC346);
							else if(BT_PLUS_PAR.flag_click)  send_led(0x1B46);
							else if(BT_MODE_PAR.flag_click)  send_led(0xDB40);
							else send_led(0xDB46);
						}

					}


					break;

				case STANDBY_WIN:

					if(WIFI)
					{
						if(Relay_flag)
							{
								if(BT_MODE_PAR.flag_click)  send_led(0xFF01);
								else send_led(0xFF09);
							}
						else
							{
								if(BT_MODE_PAR.flag_click)  send_led(0xFF01);
								else send_led(0xFF03);
							}
					}
					else
					{
						if(Relay_flag)
							{
								if(BT_MODE_PAR.flag_click)  send_led(0xFF01);
								else send_led(0xFF0D);
							}
						else
							{
								if(BT_MODE_PAR.flag_click)  send_led(0xFF01);
								else send_led(0xFF07);
							}
					}


				break;


			}

			break;


		case 2:
			switch(WINDOW)
				{
					case MAIN_WIN:

						if(WIFI)
						{
							if(Relay_flag){
								if(BT_HEART_PAR.flag_click) 	 send_led(0xD812);
								else if(BT_MINUS_PAR.flag_click) send_led(0xC312);
								else if(BT_PLUS_PAR.flag_click)  send_led(0x1B12);
								else if(BT_MODE_PAR.flag_click)  send_led(0xDB10);
								else send_led(0xDB12);
							}
							else
							{
								if(BT_HEART_PAR.flag_click) 	 send_led(0xD842);
								else if(BT_MINUS_PAR.flag_click) send_led(0xC342);
								else if(BT_PLUS_PAR.flag_click)  send_led(0x1B42);
								else if(BT_MODE_PAR.flag_click)  send_led(0xDB40);
								else send_led(0xDB42);
							}
						}
						else
						{
							if(Relay_flag){
								if(BT_HEART_PAR.flag_click) 	 send_led(0xD816);
								else if(BT_MINUS_PAR.flag_click) send_led(0xC316);
								else if(BT_PLUS_PAR.flag_click)  send_led(0x1B16);
								else if(BT_MODE_PAR.flag_click)  send_led(0xDB10);
								else send_led(0xDB16);
							}
							else
							{
								if(BT_HEART_PAR.flag_click) 	 send_led(0xD846);
								else if(BT_MINUS_PAR.flag_click) send_led(0xC346);
								else if(BT_PLUS_PAR.flag_click)  send_led(0x1B46);
								else if(BT_MODE_PAR.flag_click)  send_led(0xDB40);
								else send_led(0xDB46);
							}

						}


						break;

					case STANDBY_WIN:

						if(WIFI)
						{
							if(Relay_flag)
								{
									if(BT_MODE_PAR.flag_click)  send_led(0xFF31);
									else 						send_led(0xFF3F);
								}
							else
								{
									if(BT_MODE_PAR.flag_click)  send_led(0xFF61);
									else 						send_led(0xFF6F);
								}
						}
						else
						{
							if(Relay_flag)
								{
									if(BT_MODE_PAR.flag_click)  send_led(0xFF11);
									else 						send_led(0xFF1F);
								}
							else
								{
									if(BT_MODE_PAR.flag_click)  send_led(0xFF41);
									else 						send_led(0xFF4F);
								}
						}


					break;


				}

				break;



		case 3:
			switch(WINDOW)
				{
					case MAIN_WIN:

						if(WIFI)
						{
							if(Relay_flag){
								if(BT_HEART_PAR.flag_click) 	 send_led(0xD812);
								else if(BT_MINUS_PAR.flag_click) send_led(0xC312);
								else if(BT_PLUS_PAR.flag_click)  send_led(0x1B12);
								else if(BT_MODE_PAR.flag_click)  send_led(0xDB10);
								else send_led(0xDB12);
							}
							else
							{
								if(BT_HEART_PAR.flag_click) 	 send_led(0xD842);
								else if(BT_MINUS_PAR.flag_click) send_led(0xC342);
								else if(BT_PLUS_PAR.flag_click)  send_led(0x1B42);
								else if(BT_MODE_PAR.flag_click)  send_led(0xDB40);
								else send_led(0xDB42);
							}
						}
						else
						{
							if(Relay_flag){
								if(BT_HEART_PAR.flag_click) 	 send_led(0xD816);
								else if(BT_MINUS_PAR.flag_click) send_led(0xC316);
								else if(BT_PLUS_PAR.flag_click)  send_led(0x1B16);
								else if(BT_MODE_PAR.flag_click)  send_led(0xDB10);
								else send_led(0xDB16);
							}
							else
							{
								if(BT_HEART_PAR.flag_click) 	 send_led(0xD846);
								else if(BT_MINUS_PAR.flag_click) send_led(0xC346);
								else if(BT_PLUS_PAR.flag_click)  send_led(0x1B46);
								else if(BT_MODE_PAR.flag_click)  send_led(0xDB40);
								else send_led(0xDB46);
							}

						}


						break;

					case STANDBY_WIN:

						if(WIFI)
						{
							if(Relay_flag)
								{
									if(BT_MODE_PAR.flag_click)  send_led(0xFF11);
									else 						send_led(0xFF19);
								}
							else
								{
									if(BT_MODE_PAR.flag_click)  send_led(0xFF41);
									else 						send_led(0xFF43);
								}
						}
						else
						{
							if(Relay_flag)
								{
									if(BT_MODE_PAR.flag_click)  send_led(0xFF11);
									else 						send_led(0xFF1D);
								}
							else
								{
									if(BT_MODE_PAR.flag_click)  send_led(0xFF41);
									else 						send_led(0xFF47);
								}
						}


					break;


				}

				break;




		case 4:
			switch(WINDOW)
			{
				case MAIN_WIN:

					if(WIFI)
					{
						if(Relay_flag){
							if(BT_HEART_PAR.flag_click) 	 send_led(0xB019);
							else if(BT_MINUS_PAR.flag_click) send_led(0x8619);
							else if(BT_PLUS_PAR.flag_click)  send_led(0x3618);
							else if(BT_MODE_PAR.flag_click)  send_led(0xB611);
							else send_led(0xB619);
						}
						else
						{
							if(BT_HEART_PAR.flag_click) 	 send_led(0xD842);
							else if(BT_MINUS_PAR.flag_click) send_led(0xC342);
							else if(BT_PLUS_PAR.flag_click)  send_led(0x1B42);
							else if(BT_MODE_PAR.flag_click)  send_led(0xDB40);
							else send_led(0xDB42);
						}
					}
					else
					{
						if(Relay_flag){
							if(BT_HEART_PAR.flag_click) 	 send_led(0xB01D);
							else if(BT_MINUS_PAR.flag_click) send_led(0x861D);
							else if(BT_PLUS_PAR.flag_click)  send_led(0x361C);
							else if(BT_MODE_PAR.flag_click)  send_led(0xB611);
							else send_led(0xB61D);
						}
						else
						{
							if(BT_HEART_PAR.flag_click) 	 send_led(0xD846);
							else if(BT_MINUS_PAR.flag_click) send_led(0xC346);
							else if(BT_PLUS_PAR.flag_click)  send_led(0x1B46);
							else if(BT_MODE_PAR.flag_click)  send_led(0xDB40);
							else send_led(0xDB46);
						}

					}


					break;

				case STANDBY_WIN:

					if(WIFI)
					{
						if(Relay_flag)
							{
								if(BT_MODE_PAR.flag_click)  send_led(0xFF01);
								else send_led(0xFF09);
							}
						else
							{
								if(BT_MODE_PAR.flag_click)  send_led(0xFF01);
								else send_led(0xFF03);
							}
					}
					else
					{
						if(Relay_flag)
							{
								if(BT_MODE_PAR.flag_click)  send_led(0xFF01);
								else send_led(0xFF0D);
							}
						else
							{
								if(BT_MODE_PAR.flag_click)  send_led(0xFF01);
								else send_led(0xFF07);
							}
					}


				break;


			}

			break;


		case 5:
			switch(WINDOW)
			{
				case MAIN_WIN:

					if(WIFI)
					{
						if(Relay_flag){
							if(BT_HEART_PAR.flag_click) 	 send_led(0x0012);
							else if(BT_MINUS_PAR.flag_click) send_led(0x0012);
							else if(BT_PLUS_PAR.flag_click)  send_led(0x0012);
							else if(BT_MODE_PAR.flag_click)  send_led(0x0010);
							else 							 send_led(0x0012);
						}
						else
						{
							if(BT_HEART_PAR.flag_click) 	 send_led(0x0042);
							else if(BT_MINUS_PAR.flag_click) send_led(0x0042);
							else if(BT_PLUS_PAR.flag_click)  send_led(0x0042);
							else if(BT_MODE_PAR.flag_click)  send_led(0x0040);
							else 							 send_led(0x0042);
						}
					}
					else
					{
						if(Relay_flag) send_led(0x0010);
						else           send_led(0x0040);
					}


					break;

				case STANDBY_WIN:

					if(WIFI)
					{
						if(Relay_flag)
							{
								if(BT_MODE_PAR.flag_click)  send_led(0xFF01);
								else send_led(0xFF09);
							}
						else
							{
								if(BT_MODE_PAR.flag_click)  send_led(0xFF01);
								else send_led(0xFF03);
							}
					}
					else
					{
						if(Relay_flag)
							{
								if(BT_MODE_PAR.flag_click)  send_led(0xFF01);
								else send_led(0xFF0D);
							}
						else
							{
								if(BT_MODE_PAR.flag_click)  send_led(0xFF01);
								else send_led(0xFF07);
							}
					}
				break;
			}
			break;
	}
}




void LedIndicatorDisplay(uint8_t data)
{
 switch(data%10)
 {
  case 1:
	  send_seg(0xF9);
   break;
  case 2:
	  send_seg(0xA4);
   break;
  case 3:
	  send_seg(0xB0);
   break;
  case 4:
	  send_seg(0x99);
   break;
  case 5:
	  send_seg(0x92);
   break;
  case 6:
	  send_seg(0x82);
   break;
  case 7:
	  send_seg(0xF8);
   break;
  case 8:
	  send_seg(0x80);
   break;
  case 9:
	  send_seg(0x90);
   break;
  case 0:
	  send_seg(0xC0); // âûâîä 0
   break;
  case 'E':
	  send_seg(0x86); // âûâîä E
   break;
  case 'L':
	  send_seg(0xC7); // âûâîä L
   break;
  case 'O':
	  send_seg(0xC0); // âûâîä O
   break;
  case 'H':
	  send_seg(0x89); // âûâîä H
   break;
  case 'I':
	  send_seg(0xF9); // âûâîä I
   break;
 }

 switch(data/10)
  {
   case 1:
 	  send_seg(0xF9);
    break;
   case 2:
 	  send_seg(0xA4);
    break;
   case 3:
 	  send_seg(0xB0);
    break;
   case 4:
 	  send_seg(0x99);
    break;
   case 5:
 	  send_seg(0x92);
    break;
   case 6:
 	  send_seg(0x82);
    break;
   case 7:
 	  send_seg(0xF8);
    break;
   case 8:
 	  send_seg(0x80);
    break;
   case 9:
 	  send_seg(0x90);
    break;
   case 0:
 	  send_seg(0xC0); // âûâîä 0
    break;
   case 'E':
 	  send_seg(0x86); // âûâîä E
    break;
   case 'L':
 	  send_seg(0xC7); // âûâîä L
    break;
   case 'O':
 	  send_seg(0xC0); // âûâîä O
    break;
   case 'H':
 	  send_seg(0x89); // âûâîä H
    break;
   case 'I':
 	  send_seg(0xF9); // âûâîä I
    break;
  }
	gpio_set_level(21, 1);
	gpio_set_level(21, 0);
}



void Menu()
{
	switch (WINDOW)
	{
		case MAIN_WIN:

			//---------------------------------------------------------------------------------------------------------------------------------------------------------FACTORY RESET
			if((!FlagWiFiMode)&&(BT_HEART_PAR.type_click>=SHORT_CLICK)&&(BT_HEART_PAR.flag_click)&&(BT_MINUS_PAR.type_click>=SHORT_CLICK)&&(BT_MINUS_PAR.flag_click)){

			        FlagWiFiMode=1;

			 }

			if((FlagWiFiMode) && (BT_HEART_PAR.type_click>=LONG_CLICK)&&(BT_HEART_PAR.flag_click)&&(BT_MINUS_PAR.type_click>=LONG_CLICK)&&(BT_MINUS_PAR.flag_click)){
					BT_HEART_PAR.type_click=NO_CLICK;
					BT_HEART_PAR.counter=0;
					BT_HEART_PAR.flag_click=0;
			        BT_MINUS_PAR.type_click=NO_CLICK;
			        BT_MINUS_PAR.counter=0;
			        BT_MINUS_PAR.flag_click=0;
			        FlagWiFiMode = 0;
			        Sound(20);

					conditions_register = (conditions_register & (~LIGHT));
					led_Counter = 0;
					first_start = 1;
					nvs_open("storage", NVS_READWRITE, &my_handle);
					nvs_set_u8(my_handle, "first_start", first_start);
					nvs_commit(my_handle);
					nvs_close(my_handle);
					esp_restart();

			}

			//---------------------------------------------------------------------------------------------------------------------------------------------------------STA MODE
			if((!FlagWiFiMode)&&(!sta_ap_flag)&&(BT_PLUS_PAR.type_click==SHORT_CLICK)&&(BT_PLUS_PAR.flag_click)&&(BT_MODE_PAR.type_click==SHORT_CLICK)&&(BT_MODE_PAR.flag_click)){

					FlagWiFiMode=1;
			 }


			if((FlagWiFiMode) && (BT_PLUS_PAR.type_click>=LONG_CLICK)&&(BT_PLUS_PAR.flag_click)&&(BT_MODE_PAR.type_click>=LONG_CLICK)&&(BT_MODE_PAR.flag_click)){

					BT_PLUS_PAR.type_click=NO_CLICK;
					BT_PLUS_PAR.counter=0;
					BT_PLUS_PAR.flag_click=0;
					BT_MODE_PAR.type_click=NO_CLICK;
					BT_MODE_PAR.counter=0;
					BT_MODE_PAR.flag_click=0;

					conditions_register = (conditions_register & (~LIGHT));
					conditions_register=(conditions_register|SC_AP);
					Sound(20);
					led_Counter = 0;
					uint16_t seg[1] = { 0x92C6 };
					trans_desc.length = 16;
					trans_desc.tx_buffer = seg;
					ESP_ERROR_CHECK(spi_device_transmit(handle1, &trans_desc));
					show_seg();
					FlagWiFiMode = 0;
					set_wifi_sta();
					wifi_conn_Counter = 10000;

			}
			//--------------------------------------------------------------------------------------------------------------------------------------------------------- AP MODE

			if((!FlagWiFiMode)&&(BT_MINUS_PAR.type_click>=SHORT_CLICK)&&(BT_MINUS_PAR.flag_click)&&(BT_MODE_PAR.type_click>=SHORT_CLICK)&&(BT_MODE_PAR.flag_click)){

			        	FlagWiFiMode=1;
			 }

			if((BT_MINUS_PAR.type_click>=LONG_CLICK)&&(BT_MINUS_PAR.flag_click)&&(BT_MODE_PAR.type_click>=LONG_CLICK)&&(BT_MODE_PAR.flag_click)&&(FlagWiFiMode)){
					BT_MINUS_PAR.type_click=NO_CLICK;
					BT_MINUS_PAR.counter=0;
					BT_MINUS_PAR.flag_click=0;
			        BT_MODE_PAR.type_click=NO_CLICK;
			        BT_MODE_PAR.counter=0;
			        BT_MODE_PAR.flag_click=0;

			        FlagWiFiMode=0;
			        conditions_register = (conditions_register & (~LIGHT));
			        led_Counter = 0;
			        conditions_register=(conditions_register|SC_AP);
			        Sound(20);
					uint16_t seg[1] = { 0x888C };
					trans_desc.length = 16;
					trans_desc.tx_buffer = seg;
					ESP_ERROR_CHECK(spi_device_transmit(handle1, &trans_desc));
					show_seg();
			        set_wifi_ap();
			        wifi_conn_Counter = 10000;
			        //led_Counter=0;

			}

			//=============================================================================================================================================================================
			if(!Sensor_err){ //if err, block "show temperature"
				if((!FlagWiFiMode)&&(BT_PLUS_PAR.type_click>=SHORT_CLICK)&&(BT_PLUS_PAR.flag_click)&&(BT_MINUS_PAR.type_click>=SHORT_CLICK)&&(BT_MINUS_PAR.flag_click)){

						FlagWiFiMode=1;
						Sound(20);
						conditions_register=(conditions_register|LIGHT);
						led_Counter = 0;
						if(delta_air_temperature>5)
							 LedIndicatorDisplay((Themperature+(delta_air_temperature-5)));
						else
							LedIndicatorDisplay((Themperature-(5-delta_air_temperature)));

				 }

				if((FlagWiFiMode) && (BT_PLUS_PAR.type_click>=SHORT_CLICK)&&(!BT_PLUS_PAR.flag_click)&&(BT_MINUS_PAR.type_click>=SHORT_CLICK)&&(!BT_MINUS_PAR.flag_click)){


						BT_PLUS_PAR.type_click=NO_CLICK;
						BT_PLUS_PAR.counter=0;
						BT_PLUS_PAR.flag_click=0;
						BT_MINUS_PAR.type_click=NO_CLICK;
						BT_MINUS_PAR.counter=0;
						BT_MINUS_PAR.flag_click=0;
						FlagWiFiMode = 0;
						display_dig();

				}
			}
			//=============================================================================================================================================================================

			if(!Sensor_err)
			{
				if((!FlagWiFiMode)&&(BT_HEART_PAR.type_click==SHORT_CLICK)&&(!BT_HEART_PAR.flag_click)){
					BT_HEART_PAR.type_click=NO_CLICK;
					BT_HEART_PAR.counter=0;
					BT_HEART_PAR.flag_click=0;

					nvs_open("storage", NVS_READWRITE, &my_handle);
					nvs_get_u8(my_handle, "love_temp", &love_temperature);

					if(Heat_mode == 0)
					{
						set_floor_temperature = love_temperature;
						nvs_set_u8(my_handle, "set_floor_temp", set_floor_temperature);
						LedIndicatorDisplay(set_floor_temperature);
					}
					else
					{
						set_floor_temperature_prog = love_temperature;
						LedIndicatorDisplay(set_floor_temperature_prog);
					}

					nvs_commit(my_handle);
					nvs_close(my_handle);

					conditions_register=(conditions_register|LIGHT);
					led_Counter = 0;
					Sound(20);
				}

				if((!FlagWiFiMode)&&(BT_HEART_PAR.type_click==LONG_CLICK)&&(BT_HEART_PAR.flag_click)){
					BT_HEART_PAR.type_click=NO_CLICK;
					BT_HEART_PAR.counter=0;
					BT_HEART_PAR.flag_click=0;
					Sound(20);
					love_temperature = set_floor_temperature;

					if(Heat_mode == 0)
					{
						love_temperature = set_floor_temperature;
					}
					else
					{
						love_temperature = set_floor_temperature_prog;
					}
					nvs_open("storage", NVS_READWRITE, &my_handle);
					nvs_set_u8(my_handle, "love_temp", love_temperature);
					nvs_commit(my_handle);
					nvs_close(my_handle);
					count_register=(count_register|HEART_SET);
					conditions_register=(conditions_register|LIGHT);
					led_Counter = 0;
					vTaskDelay(500 / portTICK_PERIOD_MS);
				}

				if((!FlagWiFiMode)&&(BT_PLUS_PAR.type_click==SHORT_CLICK)&&(!BT_PLUS_PAR.flag_click)){
					BT_PLUS_PAR.type_click=NO_CLICK;
					BT_PLUS_PAR.counter=0;
					BT_PLUS_PAR.flag_click=0;

					if((set_floor_temperature < 45) && (Heat_mode == 0))
					{
						Sound(20);
						set_floor_temperature++;
						LedIndicatorDisplay(set_floor_temperature);

						conditions_register = (conditions_register | LIGHT);
						led_Counter = 0;

						nvs_open("storage", NVS_READWRITE, &my_handle);
						nvs_set_u8(my_handle, "set_floor_temp", set_floor_temperature);
						nvs_commit(my_handle);
						nvs_close(my_handle);
					}
					else if((set_floor_temperature_prog < 45) && (Heat_mode == 1))
					{
						Sound(20);
						set_floor_temperature_prog++;
						LedIndicatorDisplay(set_floor_temperature_prog);

						conditions_register = (conditions_register | LIGHT);
						led_Counter = 0;
					}
				}


				if((!FlagWiFiMode)&&(BT_MINUS_PAR.type_click==SHORT_CLICK)&&(!BT_MINUS_PAR.flag_click)){
					BT_MINUS_PAR.type_click=NO_CLICK;
					BT_MINUS_PAR.counter=0;
					BT_MINUS_PAR.flag_click=0;

					if((set_floor_temperature > 5) && (Heat_mode == 0))
					{
						Sound(20);
						set_floor_temperature--;
						LedIndicatorDisplay(set_floor_temperature);

						conditions_register = (conditions_register | LIGHT);
						led_Counter = 0;

						nvs_open("storage", NVS_READWRITE, &my_handle);
						nvs_set_u8(my_handle, "set_floor_temp", set_floor_temperature);
						nvs_commit(my_handle);
						nvs_close(my_handle);
					}
					else if((set_floor_temperature_prog > 5) && (Heat_mode == 1))
					{
						Sound(20);
						set_floor_temperature_prog--;
						LedIndicatorDisplay(set_floor_temperature_prog);

						conditions_register = (conditions_register | LIGHT);
						led_Counter = 0;
					}

				}


				if((!FlagWiFiMode)&&(BT_PLUS_PAR.type_click==DSHORT_CLICK)&&(BT_PLUS_PAR.flag_click)){
					BT_PLUS_PAR.type_click=NO_CLICK;
					BT_PLUS_PAR.counter=30;             //Counter from 80 for fast changing temp
					if((set_floor_temperature < 45) && (Heat_mode == 0))
					{
						Sound(20);
						set_floor_temperature++;
						LedIndicatorDisplay(set_floor_temperature);

						conditions_register = (conditions_register | LIGHT);
						led_Counter = 0;

						nvs_open("storage", NVS_READWRITE, &my_handle);
						nvs_set_u8(my_handle, "set_floor_temp", set_floor_temperature);
						nvs_commit(my_handle);
						nvs_close(my_handle);
					}
					else if((set_floor_temperature_prog < 45) && (Heat_mode == 1))
					{
						Sound(20);
						set_floor_temperature_prog++;
						LedIndicatorDisplay(set_floor_temperature_prog);

						conditions_register = (conditions_register | LIGHT);
						led_Counter = 0;
					}
				}



				if((!FlagWiFiMode)&&(BT_MINUS_PAR.type_click==DSHORT_CLICK)&&(BT_MINUS_PAR.flag_click)){
					BT_MINUS_PAR.type_click=NO_CLICK;
					BT_MINUS_PAR.counter=30;             //Counter from 80 for fast changing temp
					if((set_floor_temperature > 5) && (Heat_mode == 0))
					{
						Sound(20);
						set_floor_temperature--;
						LedIndicatorDisplay(set_floor_temperature);

						conditions_register = (conditions_register | LIGHT);
						led_Counter = 0;

						nvs_open("storage", NVS_READWRITE, &my_handle);
						nvs_set_u8(my_handle, "set_floor_temp", set_floor_temperature);
						nvs_commit(my_handle);
						nvs_close(my_handle);
					}
					else if((set_floor_temperature_prog > 5) && (Heat_mode == 1))
					{
						Sound(20);
						set_floor_temperature_prog--;
						LedIndicatorDisplay(set_floor_temperature_prog);

						conditions_register = (conditions_register | LIGHT);
						led_Counter = 0;
					}
				}
			}

			//=============================================================================================================================================================================
			if((!FlagWiFiMode)&&(BT_HEART_PAR.type_click>=SHORT_CLICK)&&(BT_HEART_PAR.flag_click)&&(BT_MODE_PAR.type_click>=SHORT_CLICK)&&(BT_MODE_PAR.flag_click)){

			        FlagWiFiMode=1;
					conditions_register=(conditions_register|LIGHT);
					led_Counter = 0;
			        light_mode++;
			        if(light_mode>=6) light_mode=1;
		    		nvs_open("storage", NVS_READWRITE, &my_handle);
		    		nvs_set_u8(my_handle, "light_mode", light_mode);
		    		nvs_commit(my_handle);
		    		nvs_close(my_handle);

			        LedIndicatorDisplay(light_mode);
			        Sound(20);
			 }

			if((FlagWiFiMode) && (BT_HEART_PAR.type_click>=SHORT_CLICK)&&(!BT_HEART_PAR.flag_click)&&(BT_MODE_PAR.type_click>=SHORT_CLICK)&&(!BT_MODE_PAR.flag_click)){

			        BT_HEART_PAR.type_click=NO_CLICK;
			        BT_HEART_PAR.counter=0;
			        BT_HEART_PAR.flag_click=0;
			        BT_MODE_PAR.type_click=NO_CLICK;
			        BT_MODE_PAR.counter=0;
			        BT_MODE_PAR.flag_click=0;
			        FlagWiFiMode = 0;

			        display_dig();

			}
			//=============================================================================================================================================================================


			if((!FlagBlock)&&(BT_MODE_PAR.counter==SHORT_CLICK)){
				//Sound(20);
			}

			if((!FlagBlock)&&(!FlagWiFiMode)&&(BT_MODE_PAR.type_click==DSHORT_CLICK)&&(BT_MODE_PAR.flag_click)){
				if((!BT_MINUS_PAR.flag_click)&&(!BT_HEART_PAR.flag_click)&&(!BT_PLUS_PAR.flag_click)){

				BT_MODE_PAR.type_click=NO_CLICK;
				BT_MODE_PAR.counter=0;
				Sound(20);
				count_register=(count_register|BLOCK_COUNT);
				FlagBlock = 1;
				conditions_register=(conditions_register|LIGHT);
				led_Counter = 0;
				}
				else
				{
					count_register = (count_register & (~BLOCK_COUNT));
					block_counter = 0;
				}
			}
			else if(FlagWiFiMode)
			{
				count_register = (count_register & (~BLOCK_COUNT));
				block_counter = 0;
			}


			if((!FlagWiFiMode)&&(FlagBlock)&&(!BT_MODE_PAR.flag_click)){
				BT_MODE_PAR.type_click=NO_CLICK;
				BT_MODE_PAR.counter=0;
				//Sound(20);
				count_register = (count_register & (~BLOCK_COUNT));
				FlagBlock = 0;
				if((block_counter<300))
				{
					if(!Sensor_err){
					uint16_t seg[1] = { 0xFFFF };
					trans_desc.length = 16;
					trans_desc.tx_buffer = seg;
					ESP_ERROR_CHECK(spi_device_transmit(handle1, &trans_desc));
					show_seg();}
					else
					{
						if ((adc_floor >= 3500))
						{
							uint16_t seg[1] = { 0x86F9 };
							trans_desc.length = 16;
							trans_desc.tx_buffer = seg;
							ESP_ERROR_CHECK(spi_device_transmit(handle1, &trans_desc));
							show_seg();
						}
						else if((adc_floor <= 20))
						{
							uint16_t seg[1] = { 0x86A4 };
							trans_desc.length = 16;
							trans_desc.tx_buffer = seg;
							ESP_ERROR_CHECK(spi_device_transmit(handle1, &trans_desc));
							show_seg();
						}
					}
					WINDOW=STANDBY_WIN;
		    		nvs_open("storage", NVS_READWRITE, &my_handle);
		    		nvs_set_u8(my_handle, "WINDOW", WINDOW);
		    		nvs_commit(my_handle);
		    		nvs_close(my_handle);
					count_register = (count_register & (~BLOCK_COUNT));
					led_Counter = 0;
					block_counter = 0;
				}
				else
				{
					display_dig();
					block_counter = 0;
				}

			}

			break;

		case STANDBY_WIN:

			if((BT_MODE_PAR.counter==SHORT_CLICK)){
				Sound(20);
			}

			if(!Sensor_err){
				if((BT_MODE_PAR.type_click>=SHORT_CLICK)&&(!BT_MODE_PAR.flag_click)){
					BT_MODE_PAR.type_click=NO_CLICK;
					BT_MODE_PAR.flag_click=0;
					BT_MODE_PAR.counter=0;
					if(delta_air_temperature>5)
						 LedIndicatorDisplay((Themperature+(delta_air_temperature-5)));
					else
						LedIndicatorDisplay((Themperature-(5-delta_air_temperature)));

					vTaskDelay(2000 / portTICK_PERIOD_MS);

					send_seg(255);
					send_seg(255);
					show_seg();
					conditions_register=(conditions_register|LIGHT);
					led_Counter = 0;
				}
			}

			if((BT_MODE_PAR.type_click==LONG_CLICK)&&(BT_MODE_PAR.flag_click)){
				BT_MODE_PAR.type_click=NO_CLICK;
				//BT_MODE_PAR.flag_click=0;
				//BT_MODE_PAR.counter=0;
				Sound(20);
				nvs_open("storage", NVS_READWRITE, &my_handle);
				if(!Sensor_err)
				{
					display_dig();
					POWER_ON_OFF = 1;
					WINDOW=MAIN_WIN;
					nvs_set_u8(my_handle, "POWER_ON_OFF", POWER_ON_OFF);
					nvs_set_u8(my_handle, "WINDOW", WINDOW);
					conditions_register=(conditions_register|LIGHT);
					led_Counter = 0;
				}
				else
				{
					led_Counter = 0;
					conditions_register = (conditions_register & (~LIGHT));
					POWER_ON_OFF = 0;
					WINDOW=POWER_OFF_WIN;
					nvs_set_u8(my_handle, "POWER_ON_OFF", POWER_ON_OFF);
					nvs_set_u8(my_handle, "WINDOW", WINDOW);
					Relay_flag = 0;
					RELAY_OFF;
					OFF_ALL;

					Time_relay_ON += temp_Time_relay_ON;
		    		nvs_set_u32(my_handle, "Time_relay", Time_relay_ON);
		    		temp_Time_relay_ON = 0;

				}
	    		nvs_commit(my_handle);
	    		nvs_close(my_handle);
			}

			if((!FlagWiFiMode)&&(BT_HEART_PAR.type_click==LONG_CLICK)&&(BT_HEART_PAR.flag_click)){
				BT_HEART_PAR.type_click=NO_CLICK;
				BT_HEART_PAR.counter=0;
				BT_HEART_PAR.flag_click=0;

				ledc_timer_resume(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0);
				vTaskDelay(100 / portTICK_PERIOD_MS);
				ledc_timer_pause(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0);

				no_sound = !no_sound;
				nvs_open("storage", NVS_READWRITE, &my_handle);
				nvs_set_u8(my_handle, "no_sound", no_sound);
	    		nvs_commit(my_handle);
	    		nvs_close(my_handle);
			}

			break;



		case POWER_OFF_WIN:

			if((BT_MODE_PAR.type_click==LONG_CLICK)&&(BT_MODE_PAR.flag_click)){
				BT_MODE_PAR.type_click=NO_CLICK;
				//BT_MODE_PAR.counter=0;
				Sound(20);
				display_dig();
				conditions_register=(conditions_register|LIGHT);
				led_Counter = 0;
				ON_ALL;
				POWER_ON_OFF = 1;
				WINDOW=MAIN_WIN;
				nvs_open("storage", NVS_READWRITE, &my_handle);
				nvs_set_u8(my_handle, "POWER_ON_OFF", POWER_ON_OFF);
	    		nvs_set_u8(my_handle, "WINDOW", WINDOW);
	    		nvs_commit(my_handle);
	    		nvs_close(my_handle);
				ON_ALL;
			}
			if((BT_MODE_PAR.type_click<=DLONG_CLICK)&&(!BT_MODE_PAR.flag_click)){

				BT_MODE_PAR.type_click=NO_CLICK;
			}

			break;
	}
	//---------------------------------------------------------------------------------------------------------- RESET BUTTONS
	if(!FlagWiFiMode){
		if((BT_MODE_PAR.type_click<=DLONG_CLICK)&&(!BT_MODE_PAR.flag_click)){

			BT_MODE_PAR.type_click=NO_CLICK;
		}
		if((BT_MINUS_PAR.type_click<=DLONG_CLICK)&&(!BT_MINUS_PAR.flag_click)){

			BT_MINUS_PAR.type_click=NO_CLICK;
		}
		if((BT_PLUS_PAR.type_click<=DLONG_CLICK)&&(!BT_PLUS_PAR.flag_click)){

			BT_PLUS_PAR.type_click=NO_CLICK;
		}
		if((BT_HEART_PAR.type_click<=DLONG_CLICK)&&(!BT_HEART_PAR.flag_click)){

			BT_HEART_PAR.type_click=NO_CLICK;
		}
	}
	else if(FlagWiFiMode&&(!BT_MODE_PAR.flag_click)&&(!BT_MINUS_PAR.flag_click)&&(!BT_PLUS_PAR.flag_click)) {
		if((BT_MODE_PAR.type_click<=DLONG_CLICK)&&(!BT_MODE_PAR.flag_click)){

			BT_MODE_PAR.type_click=NO_CLICK;
		}
		if((BT_MINUS_PAR.type_click<=DLONG_CLICK)&&(!BT_MINUS_PAR.flag_click)){

			BT_MINUS_PAR.type_click=NO_CLICK;
		}
		if((BT_PLUS_PAR.type_click<=DLONG_CLICK)&&(!BT_PLUS_PAR.flag_click)){

			BT_PLUS_PAR.type_click=NO_CLICK;
		}
		if((BT_HEART_PAR.type_click<=DLONG_CLICK)&&(!BT_HEART_PAR.flag_click)){

			BT_HEART_PAR.type_click=NO_CLICK;
		}

		FlagWiFiMode = 0;
	}
	//----------------------------------------------------------------------------------------------------------

}


