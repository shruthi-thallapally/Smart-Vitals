/*
 * timers.c
 *
 *  Created on: Jan 28, 2024
 *      Author: Tharuni Gelli
 */

#define INCLUDE_LOG_DEBUG 1 // Including logging for this file
#include "src/log.h"

#include "src/timers.h"

// Define ACTUAL_CLK_FREQ as the result of calling the required_oscillator() function
#define ACTUAL_CLK_FREQ required_oscillator()

// Calculate the value to load into LETIMER_COMP0 register
// based on the LETIMER_PERIOD_MS and ACTUAL_CLK_FREQ, then convert it to microseconds
#define VALUE_TO_LOAD_COMP0 (LETIMER_PERIOD_MS * ACTUAL_CLK_FREQ) / 1000

// Calculate the value to load into LETIMER_COMP1 register
// based on the difference between LETIMER_PERIOD_MS and LETIMER_ON_TIME_MS,
// and ACTUAL_CLK_FREQ, then convert it to microseconds
#define VALUE_TO_LOAD_COMP1 ((LETIMER_PERIOD_MS - LETIMER_ON_TIME_MS) * ACTUAL_CLK_FREQ) / 1000


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
        false,                   // Start counting when initialized
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

    // Set the value of COMP1 in LETIMER0
    LETIMER_CompareSet(LETIMER0, 1, VALUE_TO_LOAD_COMP1);

    // Enable LETIMER0
    LETIMER_Enable (LETIMER0, true);
} // init_LETIMER0 ()












