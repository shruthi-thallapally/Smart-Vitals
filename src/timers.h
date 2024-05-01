/*
 * timers.h
 *
 *  Created on: 31-Jan-2024
 *      Author: Shruthi Thallapally
 * Description: Header file for functions related to Low Energy Timer (LETIMER0) configuration and initialization
 */

#ifndef SRC_TIMERS_H_
#define SRC_TIMERS_H_

#include "stdint.h"

#define LETIMER_PERIOD_MS 3000
#define VALUE_TO_COMP0 (LETIMER_PERIOD_MS*ACTUAL_FREQ)/1000

/**
 * @brief Initializes the Low Energy Timer (LETIMER0) with the specified configuration based on the lowest energy mode.
 *        Configures LETIMER0 to generate periodic interrupts and sets compare values.
 *
 * @param None
 * @return None
 */
void init_LETIMER0();

/**
 * @brief Waits for a specified duration in microseconds using the timer.
 *
 * @param us_wait The duration to wait in microseconds.
 * @note The function will wait for the specified duration using the LETIMER timer.
 */
void timerWaitUs_polled(uint32_t us_wait);
/**
 * @brief Waits for a specified duration in microseconds using the LETIMER peripheral with interrupts.
 * @param us_wait The duration to wait in microseconds.
 *        It should be within the range [MIN_WAIT, MAX_WAIT].
 */
void timerWaitUs_irq(uint32_t us_wait);

#endif /* SRC_TIMERS_H_ */
