/*
 * oscillators.c
 *
 *  Created on: Jan 28, 2024
 *      Author: Tharuni Gelli
 */

#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"

#include "src/oscillators.h"
#include "app.h"


// Define the frequency (in Hertz) for the Ultra Low-Frequency RC Oscillator (ULFRCO)
#define ULFRCO_FREQ 1000

// Define the prescaler value for the Ultra Low-Frequency RC Oscillator (ULFRCO)
#define ULFRCO_PRESCALER 1

// Define the frequency (in Hertz) for the Low-Frequency Crystal Oscillator (LFXO)
#define LFXO_FREQ 32768

// Define the prescaler value for the Low-Frequency Crystal Oscillator (LFXO)
#define LFXO_PRESCALER 4



// Function to determine the required oscillator frequency based on the lowest energy mode
int required_oscillator()
{
  // Check if the lowest energy mode is set to 3
  if (LOWEST_ENERGY_MODE == 3)
  {
      // Return the calculated frequency for the Ultra Low-Frequency RC Oscillator (ULFRCO)
      return (ULFRCO_FREQ / ULFRCO_PRESCALER);
  }
  // If the lowest energy mode is not 3
  else
  {
      // Return the calculated frequency for the Low-Frequency Crystal Oscillator (LFXO)
      return (LFXO_FREQ / LFXO_PRESCALER);
  }
}


// Function to initialize the oscillator based on the LOWEST_ENERGY_MODE
void init_oscillator()
{
  // Check if the lowest energy mode is 3
  if(LOWEST_ENERGY_MODE == 3)
  {
      // Enable the Ultra Low-Frequency RC Oscillator (ULFRCO)
      CMU_OscillatorEnable(cmuOsc_ULFRCO, true, true);

      // Select ULFRCO as the clock source for Low-Frequency A Clock (LFA)
      CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_ULFRCO);

      // Set the clock division factor for LETIMER0 to 1
      CMU_ClockDivSet(cmuClock_LETIMER0, cmuClkDiv_1);

      // Enable the clock for Low Energy Timer 0 (LETIMER0)
      CMU_ClockEnable(cmuClock_LETIMER0, true);
  }
  else
  {
      // If lowest energy mode is not 3, enable the Low-Frequency Crystal Oscillator (LFXO)
      CMU_OscillatorEnable(cmuOsc_LFXO, true, true);

      // Select LFXO as the clock source for Low-Frequency A Clock (LFA)
      CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFXO);

      // Set the clock division factor for LETIMER0 to 4
      CMU_ClockDivSet(cmuClock_LETIMER0, cmuClkDiv_4);

      // Enable the clock for Low Energy Timer 0 (LETIMER0)
      CMU_ClockEnable(cmuClock_LETIMER0, true);
  }
}

