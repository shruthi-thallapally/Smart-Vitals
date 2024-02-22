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
};


typedef enum uint32_t // Define a new enumeration type with a fixed size of 32 bits
{
  StateA_IDLE, // Define the first enumeration constant, representing state A being idle
  StateB_TIMER_WAIT, // Define the second enumeration constant, representing state B waiting for a timer
  StateC_WRITE_CMD, // Define the third enumeration constant, representing state C writing a command
  StateD_WRITE_WAIT, // Define the fourth enumeration constant, representing state D waiting for a write operation
  StateE_READ // Define the fifth enumeration constant, representing state E reading
} My_State; // Name the enumeration type as My_State


void state_machine(sl_bt_msg_t *evt)
{
  My_State Current_State; // Declare a variable to hold the current state of the state machine.

  static My_State Next_State = StateA_IDLE; // Declare a static variable to hold the next state, initialized to StateA_IDLE.

  ble_data_struct_t *ble_Data = get_ble_DataPtr();

  if((SL_BT_MSG_ID(evt->header)==sl_bt_evt_system_external_signal_id)
       && (ble_Data->Connected==true)
       && (ble_Data->Indication==true))
  {

  Current_State = Next_State; // Set the current state to the next state.

  switch(Current_State) // Start a switch statement based on the current state.

  {

    case StateA_IDLE: // If the current state is StateA_IDLE...

      Next_State = StateA_IDLE; // Set the next state to StateA_IDLE.

      if(evt->data.evt_system_external_signal.extsignals == evt_LETIMER0_UF) // Check if the event includes LETIMER0 underflow.
      {
          sensor_enable(); // Enable the sensor.

          timerWaitUs_irq(80000); // Wait for a certain amount of time using a timer interrupt.

          Next_State = StateB_TIMER_WAIT; // Set the next state to StateB_TIMER_WAIT.
      }

      break;

    case StateB_TIMER_WAIT: // If the current state is StateB_TIMER_WAIT...

      Next_State = StateB_TIMER_WAIT; // Set the next state to StateB_TIMER_WAIT.

      if(evt->data.evt_system_external_signal.extsignals == evt_COMP1) // Check if the event includes a comparison event from COMP1.
      {

          sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1); // Add energy mode requirement EM1.

          Write_i2c(); // Perform an I2C write operation.

          Next_State = StateC_WRITE_CMD; // Set the next state to StateC_WRITE_CMD.
      }

      break;

    case StateC_WRITE_CMD: // If the current state is StateC_WRITE_CMD...

      Next_State = StateC_WRITE_CMD; // Set the next state to StateC_WRITE_CMD.

      if(evt->data.evt_system_external_signal.extsignals == evt_Transfer_Done) // Check if the event includes a transfer done event.
      {
          sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1); // Remove energy mode requirement EM1.

          timerWaitUs_irq(10800); // Wait for a certain amount of time using a timer interrupt.

          Next_State = StateD_WRITE_WAIT; // Set the next state to StateD_WRITE_WAIT.
      }

      break;

    case StateD_WRITE_WAIT: // If the current state is StateD_WRITE_WAIT...

      Next_State = StateD_WRITE_WAIT; // Set the next state to StateD_WRITE_WAIT.

      if(evt->data.evt_system_external_signal.extsignals == evt_COMP1) // Check if the event includes a comparison event from COMP1.
      {
          Read_i2c(); // Perform an I2C read operation.

          sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1); // Add energy mode requirement EM1.

          Next_State = StateE_READ; // Set the next state to StateE_READ.
      }

      break;

    case StateE_READ: // If the current state is StateE_READ...

      Next_State = StateE_READ; // Set the next state to StateE_READ.

      if(evt->data.evt_system_external_signal.extsignals == evt_Transfer_Done) // Check if the event includes a transfer done event.
      {
          sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1); // Remove energy mode requirement EM1.

          sensor_disable(); // Disable the sensor.

          NVIC_DisableIRQ(I2C0_IRQn); // Disable the I2C0 interrupt.

          SendTemp_ble();

          Next_State = StateA_IDLE; // Set the next state to StateA_IDLE.
      }

      break;

    default: // If the current state is not recognized...

      LOG_ERROR("Not related to state machine\n\r"); // Log an error message indicating that the state is not recognized.

      break;
  }
}
return;
}



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




