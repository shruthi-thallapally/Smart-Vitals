/*
 * schedulers.h
 *
 *  Created on: Feb 3, 2024
 *      Author: Tharuni Gelli
 */

#ifndef SRC_SCHEDULER_H_
#define SRC_SCHEDULER_H_

#include "em_letimer.h"

#include "app.h"
#include "src/timers.h"
#include "src/oscillators.h"
#include "src/gpio.h"

/**
 * @brief Sets an event flag for underflow event from LETIMER0.
 *
 * This function sets an event flag in the global variable my_event to indicate
 * an underflow event from LETIMER0 has occurred.
 */
void schedulerSetEventUF();



/**
 * @brief Retrieves and clears the next event flag.
 *
 * This function retrieves the next event flag from the global variable my_event,
 * clears the flag, and returns the retrieved event.
 *
 * @return The retrieved event flag.
 */
uint32_t getNextEvent();

#endif /* SRC_SCHEDULER_H_ */
