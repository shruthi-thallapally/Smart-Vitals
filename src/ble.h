/*
 * ble.h
 *
 *  Created on: Feb 18, 2024
 *      Author: Tharuni Gelli
 */

#ifndef SRC_BLE_H_
#define SRC_BLE_H_

#include "app.h"
#include "em_letimer.h"
#include "em_gpio.h"
#include "em_i2c.h"
#include "src/timers.h"
#include "src/oscillators.h"
#include "src/gpio.h"
#include "src/i2c.h"
#include "sl_i2cspm.h"


#define UINT8_TO_BITSTREAM(p, n) { *(p)++ = (uint8_t)(n); } // use this for the flags byte, which you set = 0
#define UINT32_TO_BITSTREAM(p, n) {*(p)++ = (uint8_t)(n); *(p)++ = (uint8_t)((n) >> 8); \
                                   *(p)++ = (uint8_t)((n) >> 16); *(p)++ = (uint8_t)((n) >> 24); }
#define INT32_TO_FLOAT(m, e) ( (int32_t) (((uint32_t) m) & 0x00FFFFFFU) | (((uint32_t) e) << 24))

typedef struct
{
  bd_addr My_Address;

  bool    Connected;

  bool    Indication;

  bool    Indication_InFlight;

  uint8_t My_AddressType;

  uint8_t Advertising_Set_Handle;

  uint8_t Connection_Handle;

  uint32_t Rollover_Count;

  // values unique for client
} ble_data_struct_t;

ble_data_struct_t* get_ble_DataPtr(void);

void SendTemp_ble();

void handle_ble_event(sl_bt_msg_t *evt);

#endif /* SRC_BLE_H_ */
