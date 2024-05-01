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
 * @student    Shruthi Thallapally, Shruthi.Thallapally@Colorado.edu
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

#include "stdbool.h"

#define LED_port   (gpioPortF) //port 5
#define LED0_pin   (4)
#define LED1_pin   (5)
#define TEMP_SENSOR_PORT gpioPortD  //port 3
#define TEMP_SENSOR_PIN (15)
#define LCD_port    gpioPortD   //port 3
#define LCD_pin     (13)
#define BUTTON_PORT  (5)
#define PB0_BUTTON_PIN  (6)
#define PB1_BUTTON_PIN  (7)
#define GESTURE_PORT gpioPortD
#define GESTURE_PIN (11)


// Function prototypes
void gpioInit();
void gpioLed0SetOn();
void gpioLed0SetOff();
void gpioLed1SetOn();
void gpioLed1SetOff();
void gpioTempSensorEnable();
void gpioTempSensorDisable();
void gpioSetDisplayExtcomin(bool value);


#endif /* SRC_GPIO_H_ */
