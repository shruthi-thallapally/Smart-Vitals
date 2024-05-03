/***************************************************************************//**
 * @file
 * @brief Core application logic.
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * Date:        02-25-2022
 * Author:      Dave Sluiter
 * Description: This code was created by the Silicon Labs application wizard
 *              and started as "Bluetooth - SoC Empty".
 *              It is to be used only for ECEN 5823 "IoT Embedded Firmware".
 *              The MSLA referenced above is in effect.
 *
 *
 *
 * Student edit: Add your name and email address here:
 * @student    Shruthi Thallapally, Shruthi.thallapally@Colorado.edu
 *
 *
 *
 ******************************************************************************/
#include "em_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "src/scheduler.h"
#include "src/ble_device_type.h"
#include "app.h"

#include "src/oscillators.h"
#include "src/timers.h"


#include "src/irq.h"
#include "src/i2c.h"
#include "src/ble.h"
#include "em_letimer.h"

// *************************************************
// Students: It is OK to modify this file.
//           Make edits appropriate for each
//           assignment.
// *************************************************

#include "sl_status.h"             // for sl_status_print()


#include "src/gpio.h"
#include "src/lcd.h"


// Students: Here is an example of how to correctly include logging functions in
//           each .c file.
//           Apply this technique to your other .c files.
//           Do not #include "src/log.h" in any .h file! This logging scheme is
//           designed to be included at the top of each .c file that you want
//           to call one of the LOG_***() functions from.

// Include logging specifically for this .c file
#define INCLUDE_LOG_DEBUG 1

#include "src/log.h"


// *************************************************
// Power Manager
// *************************************************

// See: https://docs.silabs.com/gecko-platform/latest/service/power_manager/overview
#if defined(SL_CATALOG_POWER_MANAGER_PRESENT)

// -----------------------------------------------------------------------------
// defines for power manager callbacks
// -----------------------------------------------------------------------------
// Return values for app_is_ok_to_sleep():
//   Return false to keep sl_power_manager_sleep() from sleeping the MCU.
//   Return true to allow system to sleep when you expect/want an IRQ to wake
//   up the MCU from the call to sl_power_manager_sleep() in the main while (1)
//   loop.
//
// Students: We'll need to modify this for A2 onward so that compile time we
//           control what the lowest EM (energy mode) the MCU sleeps to. So
//           think "#if (expression)".
#if defined(EM0)
  #define APP_IS_OK_TO_SLEEP      (false)
#else
  #define APP_IS_OK_TO_SLEEP      (true)
#endif

// Return values for app_sleep_on_isr_exit():
//   SL_POWER_MANAGER_IGNORE; // The module did not trigger an ISR and it doesn't want to contribute to the decision
//   SL_POWER_MANAGER_SLEEP;  // The module was the one that caused the system wakeup and the system SHOULD go back to sleep
//   SL_POWER_MANAGER_WAKEUP; // The module was the one that caused the system wakeup and the system MUST NOT go back to sleep
//
// Notes:
//       SL_POWER_MANAGER_IGNORE, we see calls to app_process_action() on each IRQ. This is the
//       expected "normal" behavior.
//
//       SL_POWER_MANAGER_SLEEP, the function app_process_action()
//       in the main while(1) loop will not be called! It would seem that sl_power_manager_sleep()
//       does not return in this case.
//
//       SL_POWER_MANAGER_WAKEUP, doesn't seem to allow ISRs to run. Main while loop is
//       running continuously, flooding the VCOM port with printf text with LETIMER0 IRQs
//       disabled somehow, LED0 is not flashing.

#define APP_SLEEP_ON_ISR_EXIT   (SL_POWER_MANAGER_IGNORE)
//#define APP_SLEEP_ON_ISR_EXIT   (SL_POWER_MANAGER_SLEEP)
//#define APP_SLEEP_ON_ISR_EXIT   (SL_POWER_MANAGER_WAKEUP)

#endif // defined(SL_CATALOG_POWER_MANAGER_PRESENT)




// *************************************************
// Power Manager Callbacks
// The values returned by these 2 functions AND
// adding and removing power manage requirements is
// how we control when EM mode the MCU goes to when
// sl_power_manager_sleep() is called in the main
// while (1) loop.
// *************************************************

#if defined(SL_CATALOG_POWER_MANAGER_PRESENT)

bool app_is_ok_to_sleep(void)
{
  return APP_IS_OK_TO_SLEEP;
} // app_is_ok_to_sleep()

sl_power_manager_on_isr_exit_t app_sleep_on_isr_exit(void)
{
  return APP_SLEEP_ON_ISR_EXIT;
} // app_sleep_on_isr_exit()

#endif // defined(SL_CATALOG_POWER_MANAGER_PRESENT)




/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
/*
 * Function: app_init
 * -------------------
 * Initializes the application by performing 1-time setup tasks.
 * This function is called once during start-up. It includes calls to initialize GPIO,
 * turn off LED, configure oscillators, and initialize the Low Energy Timer (LETIMER0).
 * Additionally, it clears and enables the LETIMER0 interrupt in the NVIC (Nested Vector Interrupt Controller).
 *
 * Parameters:
 *    None
 *
 * Returns:
 *    None
 */
SL_WEAK void app_init(void)
{
  // Put your application 1-time initialization code here.
  // This is called once during start-up.
  // Don't call any Bluetooth API functions until after the boot event.

  // Student Edit: Add a call to gpioInit() here

  // Initialize GPIO
  gpioInit();

  // Initialize oscillators for clock sources
  init_oscillator();
  // Initialize Low Energy Timer (LETIMER0)
  init_LETIMER0();

  Init_i2c();


#if (LOWEST_ENERGY_MODE > 2)
     LOWEST_ENERGY_MODE = 2;
#endif

   // Add energy mode requirements based on LOWEST_ENERGY_MODE
   if((LOWEST_ENERGY_MODE >0) & (LOWEST_ENERGY_MODE < 3))
     {
         if(LOWEST_ENERGY_MODE == 1)
         {
             sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1); // adding a power requirement for EM1
         }
         else if(LOWEST_ENERGY_MODE == 2)
         {
             sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM2); // adding a power requirement for EM2
         }
     }
  // Clear pending LETIMER0 and I2C0 interrupts in NVIC
  NVIC_ClearPendingIRQ(LETIMER0_IRQn);
  // Enable LETIMER0 and I2C0 interrupts interrupt in NVIC
  NVIC_EnableIRQ(LETIMER0_IRQn);

  NVIC_ClearPendingIRQ(GPIO_EVEN_IRQn);
  NVIC_EnableIRQ(GPIO_EVEN_IRQn);

  NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
  NVIC_EnableIRQ(GPIO_ODD_IRQn);
} // app_init()




/*****************************************************************************
 * delayApprox(), private to this file.
 * A value of 3500000 is ~ 1 second. After assignment 1 you can delete or
 * comment out this function. Wait loops are a bad idea in general.
 * We'll discuss how to do this a better way in the next assignment.
 *****************************************************************************/
//static void delayApprox(int delay)
//{
//  volatile int i;
//
//  for (i = 0; i < delay; ) {
//      i=i+1;
//  }
//
//} // delayApprox()





/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
SL_WEAK void app_process_action(void)
{
  // Put your application code here for A1 to A4.
  // This is called repeatedly from the main while(1) loop
  // Notice: This function is not passed or has access to Bluetooth stack events.
  //         We will create/use a scheme that is far more energy efficient in
  //         later assignments.
//  uint32_t evt;
//  evt = getNextEvent();
//
//  statemachine(evt);


} // app_process_action()





/**************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *
 * The code here will process events from the Bluetooth stack. This is the only
 * opportunity we will get to act on an event.
 *
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  // Get pointer to BLE data structure
   ble_data_struct_t *bleData = getBleDataPtr();

  // Just a trick to hide a compiler warning about unused input parameter evt.
  (void) evt;

  // For A5 onward:
  // Some events require responses from our application code,
  // and don’t necessarily advance our state machines.
  // For A5 uncomment the next 2 function calls
  handle_ble_event(evt); // put this code in ble.c/.h

  // sequence through states driven by events
#if DEVICE_IS_BLE_SERVER
  //FOR SERVER
  // sequence through states driven by events
  gesture_state_machine(evt);
 if((bleData->gesture_value == 0x01) || (bleData->gesture_value == 0x02)){
     bleData->pulse_on = true;
      oximeter_state_machine(evt);
  }

  if(bleData->gesture_value==0x03)
  {
          // LOG_INFO("Down\n\r");

           temp_state_machine(evt);
  }

#else
  //FOR CLIENT
  discovery_state_machine(evt);

#endif


} // sl_bt_on_event()
