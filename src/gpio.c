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


// Set GPIO drive strengths and modes of operation
void gpioInit()
{
   // Student Edit:

  // Set the drive strength of the GPIO port connected to LEDs to strong alternate strong, which allows for a higher current draw (up to 10mA).
  // This is useful for driving higher power components like LEDs that may require more current.
  //GPIO_DriveStrengthSet(LED_port, gpioDriveStrengthStrongAlternateStrong); // Strong, 10mA

  // Set the drive strength of the GPIO port connected to LEDs to weak alternate weak, reducing the current draw to a lower level (up to 1mA).
  // This configuration conserves power and is suitable for low-power applications or when high current is not needed.
  GPIO_DriveStrengthSet(LED_port, gpioDriveStrengthWeakAlternateWeak); // Weak, 1mA

  // Configure the pin connected to LED0 for push-pull mode, which allows the pin to actively drive the output high or low.
  // The false parameter indicates that the pin's initial state is low (LED off). This mode is used for driving outputs like LEDs.
  GPIO_PinModeSet(LED_port, LED0_pin, gpioModePushPull, false); // Push pull mode for LED0

  // Configure the pin connected to LED1 in the same way as LED0, for push-pull mode with an initial state of low.
  // Push-pull mode is ideal for outputs where the pin needs to actively drive the state, such as lighting an LED.
  GPIO_PinModeSet(LED_port, LED1_pin, gpioModePushPull, false); // Push pull mode for LED1

  // Set the drive strength for the GPIO port connected to the sensor to weak alternate weak, which is suitable for low-power applications.
  // This reduces the amount of current the pin can source or sink, which is generally acceptable for sensor inputs that require minimal power.
  GPIO_DriveStrengthSet(SENSOR_port, gpioDriveStrengthWeakAlternateWeak);

  // Configure the sensor pin for push-pull mode with an initial state of low. This configuration might be used if the sensor pin needs
  // to output a signal, although sensors typically are configured as inputs. The exact purpose depends on the sensor's requirements.
  GPIO_PinModeSet(SENSOR_port, SENSOR_pin, gpioModePushPull, false);

  // Set the drive strength for the GPIO port connected to the LCD to weak alternate weak, optimizing for low power consumption.
  // This is often adequate for LCD displays that do not require high current for control signals.
  GPIO_DriveStrengthSet(LCD_port, gpioDriveStrengthWeakAlternateWeak);

  // Configure the LCD control pin for push-pull mode with an initial state of low. This ensures the pin can actively control
  // the LCD's operation by driving the pin high or low, suitable for sending control signals to the LCD.
  GPIO_PinModeSet(LCD_port, LCD_pin, gpioModePushPull, false);

  GPIO_PinModeSet(BUTTON_PORT, BUTTON_PIN, gpioModeInputPullFilter, true);
  GPIO_ExtIntConfig(BUTTON_PORT, BUTTON_PIN, BUTTON_PIN, true, true, true);
} // gpioInit()


// Function to turn on LED 0
void gpioLed0SetOn()
{
    // Set the output of the LED0 pin to high, turning the LED on
  GPIO_PinOutSet(LED_port, LED0_pin);
}

// Function to turn off LED 0
void gpioLed0SetOff()
{
    // Clear the output of the LED0 pin to low, turning the LED off
  GPIO_PinOutClear(LED_port, LED0_pin);
}

// Function to turn on LED 1
void gpioLed1SetOn()
{
    // Set the output of the LED1 pin to high, turning the LED on
  GPIO_PinOutSet(LED_port, LED1_pin);
}

// Function to turn off LED 1
void gpioLed1SetOff()
{
    // Clear the output of the LED1 pin to low, turning the LED off
  GPIO_PinOutClear(LED_port, LED1_pin);
}

// Function to enable the sensor
void sensor_enable()
{
    // Set the output of the SENSOR pin to high, enabling the sensor
  GPIO_PinOutSet(SENSOR_port, SENSOR_pin);
}

// Function to disable the sensor
void sensor_disable()
{
    // Clear the output of the SENSOR pin to low, disabling the sensor
  GPIO_PinOutClear(SENSOR_port, SENSOR_pin);
}

// Function to set the display external COM inversion (EXTCOMIN) signal
void gpioSetDisplayExtcomin(bool data )
{
    // Check if the data is true
  if(data == true)
   {
       // If true, set the LCD pin to high
       GPIO_PinOutSet(LCD_port, LCD_pin);
   }
  else
   {
       // If false, clear the LCD pin to low
       GPIO_PinOutClear(LCD_port, LCD_pin);
   }
}





