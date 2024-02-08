/*
 * i2c.c
 *
 *  Created on: Feb 7, 2024
 *      Author: Tharuni Gelli
 *
 *  References: Reference taken from the following files porvoded by silicon labs em_i2c.h, efr32bg13p_i2c.h and sl_i2cspm.h
 */

#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"

#include "em_i2c.h"

#include "src/i2c.h"
#include "app.h"

#define SI7021_ADDR 0x40 // Device address of the sensor as per datasheet

I2CSPM_Init_TypeDef I2C_Config =
{
    .port = I2C0,                    /**< Peripheral port */
    .sclPort = gpioPortC,            /**< SCL pin port number */
    .sclPin =  10,                   /**< SCL pin number */
    .sdaPort = gpioPortC,            /**< SDA pin port number */
    .sdaPin = 11,                    /**< SDA pin number */
    .portLocationScl = 14,           /**< Port location of SCL signal */
    .portLocationSda = 16,           /**< Port location of SDA signal */
    .i2cRefFreq = 0,                 /**< I2C reference clock */
    .i2cMaxFreq = I2C_FREQ_STANDARD_MAX, /**< I2C max bus frequency to use */
    .i2cClhr = i2cClockHLRStandard       /**< Clock low/high ratio control */
};

I2C_TransferReturn_TypeDef TransferStatus;  // Enum variable declaration for checking status of the transfer

// Variables declaration to a structure that defines a complete I2C transfer sequence (from start to stop)(ref from sl_i2cspm.h).
I2C_TransferSeq_TypeDef write_seq, read_seq;

// Variable to store one byte of data during transfer
uint8_t write_data, read_data[2];

void Init_i2c()
{
    // Initialize the data to be written. (As per data sheet - do not hold master mode)
    write_data = 0xF3;

    // Set up the write sequence with the I2C address, flags, and data buffer.
    write_seq.addr = SI7021_ADDR << 1; // Shift the I2C address and set the LSB for write operation.
    write_seq.flags = I2C_FLAG_WRITE; // Set the flag to indicate a write operation.
    write_seq.buf[0].data = &write_data; // Set the data buffer for writing.
    write_seq.buf[0].len = sizeof(write_data); // Set the length of the data to be written.

    // Set up the read sequence with the I2C address, flags, and data buffer.
    read_seq.addr = SI7021_ADDR << 1; // Shift the I2C address and set the LSB for read operation.
    read_seq.flags = I2C_FLAG_READ; // Set the flag to indicate a read operation.
    read_seq.buf[0].data = &read_data[0]; // Set the data buffer for reading.
    read_seq.buf[0].len = sizeof(read_data); // Set the length of the data to be read.

    // Initialize the I2C module with the specified configuration.
    I2CSPM_Init(&I2C_Config);
}

void Write_i2c()
{
    // Initiate the I2C transfer with the configured write sequence.
    TransferStatus = I2CSPM_Transfer(I2C0, &write_seq);

    // Check the status of the I2C transfer.
    if(TransferStatus != i2cTransferDone)
    {
        // Log an error message if the I2C transfer fails.
        LOG_ERROR("I2CSPM_Transfer status %d write: failed\n\r", (uint32_t)TransferStatus);
    }
}

void Read_i2c()
{
    // Initiate the I2C transfer with the configured read sequence.
    TransferStatus = I2CSPM_Transfer(I2C0, &read_seq);

    // Check the status of the I2C transfer.
    if(TransferStatus != i2cTransferDone)
    {
        // Log an error message if the I2C transfer fails.
        LOG_ERROR("I2CSPM_Transfer %d read: failed\n\r", (uint32_t)TransferStatus);
    }
}


// Calculation reference taken from the datasheet (https://www.silabs.com/documents/public/data-sheets/Si7021-A20.pdf)
float ConvertValueToCelcius()
{
    // Declare variables to store the raw temperature value and the temperature in Celsius.
    uint16_t GetTemp;
    float TempCelcius;

    // Combine the two bytes of the raw temperature value to form a 16-bit value.
    GetTemp = (read_data[0] << 8); // Shift the first byte to the left by 8 bits.
    GetTemp += read_data[1]; // Add the second byte to the result.

    // Calculate the temperature in Celsius using the formula provided in the sensor datasheet.
    TempCelcius = (175.72 * (GetTemp)); // Multiply the raw value by a constant.
    TempCelcius /= 65536; // Divide by the maximum value of a 16-bit unsigned integer.
    TempCelcius -= 46.85; // Subtract an offset value.

    // Return the temperature in Celsius.
    return TempCelcius;
}


void Read_si7021()
{
    // Declare a variable to store the temperature value.
    float sensor_value;

    // Enable the Si7021 sensor.
    sensor_enable();

    // Initialize the I2C communication.
    I2CSPM_Init(&I2C_Config);

    // Wait for a specified duration before writing data to the sensor.
    timerWaitUs(90000);

    // Write data to the sensor over I2C.
    Write_i2c();

    // Wait for a specified duration before reading data from the sensor.
    timerWaitUs(12000);

    // Read data from the sensor over I2C.
    Read_i2c();

    // Disable the Si7021 sensor.
    sensor_disable();

    // Convert the read sensor value to Celsius.
    sensor_value = ConvertValueToCelcius();

    // Log the temperature data.
    LOG_INFO("Temperature = %f C\n\r", sensor_value);
}
