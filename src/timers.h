/*
 * timers.h
 *
 *  Created on: Jan 28, 2024
 *      Author: Tharuni Gelli
 */

#ifndef SRC_TIMERS_H_
#define SRC_TIMERS_H_

#define INCLUDE_LOG_DEBUG 1 // Including logging for this file
#include "src/log.h"

#include "src/oscillators.h"
#include "app.h"
#include "em_letimer.h"

//
#define LETIMER_PERIOD_MS  3000 // Total Period in seconds

/*
 * Function: void init_LETIMER0 ()
 * ----------------------------------------------
 * Initializes the LETIMER0 peripheral with the specified configuration.
 */
void init_LETIMER0 ();

/**
 * @brief Wait for a specified duration in microseconds using a timer.
 *
 * This function waits for a specified duration in microseconds by utilizing a timer.
 * It calculates the required number of timer ticks based on the input duration,
 * then waits until the timer reaches the required count.
 *
 * @param us_wait Duration to wait in microseconds.
 */
void timerWaitUs(uint32_t us_wait);


#endif /* SRC_TIMERS_H_ */
