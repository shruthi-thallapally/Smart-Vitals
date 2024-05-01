/*
 * i2c.h
 *
 *  Created on: 08-Feb-2024
 *      Author: Shruthi Thallapally
 */

#ifndef SRC_I2C_H_
#define SRC_I2C_H_

#include "stdint.h"
/**
 * @brief Reads temperature from the sensor via I2C and logs the value.
 */
//void Read_temp();

/**
 * @brief Initializes the I2C peripheral and sets up write and read sequences.
 */
void Init_i2c();

/**
 * @brief Performs an I2C read operation.
 */
void i2c_Read();

/**
 * @brief Performs an I2C write operation.
 */
void i2c_Write();

uint32_t writeAdd_writeData(uint8_t reg,uint8_t data);

uint32_t writeAdd_readData(uint8_t reg,uint8_t *data);

int read_block_data(uint8_t reg,uint8_t *data,uint8_t len);
void check_read_return();
void I2C_write_polled_pulse(uint8_t* cmd,int arr_length);
void I2C_read_polled_pulse();

//uint32_t writeAdd_readData_MAX(uint8_t reg,uint8_t *data);

//uint32_t writeAdd_writeData_MAX(uint8_t reg,uint8_t data);

int pulse_data_extract();

/**
 * @brief Converts raw temperature data to Celsius.
 * @return Temperature in Celsius.
 */
int32_t ConvertTempToCelcius();

#endif /* SRC_I2C_H_ */
