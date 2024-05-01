/*
 * oscillators.c
 *
 *  Created on: 31-Jan-2024
 *      Author: Shruthi thallapally
 * Description: Initialization of oscillators for different energy modes
 * Attribution: Code references are taken from professor's lecture slides
 */
#include "src/oscillators.h"  // Include the header file for the oscillators module
#include "em_letimer.h"       // Include the Energy Micro Low Energy Timer header file
#include "app.h"              // Include the application header file
#include "em_cmu.h"           // Include the Energy Micro Clock Management Unit header file



/*
 * Function: init_oscillator
 * -------------------------
 * Initializes the oscillator settings based on the lowest energy mode.
 * Configures the Low-Frequency Crystal Oscillator (LFXO) or Ultra-Low Frequency
 * RC Oscillator (ULFRCO) and sets the clock source for Low Energy Timer (LETIMER0).
 *
 * Parameters:
 *    None
 *
 * Returns:
 *    None
 */

void init_oscillator()
{
  // Check if the lowest energy mode is less than 3
  if(LOWEST_ENERGY_MODE <3)
  {
      // Enable the Low-Frequency Crystal Oscillator (LFXO) with low power mode
      CMU_OscillatorEnable(cmuOsc_LFXO, true, true);
      // Select LFXO as the clock source for Low-Frequency Clock A (LFA)
      CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFXO);
      // Set the clock division for Low Energy Timer 0 (LETIMER0)
      CMU_ClockDivSet(cmuClock_LETIMER0,cmuClkDiv_2);
      // Enable the clock for Low Energy Timer 0 (LETIMER0)
      CMU_ClockEnable(cmuClock_LETIMER0, true);
  }

  // Check if the lowest energy mode is 3
  if(LOWEST_ENERGY_MODE==3)
  {
      // Enable the Ultra-Low Frequency RC Oscillator (ULFRCO) with low power mode
      CMU_OscillatorEnable(cmuOsc_ULFRCO, true, true);
      // Select ULFRCO as the clock source for Low-Frequency Clock A (LFA)
      CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_ULFRCO);
      // Set the clock division for Low Energy Timer 0 (LETIMER0)
      CMU_ClockDivSet(cmuClock_LETIMER0,cmuClkDiv_1);
      // Enable the clock for Low Energy Timer 0 (LETIMER0)
      CMU_ClockEnable(cmuClock_LETIMER0, true);
  }
}
