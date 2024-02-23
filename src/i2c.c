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


// Variables declaration to a structure that defines a complete I2C transfer sequence (from start to stop)(ref from sl_i2cspm.h).
I2C_TransferSeq_TypeDef transfer_seq;

// Variable to store one byte of data during transfer
uint8_t write_data, read_data[2];

void Init_i2c()
{

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
    // Initialize the I2C module with the specified configuration.
    I2CSPM_Init(&I2C_Config);
}

void Write_i2c()
{
    I2C_TransferReturn_TypeDef Transfer_Status;  // Enum variable declaration for checking status of the transfer

    Init_i2c();

    // Initialize the data to be written. (As per data sheet - do not hold master mode)
    write_data = 0xF3;

    // Set up the write sequence with the I2C address, flags, and data buffer.
    transfer_seq.addr = SI7021_ADDR << 1; // Shift the I2C address and set the LSB for write operation.
    transfer_seq.flags = I2C_FLAG_WRITE; // Set the flag to indicate a write operation.
    transfer_seq.buf[0].data = &write_data; // Set the data buffer for writing.
    transfer_seq.buf[0].len = sizeof(write_data); // Set the length of the data to be written.

    NVIC_EnableIRQ(I2C0_IRQn);

    Transfer_Status = I2C_TransferInit(I2C0, &transfer_seq);

    // Check the status of the I2C transfer.
    if(Transfer_Status < 0 )
    {
        // Log an error message if the I2C transfer fails.
        LOG_ERROR("I2CSPM_Transfer status %d write: failed\n\r", (uint32_t)Transfer_Status);
    }
}

void Read_i2c()
{
    I2C_TransferReturn_TypeDef Transfer_Status;  // Enum variable declaration for checking status of the transfer

    Init_i2c();

    // Set up the read sequence with the I2C address, flags, and data buffer.
    transfer_seq.addr = SI7021_ADDR << 1; // Shift the I2C address and set the LSB for read operation.
    transfer_seq.flags = I2C_FLAG_READ; // Set the flag to indicate a read operation.
    transfer_seq.buf[0].data = &read_data[0]; // Set the data buffer for reading.
    transfer_seq.buf[0].len = sizeof(read_data); // Set the length of the data to be read.

    NVIC_EnableIRQ(I2C0_IRQn);

    Transfer_Status = I2C_TransferInit(I2C0, &transfer_seq);

    // Check the status of the I2C transfer.
    if(Transfer_Status <0)
    {
        // Log an error message if the I2C transfer fails.
        LOG_ERROR("I2CSPM_Transfer %d read: failed\n\r", (uint32_t)Transfer_Status);
    }
}


// Calculation reference taken from the datasheet (https://www.silabs.com/documents/public/data-sheets/Si7021-A20.pdf)
int32_t ConvertValueToCelcius()
{
    // Declare variables to store the raw temperature value and the temperature in Celsius.
    uint16_t GetTemp;
    int32_t TempCelcius;

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
