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
#include "src/irq.h"

// Function definition for the LETIMER0 interrupt handler
void LETIMER0_IRQHandler(void)
{
  // Declare a variable to store the state of the CPU interrupt and enter a critical section
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();

  // Read the pending interrupt flags and store them in the 'value' variable
  uint32_t value = LETIMER_IntGetEnabled(LETIMER0);

  // Clear the pending interrupts for the specified flags
  LETIMER_IntClear(LETIMER0, value);

  // Exit the critical section
  CORE_EXIT_CRITICAL();

  // Check if the interrupt was triggered by a specific event (value==2)
  if(value == 2)
  {
    // Call a function to turn off LED 0
    gpioLed0SetOff();
  }

  // Check if the interrupt was triggered by another specific event (value==4)
  else if(value == 4)
  {
    // Call a function to turn on LED 0
    gpioLed0SetOn();
  }
}

