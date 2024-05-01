/*
 * oscillators.h
 *
 *  Created on: 31-Jan-2024
 *      Author: Shruthi Thallapally
 * Description: Header file for oscillator initialization functions
 */

#ifndef SRC_OSCILLATORS_H_
#define SRC_OSCILLATORS_H_

#define LFXO_FREQ 32768
#define ULFRCO_FREQ 1000
#define PRESCALER 2
#define LFXO_FREQ_USED (LFXO_FREQ/PRESCALER)
#define ULFRCO_FREQ_USED (ULFRCO_FREQ/1)
#define ACTUAL_FREQ (LOWEST_ENERGY_MODE==3?ULFRCO_FREQ_USED:LFXO_FREQ_USED)

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
void init_oscillator();

#endif /* SRC_OSCILLATORS_H_ */
