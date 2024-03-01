/*
  gpio.c
 
   Created on: Dec 12, 2018
       Author: Dan Walkes
   Updated by Dave Sluiter Dec 31, 2020. Minor edits with #defines.

   March 17
   Dave Sluiter: Use this file to define functions that set up or control GPIOs.
   
   Jan 24, 2023
   Dave Sluiter: Cleaned up gpioInit() to make it less confusing for students regarding
                 drive strength setting. 

 *
 * Student edit: Add your name and email address here:
 * @student    Tharuni Gelli, Tharuni.gelli@Colorado.edu
 *
 
 */


// *****************************************************************************
// Students:
// We will be creating additional functions that configure and manipulate GPIOs.
// For any new GPIO function you create, place that function in this file.
// *****************************************************************************

#include <stdbool.h>
#include "em_gpio.h"
#include <string.h>

#include "gpio.h"


// Student Edit: Define these, 0's are placeholder values.
//
// See the radio board user guide at https://www.silabs.com/documents/login/user-guides/ug279-brd4104a-user-guide.pdf
// and GPIO documentation at https://siliconlabs.github.io/Gecko_SDK_Doc/efm32g/html/group__GPIO.html
// to determine the correct values for these.
// If these links have gone bad, consult the reference manual and/or the datasheet for the MCU.

#define LED_port    (5) // UIF_LED's are associated with port f(PF), whose value is 5 as per em_gpio.h enum declarations.
#define LED0_pin    (4) // Pin 4 of PF is connected to on-board UIF LED0
#define LED1_pin    (5) // Pin 5 of PF is connected to on-board UIF LED1
#define SENSOR_port (3) // UIF_LED's are associated with port f(PD), whose value is 3 as per em_gpio.h enum declarations.
#define SENSOR_pin  (15)// Pin 15 of PD is connected to sensor
#define LCD_port    (3)
#define LCD_pin     (13)

// Set GPIO drive strengths and modes of operation
void gpioInit()
{
   // Student Edit:

	//GPIO_DriveStrengthSet(LED_port, gpioDriveStrengthStrongAlternateStrong); // Strong, 10mA
  GPIO_DriveStrengthSet(LED_port, gpioDriveStrengthWeakAlternateWeak); // Weak, 1mA

	// Set the 2 GPIOs mode of operation
	GPIO_PinModeSet(LED_port, LED0_pin, gpioModePushPull, false); // Push pull mode for LED0

	GPIO_PinModeSet(LED_port, LED1_pin, gpioModePushPull, false); // push pull mode for LED1

  GPIO_DriveStrengthSet(SENSOR_port, gpioDriveStrengthWeakAlternateWeak);
  GPIO_PinModeSet(SENSOR_port, SENSOR_pin, gpioModePushPull, false);

  GPIO_DriveStrengthSet(LCD_port, gpioDriveStrengthWeakAlternateWeak);
  GPIO_PinModeSet(LCD_port, LCD_pin, gpioModePushPull, false);

} // gpioInit()


void gpioLed0SetOn()
{
	GPIO_PinOutSet(LED_port, LED0_pin);
}


void gpioLed0SetOff()
{
	GPIO_PinOutClear(LED_port, LED0_pin);
}


void gpioLed1SetOn()
{
	GPIO_PinOutSet(LED_port, LED1_pin);
}


void gpioLed1SetOff()
{
	GPIO_PinOutClear(LED_port, LED1_pin);
}

void sensor_enable()
{
  GPIO_PinOutSet(SENSOR_port, SENSOR_pin);
}

void sensor_disable()
{
  GPIO_PinOutClear(SENSOR_port, SENSOR_pin);
}

void gpioSetDisplayExtcomin(bool data )
{
  if(data == true)
   {
       GPIO_PinOutSet(LCD_port, LCD_pin);
   }
  else
   {
       GPIO_PinOutClear(LCD_port, LCD_pin);
   }
}




