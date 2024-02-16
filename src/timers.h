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

// Define ACTUAL_CLK_FREQ as the result of calling the required_oscillator() function
#define ACTUAL_CLK_FREQ required_oscillator()

// Calculate the value to load into LETIMER_COMP0 register
// based on the LETIMER_PERIOD_MS and ACTUAL_CLK_FREQ, then convert it to microseconds
#define VALUE_TO_LOAD_COMP0 (LETIMER_PERIOD_MS * ACTUAL_CLK_FREQ) / 1000

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
void timerWaitUs_polled(uint32_t us_wait);

/**
 * @brief Wait for a specified duration in microseconds using interrupts.
 *
 * This function waits for a specified duration in microseconds using interrupts.
 * It configures the LETIMER module to generate an interrupt after the specified
 * time period and sets up the required counter value accordingly. The function
 * then enables the COMP1 interrupt in both the LETIMER module and the LETIMER0
 * interrupt enable register.
 *
 * @param us_wait The duration to wait in microseconds.
 * @return void
 */
void timerWaitUs_irq(uint32_t us_wait);


#endif /* SRC_TIMERS_H_ */
