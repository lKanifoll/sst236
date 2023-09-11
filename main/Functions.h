/*
 * Functions.h
 *
 *  Created on: Dec 3, 2017
 *      Author: sergey
 */

#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_
//#define DEBUG
#include <errno.h>
#include "esp_types.h"
#include "okeTimer.h"
#include "constant.h"
#include "driver/gpio.h"
#include <driver/spi_master.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/ledc.h>
#include "driver/adc.h"
#include <driver/i2c.h>
#include <driver/adc.h>
#include "esp_adc_cal.h"
#include "soc/sens_reg.h"
#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include <freertos/queue.h>
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_ota_ops.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "ota.h"
#include <sys/socket.h>
#include <netdb.h>
#include <newlib.h>
#include "freertos/event_groups.h"
#include "esp_smartconfig.h"
#include "smartconfig.h"
#include "ESPMQTT.h"
#include "TCP_UDP.h"
#include "rtc.h"
#include "lwip/api.h"
#include "lwip/err.h"
#include "lwip/netdb.h"
#include "lwip/netbuf.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "touch.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "rom/ets_sys.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"
#include "driver/touch_pad.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/timer_group_struct.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"
//#include "esp_pm.h"
//#include "queue.h"

enum {STANDBY=0, MAIN_AIR, MAIN_FLOOR, CORR_AIR, ATT_MESG, PWM};

spi_transaction_t trans_desc;
spi_transaction_t trans_desc1;
spi_device_handle_t handle;
spi_device_handle_t handle1;
spi_device_interface_config_t dev_config;
spi_device_interface_config_t dev_config1;

uint8_t button_fl[6];
uint8_t button_en[6];
uint8_t button_count[6];
uint8_t minus_count;
uint8_t mode_count;
uint8_t heart_count;
//uint16_t counter_calib[6];
uint16_t counter_calib;
esp_interface_t interface;
esp_interface_t curr_interface;
esp_adc_cal_characteristics_t *adc_chars;
i2c_config_t conf;

wifi_ap_record_t ap_info;
xTaskHandle heat_task_handle;
xTaskHandle read_temperature_handle;
xTaskHandle RTC_task_handle;
xTaskHandle Counter_handle;
xTaskHandle Sending_msg_handle;
xTaskHandle Button_handle;
nvs_handle my_handle;
//------------------------------------BUTTON STRUCT-----------------------------------

typedef enum {NO_CLICK=0, SHORT_CLICK, DSHORT_CLICK, LONG_CLICK, DLONG_CLICK} TypeClick;

typedef struct{
 uint8_t flag_click;
 uint16_t counter;
 TypeClick type_click;
 uint16_t clock_short_click;
 uint16_t clock_dshort_click;
 uint16_t clock_long_click;
 uint16_t clock_dlong_click;
} Key_Param;

Key_Param BT_PLUS_PAR;
Key_Param BT_MINUS_PAR;
Key_Param BT_MODE_PAR;
Key_Param BT_HEART_PAR;

//------------------------------------VARIABLES-----------------------------------

typedef struct
	{
		uint16_t Time_prog;
		uint8_t  Temp_set;
	} program;

program Program_step[7][4];
uint8_t floor_temperature;
uint8_t  Flag_PreHeat, TMP_Event_now, i;
uint8_t conditions_register;
uint8_t count_register;
uint8_t button_register;
uint8_t Themperature;
uint8_t air_temperature;
uint8_t love_temperature;
uint8_t set_floor_temperature;
uint8_t set_floor_temperature_prog;
uint8_t temp_set_floor_temperature;
uint8_t delta_air_temperature;
uint8_t Table_HEAT_TIME[40];
uint8_t POWER_ON_OFF;
uint8_t WIFI_Level;
uint8_t data[7];
uint8_t first_start;
uint8_t first_sub;
uint8_t corr_temp;
uint8_t Sensor_set;
uint8_t temp_Sensor_set;
uint8_t Heat_mode;
uint8_t sensor_num;
uint8_t Antifr_Temp;
uint8_t set_air_temperature;
uint8_t Temper_Start;
uint8_t reconn_count;
uint8_t no_floor_flag;
uint8_t FlagRestore;
uint8_t FlagWiFiMode;
uint8_t FlagPlusMinus;
uint8_t no_sound;
uint8_t FlagBlock;
uint8_t upd_tmp_tbl;
uint8_t force_update;
uint8_t temp_mac[6];
uint8_t WINDOW;
uint8_t sta_ap_flag;
uint8_t Relay_flag;
uint8_t Sensor_err;
uint8_t Power2;
uint8_t light_mode;
int raw_out;
int line[1];
uint32_t calib_ref;
uint16_t Power1;
uint16_t led_Counter;
uint16_t rtc_counter;
uint16_t conn_Counter;
uint16_t wifi_conn_Counter;
uint16_t heart_counter;
uint16_t block_counter;
uint16_t ota_count;
uint16_t clock_count;
uint16_t min_count;
uint16_t seg_calls;
uint16_t sc_ap_dig;
uint16_t sc_ap_counter;
uint16_t temp_Time_relay_ON;
uint8_t  temp_time_relay;
uint32_t Time_relay_ON;
int adc_floor;

uint32_t pad_intr;

uint32_t ip_first;
uint32_t ip_last;
//-------------------------------------------
uint8_t set_temperature;
uint8_t DisplayDigit;
uint8_t CurentData;
uint8_t Flag_OFF;
//------------------------------------FUNCTIONS-----------------------------------
void buffer_add_char(uint8_t, uint16_t);
void buffer_add_string(uint8_t *new_part, uint16_t position, uint16_t count);
void buffer_add_int(uint16_t new_int, uint16_t position);
void sending_msg(void *);
void SPI_Init();
void GPIO_Conf();
void Sound (uint8_t);
void Button_scan(void);
void Menu();
void adc_init();
void reset_LCD();
void init_i2c_rtc();
void clear_wifi();
void wifi_rssi();
void Factory_SAVE(void);
void Load_from_flash(void);
void LedIndicatorDisplay(uint8_t data);
void send_seg(uint8_t data);
void show_seg();
void send_led(uint16_t data);
void display_dig();
void LedButtonDisplay();
void reset_clock();
uint16_t touch_value;
 uint8_t hex2bcd(uint8_t);
  int8_t Calculate_Temper(uint16_t Code, uint8_t Sens_tipe);
 uint8_t ADC2temperature_C(uint16_t ADC_data);
uint16_t EX_Send_Config(uint16_t pos);
void heat_task(void *pvParameters);

#endif /* FUNCTIONS_H_ */
