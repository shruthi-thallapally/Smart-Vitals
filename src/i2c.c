/*
 * i2c.c
 *
 *  Created on: 08-Feb-2024
 *      Author: Shruthi Thallapally
 *  References:Reference taken from the following files provided by silicon labs em_i2c.h,
 *   efr32bg13p_i2c.h and sl_i2cspm.h
 *   Note: Sometimes warnings are generated due to the log statements.
 *   I have commented LOG_ERROR statements to avoid generating unwanted warnings
 */
#include "stdint.h"
#include "src/i2c.h"
#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"
#include "src/gpio.h"
#include "src/timers.h"
#include "src/oscillators.h"
#include "src/ble_device_type.h"
#include "src/lcd.h"
#include "src/ble.h"

#include "app.h"

#include "sl_i2cspm.h"

#include "em_letimer.h"
#include "em_gpio.h"
#include "em_i2c.h"


#define SI7021_ADD 0x40   // Device address of the sensor as per datasheet
#define MAX32664_DEVICE_ADD 0x55
#define APDS9960_DEVICE_ADD 0x39

uint8_t data_write; ///< Variable to hold data to be written via I2C
uint8_t data_read[2]; ///< Array to store data read via I2C (2 bytes)

uint8_t pulse_data[8];
uint16_t heart_rate[10]; // LSB = 0.1bpm

uint16_t o2[10]; // 0-100% LSB = 1%
uint8_t  confidence; // 0-100% LSB = 1%
//Status values
// 0: Success,
//1: Not Ready,
//2: Object Detectected,
//3: Finger Detected
uint8_t  status=0;
uint8_t send_max30101_data[2];
int i=0;
uint8_t max_heart_rate=0;
uint8_t max_o2=0;


I2C_TransferSeq_TypeDef transfer_sequence;

void Init_i2c()
{
  /**
   * @brief Structure to hold I2C initialization configuration
   */
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
  // Initialize I2C
  I2CSPM_Init(&I2C_Config);
}

/**
 * @brief Initializes the I2C peripheral and performs a read operation.
 *        This function reads data from a sensor connected via I2C.
 */
void i2c_Read()
{
  I2C_TransferReturn_TypeDef transferStatus;
  // Initialize I2C
      Init_i2c();

      // Setup read sequence
      transfer_sequence.addr = SI7021_ADD << 1; // Set I2C address (shifted left by 1 to include read/write bit)
      transfer_sequence.flags = I2C_FLAG_READ;  // Set read flag
      transfer_sequence.buf[0].data = &data_read[0]; // Set data buffer for read
      transfer_sequence.buf[0].len = sizeof(data_read); // Set length of data to read

      // Enable I2C interrupt
      NVIC_EnableIRQ(I2C0_IRQn);

      // Initialize I2C transfer
      transferStatus = I2C_TransferInit(I2C0, &transfer_sequence);

      // Check transfer status
      if (transferStatus < 0) {
          LOG_ERROR("I2C_TransferInit %d read: failed\n\r", (uint32_t)transferStatus);
      }
}
/**
 * @brief Initializes the I2C peripheral and performs a write operation.
 *        This function writes data to a sensor connected via I2C.
 */
void i2c_Write()
{
  I2C_TransferReturn_TypeDef transferStatus;

     // Initialize I2C
     Init_i2c();

     data_write = 0xF3; // Initialize data to be written

     // Setup write sequence
     transfer_sequence.addr = SI7021_ADD << 1; // Set I2C address (shifted left by 1 to include read/write bit)
     transfer_sequence.flags = I2C_FLAG_WRITE; // Set write flag
     transfer_sequence.buf[0].data = &data_write; // Set data buffer for write
     transfer_sequence.buf[0].len = sizeof(data_write); // Set length of data to write

     // Enable I2C interrupt
     NVIC_EnableIRQ(I2C0_IRQn);

     // Initialize I2C transfer
     transferStatus = I2C_TransferInit(I2C0, &transfer_sequence);

     // Check transfer status
     if (transferStatus < 0) {
         LOG_ERROR("I2C_TransferInit status %d write: failed\n\r", (uint32_t)transferStatus);
     }
}

uint32_t writeAdd_readData(uint8_t reg,uint8_t *data)
{
  uint8_t command_data[1];
  I2C_TransferReturn_TypeDef transferStatus;
  Init_i2c();

  command_data[0]=reg;

  transfer_sequence.addr=APDS9960_DEVICE_ADD<<1;
  transfer_sequence.flags = I2C_FLAG_WRITE_READ;
  transfer_sequence.buf[0].data = command_data;
  transfer_sequence.buf[0].len = 1;
  transfer_sequence.buf[1].data = data;
  transfer_sequence.buf[1].len = 1;

  //initialize I2C transfer
  transferStatus = I2CSPM_Transfer(I2C0, &transfer_sequence);

  //check transfer function return status
  if(transferStatus != i2cTransferDone)
    {
     LOG_ERROR("I2CSPM_Transfer status %d writeAdd_readData: failed\n\r", (uint32_t)transferStatus);
     return (uint32_t)transferStatus;
    }

 return (uint32_t)1;
}

uint32_t writeAdd_writeData(uint8_t reg,uint8_t data)
{
  uint8_t command_data[2];
  uint8_t  no_add_data[1];
  I2C_TransferReturn_TypeDef transferStatus;
  Init_i2c();

  command_data[0]=reg;
  command_data[1]=data;

  transfer_sequence.addr=APDS9960_DEVICE_ADD<<1;
  transfer_sequence.flags = I2C_FLAG_WRITE;
  transfer_sequence.buf[0].data = command_data;
  transfer_sequence.buf[0].len = 2;
  transfer_sequence.buf[1].data = no_add_data;
  transfer_sequence.buf[1].len = 0;

  //initialize I2C transfer
   transferStatus = I2CSPM_Transfer(I2C0, &transfer_sequence);

  //check transfer function return status
  if(transferStatus != i2cTransferDone)
    {
     LOG_ERROR("I2CSPM_Transfer status %d writeAdd_writeData: failed\n\r", (uint32_t)transferStatus);
     return (uint32_t)transferStatus;
    }

 return (uint32_t)1;
}

int read_block_data(uint8_t reg,uint8_t *data,uint8_t len)
{
  I2C_TransferReturn_TypeDef transferStatus;
  uint8_t command_data[1];
  Init_i2c();
  transfer_sequence.addr=APDS9960_DEVICE_ADD<<1;
  transfer_sequence.flags = I2C_FLAG_WRITE_READ;
  command_data[0]=reg;
  transfer_sequence.buf[0].data = command_data;
  transfer_sequence.buf[0].len = 1;
  transfer_sequence.buf[1].data = data;
  transfer_sequence.buf[1].len = len;

  //initialize I2C transfer
   transferStatus = I2CSPM_Transfer(I2C0, &transfer_sequence);

    //check transfer function return status
    if(transferStatus != i2cTransferDone)
      {
       LOG_ERROR("I2CSPM_Transfer status %d read_block_data: failed\n\r", (uint32_t)transferStatus);
       return (int)transferStatus;
      }

   return (int)len;

}

/*
uint32_t writeAdd_readData_MAX(uint8_t reg,uint8_t *data)
{
  uint8_t command_data[1];
  I2C_TransferReturn_TypeDef transferStatus;
  Init_i2c();

  command_data[0]=reg;

  transfer_sequence.addr=MAX30101_DEVICE_ADD<<1;
  transfer_sequence.flags = I2C_FLAG_WRITE_READ;
  transfer_sequence.buf[0].data = command_data;
  transfer_sequence.buf[0].len = 1;
  transfer_sequence.buf[1].data = data;
  transfer_sequence.buf[1].len = 1;

  //initialize I2C transfer
  transferStatus = I2CSPM_Transfer(I2C0, &transfer_sequence);

  //check transfer function return status
  if(transferStatus != i2cTransferDone)
    {
     LOG_ERROR("I2CSPM_Transfer status %d writeAdd_readData_Max: failed\n\r", (uint32_t)transferStatus);
     return (uint32_t)transferStatus;
    }

 return (uint32_t)1;
}

uint32_t writeAdd_writeData_MAX(uint8_t reg,uint8_t data)
{
  uint8_t command_data[2];
  uint8_t  no_add_data[1];
  I2C_TransferReturn_TypeDef transferStatus;
  Init_i2c();

  command_data[0]=reg;
  command_data[1]=data;

  transfer_sequence.addr=MAX30101_DEVICE_ADD<<1;
  transfer_sequence.flags = I2C_FLAG_WRITE;
  transfer_sequence.buf[0].data = command_data;
  transfer_sequence.buf[0].len = 2;
  transfer_sequence.buf[1].data = no_add_data;
  transfer_sequence.buf[1].len = 0;

  //initialize I2C transfer
   transferStatus = I2CSPM_Transfer(I2C0, &transfer_sequence);

  //check transfer function return status
  if(transferStatus != i2cTransferDone)
    {
     LOG_ERROR("I2CSPM_Transfer status %d writeAdd_writeData_MAX: failed\n\r", (uint32_t)transferStatus);
     return (uint32_t)transferStatus;
    }

 return (uint32_t)1;
}
*/


void I2C_read_polled_pulse()
{
  I2C_TransferReturn_TypeDef transferStatus;
  Init_i2c();
  transfer_sequence.addr=MAX32664_DEVICE_ADD<<1;
  transfer_sequence.flags = I2C_FLAG_READ;
  transfer_sequence.buf[0].data =pulse_data ;
  transfer_sequence.buf[0].len = sizeof(pulse_data);

  //initialize I2C transfer
   transferStatus = I2CSPM_Transfer(I2C0, &transfer_sequence);

     //check transfer function return status
  if(transferStatus < 0 )
       {
       LOG_ERROR("I2CSPM_Transfer status %d I2C_read_polled_pulse: failed\n\r", (uint32_t)transferStatus);

       }
}

void I2C_write_polled_pulse(uint8_t* cmd,int arr_length)
{
  I2C_TransferReturn_TypeDef transferStatus;
  Init_i2c();
  transfer_sequence.addr=MAX32664_DEVICE_ADD<<1;
  transfer_sequence.flags = I2C_FLAG_WRITE;
  transfer_sequence.buf[0].data = cmd;
  transfer_sequence.buf[0].len = arr_length;


  //initialize I2C transfer
   transferStatus = I2CSPM_Transfer(I2C0, &transfer_sequence);

     //check transfer function return status
  if(transferStatus < 0)
       {
        LOG_ERROR("I2CSPM_Transfer status %d I2C_write_polled_pulse: failed\n\r", (uint32_t)transferStatus);

       }

}


void check_read_return()
{
  if(pulse_data[0] == 0x00){
      LOG_INFO("Return value check successfull\n\r");
  }
  else{
     LOG_ERROR("MAX30101 Pulse oximeter sensor not initialized!!\n\r");
  }
}

#if DEVICE_IS_BLE_SERVER

int pulse_data_extract()
{
  // Get pointer to BLE data structure
   ble_data_struct_t *bleData = getBleDataPtr();
   status=pulse_data[6];

   if(status==3)
     {
       displayPrintf(DISPLAY_ROW_ACTION, "Do not move the finger!");
       displayPrintf(DISPLAY_ROW_TEMPVALUE, "Loading: %d", (10 - i));
       heart_rate[i] = ((uint16_t)(pulse_data[1])) << 8;
       heart_rate[i] |= pulse_data[2];
       heart_rate[i] = heart_rate[i]/10;

       confidence = pulse_data[3];

       o2[i] = ((uint16_t)(pulse_data[4])) << 8;
       o2[i] |= pulse_data[5];
       o2[i] = o2[i]/10;

       LOG_INFO("heart_rate = %d\n\r", heart_rate[i]);
       LOG_INFO("confidence = %d\n\r", confidence);
       LOG_INFO("o2 level = %d\n\r", o2[i]);
       LOG_INFO("status = %d\n\n\r", status);

       i++;
     }
   else
     {
       LOG_INFO("Please place the finger correctly!!\n\n\r");

       displayPrintf(DISPLAY_ROW_ACTION,"Place finger!");
   }

  if(i==10)
    {
      max_heart_rate= heart_rate[0];
      max_o2= o2[0];

      for(int a=0;a<i;a++)
        {
         if(max_heart_rate < heart_rate[a])
           {
             max_heart_rate = (uint8_t)heart_rate[a];
         }

         if(max_o2 < o2[a]){
             max_o2 = (uint8_t)o2[a];
         }

    }
      LOG_INFO("***************************************FINAL VALUES**********************************************\n\r");
      LOG_INFO("heart_rate = %d\n\r", max_heart_rate);
      LOG_INFO("oxygen level = %d\n\r", max_o2);

      displayPrintf(DISPLAY_ROW_8, "");

      if(bleData->gesture_value==0x01){
          send_max30101_data[0] = max_o2;
          send_max30101_data[1] = 0;
          displayPrintf(DISPLAY_ROW_TEMPVALUE, "Oxygen level: %d",max_o2);
      }
      if(bleData->gesture_value==0x02){
          send_max30101_data[0] = max_heart_rate;
          send_max30101_data[1] = 0;
          displayPrintf(DISPLAY_ROW_TEMPVALUE, "Heart rate: %d",max_heart_rate);
      }

      ble_SendPulseState(send_max30101_data);


}
  if(i==10)
    {
      i=0;
    return 1;
  }
  else
    {
    return 0;
  }
}

#endif

// Calculation reference taken from the datasheet (https://www.silabs.com/documents/public/data-sheets/Si7021-A20.pdf)
/**
 * @brief Converts raw temperature data to Celsius.
 * @return Temperature in Celsius.
 */
int32_t ConvertTempToCelcius()
{
    int16_t Celcius; // Variable to hold temperature in Celsius
    int32_t Temp; // Variable to hold raw temperature data

    Temp = (data_read[0] << 8) + data_read[1]; // Combine MSB and LSB

    Celcius = (175.72 * Temp) / 65536; // Perform temperature conversion
    Celcius -= 46.85; // Apply offset

    return Celcius; // Return temperature in Celsius
}

