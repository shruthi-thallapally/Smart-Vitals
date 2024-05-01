/*
 * timers.c
 *
 *  Created on: 31-Jan-2024
 *      Author: Shruthi Thallapally
 * Description: Implementation of functions for configuring and initializing the Low Energy Timer (LETIMER0)
 * Attribution: Code references are taken from professor's lecture slides
 */

#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"

#include "timers.h"        // Include the timers module header file
#include "stdlib.h"        // Include the standard library header file for using 'NULL'
#include "stdio.h" // Include the standard I/O library header file
#include "stdint.h"
#include "em_letimer.h"    // Include the Energy Micro Low Energy Timer header file
#include "app.h"           // Include the application header file
#include "src/oscillators.h"

#define CLOCK_RESOLUTION (LOWEST_ENERGY_MODE==3?1000:61)   // Clock resolution

#define MIN_WAIT ((LOWEST_ENERGY_MODE==3)?1000:61) // Minimum wait time range check
#define MAX_WAIT 3000000        //Maximum wait time range check


/*
 * Function: init_LETIMER0
 * -----------------------
 * Initializes the Low Energy Timer (LETIMER0) with the specified configuration based on the
 * lowest energy mode. Configures LETIMER0 to generate periodic interrupts and sets compare values.
 *
 * Parameters:
 *    None
 *
 * Returns:
 *    None
 */
void init_LETIMER0 ()
{

// Configure LETIMER0 initialization data structure with default values
const LETIMER_Init_TypeDef letimerInitData = {
false,
true,
true,
false,
0,
0,
letimerUFOANone,
letimerUFOANone,
letimerRepeatFree,
0
};

// Initialize LETIMER0 with the specified configuration
LETIMER_Init (LETIMER0, &letimerInitData);

LETIMER_CompareSet(LETIMER0,0,VALUE_TO_COMP0);  //In mode EM3 , UFLRCO is source, COMP0=562

// Enable/Start LETIMER0
LETIMER_Enable (LETIMER0, true);

LETIMER_IntEnable (LETIMER0, LETIMER_IEN_UF);

} // initLETIMER0 (

/**
 * @brief Waits for a specified duration in microseconds using the timer.
 *
 * @param us_wait The duration to wait in microseconds.
 * @note The function will wait for the specified duration using the LETIMER timer.
 */
void timerWaitUs_polled(uint32_t us_wait)
{
    uint16_t current_cnt = 0, req_cnt = 0, req_ticks = 0;

    // Calculate the required number of ticks for the given duration
    req_ticks = (us_wait / CLOCK_RESOLUTION);

    // Get the current counter value
    current_cnt = LETIMER_CounterGet(LETIMER0);

    // Calculate the required counter value to reach the desired wait duration
    req_cnt = current_cnt - req_ticks;

    // Check if the current counter value is greater than or equal to the required ticks
    if (current_cnt >= req_ticks)
    {
        // Wait until the current counter reaches the required counter value
        while ((LETIMER_CounterGet(LETIMER0)) != (req_cnt));
    }
    else
    {
        // Calculate the required counter value in case of counter overflow
        while ((LETIMER_CounterGet(LETIMER0)) != (uint32_t)(VALUE_TO_COMP0 - (req_ticks - current_cnt)));
    }
}

/**
 * @brief Waits for a specified duration in microseconds using the LETIMER peripheral with interrupts.
 * @param us_wait The duration to wait in microseconds.
 *        It should be within the range [MIN_WAIT, MAX_WAIT].
 */
void timerWaitUs_irq(uint32_t us_wait)
{
  uint16_t current_cnt = 0;   // Current counter value
  uint16_t req_cnt = 0;       // Required counter value
  uint16_t req_ticks = 0;     // Required number of ticks for the given duration

     // Check if the specified duration is within the valid range
  if ((us_wait < (uint32_t)MIN_WAIT) || (us_wait > (uint32_t)MAX_WAIT))
    {
     LOG_INFO("TimerWait range\n\r");

         // Adjust the specified duration to the valid range
     if (us_wait > (uint32_t)MAX_WAIT)
       {
           us_wait = MAX_WAIT;
       }
     else if (us_wait < (uint32_t)MIN_WAIT)
       {
           us_wait = MIN_WAIT;
       }
     }

     // Calculate the required number of ticks for the given duration
     req_ticks = (us_wait / CLOCK_RESOLUTION);

     // Get the current counter value
     current_cnt = LETIMER_CounterGet(LETIMER0);

     // Calculate the required counter value to reach the desired wait duration
     req_cnt = current_cnt - req_ticks;

     // Handle counter overflow
     if (req_cnt > VALUE_TO_COMP0)
     {
         req_cnt = VALUE_TO_COMP0 - (0xFFFF - req_cnt);
     }

     LETIMER_IntClear(LETIMER0, LETIMER_IFC_COMP1);
     // Set the compare value to trigger interrupt after the specified duration
     LETIMER_CompareSet(LETIMER0, 1, req_cnt);

     // Enable COMP1 interrupt
     LETIMER_IntEnable(LETIMER0, LETIMER_IEN_COMP1);

     // Enable COMP1 interrupt in LETIMER0->IEN register
     LETIMER0->IEN |= LETIMER_IEN_COMP1;

}
