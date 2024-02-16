/*
 * timers.c
 *
 *  Created on: Jan 28, 2024
 *      Author: Tharuni Gelli
 */

#define INCLUDE_LOG_DEBUG 1 // Including logging for this file
#include "src/log.h"

#include "src/timers.h"


//LETIMER macros for range check limit

#if (LOWEST_ENERGY_MODE < 3)
#define CLK_RSL 61                             // Clock resolution in EM0,1,2
#define MIN_WAIT_TIME 61                       // Minimum wait time range check in EM0,1,2

#else
#define CLK_RSL 1000                           // Clock resolution in EM3
#define MIN_WAIT_TIME 1000                     // Minimum wait time range check in EM3
#endif

#define MAX_WAIT_TIME 3000000                  // Maximum wait time range check in EM0,1,2,3



/*
 * Function: void init_LETIMER0 ()
 * ----------------------------------------------
 * Initializes the LETIMER0 peripheral with the specified configuration.
 */
void init_LETIMER0 ()
{
    // Configuration structure for LETIMER initialization
    const LETIMER_Init_TypeDef letimerInitData =
    {
        false,                  // Enable or disable the LETIMER after initialization
        true,                   // Start counting when initialized
        true,                   // Counter continues while the CPU is halted in debug mode
        false,                  // Debug mode will not affect outputs
        0,                      // Counter value to load into COMP0 after overflow
        0,                      // Counter value to load into COMP1 after overflow
        letimerUFOANone,       // Action to be taken on underflow in the LETIMER0
        letimerUFOANone,       // Action to be taken on underflow in the LETIMER1
        letimerRepeatFree,      // Repeat mode for the LETIMER
        0                       // Output mode for the LETIMER0
    };

    // Initialize LETIMER0 with the specified configuration
    LETIMER_Init (LETIMER0, &letimerInitData);

    // Set the value of COMP0 in LETIMER0
    LETIMER_CompareSet(LETIMER0, 0, VALUE_TO_LOAD_COMP0);

    // Enable LETIMER0
    LETIMER_Enable (LETIMER0, true);

    //Timer peripheral underflow interrupt enable
    LETIMER_IntEnable(LETIMER0, LETIMER_IEN_UF);

} // init_LETIMER0 ()


// Function to make wait for a specified duration in microseconds using a letimer0.
void timerWaitUs_polled(uint32_t us_wait)
{
    // Declare variables for current counter value, required counter value,
    // and required ticks for the given microsecond duration.
    uint16_t current_cnt, required_cnt, required_tick;

    // Check if the input duration is within the acceptable range.
    if((us_wait < (uint32_t)MIN_WAIT_TIME) | (us_wait > (uint32_t)MAX_WAIT_TIME))
    {
        // Log an error message if the input duration is out of range.
        LOG_ERROR("TimerWait range\n\r");

        // Adjust the input duration to the minimum or maximum allowed value if it's out of range.
        if(us_wait < (uint16_t)MIN_WAIT_TIME)
        {
            us_wait = MIN_WAIT_TIME;
        }
        else if(us_wait > (uint16_t)MAX_WAIT_TIME)
        {
            us_wait = MAX_WAIT_TIME;
        }
    }

    // Calculate the required number of ticks for the given microsecond duration.
    required_tick = (us_wait / CLK_RSL);

    // Get the current counter value from the timer.
    current_cnt = LETIMER_CounterGet(LETIMER0);

    // Calculate the required counter value for the timer.
    required_cnt = current_cnt - required_tick;

    // Check if the current counter value is greater than or equal to the required ticks.
    if(current_cnt >= required_tick)
    {
        // Wait until the timer counter reaches the required count.
        while((LETIMER_CounterGet(LETIMER0)) != (required_cnt));
    }
    else
    {
        // Wait until the timer counter reaches the adjusted required count,
        // considering the overflow scenario.
        while((LETIMER_CounterGet(LETIMER0)) != (uint32_t)(VALUE_TO_LOAD_COMP0 - (required_tick - current_cnt)));
    }
}


void timerWaitUs_irq(uint32_t us_wait)
{
  uint16_t current_cnt, required_cnt, required_tick; // Declare variables for current counter, required counter, and required tick.

  // Check if the wait time is within the acceptable range.
  if((us_wait<(uint32_t)MIN_WAIT_TIME) | (us_wait>(uint32_t)MAX_WAIT_TIME))
  {
      LOG_ERROR("TimerWait range\n\r"); // Log an error message if the wait time is out of range.

      // Adjust the wait time to the minimum if it's below the minimum threshold.
      if(us_wait < (uint32_t)MIN_WAIT_TIME)
      {
          us_wait = MIN_WAIT_TIME;
      }

      // Adjust the wait time to the maximum if it's above the maximum threshold.
      else if(us_wait > (uint32_t)MAX_WAIT_TIME)
      {
          us_wait = MAX_WAIT_TIME;
      }
  }

  // Calculate the number of timer ticks required for the given wait time.
  required_tick = (us_wait/CLK_RSL);

  // Get the current count value of the LETIMER.
  current_cnt = LETIMER_CounterGet(LETIMER0);

  // Calculate the required counter value by subtracting the required ticks from the current count.
  required_cnt = current_cnt - required_tick;

  // Handle wrap-around case if the required count is greater than the maximum value.
  if(required_cnt > VALUE_TO_LOAD_COMP0)
  {
      required_cnt = VALUE_TO_LOAD_COMP0 - (0xFFFF - required_cnt);
  }

  // Set the compare value for the COMP1 register to trigger an interrupt.
  LETIMER_CompareSet(LETIMER0, 1, required_cnt);

  // Enable the COMP1 interrupt in the LETIMER module.
  LETIMER_IntEnable(LETIMER0, LETIMER_IEN_COMP1);

  // Enable the COMP1 interrupt in the LETIMER0 interrupt enable register.
  LETIMER0->IEN |= LETIMER_IEN_COMP1;
}











