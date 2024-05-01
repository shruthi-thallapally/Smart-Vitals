/*
 * scheduler.c
 *
 *  Created on: 04-Apr-2024
 *      Author: Tharuni Gelli
 *  Reference: Code reference taken from the lecture slides
 *  Note: Sometimes warnings are generated due to the log statements.
 *   I have commented LOG_ERROR statements to avoid generating unwanted warnings
 */

// Including logging files to the file
#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"
#include "src/ble_device_type.h"
#include "src/scheduler.h"
#include "em_letimer.h"
#include "app.h"
#include "src/timers.h"
#include "src/oscillators.h"
#include "src/gpio.h"
#include "stdint.h"
#include "src/irq.h"
#include "sl_bluetooth.h"
#include "src/i2c.h"
#include "src/ble.h"
#include "sl_bt_api.h"
#include "src/lcd.h"
#include "src/SparkFun_APDS9960.h"
#include "src/pulse_oximeter.h"


#if !DEVICE_IS_BLE_SERVER

static const uint8_t thermo_service[2]={0x09,0x18};

static const uint8_t thermo_char[2]={0x1c,0x2a};

static const uint8_t button_service[16]={0x89,0x62,0x13,0x2d,0x2a,0x65,0xec,0x87,0x3e,0x43,0xc8,0x38,0x01,0x00,0x00,0x00};

static const uint8_t button_charac[16] = { 0x89, 0x62, 0x13, 0x2d, 0x2a, 0x65, 0xec, 0x87, 0x3e, 0x43, 0xc8, 0x38, 0x02, 0x00, 0x00, 0x00 };

//service uuid - gesture sensor
//855eb9c5-82e1-4f81-961f-9bce01d3547d
static const uint8_t gesture_service[16] = { 0x7d, 0x54, 0xd3, 0x01, 0xce, 0x9b, 0x1f, 0x96, 0x81, 0x4f, 0xe1, 0x82, 0xc5, 0xb9, 0x5e, 0x85 };

//characteristic uuid - gesture sensor
//c07a50a6-0642-49f0-839d-2933b25c414b
static const uint8_t gesture_char[16] = { 0x4b, 0x41, 0x5c, 0xb2, 0x33, 0x29, 0x9d, 0x83, 0xf0, 0x49, 0x42, 0x06, 0xa6, 0x50, 0x7a, 0xc0 };

#endif

int Count_PulseData=0;

uint8_t Mode_ReadDevice[2] = {0x02, 0x00};  //send this command to begin the communication with pulse oximeter

sl_status_t rc=0;

uint32_t my_event; ///< Global variable to store the scheduler event


#if DEVICE_IS_BLE_SERVER

void handle_gesture() {

  ble_data_struct_t *bleData = getBleDataPtr();

  if ( isGestureAvailable() ) {
      //LOG_INFO("Is gesture available?\n\r");

      switch ( readGesture() ) {

        case DIR_DOWN:
          LOG_INFO("DOWN\n\r");
          bleData->gesture_value = 0x04;
          displayPrintf(DISPLAY_ROW_9, "Gesture = DOWN");
          disableGestureSensor();
          bleData->gesture_on = false;
          displayPrintf(DISPLAY_ROW_ACTION, "Gesture sensor OFF");
          //LOG_INFO("Sending down gesture\n\r");
          ble_SendGesture(0x04);
          break;

        case DIR_UP:
          LOG_INFO("UP\n\r");
          bleData->gesture_value = 0x03;
          displayPrintf(DISPLAY_ROW_9, "Gesture = UP");
          //LOG_INFO("Sending up gesture\n\r");
          ble_SendGesture(0x03);
          break;

        case DIR_LEFT:
          LOG_INFO("LEFT\n\r");
          bleData->gesture_value = 0x01;
          displayPrintf(DISPLAY_ROW_9, "Gesture = LEFT");
          //LOG_INFO("Sending left gesture\n\r");
          ble_SendGesture(0x01);
          break;

        case DIR_RIGHT:
          LOG_INFO("RIGHT\n\r");
          bleData->gesture_value = 0x02;
          displayPrintf(DISPLAY_ROW_9, "Gesture = RIGHT");
          // LOG_INFO("Sending right gesture\n\r");
          ble_SendGesture(0x02);
          break;

        case DIR_NEAR:
          LOG_INFO("NEAR\n\r");
          bleData->gesture_value = 0x05;
          displayPrintf(DISPLAY_ROW_9, "Gesture = NEAR");
          // LOG_INFO("Sending near gesture\n\r");
          ble_SendGesture(0x05);
          break;

        case DIR_FAR:
          LOG_INFO("FAR\n\r");
          bleData->gesture_value = 0x06;
          displayPrintf(DISPLAY_ROW_9, "Gesture = FAR");
          //LOG_INFO("Sending far gesture\n\r");
          ble_SendGesture(0x06);
          break;

        default:
          LOG_INFO("NONE");
          bleData->gesture_value = 0x00;
          displayPrintf(DISPLAY_ROW_9, "Gesture = NONE");
          ble_SendGesture(0x00);
      }
  }
}

void gesture_state_machine(sl_bt_msg_t *evt) {

  state currentState;
  static state nextState = State0_Gesture_Wait;


  currentState = nextState;     //set current state of the process

  switch(currentState) {

    case State0_Gesture_Wait:

      nextState = State0_Gesture_Wait;          //default

      //check for underflow event
      if(evt->data.evt_system_external_signal.extsignals == Evt_GestureInt) {

          LOG_INFO("GestureInt event\n\r");

          handle_gesture();

          nextState = State1_Gesture;
      }

      break;

    case State1_Gesture:

      nextState = State0_Gesture_Wait;

      break;

    default:
      LOG_ERROR("Should not be here in gesture state machine\n\r");
  }

  return;

}

void oximeter_state_machine(sl_bt_msg_t *evt) {

  state currentState;
  static state nextState = state_pulse_sensor_init;
  ble_data_struct_t *bleData = getBleDataPtr();
  //bool gesture_check =false;

    currentState = nextState;     //set current state of the process

    switch(currentState) {

            case state_pulse_sensor_init:
              LOG_INFO("In state_pulse_sensor_init\n\r");
              //setting MFIO and RESET as output, reset is set and mfio is cleared
              pulse_oximeter_init_pins();
              //bio_hub_init();
              //clear the reset in and set the MFIO pin
              turn_off_reset();

              //Set the MFIO pin
              turn_on_mfio();

              //wait 10ms
              timerWaitUs_irq(10000);

              nextState = state_wait_10ms;

              break;

            case state_wait_10ms:
              LOG_INFO("In state_wait_10ms\n\r");
              if(evt->data.evt_system_external_signal.extsignals == LETIMER0_COMP1 ){
                  LOG_INFO("In if case of state_wait_10ms\n\r");
                //set reset pin
                turn_on_reset();

                //wait for 1 second
                timerWaitUs_irq(1000000);

                nextState = state_wait_1s;
              }

              break;

            case state_wait_1s:
              LOG_INFO("In state_wait_1s\n\r");
              if(evt->data.evt_system_external_signal.extsignals == LETIMER0_COMP1 ){
                  LOG_INFO("In if case of state_wait_1s\n\r");
                //set MFIO pin as an interrupt
                set_MFIO_interrupt();
                //bio_hub_init_bootloader_mode();

                //settings for putting the device into read mode
                I2C_write_polled_pulse(Mode_ReadDevice, 2);

                //wait for 10ms before performing a read
                timerWaitUs_irq(10000);

                nextState = state_read_return_check;
              }

              break;

            case state_read_return_check:
              LOG_INFO("In state_read_return_check\n\r");
              if(evt->data.evt_system_external_signal.extsignals == LETIMER0_COMP1 ){

                  //perform read to check the return value
                  I2C_read_polled_pulse();

                  //perform read to check if it returns 0
                  check_read_return();
                 // bio_hub_read_sensor_hub_status();
                  //set the output mode
                  set_output_mode_func();
                //  bio_hub_set_output_mode();
                  //wait for 6ms between a write and a read
                  timerWaitUs_irq(6000);

                  nextState = state_set_output_mode;
              }

              break;

            case state_set_output_mode:
              LOG_INFO("In state_set_output_mode\n\r");
              if(evt->data.evt_system_external_signal.extsignals == LETIMER0_COMP1 ){

                 //perform read to check the return value
                  I2C_read_polled_pulse();

                  //perform read to check if it returns 0
                  check_read_return();
                  // bio_hub_read_sensor_hub_status();
                  //set the fifo threshold
                  setFifoThreshold_func();
                 // bio_hub_set_fifo_threshold();
                  //wait for 6ms before performing a read
                  timerWaitUs_irq(6000);

                  nextState = state_setFifoThreshold;
              }

              break;

            case state_setFifoThreshold:
              LOG_INFO("In state_setFifoThreshold\n\r");
              if(evt->data.evt_system_external_signal.extsignals == LETIMER0_COMP1 ){

                  //LOG_INFO("In state_setFifoThreshold!!\n\r");

                  //wait for 100ms before performing red
                  //NOTE:This is set to match the timings as per the arduino code provided in Github
                  timerWaitUs_polled(100000);

                  //perform read to check the return value
                  I2C_read_polled_pulse();

                   //perform read to check if it returns 0
                  check_read_return();
                  // bio_hub_read_sensor_hub_status();
                   //agc algo control commands
                   agcAlgoControl_func();
       //            bio_hub_agc_algo_control();
                   //wait for 6ms before performing a read
                   timerWaitUs_irq(6000);

                   nextState = state_agcAlgoControl;
              }

              break;

            case state_agcAlgoControl:
              LOG_INFO("In state_agcAlgoControl\n\r");
              if(evt->data.evt_system_external_signal.extsignals == LETIMER0_COMP1 ){

                  //LOG_INFO("In state_agcAlgoControl!!\n\r");

                  //wait for 100ms before performing red
                  //NOTE:This is set to match the timings as per the arduino code provided in Github
                  timerWaitUs_polled(100000);

                  //perform read to check the return value
                  I2C_read_polled_pulse();

                   //set the control for the sensor
                   max30101Control_func();

                   //wait for 6ms before performing a read
                   timerWaitUs_irq(6000);

                   nextState = state_max30101Control;

              }

              break;

            case state_max30101Control:
              LOG_INFO("In state_max30101Control\n\r");
              if(evt->data.evt_system_external_signal.extsignals == LETIMER0_COMP1 ){

                  //LOG_INFO("In state_max30101Control!!\n\r");
                  //wait for 100ms before performing red
                  //NOTE:This is set to match the timings as per the arduino code provided in Github
                  timerWaitUs_polled(100000);

                  //perform read to check the return value
                  I2C_read_polled_pulse();

                   //maximum fast algo control
                   maximFastAlgoControl_func();
                  // bio_hub_maxim_fast_algo_control)();
                   //wait for 6ms before performing a read
                   timerWaitUs_irq(6000);

                   nextState = state_maximFastAlgoControl;

              }

              break;

            case state_maximFastAlgoControl:
              LOG_INFO("In state_maximFastAlgoControl\n\r");
              if(evt->data.evt_system_external_signal.extsignals == LETIMER0_COMP1 ){

                  //LOG_INFO("In state_maximFastAlgoControl!!\n\r");
                  //wait for 100ms before performing red
                  //NOTE:This is set to match the timings as per the arduino code provided in Github
                  timerWaitUs_polled(100000);

                  //perform read to check the return value
                  I2C_read_polled_pulse();

                   //read the algo samples before taking the actual readings
                   readAlgoSamples_func();

                   //wait for 6ms before performing a read
                   timerWaitUs_irq(6000);

                   nextState = state_readAlgoSamples;

              }

              break;

            case state_readAlgoSamples:
              LOG_INFO("In state_readAlgoSamples\n\r");
              if(evt->data.evt_system_external_signal.extsignals == LETIMER0_COMP1 ){


                  //perform read to check the return value
                  I2C_read_polled_pulse();

                   //perform read to check if it returns 0
                  check_read_return();

                   //wait 6 seconds before taking the actual reading
                   timerWaitUs_irq(6000000);

                   nextState = state_wait_before_reading;

              }

              break;

            case state_wait_before_reading:
              LOG_INFO("In state_wait_before_reading\n\r");
              if(evt->data.evt_system_external_signal.extsignals == LETIMER0_COMP1 ){

                  //read the sensor status
                   read_sensor_hub_status_func();

                   //wait 6 seconds before taking the actual reading
                   timerWaitUs_irq(6000);

                   nextState = state_read_sensor_hub_status;

              }

              else if(bleData->pulse_on){

                  LOG_INFO("In else if of state_wait_before_reading\n\r");
                  //read the sensor status
                   read_sensor_hub_status_func();

                   //wait 6 seconds before taking the actual reading
                   timerWaitUs_irq(6000);

                   nextState = state_read_sensor_hub_status;
              }

              break;

            case state_read_sensor_hub_status:
              LOG_INFO("In state_read_Sensor_hub_status\n\r");
                  if(evt->data.evt_system_external_signal.extsignals == LETIMER0_COMP1 ){

                        //read for checking the return value
                      I2C_read_polled_pulse();

                        //perform read to check if it returns 0
                      check_read_return();

                        //number of samples out of the fifo
                        numSamplesOutFifo_func();

                        //wair for 6ms before performing a read
                        timerWaitUs_irq(6000);

                         nextState = state_numSamplesOutFifo;
                  }

              break;

            case state_numSamplesOutFifo:
              LOG_INFO("In state_numSamplesOutFifo\n\r");
                if(evt->data.evt_system_external_signal.extsignals == LETIMER0_COMP1 ){

                    //perform a read to check the return value
                    I2C_read_polled_pulse();

                    //perform read to check if it returns 0
                    check_read_return();

                    //start reading sesnor data by sending read commands
                    read_fill_array_func();

                    //wair for 6ms before performing a read
                    timerWaitUs_irq(6000);

                     nextState = state_read_fill_array;

                }

              break;

            case state_read_fill_array:
              LOG_INFO("In state_read_fill_array\n\r");
                if(evt->data.evt_system_external_signal.extsignals == LETIMER0_COMP1){

                    //read the actual data
                    I2C_read_polled_pulse();

                    //read 15 values to give sensor time to acquire appropriate values
                      if(Count_PulseData <1){

                          //stop after 15 counts
                          Count_PulseData = pulse_data_extract();

                          //read after every second
                          timerWaitUs_irq(1000000);

                          nextState = state_wait_before_reading;
                      }

                      else{
                          bleData->pulse_on = false;
                          Count_PulseData = 0;
                          timerWaitUs_irq(6000);
                          nextState = state_disable_AFE;

                      }

                }

              break;

            case state_disable_AFE:
              LOG_INFO("In state_disable_AFE\n\r");
                  if(evt->data.evt_system_external_signal.extsignals == LETIMER0_COMP1){
                  //disable AFE
                              disable_AFE_func();

                              //wair for 6ms before performing a read
                              timerWaitUs_irq(6000);

                               nextState = state_disable_algo;
                  }

              break;

            case state_disable_algo:
              LOG_INFO("In state_disableAlgo\n\r");
              if(evt->data.evt_system_external_signal.extsignals == LETIMER0_COMP1){

                          //perform a read to check the return value
                  I2C_read_polled_pulse();

                          //perform read to check if it returns 0
                  check_read_return();

                          //start reading sesnor data by sending read commands
                          disable_algo_func();

                          //wair for 6ms before performing a read
                          timerWaitUs_irq(6000);

                           nextState = state_pulse_done;

                    }
              break;

           case state_pulse_done:
             LOG_INFO("In state_pulse_dones\n\r");
              if(evt->data.evt_system_external_signal.extsignals == LETIMER0_COMP1){
                            //perform a read to check the return value
                  I2C_read_polled_pulse();

                            //perform read to check if it returns 0
                  check_read_return();

                            nextState = state_pulse_done;
              }
              else if(bleData->pulse_on){
                  bleData->gesture_value = 0x00;
                  bleData->gesture_on = true;
                  nextState = state_pulse_sensor_init;
              }

          break;


            default:
              LOG_INFO("Something wrong!! In the default state of oximeter state machine");

              break;

      }
    return;
}
/**
 * @brief State machine to control the temperature sensor
 * @param event The event triggered by interrupts
 */
void temp_state_machine(sl_bt_msg_t *event)
{
    state Present_State ;       // Current state
    static state Next_State = StateA_Sleep; // Next state, initialized to StateA_Sleep
    ble_data_struct_t *bleData = getBleDataPtr();

    Present_State = Next_State;

    switch (Present_State)
    {
       case StateA_Sleep:
       Next_State = StateA_Sleep; // Default next state

       // Check if LETIMER0 underflow event occurred
        if (event->data.evt_system_external_signal.extsignals == LETIMER0_UF)
        {
          LOG_INFO("timerUF event\n\r");
          timerWaitUs_irq(80000);        // Wait for 80 milliseconds
          Next_State = StateB_Wait;      // Transition to StateB_Wait
        }
       break;

        case StateB_Wait:
            Next_State = StateB_Wait; // Default next state

            // Check if LETIMER0 compare1 event occurred
            if (event->data.evt_system_external_signal.extsignals ==LETIMER0_COMP1 )
            {
                LOG_INFO("Comp1 event\n\r");
                // Remove energy mode requirement EM1.
                //sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1); // Add EM1 requirement
                i2c_Write();                               // Perform I2C write operation
                Next_State = StateC_WriteCmd;             // Transition to StateC_WriteCmd
            }
            break;

        case StateC_WriteCmd:
            Next_State = StateC_WriteCmd; // Default next state

            // Check if I2C operation is complete
            if (event->data.evt_system_external_signal.extsignals ==I2C_COMPLETE )
            {
                LOG_INFO("write transfer done\n\r");
                //sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1); // Remove EM1 requirement

                timerWaitUs_irq(10800);         // Wait for 10.8 milliseconds

                Next_State = StateD_WriteWait;  // Transition to StateD_WriteWait
            }
            break;

        case StateD_WriteWait:
            Next_State = StateD_WriteWait; // Default next state

            // Check if LETIMER0 compare1 event occurred
            if (event->data.evt_system_external_signal.extsignals ==LETIMER0_COMP1 )
            {
                // Remove energy mode requirement EM1.
                // sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1); // Add EM1 requirement
                i2c_Read();                                      // Read temperature
                Next_State = StateE_Read;                         // Transition to StateE_Read
            }
            break;

        case StateE_Read:
            Next_State = StateE_Read; // Default next state

            // Check if I2C operation is complete
            if (event->data.evt_system_external_signal.extsignals ==I2C_COMPLETE)
            {
                LOG_INFO("read transfer  done\n\r");
               //sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1); // Remove EM1 requirement
                                                   // Disable temperature sensor
                NVIC_DisableIRQ(I2C0_IRQn);                                  // Disable I2C interrupt
                LOG_INFO("Temperature = %f C\n\r", ConvertTempToCelcius());  // Log temperature
                ble_SendTemperature();
                bleData->gesture_value = 0x00;
                Next_State = StateA_Sleep;                                   // Transition to StateA_Sleep
            }
            break;

        default:
  //          LOG_ERROR("not in the state machine\n\r"); // Log error if not in any defined state
            break;
    }
return;
}


#else

/**
 * @brief Handles the state machine for BLE discovery.
 *
 * This function implements a state machine to handle BLE discovery process.
 * It transitions between different states based on events received.
 *
 * @param evt Pointer to the BLE event message structure.
 */
void discovery_state_machine(sl_bt_msg_t *evt){
  static state Next_State = StateA_client_idle; // Next state, initialized to StateA_Sleep
  state Present_State ;       // Current state
  ble_data_struct_t *bleData = getBleDataPtr();   // Get pointer to BLE data structure

  // If connection closed event, reset to initial state
  if(SL_BT_MSG_ID(evt->header)==sl_bt_evt_connection_closed_id)
  {
      Next_State=StateA_client_idle;
  }

  Present_State=Next_State;   // Update current state

  // State machine switch case
  switch(Present_State)
  {
    case StateA_client_idle:

      Next_State = StateA_client_idle;          //default

      // If connection opened event
      if(SL_BT_MSG_ID(evt->header) == sl_bt_evt_connection_opened_id)
      {

       // Perform GATT procedure: discover primary services by UUID



      rc = sl_bt_gatt_discover_primary_services_by_uuid(bleData->connection_handle,
                                                                  sizeof(thermo_service),
                                                                  (const uint8_t*)thermo_service);
      if(rc != SL_STATUS_OK)
      {
   //       LOG_ERROR("sl_bt_gatt_discover_primary_services_by_uuid() returned != 0 status=0x%04x\n\r", (unsigned int)rc);
      }
      bleData->gatt_procedure = true;
      Next_State = StateB_get_another_service;// Transition to next state
      }


    break;

    case StateB_get_another_service:
      Next_State = StateB_get_another_service;

      // If GATT procedure completed event
      if(SL_BT_MSG_ID(evt->header) == sl_bt_evt_gatt_procedure_completed_id)
      {

          // Perform GATT procedure: discover characteristics by UUID
        rc = sl_bt_gatt_discover_primary_services_by_uuid(bleData->connection_handle,
                                                          sizeof(button_service),
                                                         (const uint8_t*)button_service);

        if(rc != SL_STATUS_OK)
        {
             LOG_ERROR("sl_bt_gatt_discover_characteristics_by_uuid() returned != 0 status=0x%04x\n\r", (unsigned int)rc);
        }

        bleData->gatt_procedure = true;
        Next_State =     StateC_got_service;// Transition to next state
       }


    break;
    case StateC_got_service :
         Next_State = StateC_got_service ;

         // If connection closed event
         if(SL_BT_MSG_ID(evt->header) == sl_bt_evt_gatt_procedure_completed_id)
           {

            rc = sl_bt_gatt_discover_characteristics_by_uuid(bleData->connection_handle,
                                                             bleData->service_handle,
                                                             sizeof(thermo_char),
                                                            (const uint8_t*)thermo_char);

              if(rc != SL_STATUS_OK)
                {
                   LOG_ERROR("sl_bt_gatt_discover_characteristics_by_uuid() 2 returned != 0 status=0x%04x\n\r", (unsigned int)rc);
                }

              bleData->gatt_procedure = true;
             // Go to idle state to wait for connection open event
             Next_State = StateD_got_another_service;
         }

      break;
    case  StateD_got_another_service :

      Next_State =  StateD_got_another_service ;

      // If GATT procedure completed event
      if(SL_BT_MSG_ID(evt->header) == sl_bt_evt_gatt_procedure_completed_id)
     {

      rc = sl_bt_gatt_discover_characteristics_by_uuid(bleData->connection_handle,
                                                       bleData->button_service_handle,
                                                       sizeof(button_charac),
                                                       (const uint8_t*) button_charac);
       if(rc != SL_STATUS_OK)
      {
 //         LOG_ERROR("sl_bt_gatt_discover_characteristics_by_uuid() 1 returned != 0 status=0x%04x\n\r", (unsigned int)rc);
      }

       bleData->gatt_procedure = true;
      Next_State = StateE_got_char;     // Transition to next state
      }


    break;

    case StateE_got_char:
      Next_State = StateE_got_char;

      // If GATT procedure completed event
      if(SL_BT_MSG_ID(evt->header) == sl_bt_evt_gatt_procedure_completed_id)
        {

          rc = sl_bt_gatt_set_characteristic_notification(bleData->connection_handle,
                                                                   bleData->char_handle,
                                                                   sl_bt_gatt_indication);
           if(rc != SL_STATUS_OK)
             {
                LOG_ERROR("sl_bt_gatt_set_characteristic_notification() returned != 0 status=0x%04x\n\r", (unsigned int)rc);
             }

           bleData->gatt_procedure = true;
          Next_State = StateF_got_another_char; // Transition to next state
      }



     break;

    case StateF_got_another_char:
      Next_State =StateF_got_another_char;
      if(SL_BT_MSG_ID(evt->header) == sl_bt_evt_gatt_procedure_completed_id)
        {


                ///LOG_INFO("Enabling notifications 2\n\r");

           rc = sl_bt_gatt_set_characteristic_notification(bleData->connection_handle,
                                                           bleData->button_char_handle,
                                                           sl_bt_gatt_indication);
           if(rc != SL_STATUS_OK)
             {
                 LOG_ERROR("sl_bt_gatt_set_characteristic_notification() returned != 0 status=0x%04x\n\r", (unsigned int)rc);
             }

                //gatt command in process
           bleData->gatt_procedure = true;
           bleData->button_indication = true;

           displayPrintf(DISPLAY_ROW_CONNECTION, "Handling indications");
           Next_State = StateG_set_indication;
            }

            break;

    case StateG_set_indication:
      Next_State =StateG_set_indication ;
      if(SL_BT_MSG_ID(evt->header) == sl_bt_evt_gatt_procedure_completed_id)
        {

          Next_State = StateH_wait_for_close;
        }

        break;

    case StateH_wait_for_close:
      Next_State =StateH_wait_for_close ;
      if(SL_BT_MSG_ID(evt->header) == sl_bt_evt_connection_closed_id)
        {

           //go in idle state to wait for a connection open event
          Next_State = StateA_client_idle;
       }

       break;

    default:

 //      LOG_ERROR("Should not be here in state machine\n\r");

    break;
  }


}

#endif

/**
 * @brief Sets a scheduler event when LETIMER0 underflow interrupt occurs.
 */
void schedulerSetEventUF()
{
// enter critical section
  CORE_DECLARE_IRQ_STATE;
  // Disable interrupts
  CORE_ENTER_CRITICAL();
// set the event in your data structure, this has to be a read-modify-write
 // my_event|=LETIMER0_UF;

  sl_bt_external_signal(LETIMER0_UF);
// exit critical section
    // Enable interrupts
  CORE_EXIT_CRITICAL();
} // schedulerSetEventUF()

/**
 * @brief Sets a scheduler event when LETIMER0 COMP1 interrupt occurs.
 */
void schedulerSetEventCOMP1()
{
// enter critical section
  CORE_DECLARE_IRQ_STATE;
  // Disable interrupts
  CORE_ENTER_CRITICAL();
// set the event in your data structure, this has to be a read-modify-write
 // my_event|=LETIMER0_COMP1;

  sl_bt_external_signal(LETIMER0_COMP1);
// exit critical section
    // Enable interrupts
  CORE_EXIT_CRITICAL();
} // schedulerSetEventCOMP1()


/**
 * @brief Sets a scheduler event when I2C transfer completes.
 */
void schedulerSetEventI2Ccomplete()
{
// enter critical section
  CORE_DECLARE_IRQ_STATE;
  // Disable interrupts
  CORE_ENTER_CRITICAL();
// set the event in your data structure, this has to be a read-modify-write
//  my_event|=I2C_COMPLETE;

  sl_bt_external_signal(I2C_COMPLETE);
// exit critical section
    // Enable interrupts
  CORE_EXIT_CRITICAL();
} // schedulerSetEventI2C_COMPLETE()

/**
 * @brief Sets the event indicating a button press in the scheduler.
 *
 * This function sets the event indicating that a button press has occurred in the scheduler.
 * It disables interrupts before setting the event to ensure atomicity of the operation.
 * After setting the event, interrupts are re-enabled.
 */
void schedulerSetEventButtonPressed()
{
  // enter critical section
    CORE_DECLARE_IRQ_STATE;
    // Disable interrupts
    CORE_ENTER_CRITICAL();

    // Signal the scheduler about button press event
    sl_bt_external_signal(Evt_Button_Pressed);
  // exit critical section
      // Enable interrupts
    CORE_EXIT_CRITICAL();

}     //schedulerSetEventPB0ButtonPressed()

/**
 * @brief Sets the event indicating a button release in the scheduler.
 *
 * This function sets the event indicating that a button release has occurred in the scheduler.
 * It disables interrupts before setting the event to ensure atomicity of the operation.
 * After setting the event, interrupts are re-enabled.
 */
void schedulerSetEventButtonReleased()
{
  // enter critical section
    CORE_DECLARE_IRQ_STATE;
    // Disable interrupts
    CORE_ENTER_CRITICAL();

    // Signal the scheduler about button release event
    sl_bt_external_signal(Evt_Button_Released);
  // exit critical section
      // Enable interrupts
    CORE_EXIT_CRITICAL();

}     //schedulerSetEventButtonReleased()

// scheduler routine to set a scheduler event
void schedulerSetGestureEvent()
{
  // enter critical section
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();

  sl_bt_external_signal(Evt_GestureInt);

  // exit critical section
  CORE_EXIT_CRITICAL();

} // schedulerSetEventXXX()

