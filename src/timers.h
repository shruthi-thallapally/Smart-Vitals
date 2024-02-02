/*
 * timers.h
 *
 *  Created on: Jan 28, 2024
 *      Author: Tharuni Gelli
 */

#ifndef SRC_TIMERS_H_
#define SRC_TIMERS_H_

#include "src/oscillators.h"
#include "app.h"
#include "em_letimer.h"

//
#define LETIMER_PERIOD_MS  2250 // Total Period in seconds
#define LETIMER_ON_TIME_MS 175  // ON period in seconds

/*
 * Function: void init_LETIMER0 ()
 * ----------------------------------------------
 * Initializes the LETIMER0 peripheral with the specified configuration.
 */
void init_LETIMER0 ();


#endif /* SRC_TIMERS_H_ */
