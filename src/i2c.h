/*
 * i2c.h
 *
 *  Created on: Feb 7, 2024
 *      Author: Tharuni Gelli
 */

#ifndef SRC_I2C_H_
#define SRC_I2C_H_

#include "sl_i2cspm.h"
#include "em_i2c.h"
#include "em_letimer.h"
#include "em_gpio.h"

#include "app.h"
#include "src/gpio.h"
#include "src/timers.h"
#include "src/oscillators.h"

/**
 * @brief Initializes the I2C communication.
 *
 * This function initializes the I2C communication by configuring write and read sequences,
 * setting up the I2C address, flags, and data buffers, and then initializing the I2C module
 * with the specified configuration.
 */
void Init_i2c();


/**
 * @brief Writes data over I2C communication.
 *
 * This function initiates a write operation over the I2C bus using the configured write sequence.
 * It checks the status of the write operation and logs an error if the transfer fails.
 */
void Write_i2c();


/**
 * @brief Reads data over I2C communication.
 *
 * This function initiates a read operation over the I2C bus using the configured read sequence.
 * It checks the status of the read operation and logs an error if the transfer fails.
 */
void Read_i2c();

/**
 * @brief Converts the temperature value read from the sensor to Celsius.
 *
 * This function takes the raw temperature value read from the sensor, performs
 * necessary calculations, and returns the temperature in Celsius.
 *
 * @return The temperature value in Celsius.
 */
float ConvertValueToCelcius();


/**
 * @brief Reads temperature data from the Si7021 sensor.
 *
 * This function reads temperature data from the Si7021 sensor by following a sequence of steps:
 * 1. Enable the sensor.
 * 2. Initialize the I2C communication.
 * 3. Wait for a specified duration.
 * 4. Write data to the sensor over I2C.
 * 5. Wait for a specified duration.
 * 6. Read data from the sensor over I2C.
 * 7. Disable the sensor.
 * 8. Convert the read sensor value to Celsius.
 * 9. Log the temperature data.
 */
void Read_si7021();


#endif /* SRC_I2C_H_ */
