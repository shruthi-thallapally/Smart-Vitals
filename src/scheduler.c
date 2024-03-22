/*
 * scheduler.c
 *
 *  Created on: Feb 3, 2024
 *      Author: Tharuni Gelli
 */

// Including logging files to the file
#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"

#include "app.h"
#include "src/scheduler.h"
#include "src/i2c.h"
#include "sl_bt_api.h"
#include "src/ble.h"

// Declare a global variable named my_event of type uint32_t to store event flags.
uint32_t my_event;

#if !DEVICE_IS_BLE_SERVER

// Definition for the UUID of the Health Thermometer service as specified by the Bluetooth Special Interest Group (SIG)
static const uint8_t thermometer_service[2] = { 0x09, 0x18 };

// UUID for the Temperature Measurement characteristic as specified by the Bluetooth Special Interest Group (SIG)
static const uint8_t thermometer_charac[2] = { 0x1c, 0x2a };

// button state service UUID defined by Bluetooth SIG
// 00000001-38c8-433e-87ec-652a2d136289
static const uint8_t button_service[16] = { 0x89, 0x62, 0x13, 0x2d, 0x2a, 0x65, 0xec, 0x87, 0x3e, 0x43, 0xc8, 0x38, 0x01, 0x00, 0x00, 0x00 };
// button state characteristic UUID defined by Bluetooth SIG
// 00000002-38c8-433e-87ec-652a2d136289
static const uint8_t button_char[16] = { 0x89, 0x62, 0x13, 0x2d, 0x2a, 0x65, 0xec, 0x87, 0x3e, 0x43, 0xc8, 0x38, 0x02, 0x00, 0x00, 0x00 };

#endif

// Declare a variable 'rc' of type 'sl_status_t' and initialize it with 0.
//This variable is typically used to store the status of a function call in Silicon Labs' SDK, where 0 usually indicates success.
sl_status_t rc=0;



//enum to define scheduler events
typedef enum uint32_t
{
  State0_IDLE,
  State1_TIMER_WAIT,
  State2_WRITE_CMD,
  State3_WRITE_WAIT,
  State4_READ,
  State0_IDLE_CLIENT,
  State0_GET_ANOTHER_SERVICE,
  State1_GOT_SERVICES,
  State1_GOT_ANOTHER_SERVICE,
  State2_GOT_CHAR,
  State2_GOT_ANOTHER_CHAR,
  State3_SET_INDICATION,
  State4_WAIT_FOR_CLOSE,
}My_State;



#if DEVICE_IS_BLE_SERVER

void temperature_state_machine(sl_bt_msg_t *evt)
{
  My_State Current_State; // Declare a variable to hold the current state of the state machine.

  static My_State Next_State = State0_IDLE; // Declare a static variable to hold the next state, initialized to StateA_IDLE.

  ble_data_struct_t *ble_Data = get_ble_DataPtr();

  if((SL_BT_MSG_ID(evt->header)==sl_bt_evt_system_external_signal_id)
       && (ble_Data->Connected==true)
       && (ble_Data->Indication==true))
  {

  Current_State = Next_State; // Set the current state to the next state.

  switch(Current_State) // Start a switch statement based on the current state.

  {

    case State0_IDLE: // If the current state is StateA_IDLE...

      Next_State = State0_IDLE; // Set the next state to StateA_IDLE.

      if(evt->data.evt_system_external_signal.extsignals == evt_LETIMER0_UF) // Check if the event includes LETIMER0 underflow.
      {

          timerWaitUs_irq(80000); // Wait for a certain amount of time using a timer interrupt.

          Next_State = State1_TIMER_WAIT; // Set the next state to StateB_TIMER_WAIT.
      }

      break;

    case State1_TIMER_WAIT: // If the current state is StateB_TIMER_WAIT...

      Next_State = State1_TIMER_WAIT; // Set the next state to StateB_TIMER_WAIT.

      if(evt->data.evt_system_external_signal.extsignals == evt_COMP1) // Check if the event includes a comparison event from COMP1.
      {

          sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1); // Add energy mode requirement EM1.

          Write_i2c(); // Perform an I2C write operation.

          Next_State = State2_WRITE_CMD; // Set the next state to StateC_WRITE_CMD.
      }

      break;

    case State2_WRITE_CMD: // If the current state is StateC_WRITE_CMD...

      Next_State = State2_WRITE_CMD; // Set the next state to StateC_WRITE_CMD.

      if(evt->data.evt_system_external_signal.extsignals == evt_Transfer_Done) // Check if the event includes a transfer done event.
      {
          sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1); // Remove energy mode requirement EM1.

          timerWaitUs_irq(10800); // Wait for a certain amount of time using a timer interrupt.

          Next_State = State3_WRITE_WAIT; // Set the next state to StateD_WRITE_WAIT.
      }

      break;

    case State3_WRITE_WAIT: // If the current state is StateD_WRITE_WAIT...

      Next_State = State3_WRITE_WAIT; // Set the next state to StateD_WRITE_WAIT.

      if(evt->data.evt_system_external_signal.extsignals == evt_COMP1) // Check if the event includes a comparison event from COMP1.
      {
          Read_i2c(); // Perform an I2C read operation.

          sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1); // Add energy mode requirement EM1.

          Next_State = State4_READ; // Set the next state to StateE_READ.
      }

      break;

    case State4_READ: // If the current state is StateE_READ...

      Next_State = State4_READ; // Set the next state to StateE_READ.

      if(evt->data.evt_system_external_signal.extsignals == evt_Transfer_Done) // Check if the event includes a transfer done event.
      {
          sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1); // Remove energy mode requirement EM1.

          NVIC_DisableIRQ(I2C0_IRQn); // Disable the I2C0 interrupt.

          SendTemp_ble();

          Next_State = State0_IDLE; // Set the next state to StateA_IDLE.
      }

      break;

    default: // If the current state is not recognized...

      LOG_ERROR("Not related to state machine\n\r"); // Log an error message indicating that the state is not recognized.

      break;
  }
}
return;
}

#else

void discovery_state_machine(sl_bt_msg_t *evt)
{
  // Declare current state variable
  My_State Current_State;

  // Initialize the next state variable with the idle client state
  static My_State Next_State = State0_IDLE_CLIENT;

  // Retrieve pointer to the BLE data structure
  ble_data_struct_t *ble_Data = get_ble_DataPtr();

  // Check if the event is a connection closed event
  if(SL_BT_MSG_ID(evt->header) == sl_bt_evt_connection_closed_id)
  {
      // Reset to idle client state upon disconnection
      Next_State = State0_IDLE_CLIENT;
  }

  // Update the current state based on the next state
  Current_State = Next_State;

  // Process states based on the current state
  switch(Current_State)
  {
    case State0_IDLE_CLIENT:
      // Default behavior is to stay in the idle state
      Next_State = State0_IDLE_CLIENT;

      // Check for a connection opened event to move forward
      if(SL_BT_MSG_ID(evt->header) == sl_bt_evt_connection_opened_id)
        {

          // Attempt to discover primary services with a specified UUID
          rc = sl_bt_gatt_discover_primary_services_by_uuid(ble_Data->Connection_Handle,
                                                            sizeof(thermometer_service),
                                                            (const uint8_t*)thermometer_service);
          // Check for errors in the discovery process
          if(rc != SL_STATUS_OK)
          {
              LOG_ERROR("Error in discovering primary services, status=0x%04x", (unsigned int)rc);
          }

          // Mark that a GATT procedure is in progress
          ble_Data->Gatt_Procedure = true;

          // Move to the next state to handle service discovery
          Next_State = State1_GOT_SERVICES;
      }
      break;
    case State0_GET_ANOTHER_SERVICE:

         Next_State = State0_GET_ANOTHER_SERVICE;          //default

         //wait for previous gatt command to be completed
         if(SL_BT_MSG_ID(evt->header) == sl_bt_evt_gatt_procedure_completed_id)
         {


             //LOG_INFO("Discovering services 2\n\r");

             rc = sl_bt_gatt_discover_primary_services_by_uuid(ble_Data->Connection_Handle,
                                                               sizeof(button_service),
                                                               (const uint8_t*)button_service);
             if(rc != SL_STATUS_OK) {
                 LOG_ERROR("sl_bt_gatt_discover_primary_services_by_uuid() 2 returned != 0 status=0x%04x\n\r", (unsigned int)rc);
             }

             //gatt command in process
             ble_Data->Gatt_Procedure = true;

             Next_State = State0_GET_ANOTHER_SERVICE;
         }
         break;
    case State1_GOT_SERVICES:
      // Remain in the current state until service discovery completes
      Next_State = State1_GOT_SERVICES;

      // Proceed when the previous GATT command is completed
      if(SL_BT_MSG_ID(evt->header) == sl_bt_evt_gatt_procedure_completed_id)
      {
          // Discover all characteristics for a service with the specified UUID
          rc = sl_bt_gatt_discover_characteristics_by_uuid(ble_Data->Connection_Handle,
                                                           ble_Data->Service_Handle,
                                                           sizeof(thermometer_charac),
                                                           (const uint8_t*)thermometer_charac);
          // Handle errors in characteristic discovery
          if(rc != SL_STATUS_OK)
          {
              LOG_ERROR("Error in discovering characteristics, status=0x%04x", (unsigned int)rc);
          }

          // Indicate a GATT procedure is in progress
          ble_Data->Gatt_Procedure = true;

          // Advance to the next state to handle characteristic discovery
          Next_State = State2_GOT_CHAR;
      }
      break;
   //discover button state characteristics
   case State1_GOT_ANOTHER_SERVICE:
          Next_State = State1_GOT_ANOTHER_SERVICE;

          //wait for previous gatt command to be completed
          if(SL_BT_MSG_ID(evt->header) == sl_bt_evt_gatt_procedure_completed_id)
          {


          // LOG_INFO("Discovering services 2\n\r");

          rc = sl_bt_gatt_discover_characteristics_by_uuid(ble_Data->Connection_Handle,
                                                            ble_Data->Button_Service_Handle,
                                                            sizeof(button_char),
                                                            (const uint8_t*)button_char);
          if(rc != SL_STATUS_OK) {
                LOG_ERROR("sl_bt_gatt_discover_characteristics_by_uuid() 2 returned != 0 status=0x%04x\n\r", (unsigned int)rc);
          }

          //gatt command in process
          ble_Data->Gatt_Procedure = true;

          Next_State = State2_GOT_CHAR;
         }

         break;
    case State2_GOT_CHAR:
      // Stay in current state until characteristic discovery completes
      Next_State = State2_GOT_CHAR;

      // Move forward when the GATT procedure is completed
      if(SL_BT_MSG_ID(evt->header) == sl_bt_evt_gatt_procedure_completed_id)
      {
          // Indicate a GATT procedure is in progress
          ble_Data->Gatt_Procedure = true;

          // Enable indications from the server for the discovered characteristic
          rc = sl_bt_gatt_set_characteristic_notification(ble_Data->Connection_Handle,
                                                          ble_Data->Char_Handle,
                                                          sl_bt_gatt_indication);
          // Log errors in setting characteristic notification
          if(rc != SL_STATUS_OK)
          {
              LOG_ERROR("Error setting characteristic notification, status=0x%04x", (unsigned int)rc);
          }

          // Update UI to reflect handling of indications
          displayPrintf(DISPLAY_ROW_CONNECTION, "Handling indications");
          // Transition to the state indicating server setup for indications
          Next_State = State3_SET_INDICATION;
      }
      break;
      //enble indications for button state service
  case State2_GOT_ANOTHER_CHAR:
      Next_State = State2_GOT_ANOTHER_CHAR;

      //wait for previous gatt command to be completed
      if(SL_BT_MSG_ID(evt->header) == sl_bt_evt_gatt_procedure_completed_id)
      {

          ///LOG_INFO("Enabling notifications 2\n\r");

          rc = sl_bt_gatt_set_characteristic_notification(ble_Data->Connection_Handle,
                                                          ble_Data->Button_Char_Handle,
                                                          sl_bt_gatt_indication);
          if(rc != SL_STATUS_OK) {
               LOG_ERROR("sl_bt_gatt_set_characteristic_notification() returned != 0 status=0x%04x\n\r", (unsigned int)rc);
          }

          //gatt command in process
          ble_Data->Gatt_Procedure = true;
          ble_Data->Button_Indication = true;

          displayPrintf(DISPLAY_ROW_CONNECTION, "Handling indications");
          Next_State = State3_SET_INDICATION;
       }
       break;

    case State3_SET_INDICATION:
      // Maintain current state until indication setup is verified
      Next_State = State3_SET_INDICATION;

      // Check if GATT procedure has completed
      if(SL_BT_MSG_ID(evt->header) == sl_bt_evt_gatt_procedure_completed_id)
      {
          // Prepare to wait for disconnection
          Next_State = State4_WAIT_FOR_CLOSE;
      }
      break;

    case State4_WAIT_FOR_CLOSE:
      // Stay in this state to await a connection close event
      Next_State = State4_WAIT_FOR_CLOSE;

      // Return to idle state on disconnection to await new connections
      if(SL_BT_MSG_ID(evt->header) == sl_bt_evt_connection_closed_id)
      {
          Next_State = State0_IDLE_CLIENT;
      }
      break;

    default:

      LOG_ERROR("Should not be here in state machine\n\r");

      break;

  }
}

#endif



void schedulerSetEventUF()
{
    // Declare a variable to store the IRQ state.
    CORE_DECLARE_IRQ_STATE;

    // Enter a critical section to prevent interrupt nesting.
    CORE_ENTER_CRITICAL();

    sl_bt_external_signal(evt_LETIMER0_UF);
    // Set the global variable my_event to indicate an underflow event.
    //my_event |= evt_LETIMER0_UF;

    // Exit the critical section.
    CORE_EXIT_CRITICAL();
} // schedulerSetEventUF()

//Sets the event for COMP1 in the scheduler.
void schedulerSetEventCOMP1()
{
    // Declare a variable to store the state of the interrupt.
    CORE_DECLARE_IRQ_STATE;

    // Enter a critical section to prevent nesting of interrupts.
    CORE_ENTER_CRITICAL();

    sl_bt_external_signal(evt_COMP1);
    // Set the global variable 'my_event' to indicate an underflow event for COMP1.
    //my_event |= evt_COMP1;

    // Exit the critical section.
    CORE_EXIT_CRITICAL();
} // End of schedulerSetEventCOMP1()


//Sets the event for transfer completion in the scheduler.
void schedulerSetEventTransferDone()
{
    // Declare a variable to store the state of the interrupt.
    CORE_DECLARE_IRQ_STATE;

    // Enter a critical section to prevent nesting of interrupts.
    CORE_ENTER_CRITICAL();

    sl_bt_external_signal(evt_Transfer_Done);
    // Set the global variable 'my_event' to indicate a transfer completion event.
    //my_event |= evt_Transfer_Done;

    // Exit the critical section.
    CORE_EXIT_CRITICAL();
} // End of schedulerSetEventTransferDone()

// scheduler routine to set a scheduler event
void schedulerSetEventButtonPressed()
{

  // enter critical section
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();

  sl_bt_external_signal(evt_Button_Pressed);
  // set the event in your data structure, this is a read-modify-write

  // exit critical section
  CORE_EXIT_CRITICAL();

} // schedulerSetEventXXX()

// scheduler routine to set a scheduler event
void schedulerSetEventButtonReleased()
{

  // enter critical section
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();

  sl_bt_external_signal(evt_Button_Released);
  // set the event in your data structure, this is a read-modify-write

  // exit critical section
  CORE_EXIT_CRITICAL();

} // schedulerSetEventXXX()


/*uint32_t getNextEvent()
{
    // Declare a variable to store the retrieved event.
    static uint32_t the_event = evt_no_event;


    // Declare a variable to store the IRQ state.
    CORE_DECLARE_IRQ_STATE;

    // Enter a critical section to prevent interrupt nesting.
    CORE_ENTER_CRITICAL();

    // Check if the event 'evt_LETIMER0_UF' has occurred
    if (my_event & evt_LETIMER0_UF)
    {
        // If the event has occurred, set 'the_event' variable to 'evt_LETIMER0_UF'
        the_event = evt_LETIMER0_UF;
        // Clear the 'evt_LETIMER0_UF' bit from 'my_event' using bitwise XOR operation
        my_event ^= evt_LETIMER0_UF;
    }
    // If 'evt_LETIMER0_UF' hasn't occurred, check if 'evt_COMP1' has occurred
    else if (my_event & evt_COMP1)
    {
        // If 'evt_COMP1' has occurred, set 'the_event' variable to 'evt_COMP1'
        the_event = evt_COMP1;
        // Clear the 'evt_COMP1' bit from 'my_event' using bitwise XOR operation
        my_event ^= evt_COMP1;
    }
    // If neither 'evt_LETIMER0_UF' nor 'evt_COMP1' has occurred, check if 'evt_Transfer_Done' has occurred
    else if (my_event & evt_Transfer_Done)
    {
        // If 'evt_Transfer_Done' has occurred, set 'the_event' variable to 'evt_Transfer_Done'
        the_event = evt_Transfer_Done;
        // Clear the 'evt_Transfer_Done' bit from 'my_event' using bitwise XOR operation
        my_event ^= evt_Transfer_Done;
    }


    // Exit the critical section.
    CORE_EXIT_CRITICAL();

    // Return the retrieved event.
    return (the_event);
} // getNextEvent() */




