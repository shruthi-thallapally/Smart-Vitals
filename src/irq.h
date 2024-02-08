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
 * @brief Interrupt service routine for LETIMER0.
 *
 * This function is the interrupt service routine (ISR) for LETIMER0 interrupts.
 * It handles the interrupt by clearing the interrupt flags and sets an event flag
 * to indicate that an underflow event has occurred.
 */
void LETIMER0_IRQHandler(void);




#endif /* SRC_IRQ_H_ */
