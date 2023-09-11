#ifndef MAIN_TOUCH_H_
#define MAIN_TOUCH_H_


//#define TOUCH_THRESH_NO_USE   (1000)
#define TOUCH_THRESH_NO_USE   (0)
#define TOUCH_THRESH_PERCENT  (98)
#include "Functions.h"


uint16_t touch_init_val[TOUCH_PAD_MAX];
uint8_t s_pad_activated[10];
uint16_t s_pad_init_val[TOUCH_PAD_MAX];
void touch_set_thresholds(void);
void touchinit(void);
void tp_example_read_task(void *pvParameter);

#endif
