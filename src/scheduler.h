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

// Define an enumeration representing various events.
enum
{
    // Define an event flag indicating no event.
    evt_no_event = 0,

    // Define an event flag indicating an underflow event from LETIMER0.
    evt_LETIMER0_UF,

    // Define an event indicating value reach for COMP1.
    evt_COMP1,

    // Define an event indicating transfer done for i2c.
    evt_Transfer_Done,

    // Define an event indicating button 0 pressed
    evt_Button_Pressed,

    // Define an event indicating button 0 released
    evt_Button_Released,
};


/**
 * @brief Manages the state machine for temperature monitoring.
 *
 * This function controls the state machine dedicated to monitoring and managing temperature-related events
 * in a system, especially useful in contexts where temperature readings from sensors are critical, such as in
 * environmental monitoring systems or device thermal management. It processes events that indicate temperature changes,
 * threshold crossings, or sensor status updates. Based on these events, it can trigger alerts, activate cooling mechanisms,
 * or adjust operational parameters to maintain optimal temperature conditions.
 *
 * @param evt Pointer to the event message structure received from the BLE stack or a sensor controller. This structure
 * contains detailed information about the temperature event that occurred, facilitating appropriate responses or
 * adjustments in the system's behavior or state.
 *
 * @return void
 */
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
 * @brief Sets an event indicating the completion of a transfer.
 *
 * This function sets an event indicating the completion of a data transfer.
 * The event is typically used in systems where asynchronous data transfer
 * operations are performed, such as I2C or SPI communication. The event
 * is useful for triggering actions or state transitions in the system.
 *
 * @return void
 */
void schedulerSetEventButtonPressed();


/**
 * @brief Sets an event indicating that a button has been released.
 *
 * This function sets an event indicating that a button has been released after being pressed.
 * It is commonly used in embedded systems for handling user input or in applications where
 * the physical interaction needs to be monitored. This event can trigger specific actions or
 * state changes in the system, such as stopping a process that was initiated by the button press.
 *
 * @return void
 */
void schedulerSetEventButtonReleased();

/**
 * @brief Retrieves and clears the next event flag.
 *
 * This function retrieves the next event flag from the global variable my_event,
 * clears the flag, and returns the retrieved event.
 *
 * @return The retrieved event flag.
 */
uint32_t getNextEvent();


/**
 * @brief Manages the state machine for device discovery.
 *
 * This function oversees the state transitions and actions of the discovery state machine,
 * which is typically involved in scanning for and connecting to Bluetooth Low Energy (BLE) devices.
 * It handles events related to discovery, such as the detection of advertising packets or the completion
 * of a connection process. The state machine can be used to automate the process of finding and
 * establishing connections with BLE devices in a controlled and efficient manner.
 *
 * @param evt Pointer to the event message structure received from the BLE stack. This structure
 * contains information about the event that occurred, which is used to determine the next action
 * in the state machine.
 *
 * @return void
 */
void discovery_state_machine(sl_bt_msg_t *evt);

#endif /* SRC_SCHEDULER_H_ */
