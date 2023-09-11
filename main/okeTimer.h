/*
 * okeTimer.h
 *
 *  Created on: May 29, 2018
 *      Author: sergey
 */

#ifndef MAIN_OKETIMER_H_
#define MAIN_OKETIMER_H_
#include "Functions.h"


#define TIMER_DIVIDER         16  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds
#define TIMER_INTERVAL0_SEC   (3.4179) // sample test interval for the first timer
#define TIMER_INTERVAL1_SEC   (5.78)   // sample test interval for the second timer
#define TEST_WITHOUT_RELOAD   0        // testing will be done without auto reload
#define TEST_WITH_RELOAD      1        // testing will be done with auto reload


void example_tg0_timer_init(int timer_idx, bool auto_reload, double timer_interval_sec);

#endif /* MAIN_OKETIMER_H_ */
