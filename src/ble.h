/*
 * ble.h
 *
 *  Created on: Feb 18, 2024
 *      Author: tharuni gelli
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

#define QUEUE_DEPTH 16

#define MAX_BUFFER_LENGTH 5

typedef struct {

  uint16_t       charHandle;                 // GATT DB handle from gatt_db.h
  uint32_t       bufLength;                  // Number of bytes written to field buffer[5]
  uint8_t        buffer[MAX_BUFFER_LENGTH];  // The actual data buffer for the indication,
                                             //   need 5-bytes for HTM and 1-byte for button_state.
                                             //   For testing, test lengths 1 through 5,
                                             //   a length of 0 shall be considered an
                                             //   error, as well as lengths > 5

} queue_struct_t;

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
  bool Button_Indication;
  bool Button_Pressed;
  bool Bonded;
  queue_struct_t Indication_Buffer[QUEUE_DEPTH];
  uint8_t r_ptr, w_ptr;
  bool full;
  uint8_t Queued_Indication;
  uint32_t passkey;
} ble_data_struct_t;

/*
 * Retrieves a pointer to a BLE data structure. This function is used to access the global or
 * shared BLE data structure within the application. The BLE data structure contains important
 * BLE-related information and configurations that are used across the BLE module.
 *
 * @return A pointer to a ble_data_struct_t structure containing BLE data. If the data structure
 *         cannot be accessed or has not been initialized, the function may return NULL.
 */
ble_data_struct_t* get_ble_DataPtr(void);

/**
 * @brief Calculates the next pointer in a series or structure.
 *
 * This function is designed to compute the address of the next element in a sequence or data structure,
 * such as a linked list or an array. Given the current pointer's address, it calculates the address of the
 * next element based on the size of the elements or a predefined step. This can be particularly useful in
 * systems where manual pointer arithmetic is necessary to navigate through data structures efficiently.
 *
 * @param ptr The current pointer's address, represented as a 32-bit unsigned integer. This could be the address
 * of the current element in a data structure from which the address of the next element is to be calculated.
 *
 * @return uint32_t The address of the next element in the sequence or data structure, also represented as a
 * 32-bit unsigned integer. This value is calculated based on the current pointer's address and the predefined
 * criteria or structure size.
 */
uint32_t next_ptr(uint32_t ptr);

/**
 * @brief Writes data to a specified queue.
 *
 * This function is responsible for adding data to a queue structure. It is typically used in scenarios where
 * data needs to be stored temporarily before being processed or sent to another part of a system, such as in
 * message passing or producer-consumer scenarios. The function ensures that the data is correctly added to the
 * queue, managing any necessary synchronization or checking for available space to avoid overflows or data loss.
 *
 * @param write_data The data to be written to the queue. This parameter should be a structure that contains
 * the data to be added, as well as any additional information needed to manage the queue, such as priority
 * or identification information.
 *
 * @return int Returns a status code indicating the result of the operation. Common return values might include
 * 0 for success, -1 for failure due to a full queue, or other codes indicating specific error conditions or
 * requirements, such as needing to wait for space to become available.
 */
int write_queue (queue_struct_t write_data);

/**
 * @brief Reads and removes data from a specified queue.
 *
 * This function is designed to retrieve and remove the front item from a queue. It is commonly used in
 * scenarios where data has been temporarily stored for sequential processing, such as in event handling systems,
 * message queues, or data buffering applications. The function ensures that data is accessed in a first-in, first-out
 * (FIFO) manner, and manages synchronization to prevent data corruption or access conflicts in concurrent environments.
 *
 * @return int Returns a status code indicating the result of the operation. Common return values might include
 * 0 for success, indicating that data was successfully read and removed from the queue. Other return values
 * may indicate failure conditions, such as an empty queue (-1), or specific error codes related to queue management
 * or data integrity issues.
 */
int read_queue ();



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

/**
 * @brief Sends the state of a button over BLE (Bluetooth Low Energy).
 *
 * This function is designed to transmit the current state of a button (e.g., pressed or released) over BLE to a connected
 * device or central system. It is useful in applications where remote control or monitoring of button states is required,
 * such as in smart home devices, remote controllers, or wearable technology. The function packages the button state into
 * a suitable format for BLE transmission and manages the sending process, ensuring that the state is correctly communicated
 * to the receiving device.
 *
 * @param value The current state of the button to be sent over BLE. This could be represented as a simple binary value,
 * where 0 might indicate that the button is not pressed (released) and 1 indicates that the button is pressed.
 *
 * @return void
 */
void SendButtonState_ble(uint8_t value);


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
