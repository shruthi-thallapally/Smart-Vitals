/*
 * ble.c
 *
 *  Created on: 20-Feb-2024
 *      Author: Shruthi Thallapally
 *   Reference: Code reference taken from the lecture slides and reference links provided in the lecture
 *   Note: Sometimes warnings are generated due to the log statements.
 *   I have commented LOG_ERROR statements to avoid generating unwanted warnings
 */


#include "src/ble_device_type.h" // DOS moved to the top so all includes can observe the value of DEVICE_IS_BLE_SERVER

#include "src/ble.h"
#include "app.h"
#include "stdint.h"
#include "src/timers.h"
#include "src/oscillators.h"
#include "src/i2c.h"
#include "src/gpio.h"
#include "src/lcd.h"
#include "src/scheduler.h"
#include "SparkFun_APDS9960.H"
#include "em_i2c.h"
#include "em_letimer.h"
#include "em_gpio.h"
#include "sl_i2cspm.h"
#include "sl_bt_api.h"
#include "gatt_db.h"
#include "math.h"

#define LOG_VALUES 0

#define PASSIVE_SCANNING 0
#define FLAGS 0X0F
// Include logging for this file
#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"



sl_status_t sc=0;

int qc=0;

ble_data_struct_t ble_data;

int32_t temp_in_c;

uint8_t server_addr[6]= SERVER_BT_ADDRESS;

uint16_t supervision_timeout = 0x50;   //800ms supervision timeout
uint16_t connection_int = 0x3c;         //75 ms connection interval
uint32_t advertise_int=0x190;           //250 ms advertisement interval
uint16_t slave_latency = 0x03;    //3 slave latency - slave can skip upto 3 connection events
uint16_t scan_window=0x28;        //scanning window of 25ms
uint16_t scan_int=0x50;           //scanning interval of 50ms

#if !DEVICE_IS_BLE_SERVER
static int32_t FLOAT_TO_INT32(const uint8_t *buffer_ptr);
#endif


/**
 * @brief Handle BLE events.
 *
 * @param evt Pointer to the BLE event.
 */
void handle_ble_event(sl_bt_msg_t *evt) {

  // Get pointer to BLE data structure
  ble_data_struct_t *bleData = getBleDataPtr();



  // Switch case to handle different BLE events
  switch(SL_BT_MSG_ID(evt->header)) {
    //for both server and client

    // System boot event
    case sl_bt_evt_system_boot_id:

      // Initialize display
      displayInit();

      // Get device address
      sc = sl_bt_system_get_identity_address(&(bleData->myAddress),
                                             &(bleData->myAddressType));
      // Check for errors
      if(sc != SL_STATUS_OK)
        {
          //     LOG_ERROR("sl_bt_system_get_identity_address() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
        }

#if DEVICE_IS_BLE_SERVER
      //FOR BLE SERVER

      // Create advertising set
      sc = sl_bt_advertiser_create_set(&(bleData->advertisingSetHandle));
      // Check for errors
      if(sc != SL_STATUS_OK)
        {
           LOG_ERROR("sl_bt_advertiser_create_set() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
        }
      // Set advertising timing
      sc = sl_bt_advertiser_set_timing(bleData->advertisingSetHandle,
                                       advertise_int,
                                       advertise_int,
                                       0,
                                       0);
      // Check for errors
      if(sc != SL_STATUS_OK)
        {
           LOG_ERROR("sl_bt_advertiser_set_timing() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
        }


      // Start advertising
      sc = sl_bt_advertiser_start(bleData->advertisingSetHandle,
                                  sl_bt_advertiser_general_discoverable,
                                  sl_bt_advertiser_connectable_scannable);
      // Check for errors
      if(sc != SL_STATUS_OK)
        {
          LOG_ERROR("sl_bt_advertiser_start() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
      }

      // Display server information
      displayPrintf(DISPLAY_ROW_NAME, "Server");
      displayPrintf(DISPLAY_ROW_CONNECTION, "Advertising");
#else
      // For BLE client

      // Set scanner mode
      sc=sl_bt_scanner_set_mode(sl_bt_gap_1m_phy,PASSIVE_SCANNING);
      if(sc!=SL_STATUS_OK)
        {
           LOG_ERROR("sl_bt_scanner_set_mode() returned !=0 status=0x%04x\n\r",(unsigned int)sc);

        }
      // Set scanner timing
      sc=sl_bt_scanner_set_timing(sl_bt_gap_1m_phy,scan_int,scan_window);
      if(sc!=SL_STATUS_OK)
        {
           LOG_ERROR("sl_bt_scanner_set_timing() returned !=0 status=0x%04x\n\r",(unsigned int)sc);

        }

      // Set default connection parameters
      sc=sl_bt_connection_set_default_parameters(connection_int,
                                                 connection_int,
                                                 slave_latency,
                                                 supervision_timeout,
                                                 0,
                                                 4);
      if(sc!=SL_STATUS_OK)
        {
           LOG_ERROR("sl_bt_scanner_set_default_parameters() returned !=0 status=0x%04x\n\r",(unsigned int)sc);

        }
      // Start scanner
      sc=sl_bt_scanner_start(sl_bt_gap_1m_phy,sl_bt_scanner_discover_generic);
      if(sc!=SL_STATUS_OK)
        {
           LOG_ERROR("sl_bt_scanner_start() returned !=0 status=0x%04x\n\r",(unsigned int)sc);

        }
      // Display client information
      displayPrintf(DISPLAY_ROW_NAME, "Client");
      displayPrintf(DISPLAY_ROW_CONNECTION, "Discovering");
#endif
      sc=sl_bt_sm_configure(FLAGS,sl_bt_sm_io_capability_displayyesno);
      if(sc != SL_STATUS_OK)
        {
          LOG_ERROR("sl_bt_sm_configure() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
        }

      //delete bonding data from server
      sc = sl_bt_sm_delete_bondings();
      if(sc != SL_STATUS_OK)
        {
          LOG_ERROR("sl_bt_sm_delete_bondings() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
        }
      // Display Bluetooth address and assignment
      displayPrintf(DISPLAY_ROW_BTADDR, "%02X:%02X:%02X:%02X:%02X:%02X",
                    bleData->myAddress.addr[0],
                    bleData->myAddress.addr[1],
                    bleData->myAddress.addr[2],
                    bleData->myAddress.addr[3],
                    bleData->myAddress.addr[4],
                    bleData->myAddress.addr[5]);
      displayPrintf(DISPLAY_ROW_ASSIGNMENT, "Course Project");

      // Initialize flags
      bleData->indication          = false;
      bleData->connected           = false;
      bleData->bonded              = false;
      bleData->indication_inFlight = false;
      bleData->button_indication   = false;
      bleData->button_pressed      = false;
      bleData->PB1_button_pressed  = false;

      bleData->gesture_value       =0x00;
      bleData->gesture_on          =false;
      bleData->gesture_indication  =false;

      bleData->pulse_indication    =false;
      bleData->pulse_on            =false;
      break;

      // Handle connection opened event
    case sl_bt_evt_connection_opened_id:

      // Set connected flag
      bleData->connected         = true;
      // Store connection handle
      bleData->connection_handle = evt->data.evt_connection_opened.connection;
 //     bleData->button_indication   = true;
#if DEVICE_IS_BLE_SERVER
      // For BLE server

      // Stop advertising
      sc = sl_bt_advertiser_stop(bleData->advertisingSetHandle);
      // Check for errors
      if(sc != SL_STATUS_OK)
        {
          LOG_ERROR("sl_bt_advertiser_stop() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
      }
      // Set connection parameters
      sc = sl_bt_connection_set_parameters(bleData->connection_handle,
                                           connection_int,
                                           connection_int,
                                           slave_latency,
                                           supervision_timeout,
                                           0,
                                           0);
      // Check for errors
      if(sc != SL_STATUS_OK)
        {
             LOG_ERROR("sl_bt_connection_set_parameters() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
      }

#else
      // Display server Bluetooth address
      displayPrintf(DISPLAY_ROW_BTADDR2,"%02X:%02X:%02X:%02X:%02X:%02X",
                    server_addr[0],
                    server_addr[1],
                    server_addr[2],
                    server_addr[3],
                    server_addr[4],
                    server_addr[5]);
#endif
      // Display connection status
      displayPrintf(DISPLAY_ROW_CONNECTION, "Connected");
      break;

      // Handle connection closed event
    case sl_bt_evt_connection_closed_id:

      gpioLed0SetOff();
      gpioLed1SetOff();

      displayPrintf(DISPLAY_ROW_8, "");
      displayPrintf(DISPLAY_ROW_9, "");
      displayPrintf(DISPLAY_ROW_TEMPVALUE, "");
      displayPrintf(DISPLAY_ROW_PASSKEY, "");
      displayPrintf(DISPLAY_ROW_ACTION, "");
      // Reset flags
      bleData->indication          = false;
      bleData->connected           = false;
      bleData->bonded              = false;
      bleData->indication_inFlight = false;
      bleData->button_indication   = false;
      bleData->button_pressed      = false;
      bleData->PB1_button_pressed  = false;

      bleData->gesture_value       =0x00;
      bleData->gesture_on          =false;
      bleData->gesture_indication  =false;
      bleData->pulse_indication    =false;
      bleData->pulse_on            =false;
      sc = sl_bt_sm_delete_bondings();
      if(sc != SL_STATUS_OK)
        {
          LOG_ERROR("sl_bt_sm_delete_bondings() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
        }
#if DEVICE_IS_BLE_SERVER
      // Restart advertising
      sc = sl_bt_advertiser_start(bleData->advertisingSetHandle,
                                  sl_bt_advertiser_general_discoverable,
                                  sl_bt_advertiser_connectable_scannable);
      // Check for errors
      if(sc != SL_STATUS_OK)
        {
            LOG_ERROR("sl_bt_advertiser_start() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
      }
      // Display advertising status
      displayPrintf(DISPLAY_ROW_CONNECTION, "Advertising");


#else
      // Restart scanner for BLE client
      sc=sl_bt_scanner_start(sl_bt_gap_1m_phy,sl_bt_scanner_discover_generic);
      if(sc!=SL_STATUS_OK)
        {
           LOG_ERROR("sl_bt_scanner_start() returned !=0 status=0x%04x\n\r",(unsigned int)sc);

        }

      // Display discovering status
      displayPrintf(DISPLAY_ROW_CONNECTION, "Discovering");
#endif

      // Clear temperature value and server Bluetooth address from display
      displayPrintf(DISPLAY_ROW_TEMPVALUE, "");
      displayPrintf(DISPLAY_ROW_BTADDR2,"");
      break;

    case sl_bt_evt_sm_confirm_bonding_id:

      // Confirm bonding
      sc=sl_bt_sm_bonding_confirm(bleData->connection_handle,1);
      // Log error if bonding confirmation fails
      if(sc != SL_STATUS_OK)
        {
          LOG_ERROR("sl_bt_sm_bonding_confirm() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
        }

      break;

      // Handle confirm passkey event
    case sl_bt_evt_sm_confirm_passkey_id:

      // Save passkey from event data
      bleData->passkey=evt->data.evt_sm_confirm_passkey.passkey;
      // Display passkey on the appropriate display row
      displayPrintf(DISPLAY_ROW_PASSKEY, "%d", bleData->passkey);
      // Display instructions for passkey confirmation
      displayPrintf(DISPLAY_ROW_ACTION, "Confirm with PB0");

      break;

      // Handle bonded event
    case sl_bt_evt_sm_bonded_id:

      // Display bonded status
      displayPrintf(DISPLAY_ROW_CONNECTION,"Bonded");
      // Set bonded flag to true
      bleData->bonded=true;
      // Clear passkey and action display rows
      displayPrintf(DISPLAY_ROW_PASSKEY,"");
      displayPrintf(DISPLAY_ROW_ACTION,"");

      break;

      // Handle bonding failed event
    case sl_bt_evt_sm_bonding_failed_id:

      // Log bonding failure reason
      LOG_ERROR("Bonding failed reason=0x%04x\n\r", evt->data.evt_sm_bonding_failed.reason);

      // Close the connection
      sc=sl_bt_connection_close(bleData->connection_handle);
      // Log error if connection close fails
      if(sc != SL_STATUS_OK)
        {
          LOG_ERROR("sl_bt_connection_close() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
        }
      break;
      // Handle connection parameters event
    case sl_bt_evt_connection_parameters_id:

      // Log connection parameters if enabled
#if LOG_VALUES
      // Log connection parameters
      //      LOG_INFO("Connection params: connection=%d, interval=%d, latency=%d, timeout=%d, securitymode=%d\n\r",
      //               (int) (evt->data.evt_connection_parameters.connection),
      //               (int) (evt->data.evt_connection_parameters.interval*1.25),
      //               (int) (evt->data.evt_connection_parameters.latency),
      //               (int) (evt->data.evt_connection_parameters.timeout*10),
      //               (int) (evt->data.evt_connection_parameters.security_mode) );

#endif
      break;


      // Handle external signal event
    case sl_bt_evt_system_external_signal_id:

      if(evt->data.evt_system_external_signal.extsignals ==Evt_Button_Pressed)
        {

#if DEVICE_IS_BLE_SERVER

        bool flag;
        if(bleData->PB1_button_pressed)
          {
            LOG_INFO("Gesture Sensor Enabled\n\r");
            displayPrintf(DISPLAY_ROW_10, "Enable gesture sensor");
            flag=SparkFun_APDS9960_init();
            if(flag != true)
              {
                  LOG_ERROR("Error initializing APDS\n\r");
              }
            else
              {
                 LOG_INFO("APDS initialized\n\r");
              }
            flag = enableGestureSensor(true);
            if(flag != true)
              {
                 LOG_ERROR("Error enabling gesture\n\r");
              }
            else
              {
               //LOG_INFO("gesture enabled\n\r");
                 displayPrintf(DISPLAY_ROW_10, "Gesture Sensor ON!");
                 bleData->gesture_on = true;
                 bleData->gesture_value = 0x00;

             }

          }


    #endif // SERVER
#if !DEVICE_IS_BLE_SERVER

        if(bleData->PB1_button_pressed)
          {
            sc = sl_bt_gatt_read_characteristic_value(bleData->connection_handle,
                                                      bleData->button_char_handle);
            if(sc != SL_STATUS_OK)
              {
                   LOG_ERROR("sl_bt_gatt_read_characteristic_value() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
              }
          }


#endif // CLIENT

        if(bleData->button_pressed && bleData->bonded==false)
          {
            sc = sl_bt_sm_passkey_confirm(bleData->connection_handle, 1);

            if(sc != SL_STATUS_OK)
              {
                  LOG_ERROR("sl_bt_sm_passkey_confirm() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
              }
          }
        }
      break;

      // Handle system soft timer event
    case sl_bt_evt_system_soft_timer_id:

      // Update display
      displayUpdate();


      break;

      // Handle GATT server events for BLE server
#if DEVICE_IS_BLE_SERVER


      // Handle GATT server characteristic status event
    case sl_bt_evt_gatt_server_characteristic_status_id:

      // Check if it's a temperature measurement characteristic
      if(evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_gesture_state)
        {
          // Check client configuration flags
          if (sl_bt_gatt_server_client_config == (sl_bt_gatt_server_characteristic_status_flag_t)
              evt->data.evt_gatt_server_characteristic_status.status_flags)
            {
              // Disable indication if necessary
              if(evt->data.evt_gatt_server_characteristic_status.client_config_flags == gatt_disable)
                {

                    displayPrintf(DISPLAY_ROW_9, "");
                }
              else if(evt->data.evt_gatt_server_characteristic_status.client_config_flags == gatt_indication) {
                               //bleData->gesture_indication = true;
                               //gpioLed0SetOn();
                               //LOG_INFO("gesture indication on\n\r");
                           }
            }
        }
      //check if the characteristic change is from Push button
           if(evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_oximeter_state) {

               //check if any status flag has been changed by client
               if (sl_bt_gatt_server_client_config == (sl_bt_gatt_server_characteristic_status_flag_t)
                   evt->data.evt_gatt_server_characteristic_status.status_flags) {

                   //check if indication flag is disabled
                   if(evt->data.evt_gatt_server_characteristic_status.client_config_flags == gatt_disable) {
                       //bleData->oximeter_indication = false;
                       //gpioLed1SetOff();
                       //LOG_INFO("oximeter indication off\n\r");

                   }

                   //check if indication flag is enabled
                   else if(evt->data.evt_gatt_server_characteristic_status.client_config_flags == gatt_indication) {
                       //bleData->oximeter_indication = true;
                       //gpioLed1SetOn();
                       //LOG_INFO("oximeter indication on\n\r");
                   }

               }

           }
      // Check confirmation status
      if (sl_bt_gatt_server_confirmation == (sl_bt_gatt_server_characteristic_status_flag_t)
          evt->data.evt_gatt_server_characteristic_status.status_flags) {
          bleData->indication_inFlight = false;
          //LOG_INFO("\n\r Confirmation received for an indication");
      }

      break;


      // Handle GATT server indication timeout event
    case sl_bt_evt_gatt_server_indication_timeout_id:

      LOG_ERROR("server indication timeout\n\r");
      // Reset indication flag
      bleData->indication = false;
      bleData->button_indication=false;
      break;

#else
      // Handle BLE client events

      // Handle scanner scan report event
    case sl_bt_evt_scanner_scan_report_id:

      // Check if the scanned device is the server
      if(evt->data.evt_scanner_scan_report.packet_type==0)
        {
          if((evt->data.evt_scanner_scan_report.address.addr[0] == server_addr[0]) &&
              (evt->data.evt_scanner_scan_report.address.addr[1] == server_addr[1]) &&
              (evt->data.evt_scanner_scan_report.address.addr[2] == server_addr[2]) &&
              (evt->data.evt_scanner_scan_report.address.addr[3] == server_addr[3]) &&
              (evt->data.evt_scanner_scan_report.address.addr[4] == server_addr[4]) &&
              (evt->data.evt_scanner_scan_report.address.addr[5] == server_addr[5]) &&
              (evt->data.evt_scanner_scan_report.address_type==0))
            {

              //stop scanner
              sc = sl_bt_scanner_stop();
              if(sc != SL_STATUS_OK)
                {
                  //        LOG_ERROR("sl_bt_scanner_stop() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
                }

              // Open connection to server
              sl_bt_connection_open(evt->data.evt_scanner_scan_report.address,
                                    evt->data.evt_scanner_scan_report.address_type,sl_bt_gap_1m_phy,NULL);
              if(sc != SL_STATUS_OK)
                {
                       LOG_ERROR("sl_bt_connection_open() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
                }
            }
        }
      break;

      // Handle GATT client events
    case sl_bt_evt_gatt_procedure_completed_id:

      if(evt->data.evt_gatt_procedure_completed.result==0x110F)
        {

          sc=sl_bt_sm_increase_security(bleData->connection_handle);

          if(sc!=SL_STATUS_OK)
            {
              LOG_ERROR("sl_bt_sm_increase_security() returned !=0 status=0x%04x\n\r",(unsigned int)sc);

            }
        }

      break;

    case sl_bt_evt_gatt_service_id:

      //LOG_INFO("Service Found Evt"); // DOS

      // Store service handle
      if(memcmp(evt->data.evt_gatt_service.uuid.data, thermo_service, sizeof(thermo_service)) == 0)
        {
          bleData->service_handle = evt->data.evt_gatt_service.service;
          LOG_INFO("HTM Service Found Evt"); // DOS
        }
      else if(memcmp(evt->data.evt_gatt_service.uuid.data, button_service, sizeof(button_service)) == 0)
        {
          bleData->button_service_handle = evt->data.evt_gatt_service.service;
          LOG_INFO("Btn Service Found Evt"); // DOS
        }
      else if(memcmp(evt->data.evt_gatt_service.uuid.data,gesture_service,sizeof(gesture_service))==0)
        {
          bleData->gesture_service_handle = evt->data.evt_gatt_service.service;
        }
      else if(memcmp(evt->data.evt_gatt_service.uuid.data, oximeter_service, sizeof(oximeter_service)) == 0)
        {
          bleData->pulse_service_handle = evt->data.evt_gatt_service.service;
        }
      break;

    case sl_bt_evt_gatt_characteristic_id:

      //LOG_INFO("Char Found Evt"); // DOS

      // Store characteristic handle
      if(memcmp(evt->data.evt_gatt_characteristic.uuid.data, thermo_char, sizeof(thermo_char)) == 0)
        {
          bleData->char_handle = evt->data.evt_gatt_characteristic.characteristic;
          //LOG_INFO("HTM Char Found Evt"); // DOS
        }
      else if(memcmp(evt->data.evt_gatt_characteristic.uuid.data, button_charac, sizeof(button_charac)) == 0)
        {
          bleData->button_char_handle = evt->data.evt_gatt_characteristic.characteristic;
         // LOG_INFO("btn Service Found Evt"); // DOS
        }
      else if(memcmp(evt->data.evt_gatt_characteristic.uuid.data, gesture_charac, sizeof(gesture_charac)) == 0)
        {
          bleData->gesture_char_handle = evt->data.evt_gatt_characteristic.characteristic;
          //LOG_INFO("gesture Service Found Evt");
        }
      else if(memcmp(evt->data.evt_gatt_characteristic.uuid.data, oximeter_charac, sizeof(oximeter_charac)) == 0)
        {
          bleData->pulse_char_handle = evt->data.evt_gatt_characteristic.characteristic;
          //LOG_INFO("oximeter Service Found Evt");
        }

      break;

    case sl_bt_evt_gatt_characteristic_value_id:

      if(evt->data.evt_gatt_characteristic_value.att_opcode==sl_bt_gatt_handle_value_indication)
        {
          // Send characteristic confirmation
          sc=sl_bt_gatt_send_characteristic_confirmation(bleData->connection_handle);
          if(sc != SL_STATUS_OK)
            {
              //     LOG_ERROR("sl_bt_gatt_set_characteristic_notification() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
            }

      if(evt->data.evt_gatt_characteristic_value.characteristic==bleData->char_handle)
        {
          // Store characteristic value
          bleData->char_value = &(evt->data.evt_gatt_characteristic_value.value.data[0]);
          // Convert temperature value and display it
          temp_in_c = FLOAT_TO_INT32(bleData->char_value);
          displayPrintf(DISPLAY_ROW_TEMPVALUE, "Temp=%d", temp_in_c);
        }

      if(evt->data.evt_gatt_characteristic_value.characteristic == bleData->gesture_char_handle)
        {
                //LOG_INFO("Got gesture indication\n\r");
           if(evt->data.evt_gatt_characteristic_value.value.data[0] == 0x01)
             {
                 bleData->gesture_value = 0x01;
                 displayPrintf(DISPLAY_ROW_9, "LEFT Gesture");
             }
           else if(evt->data.evt_gatt_characteristic_value.value.data[0] == 0x02)
             {
                 bleData->gesture_value = 0x02;
                 displayPrintf(DISPLAY_ROW_9, "RIGHT Gesture");
             }
           else if(evt->data.evt_gatt_characteristic_value.value.data[0] == 0x03)
             {
                bleData->gesture_value = 0x03;
                displayPrintf(DISPLAY_ROW_9, "UP Gesture");
             }
           else if(evt->data.evt_gatt_characteristic_value.value.data[0] == 0x04)
             {
               bleData->gesture_value = 0x04;
               displayPrintf(DISPLAY_ROW_9, "DOWN Gesture");
             }
           else if(evt->data.evt_gatt_characteristic_value.value.data[0] == 0x05)
             {
               bleData->gesture_value = 0x05;
               displayPrintf(DISPLAY_ROW_9, "NEAR Gesture");
               }
           else if(evt->data.evt_gatt_characteristic_value.value.data[0] == 0x06)
             {
               bleData->gesture_value = 0x06;
               displayPrintf(DISPLAY_ROW_9, "FAR Gesture");
             }
           else if(evt->data.evt_gatt_characteristic_value.value.data[0] == 0x00)
             {
               bleData->gesture_value = 0x00;
               displayPrintf(DISPLAY_ROW_9, "NONE Gesture");
             }

           if(evt->data.evt_gatt_characteristic_value.value.data[0] == 0x04)
             {
                displayPrintf(DISPLAY_ROW_10, "Gesture sensor OFF");
             }
           else
             {
               displayPrintf(DISPLAY_ROW_10, "Gesture sensor ON");
             }
         }
  }
      if(evt->data.evt_gatt_characteristic_value.characteristic == bleData->pulse_char_handle) {

              //LOG_INFO("Does it get oximeter indications?\n\r");

              if(bleData->gesture_value == 0x01){
                  displayPrintf(DISPLAY_ROW_TEMPVALUE, "Oxygen level: %d", evt->data.evt_gatt_characteristic_value.value.data[0]);
              }

              else if(bleData->gesture_value == 0x02){
                  displayPrintf(DISPLAY_ROW_TEMPVALUE, "Heart rate: %d", evt->data.evt_gatt_characteristic_value.value.data[0]);
              }
      }
      break;

#endif

      // Handle confirm bonding event
  }
}

#if DEVICE_IS_BLE_SERVER
/**
 * @brief Sends temperature data over BLE.
 */
void ble_SendTemperature()
{

  // Get pointer to BLE data structure
  ble_data_struct_t *bleData = getBleDataPtr();

  // Buffer to store temperature data
  uint8_t flags = 0x00;
  uint8_t htm_temperature_buffer[5];
  uint32_t htm_temperature_flt;
  uint8_t *p = htm_temperature_buffer;

  // Check if device is connected
  if(bleData->connected == true && (bleData->bonded == true))
    {
      // Convert temperature to Celsius
      int32_t temperature_in_c = ConvertTempToCelcius();
      // Set temperature flags
      UINT8_TO_BITSTREAM(p, flags);
      // Convert temperature to fixed point
      htm_temperature_flt = INT32_TO_FLOAT(temperature_in_c*1000, -3);
      // Set temperature data in buffer
      UINT32_TO_BITSTREAM(p, htm_temperature_flt);
      // Write temperature data to GATT server attribute
      sl_status_t sc = sl_bt_gatt_server_write_attribute_value(gattdb_temperature_measurement,
                                                               0,
                                                               5,
                                                               &htm_temperature_buffer[0]);

      // Check for errors
      if(sc != SL_STATUS_OK)
        {
          //        LOG_ERROR("sl_bt_gatt_server_write_attribute_value() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
        }

          if(bleData->indication_inFlight)
            {
                LOG_ERROR("Indication is in flight\n\r");
            }
          else
            {
              // Send temperature indication
              sc = sl_bt_gatt_server_send_indication(bleData->connection_handle,
                                                     gattdb_temperature_measurement,
                                                     5,
                                                     &htm_temperature_buffer[0]);
              // Check for errors
              if(sc != SL_STATUS_OK) {
                  //       LOG_ERROR("sl_bt_gatt_server_send_indication() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
              }
              else
                {
                  // Set indication in-flight flag
                  bleData->indication_inFlight = true;
                  // Log indication sent
                  LOG_INFO("Sent HTM indication, temp=%d\n\r", temperature_in_c);
                  displayPrintf(DISPLAY_ROW_TEMPVALUE, "Temp=%d", temperature_in_c);
                }
            }

    }
}


void ble_SendPulseState(uint8_t * pulse_data)
{
  // Get pointer to BLE data structure
  ble_data_struct_t *bleData=getBleDataPtr();

  if(bleData->connected==true)
    {
      sl_status_t sc = sl_bt_gatt_server_write_attribute_value(gattdb_oximeter_state,
                                                               0,
                                                               2,
                                                               &pulse_data[0]);

      if(sc != SL_STATUS_OK)
        {
          LOG_ERROR("sl_bt_gatt_server_write_attribute_value() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
        }

      if(bleData->bonded)
        {
          //check if any indication is inFlight
          if(bleData->indication_inFlight)
          {
              LOG_INFO("Indication in flight. cannot send pulse indication\n\r");
          }
          else
            {
              sc = sl_bt_gatt_server_send_indication(bleData->connection_handle,
                                                               gattdb_oximeter_state,
                                                               2,
                                                               &pulse_data[0]);
              if(sc != SL_STATUS_OK)
                {
                   LOG_ERROR("sl_bt_gatt_server_send_indication() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
                }
              else
                {
                  //indication is sent i.e. indication is in flight
                  bleData->indication_inFlight = true;
                  LOG_INFO("Pulse indication sent\n\r");
                 }
            }
        }
    }
}
void ble_SendGesture(uint8_t state)
{
  // Get pointer to BLE data structure
    ble_data_struct_t *bleData=getBleDataPtr();

    uint8_t gesture_buffer_value[2];
    gesture_buffer_value[0]=state;
    gesture_buffer_value[1]=0;

    if (bleData->connected==true)
      {
        sl_status_t sc=sl_bt_gatt_server_write_attribute_value(gattdb_gesture_state,
                                                               0,
                                                               1,
                                                               &gesture_buffer_value[0]);

      if(sc != SL_STATUS_OK)
        {
          LOG_ERROR("sl_bt_gatt_server_write_attribute_value() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
        }

    if(bleData->bonded==true)
      {
        //check if any indication is inFlight
        if(bleData->indication_inFlight)
          {
             LOG_INFO("Indication in flight. cannot send gesture indication\n\r");
          }

        //send indication of gesture measurement if no indication is inFlight
        else
          {
              sc = sl_bt_gatt_server_send_indication(bleData->connection_handle,
                                                             gattdb_gesture_state,
                                                             2,
                                                             &gesture_buffer_value[0]);
              if(sc != SL_STATUS_OK)
                {
                   LOG_ERROR("sl_bt_gatt_server_send_indication() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
                }

              else
                {

                  //indication is sent i.e. indication is in flight
                  bleData->indication_inFlight = true;
                  LOG_INFO("Gesture indication sent, state=%d\n\r", state);
                }
           }
      }
  }
}


/**
 * @brief Sends button status over BLE.
 *
 * This function sends the button status over BLE to the connected device.
 * If the device is connected and bonded, it writes the button status to the GATT server,
 * and if enabled, sends an indication or queues it for later sending.
 *
 * @param value The button status value to be sent.
 */
void ble_SendButtonStatus(uint8_t value)
{
  // Get pointer to BLE data structure
  ble_data_struct_t *bleData=getBleDataPtr();

  // Buffer to hold button status value
  uint8_t button_value_buffer[2];


  // Set button status value in the buffer
  button_value_buffer[0]=value;
  button_value_buffer[1]=0;

  // Check if device is connected
  if(bleData->connected==true)
    {
      // Write button status to GATT server
      sl_status_t sc=sl_bt_gatt_server_write_attribute_value(gattdb_button_state,
                                                             0,
                                                             1,
                                                             &button_value_buffer[0]);
      // Check if write operation was successful
      if(sc != SL_STATUS_OK)
        {
          LOG_ERROR("sl_bt_gatt_server_write_attribute_value() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
        }

      // Check if button indication is enabled and the device is bonded
      if(bleData->bonded==true)
        {

          // Check if an indication is already in flight
          if(bleData->indication_inFlight)
            {
             // LOG_INFO("\n\r Indication is in flight\n\r");
            }
          else
            {
              // Send indication
              sc=sl_bt_gatt_server_send_indication(bleData->connection_handle,
                                                   gattdb_button_state,
                                                   2,
                                                   &button_value_buffer[0]);
              // Check if indication send was successful
              if(sc!=SL_STATUS_OK)
                {
                  LOG_ERROR("sl_bt_gatt_server_send_indication() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
                }
              else
                {
                  // Set indication in flight flag
                 // LOG_INFO("\n\r Indication Sent successfully");
                  bleData->indication_inFlight=true;
                }
            }
        }
    }


}
#else


// -----------------------------------------------
// Private function, original from Dan Walkes. I fixed a sign extension bug.
// We'll need this for Client A7 assignment to convert health thermometer
// indications back to an integer. Convert IEEE-11073 32-bit float to signed integer.
// -----------------------------------------------
static int32_t FLOAT_TO_INT32(const uint8_t *buffer_ptr)
{
  uint8_t signByte = 0;
  int32_t mantissa;
  // input data format is:
  // [0] = flags byte, bit[0] = 0 -> Celsius; =1 -> Fahrenheit
  // [3][2][1] = mantissa (2's complement)
  // [4] = exponent (2's complement)
  // BT buffer_ptr[0] has the flags byte
  int8_t exponent = (int8_t)buffer_ptr[4];
  // sign extend the mantissa value if the mantissa is negative
  if (buffer_ptr[3] & 0x80) { // msb of [3] is the sign of the mantissa
      signByte = 0xFF;
  }
  mantissa = (int32_t) (buffer_ptr[1] << 0) |
      (buffer_ptr[2] << 8) |
      (buffer_ptr[3] << 16) |
      (signByte << 24) ;
  // value = 10^exponent * mantissa, pow() returns a double type
  return (int32_t) (pow(10, exponent) * mantissa);
} // FLOAT_TO_INT32

#endif

/**
 * @brief Get pointer to the BLE data structure.
 *
 * @return Pointer to ble_data_struct_t.
 */
ble_data_struct_t * getBleDataPtr()
{

  return (&ble_data);
}
