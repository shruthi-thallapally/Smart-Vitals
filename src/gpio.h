/*
   gpio.h
  
    Created on: Dec 12, 2018
        Author: Dan Walkes

    Updated by Dave Sluiter Sept 7, 2020. moved #defines from .c to .h file.
    Updated by Dave Sluiter Dec 31, 2020. Minor edits with #defines.

    Editor: Feb 26, 2022, Dave Sluiter
    Change: Added comment about use of .h files.

 *
 * Student edit: Add your name and email address here:
 * @student    Tharuni Gelli, tharuni.gelli@Colorado.edu
 *
 
 */


// Students: Remember, a header file (a .h file) generally defines an interface
//           for functions defined within an implementation file (a .c file).
//           The .h file defines what a caller (a user) of a .c file requires.
//           At a minimum, the .h file should define the publicly callable
//           functions, i.e. define the function prototypes. #define and type
//           definitions can be added if the caller requires theses.


#ifndef SRC_GPIO_H_
#define SRC_GPIO_H_




// Function prototypes

// Function to initialize GPIO ports and pin as per required config
void gpioInit();

// Function to turn on LED0
void gpioLed0SetOn();

// Function to turn off LED0
void gpioLed0SetOff();

// Function to turn on LED1
void gpioLed1SetOn();

// Function to turn off LED0
void gpioLed1SetOff();

//Function to enable sensor
void sensor_enable();

//Function to disable sensor
void sensor_disable();

void gpioSetDisplayExtcomin(bool data );


#endif /* SRC_GPIO_H_ */
