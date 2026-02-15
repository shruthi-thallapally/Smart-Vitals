
/*
 * ble.h
 *
 *  Created on: 20-Feb-2024
 *      Author: Shruthi Thallapally
 *   Reference: Code reference taken from the lecture slides and reference links provided in the lecture
 */

#ifndef SRC_BLE_H_
#define SRC_BLE_H_

#include "stdint.h"
#include "stdbool.h"
#include "sl_bgapi.h"
#include "sl_bt_api.h"


#define UINT8_TO_BITSTREAM(p, n)      { *(p)++ = (uint8_t)(n); }
#define UINT32_TO_BITSTREAM(p, n)     { *(p)++ = (uint8_t)(n); *(p)++ = (uint8_t)((n) >> 8); \
    *(p)++ = (uint8_t)((n) >> 16); *(p)++ = (uint8_t)((n) >> 24); }
#define INT32_TO_FLOAT(m, e)        ((int32_t) (((uint32_t)m) & 0x00FFFFFFU) |(((uint32_t)e) << 24))

#define MAX_BUFFER_LENGTH  (5)
#define MIN_BUFFER_LENGTH  (1)
#define QUEUE_DEPTH      (16)


#if !DEVICE_IS_BLE_SERVER

static const uint8_t thermo_service[2]={0x09,0x18};

static const uint8_t thermo_char[2]={0x1c,0x2a};

static const uint8_t button_service[16]={0x89,0x62,0x13,0x2d,0x2a,0x65,0xec,0x87,0x3e,0x43,0xc8,0x38,0x01,0x00,0x00,0x00};

static const uint8_t button_charac[16] = {0x89, 0x62, 0x13, 0x2d, 0x2a, 0x65, 0xec, 0x87, 0x3e, 0x43, 0xc8, 0x38, 0x02, 0x00, 0x00, 0x00 };

//service uuid - gesture sensor
//855eb9c5-82e1-4f81-961f-9bce01d3547d
static const uint8_t gesture_service[16] = { 0x7d, 0x54, 0xd3, 0x01, 0xce, 0x9b, 0x1f, 0x96, 0x81, 0x4f, 0xe1, 0x82, 0xc5, 0xb9, 0x5e, 0x85 };

//characteristic uuid - gesture sensor
//c07a50a6-0642-49f0-839d-2933b25c414b
static const uint8_t gesture_charac[16] = { 0x4b, 0x41, 0x5c, 0xb2, 0x33, 0x29, 0x9d, 0x83, 0xf0, 0x49, 0x42, 0x06, 0xa6, 0x50, 0x7a, 0xc0 };

//service uuid - oximeter sensor
//cd7f3d54-989e-48a4-b742-5f5dbdd2a316

static const uint8_t oximeter_service[16] = { 0x16, 0xa3, 0xd2, 0x0bd, 0x5d, 0x5f, 0x42, 0xb7, 0xa4, 0x48, 0x9e, 0x98, 0x54, 0x3d, 0x7f, 0xcd };

//characteristic - oximeter sensor
//202ce269-3f62-485b-a283-f48088e72f39
static const uint8_t oximeter_charac[16] = { 0x39, 0x2f, 0xe7, 0x88, 0x80, 0xf4, 0x83, 0xa2, 0x5b, 0x48, 0x62, 0x3f, 0x69, 0xe2, 0x2c, 0x20 };

#endif

typedef struct {

  uint16_t       charHandle;                 // GATT DB handle from gatt_db.h
  uint32_t       bufLength;                  // Number of bytes written to field buffer[5]
  uint8_t        buffer[MAX_BUFFER_LENGTH];  // The actual data buffer for the indication,
                                             //   need 5-bytes for HTM and 1-byte for button_state.
                                             //   For testing, test lengths 1 through 5,
                                             //   a length of 0 shall be considered an
                                             //   error, as well as lengths > 5

} queue_struct_t;

typedef struct {


  uint8_t myAddressType;
  bd_addr myAddress;
  uint8_t connection_handle;
  bool    connected;
  bool    indication;
  bool    indication_inFlight;
  uint8_t advertisingSetHandle;
  uint32_t rollover_cnt;
//  bool gatt_procedure;

  uint32_t service_handle;
  uint16_t char_handle;
  uint8_t *char_value;

  uint32_t button_service_handle;
  uint16_t button_char_handle;
//  uint8_t *button_char_value;
  bool button_pressed;
  bool button_indication;
  bool PB1_button_pressed;

  bool bonded;
  uint32_t passkey;
  bool displaying_passkey_to_user;



  bool gesture_indication;
  bool gesture_on;
  uint32_t gesture_service_handle;
  uint16_t gesture_char_handle;
  uint8_t gesture_value;

  bool pulse_indication;
  bool pulse_on;
  uint32_t pulse_service_handle;
  uint16_t pulse_char_handle;

} ble_data_struct_t;

/**
 * @brief Get pointer to the BLE data structure.
 *
 * @return Pointer to ble_data_struct_t.
 */
ble_data_struct_t* getBleDataPtr(void);

/**
 * @brief Handle BLE events.
 *
 * @param evt Pointer to the BLE event.
 */
void handle_ble_event(sl_bt_msg_t *evt);

#if DEVICE_IS_BLE_SERVER
/**
 * @brief Sends temperature data over BLE.
 */
void ble_SendTemperature();

/**
 * @brief Sends button status over BLE.
 *
 * This function sends the button status over BLE to the connected device.
 * If the device is connected and bonded, it writes the button status to the GATT server,
 * and if enabled, sends an indication or queues it for later sending.
 *
 * @param value The button status value to be sent.
 */
void ble_SendButtonStatus(uint8_t value);

void ble_SendPulseState(uint8_t * pulse_data);

/** Gesture circular queue: enqueue a gesture; sends immediately or when current indication completes. */
#define GESTURE_QUEUE_SIZE  8U

void ble_SendGesture(uint8_t state);

/**
 * @brief Enqueue a gesture to send over BLE. If no indication is in flight, sends immediately.
 *        Otherwise the gesture is queued and sent when the current indication is confirmed.
 *        If the queue is full, the oldest gesture is dropped.
 * @param state Gesture value (e.g. 0x01=LEFT, 0x02=RIGHT, 0x03=UP, 0x04=DOWN, 0x05=NEAR, 0x06=FAR, 0x00=NONE).
 */
void ble_EnqueueGesture(uint8_t state);

#endif //DEVICE_IS_BLE_SERVER

#endif /* SRC_BLE_H_ */

/*
 * ble.h
 *
 *  Created on: 20-Feb-2024
 *      Author: Shruthi Thallapally
 *   Reference: Code reference taken from the lecture slides and reference links provided in the lecture
 */

#ifndef SRC_BLE_H_
#define SRC_BLE_H_

#include "stdint.h"
#include "stdbool.h"
#include "sl_bgapi.h"
#include "sl_bt_api.h"


#define UINT8_TO_BITSTREAM(p, n)      { *(p)++ = (uint8_t)(n); }
#define UINT32_TO_BITSTREAM(p, n)     { *(p)++ = (uint8_t)(n); *(p)++ = (uint8_t)((n) >> 8); \
    *(p)++ = (uint8_t)((n) >> 16); *(p)++ = (uint8_t)((n) >> 24); }
#define INT32_TO_FLOAT(m, e)        ((int32_t) (((uint32_t)m) & 0x00FFFFFFU) |(((uint32_t)e) << 24))

#define MAX_BUFFER_LENGTH  (5)
#define MIN_BUFFER_LENGTH  (1)
#define QUEUE_DEPTH      (16)


#if !DEVICE_IS_BLE_SERVER

static const uint8_t thermo_service[2]={0x09,0x18};

static const uint8_t thermo_char[2]={0x1c,0x2a};

static const uint8_t button_service[16]={0x89,0x62,0x13,0x2d,0x2a,0x65,0xec,0x87,0x3e,0x43,0xc8,0x38,0x01,0x00,0x00,0x00};

static const uint8_t button_charac[16] = {0x89, 0x62, 0x13, 0x2d, 0x2a, 0x65, 0xec, 0x87, 0x3e, 0x43, 0xc8, 0x38, 0x02, 0x00, 0x00, 0x00 };

//service uuid - gesture sensor
//855eb9c5-82e1-4f81-961f-9bce01d3547d
static const uint8_t gesture_service[16] = { 0x7d, 0x54, 0xd3, 0x01, 0xce, 0x9b, 0x1f, 0x96, 0x81, 0x4f, 0xe1, 0x82, 0xc5, 0xb9, 0x5e, 0x85 };

//characteristic uuid - gesture sensor
//c07a50a6-0642-49f0-839d-2933b25c414b
static const uint8_t gesture_charac[16] = { 0x4b, 0x41, 0x5c, 0xb2, 0x33, 0x29, 0x9d, 0x83, 0xf0, 0x49, 0x42, 0x06, 0xa6, 0x50, 0x7a, 0xc0 };

//service uuid - oximeter sensor
//cd7f3d54-989e-48a4-b742-5f5dbdd2a316

static const uint8_t oximeter_service[16] = { 0x16, 0xa3, 0xd2, 0x0bd, 0x5d, 0x5f, 0x42, 0xb7, 0xa4, 0x48, 0x9e, 0x98, 0x54, 0x3d, 0x7f, 0xcd };

//characteristic - oximeter sensor
//202ce269-3f62-485b-a283-f48088e72f39
static const uint8_t oximeter_charac[16] = { 0x39, 0x2f, 0xe7, 0x88, 0x80, 0xf4, 0x83, 0xa2, 0x5b, 0x48, 0x62, 0x3f, 0x69, 0xe2, 0x2c, 0x20 };

#endif

typedef struct {

  uint16_t       charHandle;                 // GATT DB handle from gatt_db.h
  uint32_t       bufLength;                  // Number of bytes written to field buffer[5]
  uint8_t        buffer[MAX_BUFFER_LENGTH];  // The actual data buffer for the indication,
                                             //   need 5-bytes for HTM and 1-byte for button_state.
                                             //   For testing, test lengths 1 through 5,
                                             //   a length of 0 shall be considered an
                                             //   error, as well as lengths > 5

} queue_struct_t;

typedef struct {


  uint8_t myAddressType;
  bd_addr myAddress;
  uint8_t connection_handle;
  bool    connected;
  bool    indication;
  bool    indication_inFlight;
  uint8_t advertisingSetHandle;
  uint32_t rollover_cnt;
//  bool gatt_procedure;

  uint32_t service_handle;
  uint16_t char_handle;
  uint8_t *char_value;

  uint32_t button_service_handle;
  uint16_t button_char_handle;
//  uint8_t *button_char_value;
  bool button_pressed;
  bool button_indication;
  bool PB1_button_pressed;

  bool bonded;
  uint32_t passkey;
  bool displaying_passkey_to_user;



  bool gesture_indication;
  bool gesture_on;
  uint32_t gesture_service_handle;
  uint16_t gesture_char_handle;
  uint8_t gesture_value;

  bool pulse_indication;
  bool pulse_on;
  uint32_t pulse_service_handle;
  uint16_t pulse_char_handle;

} ble_data_struct_t;

/**
 * @brief Get pointer to the BLE data structure.
 *
 * @return Pointer to ble_data_struct_t.
 */
ble_data_struct_t* getBleDataPtr(void);

/**
 * @brief Handle BLE events.
 *
 * @param evt Pointer to the BLE event.
 */
void handle_ble_event(sl_bt_msg_t *evt);

#if DEVICE_IS_BLE_SERVER
/**
 * @brief Sends temperature data over BLE.
 */
void ble_SendTemperature();

/**
 * @brief Sends button status over BLE.
 *
 * This function sends the button status over BLE to the connected device.
 * If the device is connected and bonded, it writes the button status to the GATT server,
 * and if enabled, sends an indication or queues it for later sending.
 *
 * @param value The button status value to be sent.
 */
void ble_SendButtonStatus(uint8_t value);

void ble_SendPulseState(uint8_t * pulse_data);

void ble_SendGesture(uint8_t state);

#endif //DEVICE_IS_BLE_SERVER

#endif /* SRC_BLE_H_ */

