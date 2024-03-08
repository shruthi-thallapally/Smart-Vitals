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

// Define a structure `ble_data_struct_t` to hold BLE device data and state.
typedef struct
{
  bd_addr My_Address;             // Bluetooth device address of the local device.
  bool    Connected;              // Connection status flag: true if connected to a remote device, false otherwise.
  bool    Indication;             // Flag to indicate if an indication is enabled for any characteristic.
  bool    Indication_InFlight;    // Flag to indicate if an indication transaction is currently in progress.
  uint8_t My_AddressType;         // Type of the Bluetooth device address (public or random).
  uint8_t Advertising_Set_Handle; // Handle to identify the advertising set.
  uint8_t Connection_Handle;      // Handle to identify the connection with a remote device.
  uint32_t Rollover_Count;        // Counter to track the number of times a certain threshold or event has occurred, possibly for rollover logic.
  uint32_t Service_Handle;        // Handle to identify a specific GATT service.
  uint16_t Char_Handle;           // Handle to identify a specific GATT characteristic within the service.
  uint8_t * Char_Value;           // Pointer to the value of the GATT characteristic mentioned above.
  bool Gatt_Procedure;            // Flag to indicate if a GATT procedure is currently in progress.

} ble_data_struct_t; // End of `ble_data_struct_t` structure definition.

/*
 * Retrieves a pointer to a BLE data structure. This function is used to access the global or
 * shared BLE data structure within the application. The BLE data structure contains important
 * BLE-related information and configurations that are used across the BLE module.
 *
 * @return A pointer to a ble_data_struct_t structure containing BLE data. If the data structure
 *         cannot be accessed or has not been initialized, the function may return NULL.
 */
ble_data_struct_t* get_ble_DataPtr(void);

#if DEVICE_IS_BLE_SERVER

/*
 * Sends the current temperature reading over BLE (Bluetooth Low Energy) using an established
 * advertising set or connection. This function prepares the temperature data in the appropriate
 * format and initiates the BLE transmission process. The function does not return a value and
 * assumes that any necessary BLE initialization and configuration have already been performed.
 *
 * Note: This function may utilize global or static variables to access the BLE subsystem and
 * the current temperature data.
 */
void SendTemp_ble();


#endif

/*
 * Handles BLE (Bluetooth Low Energy) events received from the BLE stack. This function is
 * designed to be called when an event occurs in the BLE operation, such as connection,
 * disconnection, advertisement, and more. It takes a pointer to an event message structure
 * as input, processes the event according to its type, and performs the necessary actions
 * or responses required by the application logic.
 *
 * @param[in] evt A pointer to the sl_bt_msg_t structure that contains the details of the BLE
 *                event that occurred. The structure includes the event type, parameters, and
 *                any other relevant information needed to handle the event appropriately.
 *
 * Note: This function is typically called in the main loop or a BLE event callback mechanism
 *       and is a critical part of the BLE event handling architecture of the application. It
 *       ensures that the application responds correctly to the BLE events, maintaining the
 *       stability and functionality of the BLE communication.
 */
void handle_ble_event(sl_bt_msg_t *evt);

#endif /* SRC_BLE_H_ */
