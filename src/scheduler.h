/*
 * scheduler.h
 *
 *  Created on: 08-Feb-2024
 *      Author: Shruthi Thallapally
 *   Reference: Code reference taken from the lecture slides
 */

#ifndef SRC_SCHEDULER_H_
#define SRC_SCHEDULER_H_

#include "sl_bt_api.h"




/**
 * @brief Enumeration of scheduler events.
 */
enum
{
  no_event     = 0             ,     /**< No event */
  LETIMER0_UF              ,     /**< LETIMER0 underflow event */
  LETIMER0_COMP1           ,
  I2C_COMPLETE             ,
  Evt_Button_Pressed       ,
  Evt_Button_Released      ,
  Evt_GestureInt           ,
};

/**
 * @brief States of the state machine
 */
typedef enum {

  //states for temperature state machine
    StateA_Sleep,      /**< State A: Sleep */        ///< StateA_Sleep
    StateB_Wait,       /**< State B: Wait */         ///< StateB_Wait
    StateC_WriteCmd,   /**< State C: Write Command *////< StateC_WriteCmd
    StateD_WriteWait,  /**< State D: Write Wait */   ///< StateD_WriteWait
    StateE_Read,        /**< State E: Read */         ///< StateE_Read

    //states for discovery state machine
    State0_client_idle,   // Initial state
    State0_get_button_service,      // Discovering services
    State0_get_gesture_service,
    State0_get_oximeter_service,
    State1_get_temp_char,
    State1_get_button_char,
    State1_get_gesture_char,
    State1_get_oximeter_char,
    State2_set_temp_ind,
    State2_set_button_ind,
    State2_set_gesture_ind,
    State2_set_oximeter_ind,
    State3_all_set,
    State4_wait_for_close,

    State0_Gesture_Wait,
    State1_Gesture,

    State_No_Gesture,

    //for oximeter state machine
    state_pulse_sensor_init,
    state_wait_10ms,
    state_wait_1s,
    state_read_return_check,
    state_set_output_mode,
    state_setFifoThreshold,
    state_agcAlgoControl,
    state_max30101Control,
    state_maximFastAlgoControl,
    state_readAlgoSamples,
    state_wait_before_reading,
    state_read_sensor_hub_status,
    state_numSamplesOutFifo,
    state_read_fill_array,
    state_disable_AFE,
    state_disable_algo,
    state_pulse_done,
}state;



/**
 * @brief Sets a scheduler event when LETIMER0 underflow interrupt occurs.
 */
void schedulerSetEventUF();

/**
 * @brief Retrieves the next event from the global variable my_event.
 *        This function handles LETIMER0 underflow, LETIMER0 COMP1, and I2C_COMPLETE events.
 * @return The next event retrieved from the global variable.
 */
uint32_t getNextEvent();

/**
 * @brief Sets a scheduler event when I2C transfer completes.
 */
void schedulerSetEventI2Ccomplete();

/**
 * @brief Sets a scheduler event when LETIMER0 COMP1 interrupt occurs.
 */
void schedulerSetEventCOMP1();

/**
 * @brief Sets the event indicating a button press in the scheduler.
 *
 * This function sets the event indicating that a button press has occurred in the scheduler.
 * It disables interrupts before setting the event to ensure atomicity of the operation.
 * After setting the event, interrupts are re-enabled.
 */
void schedulerSetEventButtonPressed();

/**
 * @brief Sets the event indicating a button release in the scheduler.
 *
 * This function sets the event indicating that a button release has occurred in the scheduler.
 * It disables interrupts before setting the event to ensure atomicity of the operation.
 * After setting the event, interrupts are re-enabled.
 */
void schedulerSetEventButtonReleased();

void schedulerSetGestureEvent();

/**
 * @brief State machine to control the temperature sensor
 * @param event The event triggered by interrupts
 */
void temp_state_machine(sl_bt_msg_t *evt);


void handle_gesture();

void gesture_state_machine(sl_bt_msg_t *evt);

void oximeter_state_machine(sl_bt_msg_t *evt);
/**
 * @brief Handles the state machine for BLE discovery.
 *
 * This function implements a state machine to handle BLE discovery process.
 * It transitions between different states based on events received.
 *
 * @param evt Pointer to the BLE event message structure.
 */
void discovery_state_machine(sl_bt_msg_t *evt);

#endif /* SRC_SCHEDULER_H_ */
