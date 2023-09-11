/*
 * rtc.h
 *
 *  Created on: Feb 9, 2018
 *      Author: sergey
 */

#ifndef MAIN_RTC_H_
#define MAIN_RTC_H_
#include "Functions.h"
#include <time.h>
#define L_YEAR           31536000
#define L_DAY            86400
#define L_HOUR           3600
#define L_MIN            60

//typedef enum {NO_CLICK=0, SHORT_CLICK, LONG_CLICK, DLONG_CLICK} TypeClick;
i2c_config_t conf;

typedef struct
{
  uint8_t RTC_Hours;
  uint8_t RTC_Minutes;
  uint8_t RTC_Seconds;
}
RTC_TimeTypeDef;

typedef struct
{
  uint8_t RTC_WeekDay;
  uint8_t RTC_Date;
  uint8_t RTC_Month;
  uint8_t RTC_Year;

}
RTC_DateTypeDef;


RTC_TimeTypeDef   RTC_TimeStr;
RTC_DateTypeDef   RTC_DateStr;

uint8_t timestamp_ascii[11];
uint8_t time_ascii[20];
uint8_t timezone[2];
uint8_t Event_now, Event_next, Day_Event_Now, Day_Event_Next;
uint16_t Time_min24;
uint16_t Temp_minuts;
uint32_t timestamp;

//void rtc_set(void);
void RTC_task();
void init_i2c_rtc();
uint8_t BCDtoBIN(uint8_t BDC);
uint8_t BINtoBCD(uint8_t BIN);
uint8_t RTC_get_week_day(uint8_t date, uint8_t month, uint16_t year);
uint32_t RTC_timestamp_get(uint8_t day, uint8_t month, uint8_t year, uint8_t hour, uint8_t min, uint8_t sec);
void reverse(char s[]);
void inttoascii(long int n, char s[]);
void EVENT(void);
void RTC_time_set_ascii(uint8_t* datetime_ascii);
void rtc_set(uint8_t sec,uint8_t min, uint8_t ours, uint8_t date, uint8_t weekday, uint8_t month, uint8_t year);
#endif /* MAIN_RTC_H_ */
