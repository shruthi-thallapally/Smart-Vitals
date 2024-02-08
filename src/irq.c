/*
 * irq.c
 *
 *  Created on: Jan 29, 2024
 *      Author: Tharuni Gelli
 *
 *
 */

// Include logging for this file
#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"

#include "app.h"
#include "src/irq.h"
#include "src/scheduler.h"

// Function definition for the LETIMER0 interrupt handler
void LETIMER0_IRQHandler(void)
{
    // Retrieve the enabled interrupt flags from LETIMER0.
    uint32_t value = LETIMER_IntGetEnabled(LETIMER0);

    // Clear the enabled interrupt flags in LETIMER0.
    LETIMER_IntClear(LETIMER0, value);

    // Set an event flag to indicate that an underflow event has occurred.
    schedulerSetEventUF();
}


