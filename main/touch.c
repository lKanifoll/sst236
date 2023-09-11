#include "Functions.h"
#include "touch.h"

//static bool s_pad_activated[TOUCH_PAD_MAX];

void tp_example_rtc_intr(void * arg)
{
    uint32_t pad_intr = touch_pad_get_status();
    //clear interrupt
    touch_pad_clear_status();
    for (int i = TOUCH_PAD_NUM2; i < TOUCH_PAD_NUM6; i++) {
        if ((pad_intr >> i) & 0x01) {
            s_pad_activated[i] = true;

        }
    }
}


void touchinit(void)
{
	printf("Initializing touch pad\n");
	touch_pad_init();

	touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);
	// Initialize and start a software filter to detect slight change of capacitance.
	//touch_pad_filter_start(100);
	//touch_pad_set_filter_period(100);
	// Set measuring time and sleep time
	// In this case, measurement will sustain 0xffff / 8Mhz = 8.19ms
	// Meanwhile, sleep time between two measurement will be 0x1000 / 150Khz = 27.3 ms
	touch_pad_set_meas_time(0x0100, 0xffff);

	//set reference voltage for charging/discharging
	// In this case, the high reference valtage will be 2.4V - 1.5V = 0.9V
	// The low reference voltage will be 0.8V, so that the procedure of charging
	// and discharging would be very fast.
	touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_0V5); /// 0.2V ON/OFF
	// Init touch pad IO
	for (int i = TOUCH_PAD_NUM2; i < TOUCH_PAD_NUM6; i++)
	{

		//init RTC IO and mode for touch pad.
		touch_pad_config(i, TOUCH_THRESH_NO_USE);
	}

	touch_pad_filter_start(200);


	vTaskDelay(1000/portTICK_PERIOD_MS);

	for (int i = TOUCH_PAD_NUM2; i< TOUCH_PAD_NUM6; i++)
	{
		touch_pad_read_filtered(i, &touch_value);
		s_pad_init_val[i] = touch_value;
		printf("touch button: %d\n", touch_value);

		ESP_ERROR_CHECK(touch_pad_set_thresh(i, touch_value * TOUCH_THRESH_PERCENT / 100));
		//touch_pad_set_cnt_mode(i, TOUCH_PAD_SLOPE_7, TOUCH_PAD_TIE_OPT_LOW);
	}



	touch_pad_isr_register(tp_example_rtc_intr, NULL);
	touch_pad_intr_enable();

}


