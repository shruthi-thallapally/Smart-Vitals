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


void temperature_state_machine(sl_bt_msg_t *evt);

/**
 * @brief Sets an event flag for underflow event from LETIMER0.
 *
 * This function sets an event flag in the global variable my_event to indicate
 * an underflow event from LETIMER0 has occurred.
 */
void schedulerSetEventUF();

/**
 * @brief Sets an event related to COMP1 for scheduling.
 *
 * This function sets an event related to the COMP1 peripheral for scheduling.
 * The event is used by the scheduler to trigger actions or state transitions
 * in the system.
 *
 * @return void
 */
void schedulerSetEventCOMP1();

/**
 * @brief Sets an event indicating the completion of a transfer.
 *
 * This function sets an event indicating the completion of a data transfer.
 * The event is typically used in systems where asynchronous data transfer
 * operations are performed, such as I2C or SPI communication. The event
 * is useful for triggering actions or state transitions in the system.
 *
 * @return void
 */
void schedulerSetEventTransferDone();

/**
 * @brief Retrieves and clears the next event flag.
 *
 * This function retrieves the next event flag from the global variable my_event,
 * clears the flag, and returns the retrieved event.
 *
 * @return The retrieved event flag.
 */
uint32_t getNextEvent();

void discovery_state_machine(sl_bt_msg_t *evt);

#endif /* SRC_SCHEDULER_H_ */
