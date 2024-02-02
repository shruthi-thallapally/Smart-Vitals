/*
 * irq.h
 *
 *  Created on: Feb 30, 2024
 *      Author: Tharuni Gelli
 */

#ifndef SRC_IRQ_H_
#define SRC_IRQ_H_

#include "src/gpio.h"
#include "src/timers.h"
#include "src/oscillators.h"
#include "app.h"

#include "em_letimer.h"


/**
 * @brief Interrupt handler for LETIMER0
 *
 * This function is the interrupt service routine (ISR) for LETIMER0.
 * It handles the pending interrupts, clears them, and takes actions based on
 * the specific interrupt flags. In this case, it checks for specific values
 * and performs corresponding operations, such as turning on or off an LED.
 */
void LETIMER0_IRQHandler(void);




#endif /* SRC_IRQ_H_ */
