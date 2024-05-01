/*
 * irq.h
 *
 *  Created on: 31-Jan-2024
 *      Author: Shruthi Thallapally
 * Description: Header file for Interrupt Service Routines (ISR)
 */

#ifndef SRC_IRQ_H_
#define SRC_IRQ_H_

/**
 * @brief Interrupt handler for LETIMER0 peripheral.
 *        Handles COMP1 and UF interrupts.
 */
void LETIMER0_IRQHandler(void);
/**
 * @brief Interrupt handler for I2C0 peripheral.
 *        Handles I2C transfer completion.
 */
void I2C0_IRQHandler(void);
/**
 * @brief Calculates the elapsed time in milliseconds using LETIMER0 counter value.
 * @return The elapsed time in milliseconds.
 */
uint32_t letimerMilliseconds();

#endif /* SRC_IRQ_H_ */
