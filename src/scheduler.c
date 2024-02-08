/*
 * scheduler.c
 *
 *  Created on: Feb 3, 2024
 *      Author: Tharuni Gelli
 */

// Including logging files to the file
#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"

#include "src/scheduler.h"

// Declare a global variable named my_event of type uint32_t to store event flags.
uint32_t my_event;

// Define an enumeration representing various events.
enum
{
    // Define an event flag indicating no event.
    evt_no_event = 0,

    // Define an event flag indicating an underflow event from LETIMER0.
    evt_LETIMER0_UF = 1,
};


void schedulerSetEventUF()
{
    // Declare a variable to store the IRQ state.
    CORE_DECLARE_IRQ_STATE;

    // Enter a critical section to prevent interrupt nesting.
    CORE_ENTER_CRITICAL();

    // Set the global variable my_event to indicate an underflow event.
    my_event = evt_LETIMER0_UF;

    // Exit the critical section.
    CORE_EXIT_CRITICAL();
} // schedulerSetEventUF()



uint32_t getNextEvent()
{
    // Declare a variable to store the retrieved event.
    uint32_t the_event;

    // Retrieve the current event from the global variable my_event.
    the_event = my_event;

    // Declare a variable to store the IRQ state.
    CORE_DECLARE_IRQ_STATE;

    // Enter a critical section to prevent interrupt nesting.
    CORE_ENTER_CRITICAL();

    // Clear the global variable my_event to indicate no pending event.
    my_event = evt_no_event;

    // Exit the critical section.
    CORE_EXIT_CRITICAL();

    // Return the retrieved event.
    return (the_event);
} // getNextEvent()




