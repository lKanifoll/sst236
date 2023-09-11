//#define  ENABLE_BIT_DEFINITIONS
#ifndef __CONSTANT_H
#define __CONSTANT_H


//#define TOUCH_THRESH  (25)
//#define TOUCH_CALIB   (25)

#define SDA_PIN 32
#define SCL_PIN 33

#define VERSION "119"

//--------------------------------------------------------------


#define MINUS   2
#define HEART   3
#define MODE    4
#define PLUS	5





#define COUNT_SHORT_CLICK       5
#define COUNT_DSHORT_CLICK      50
#define COUNT_LONG_CLICK        230
#define COUNT_DLONG_CLICK       235

//-----------condition---------------------------------------------------

#define REDRAW        0x01
#define RELAY         0x02
#define LIGHT         0x04
#define ALARM         0x08
#define ADC			  0x10
#define CONN		  0x20
#define SC_AP	      0x40
#define OTA			  0x80
//--------------------------------------------------------------
//------count--------------------------------------------------
#define HEART_SET        0x01
#define BLOCK_COUNT      0x02
//--------------------------------------------------------------

#define MAIN_WIN      	0
#define STANDBY_WIN    	1
#define POWER_OFF_WIN   2



#define CORR_MODE     3
#define ERROR_MODE    4
#define PWM_MODE      5

//--------------------------------------------------------------



#define RELAY_ON                		gpio_set_level(14, 1);
#define RELAY_OFF               		gpio_set_level(14, 0);

#define ON_ALL      					gpio_set_level(26, 0)
#define OFF_ALL       					gpio_set_level(26, 1)



#define WIFI_EXCELL				0
#define WIFI_GOOD				22
#define WIFI_FAIR				44
#define WIFI_WEAK				66
#define WIFI_EMPTY				88


#endif
