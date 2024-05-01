/*
 * irq.c
 *
 *  Created on: 31-Jan-2024
 *      Author: Shruthi Thallapally
 * Description: Interrupt Service Routine (ISR) for the Low Energy Timer (LETIMER0)
 * Note: Sometimes warnings are generated due to the log statements.
 *   I have commented LOG_ERROR statements to avoid generating unwanted warnings
 */

#define INCLUDE_LOG_DEBUG 1

#include "src/log.h"          // Include the log module header file
#include "src/irq.h"          // Include the interrupt request (IRQ) module header file
#include "em_letimer.h"       // Include the Energy Micro Low Energy Timer header file
#include "src/gpio.h"             // Include the general-purpose I/O (GPIO) header file
#include "src/scheduler.h"
#include "src/timers.h"
#include "src/oscillators.h"
#include "em_i2c.h"
#include "app.h"
#include "ble.h"
#include "sl_i2cspm.h"
#include "ble_device_type.h"

/*
 * Function: LETIMER0_IRQHandler
 * ------------------------------
 * Interrupt Service Routine (ISR) for the Low Energy Timer (LETIMER0).
 * Clears the pending interrupts, handles specific interrupt flags, and performs
 * corresponding actions, such as toggling an LED.
 *
 * Parameters:
 *    None
 *
 * Returns:
 *    None
 */

/**
 * @brief Interrupt handler for LETIMER0 peripheral.
 *        Handles COMP1 and UF interrupts.
 */
void LETIMER0_IRQHandler(void)
{
  ble_data_struct_t *bleData = getBleDataPtr();

  // Get the enabled interrupt flags
      uint32_t flag = LETIMER_IntGetEnabled(LETIMER0);

      // Clear the interrupt flags
      LETIMER_IntClear(LETIMER0, flag);

      // Check if the COMP1 (bit 1) interrupt flag is set
      if (LETIMER_IF_COMP1)
        {
          // Disable COMP1 interrupt
          LETIMER_IntDisable(LETIMER0, LETIMER_IEN_COMP1);
          // Set an event for the scheduler
          schedulerSetEventCOMP1();
      }

      // Check if the UF (bit 2) interrupt flag is set
      if (LETIMER_IF_UF)
        {
          // Set an event for the scheduler
          schedulerSetEventUF();

          bleData->rollover_cnt+=1;
      }
}

/**
 * @brief Interrupt handler for I2C0 peripheral.
 *        Handles I2C transfer completion.
 */
void I2C0_IRQHandler(void)
{
  // Perform I2C transfer and get transfer status
      I2C_TransferReturn_TypeDef Trans_Status = I2C_Transfer(I2C0);

      // Check if the transfer is done successfully
      if (Trans_Status == i2cTransferDone)
        {
          // Disable I2C0 interrupt
          NVIC_DisableIRQ(I2C0_IRQn);
          // Set an event for the scheduler
          schedulerSetEventI2Ccomplete();
      }

      // Check if there was an error during I2C transfer
      if (Trans_Status < 0)
        {
          // Log the error
          LOG_ERROR("I2C_TransferStatus %d : failed\n\r", (uint32_t)Trans_Status);
      }
}

void GPIO_EVEN_IRQHandler(void)
{
  ble_data_struct_t *bleData = getBleDataPtr();

  uint32_t flag=GPIO_IntGetEnabled();

  GPIO_IntClear(flag);

  uint8_t button_status = GPIO_PinInGet(BUTTON_PORT,PB0_BUTTON_PIN);

  if(flag==64)
  {
     if (button_status == 0)
       {
         bleData->button_pressed = true;
         schedulerSetEventButtonPressed();
       }
     else
       {
         bleData->button_pressed = false;
         schedulerSetEventButtonReleased();
       }

  }

}

void GPIO_ODD_IRQHandler(void)
{
  ble_data_struct_t *bleData = getBleDataPtr();

  uint32_t flag=GPIO_IntGetEnabled();

  GPIO_IntClear(flag);

#if DEVICE_IS_BLE_SERVER

  if(flag == 2048)
    schedulerSetGestureEvent();

#endif

  uint8_t button_status = GPIO_PinInGet(BUTTON_PORT,PB1_BUTTON_PIN);

  if(flag==128)
  {

     if (button_status == 0)
       {
         bleData->PB1_button_pressed = true;
         schedulerSetEventButtonPressed();
       }
     else
       {
         bleData->PB1_button_pressed = false;
         schedulerSetEventButtonReleased();
       }

  }

}

/**
 * @brief Calculates the elapsed time in milliseconds using LETIMER0 counter value.
 * @return The elapsed time in milliseconds.
 */
uint32_t letimerMilliseconds()
{
  uint32_t time_ms;
  ble_data_struct_t *ble_data;
  ble_data= getBleDataPtr();
  time_ms = ((ble_data->rollover_cnt)*3000);
  return time_ms;
}
