/*
 * irq.c
 *
 *  Created on: Jan 29, 2024
 *      Author: Tharuni Gelli
 *
 *
 */

// Include logging for this file
#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"

#include "app.h"
#include "em_i2c.h"


uint32_t letimerMilliseconds()  // Define a function named letimerMilliseconds returning an unsigned 32-bit integer.
{
  uint32_t time_ms;  // Declare a variable to store the time in milliseconds.
  ble_data_struct_t *ble_Data = get_ble_DataPtr();  // Calculate the time in milliseconds based on the rollover value, the value to load into the comparator, and the current counter value.
  time_ms = ((ble_Data->Rollover_Count)*3000);
  return time_ms;  // Return the calculated time in milliseconds.
}


// Function definition for the LETIMER0 interrupt handler
void LETIMER0_IRQHandler(void)
{
    ble_data_struct_t *ble_Data = get_ble_DataPtr();

    // Retrieve the enabled interrupt flags from LETIMER0.
    uint32_t value = LETIMER_IntGetEnabled(LETIMER0);

    // Clear the enabled interrupt flags in LETIMER0.
    LETIMER_IntClear(LETIMER0, value);

    if(value & LETIMER_IF_COMP1)
    {
        // Check if the COMP1 interrupt flag is set
        LETIMER_IntDisable(LETIMER0, LETIMER_IEN_COMP1); // Disable COMP1 interrupt
        schedulerSetEventCOMP1(); // Set an event for COMP1 in the scheduler
    }
    if(value & LETIMER_IF_UF)
    {
        // Check if the UF (underflow) interrupt flag is set
        schedulerSetEventUF(); // Set an event for underflow in the scheduler

        // Increment rollover_value
        ble_Data->Rollover_Count+=1;

    }

}



// This function is the interrupt handler for the I2C0 peripheral.
// It is called when an I2C transfer is complete or encounters an error.

void I2C0_IRQHandler(void)
{
  // Variable to store the status of the I2C transfer
  I2C_TransferReturn_TypeDef Transfer_Status;

  // Perform an I2C transfer and store the status
  Transfer_Status = I2C_Transfer(I2C0);

  // Check if the transfer was successful
  if(Transfer_Status == i2cTransferDone)
  {
    // Disable the I2C0 interrupt to prevent re-entry while processing
    NVIC_DisableIRQ(I2C0_IRQn);

    // Notify the scheduler that the transfer is done
    schedulerSetEventTransferDone();
  }

  // Check if there was an error during the transfer
  if(Transfer_Status < 0)
  {
    // Log the error with the status code
    LOG_ERROR("I2C_TStatus %d : failed\n\r", (uint32_t)Transfer_Status);
  }
}

void GPIO_EVEN_IRQHandler(void)
{
  ble_data_struct_t *ble_Data = get_ble_DataPtr();

  // determine pending interrupts in peripheral
  uint32_t value = GPIO_IntGet();

  GPIO_IntClear(value);

  //get the push button status
  uint8_t Button_Status = GPIO_PinInGet(BUTTON_PORT, PB0_PIN);

  //check if the interrupt triggered was from PB0
  if(value == 64)
  {
      if(!Button_Status)
      {
          ble_Data->Button_Pressed = true;
          schedulerSetEventButtonPressed();
      }

      else
      {
          ble_Data->Button_Pressed = false;
          schedulerSetEventButtonReleased();
      }
  }
}

#if !DEVICE_IS_BLE_SERVER

void GPIO_ODD_IRQHandler(void) {

  ble_data_struct_t *ble_Data = get_ble_DataPtr();

  // determine pending interrupts in peripheral
  uint32_t value = GPIO_IntGet();

  GPIO_IntClear(value);

  //get the push button status
  uint8_t button_status = GPIO_PinInGet(BUTTON_PORT, PB1_PIN);

  //check if the interrupt triggered was from PB1
  if(value == 128)
  {

      if(!button_status)
      {
          ble_Data->PB1_Button_Pressed = true;
          schedulerSetEventButtonPressed();
      }

      else
      {
          ble_Data->PB1_Button_Pressed = false;
          schedulerSetEventButtonReleased();
      }
  }
}

#endif


