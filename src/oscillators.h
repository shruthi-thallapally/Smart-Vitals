/*
 * oscillators.h
 *
 *  Created on: Jan 31, 2024
 *      Author: tharu
 */

#ifndef SRC_OSCILLATORS_H_
#define SRC_OSCILLATORS_H_

#include "em_cmu.h"

/**
 * @brief Returns the required frequency for the selected oscillator based on the LOWEST_ENERGY_MODE.
 *
 * This function calculates and returns the required frequency for the oscillator based on the
 * specified LOWEST_ENERGY_MODE. It supports two oscillator options: Ultra Low-Frequency RC Oscillator (ULFRCO)
 * for energy mode 3, and Low-Frequency Crystal Oscillator (LFXO) for other energy modes.
 *
 * @return The calculated frequency for the selected oscillator based on the LOWEST_ENERGY_MODE.
 */
int required_oscillator();

/**
 * @brief Initializes the oscillator based on the specified energy mode.
 *
 * This function configures the oscillator settings, clock source, and clock division
 * for the Low-Frequency A Clock (LFA) and the Low Energy Timer 0 (LETIMER0) based on
 * the specified energy mode (LOWEST_ENERGY_MODE).
 *
 * @note Ensure that appropriate energy mode configuration is set before calling this function.
 * @note This function supports two oscillator options: Ultra Low-Frequency RC Oscillator (ULFRCO)
 *       for energy mode 3, and Low-Frequency Crystal Oscillator (LFXO) for other energy modes.
 */
void init_oscillator();


#endif /* SRC_OSCILLATORS_H_ */
