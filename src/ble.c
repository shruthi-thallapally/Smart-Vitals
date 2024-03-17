/*
 * ble.c
 *
 *  Created on: Feb 18, 2024
 *      Author: Tharuni Gelli
 */




#include "src/ble.h"
#include "app.h"
#include "math.h"


#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"

#include "src/ble.h"
#include "src/ble_device_type.h"
#include "src/i2c.h"
#include "sl_bt_api.h"


#define LOG_PARAMETER_VALUE 0

#define SCAN_PASSIVE 0

#define FLAG 0x0F



#if !DEVICE_IS_BLE_SERVER
static int32_t FLOAT_TO_INT32(const uint8_t *buffer_ptr);  //FLT_TO_UINT32 macro you used to write the value to the buffer in your Server code

#endif

uint8_t server_address[6] = SERVER_BT_ADDRESS; // array declared for storing server address

uint32_t advertising_interval=0x190;   //Given advertisement interval is 250msecs
uint16_t connection_interval = 0x3c;   //Given connection interval is 70msecs
uint16_t slave_latency = 0x03;         //slave can skip upto 3 connection events by following (3*70 = 210msecs < 250msecs ), Hence slave latency value is 3
uint16_t supervision_timeout = 0x50;   // Supervision timeout is always one more than slave latency which is 800msecs (1+slave_latency)
uint16_t scan_int = 0x50;         //scanning interval of 50 ms
uint16_t scan_window = 0x28;      //scanning window of 25 ms

// Declaration of a variable 'sc' of type 'sl_status_t' and initialization to 0
sl_status_t sc = 0;

int qc=0;

// Declaration of a structure 'ble_data_struct_t' variable named 'ble_Data'
ble_data_struct_t ble_Data;

// Function definition for returning a pointer to a structure of type 'ble_data_struct_t'
ble_data_struct_t * get_ble_DataPtr()
{
  // Return the memory address of the 'ble_Data' structure
  return (&ble_Data);
}


/**
 * @brief Increments a given pointer and wraps it to the beginning if it reaches QUEUE_DEPTH.
 *
 * This function takes a 32-bit unsigned integer pointer and increments it. If the incremented
 * value is equal to QUEUE_DEPTH, the pointer is wrapped to the beginning (set to 0).
 *
 * @param ptr The input pointer to be incremented.
 * @return The incremented pointer value, possibly wrapped to the beginning.
 */
uint32_t next_ptr(uint32_t ptr)
{
  // Check if incrementing ptr by 1 would reach QUEUE_DEPTH
  if(ptr+1==QUEUE_DEPTH)
  {
    // If true, wrap the pointer to the beginning (set to 0)
    ptr=0;
  }else
  {
    // If false, simply increment the pointer
  ptr=ptr+1;
  }
  return(ptr);
} // nextPtr()

/**
 * @brief Writes data to the queue and updates the queue state.
 *
 * This function writes data to the queue specified by the parameters and updates
 * the state of the queue accordingly. It checks for queue fullness and buffer length
 * validity before writing to the queue.
 *
 * @param charHandle The handle associated with the character to be written.
 * @param bufLength The length of the buffer to be written.
 * @param buffer A pointer to the buffer containing the data to be written.
 * @return true if the write operation failed (due to queue being full or invalid buffer length), false otherwise.
 */
int write_queue (queue_struct_t write_data)
{
  ble_data_struct_t *ble_Data=get_ble_DataPtr();

  if(ble_Data->full)
    {
      LOG_ERROR("Buffer is full\n\r");
      return -1;
    }

  ble_Data->Indication_Buffer[ble_Data->w_ptr]=write_data;
      // Increment the write pointer using the nextPtr function
  ble_Data->w_ptr=next_ptr(ble_Data->w_ptr);
    // Check if the next position after write pointer is equal to the read pointer
    if ((ble_Data->w_ptr)==(ble_Data->r_ptr))
    {
      // If true, set 'full' flag to true to indicate the queue is full
        ble_Data->full=true;
    }

    // Return false to indicate a successful write operation
    return 0;

} // write_queue()

/**
 * @brief Reads data from the queue and updates the queue state.
 *
 * This function reads data from the queue and updates the state of the queue accordingly.
 * It checks for queue emptiness, validates buffer length, and updates the pointers.
 *
 * @param charHandle A pointer to store the character handle read from the queue.
 * @param bufLength A pointer to store the buffer length read from the queue.
 * @param buffer A pointer to store the data read from the queue.
 * @return true if the read operation failed (due to queue being empty or invalid buffer length), false otherwise.
 */
int read_queue ()
{
  ble_data_struct_t *ble_Data=get_ble_DataPtr();
  // Check if the queue is empty
  if(ble_Data->w_ptr==ble_Data->r_ptr && !ble_Data->full)
    {
        LOG_ERROR("Buffer is empty\n\r");
        return -1;
    }
  queue_struct_t read_data=ble_Data->Indication_Buffer[ble_Data->r_ptr];

  //send the indication to client
   sc = sl_bt_gatt_server_send_indication(ble_Data->Connection_Handle,
                                          read_data.charHandle,
                                          read_data.bufLength,
                                          &(read_data.buffer[0]));

   //check indication return status
   if(sc != SL_STATUS_OK) {
       LOG_ERROR("sl_bt_gatt_server_send_indication() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
       return -1;
   }
   else {

       //if buffer was previously full, reset it since we read an element from there
       if(ble_Data->full)
         ble_Data->full=false;

       //increment the read pointer
       ble_Data->r_ptr = next_ptr(ble_Data->r_ptr);

       //indication is sent i.e. indication is in flight
       ble_Data->Indication_InFlight = true;
       //decrement queued indications
       ble_Data->Queued_Indication--;

       return 0;
   }

} // read_queue()
/**
 * @brief Handle BLE events.
 *
 * @param evt Pointer to the BLE event.
 */
void handle_ble_event(sl_bt_msg_t *evt)
{

  // Get pointer to BLE data structure
  ble_data_struct_t *ble_Data = get_ble_DataPtr();

  // Switch case to handle different BLE events
  switch(SL_BT_MSG_ID(evt->header)) {
    //for both server and client

    // System boot event
     case sl_bt_evt_system_boot_id:

       // Initialize display
       displayInit();

       // Get device address
       sc = sl_bt_system_get_identity_address(&(ble_Data->My_Address),
                                             &(ble_Data->My_AddressType));
       // Check for errors
      if(sc != SL_STATUS_OK) {
     //     LOG_ERROR("sl_bt_system_get_identity_address() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
      }

#if DEVICE_IS_BLE_SERVER
      //FOR BLE SERVER

      // Create advertising set
       sc = sl_bt_advertiser_create_set(&(ble_Data->Advertising_Set_Handle));
       // Check for errors
       if(sc != SL_STATUS_OK) {
      //    LOG_ERROR("sl_bt_advertiser_create_set() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
      }
       // Set advertising timing
       sc = sl_bt_advertiser_set_timing(ble_Data->Advertising_Set_Handle,
                                        advertising_interval,
                                        advertising_interval,
                                       0,
                                       0);
       // Check for errors
      if(sc != SL_STATUS_OK) {
      //    LOG_ERROR("sl_bt_advertiser_set_timing() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
      }

      sc=sl_bt_sm_configure(FLAG,sl_bt_sm_io_capability_displayyesno);
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
      // Start advertising
        sc = sl_bt_advertiser_start(ble_Data->Advertising_Set_Handle,
                                  sl_bt_advertiser_general_discoverable,
                                  sl_bt_advertiser_connectable_scannable);
        // Check for errors
        if(sc != SL_STATUS_OK) {
     //     LOG_ERROR("sl_bt_advertiser_start() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
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
       //     LOG_ERROR("sl_bt_scanner_set_mode() returned !=0 status=0x%04x\n\r",(unsigned int)sc);

          }
        // Set scanner timing
        sc=sl_bt_scanner_set_timing(sl_bt_gap_1m_phy,scan_int,scan_window);
        if(sc!=SL_STATUS_OK)
        {
    //        LOG_ERROR("sl_bt_scanner_set_timing() returned !=0 status=0x%04x\n\r",(unsigned int)sc);

        }

        // Set default connection parameters
        sc=sl_bt_connection_set_default_parameters(connection_interval,
                                                   connection_interval,
                                                   slave_latency,
                                                   supervision_timeout,
                                                   0,
                                                   4);
        if(sc!=SL_STATUS_OK)
        {
        //    LOG_ERROR("sl_bt_scanner_set_default_parameters() returned !=0 status=0x%04x\n\r",(unsigned int)sc);

        }
        // Start scanner
        sc=sl_bt_scanner_start(sl_bt_gap_1m_phy,sl_bt_scanner_discover_generic);
        if(sc!=SL_STATUS_OK)
        {
       //     LOG_ERROR("sl_bt_scanner_start() returned !=0 status=0x%04x\n\r",(unsigned int)sc);

        }
        // Display client information
        displayPrintf(DISPLAY_ROW_NAME, "Client");
        displayPrintf(DISPLAY_ROW_CONNECTION, "Discovering");
#endif
        // Display Bluetooth address and assignment
        displayPrintf(DISPLAY_ROW_BTADDR, "%02X:%02X:%02X:%02X:%02X:%02X",
                            ble_Data->My_Address.addr[0],
                            ble_Data->My_Address.addr[1],
                            ble_Data->My_Address.addr[2],
                            ble_Data->My_Address.addr[3],
                            ble_Data->My_Address.addr[4],
                            ble_Data->My_Address.addr[5]);
        displayPrintf(DISPLAY_ROW_ASSIGNMENT, "A8");

        // Initialize flags
      ble_Data->Indication          = false;
      ble_Data->Connected           = false;
      ble_Data->Bonded              = false;
      ble_Data->Indication_InFlight = false;
      ble_Data->Button_Indication   = false;
      ble_Data->Button_Pressed      = false;
      ble_Data->Gatt_Procedure      = false;
      ble_Data->w_ptr                = 0;
      ble_Data->r_ptr                = 0;
      ble_Data->Queued_Indication  = 0;

      break;

      // Handle connection opened event
      case sl_bt_evt_connection_opened_id:

        // Set connected flag
         ble_Data->Connected         = true;
         // Store connection handle
         ble_Data->Connection_Handle = evt->data.evt_connection_opened.connection;

#if DEVICE_IS_BLE_SERVER
         // For BLE server

         // Stop advertising
       sc = sl_bt_advertiser_stop(ble_Data->Advertising_Set_Handle);
       // Check for errors
      if(sc != SL_STATUS_OK) {
   //       LOG_ERROR("sl_bt_advertiser_stop() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
      }
      // Set connection parameters
       sc = sl_bt_connection_set_parameters(ble_Data->Connection_Handle,
                                           connection_interval,
                                           connection_interval,
                                           slave_latency,
                                           supervision_timeout,
                                           0,
                                           0);
       // Check for errors
      if(sc != SL_STATUS_OK) {
     //     LOG_ERROR("sl_bt_connection_set_parameters() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
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

      displayPrintf(DISPLAY_ROW_9, "");
      displayPrintf(DISPLAY_ROW_TEMPVALUE, "");
      displayPrintf(DISPLAY_ROW_PASSKEY, "");
      displayPrintf(DISPLAY_ROW_ACTION, "");
      // Reset flags
      ble_Data->Indication          = false;
      ble_Data->Connected           = false;
      ble_Data->Bonded              = false;
      ble_Data->Indication_InFlight = false;
      ble_Data->Button_Indication   = false;
      ble_Data->Button_Pressed      = false;
      ble_Data->Gatt_Procedure      = false;
      ble_Data->w_ptr                = 0;
      ble_Data->r_ptr                = 0;
      ble_Data->Queued_Indication  = 0;

#if DEVICE_IS_BLE_SERVER
      // For BLE server

      sc = sl_bt_sm_delete_bondings();
      if(sc != SL_STATUS_OK)
      {
          LOG_ERROR("sl_bt_sm_delete_bondings() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
      }
      // Restart advertising
      sc = sl_bt_advertiser_start(ble_Data->Advertising_Set_Handle,
                                  sl_bt_advertiser_general_discoverable,
                                  sl_bt_advertiser_connectable_scannable);
      // Check for errors
      if(sc != SL_STATUS_OK) {
      //    LOG_ERROR("sl_bt_advertiser_start() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
      }
      // Display advertising status
      displayPrintf(DISPLAY_ROW_CONNECTION, "Advertising");


#else
      // Restart scanner for BLE client
      sc=sl_bt_scanner_start(sl_bt_gap_1m_phy,sl_bt_scanner_discover_generic);
      if(sc!=SL_STATUS_OK)
      {
     //     LOG_ERROR("sl_bt_scanner_start() returned !=0 status=0x%04x\n\r",(unsigned int)sc);

      }

      // Display discovering status
      displayPrintf(DISPLAY_ROW_CONNECTION, "Discovering");
#endif
      // Clear temperature value and server Bluetooth address from display
      displayPrintf(DISPLAY_ROW_TEMPVALUE, "");
      displayPrintf(DISPLAY_ROW_BTADDR2,"");

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

      // Check if the external signal is for button pressed event
      if(evt->data.evt_system_external_signal.extsignals== evt_Button_Pressed)
      {
          // Display message indicating button is pressed
          displayPrintf(DISPLAY_ROW_9, "Button Pressed");

          // If device is not bonded
          if(ble_Data->Bonded==false)
          {
              // Confirm passkey for pairing
              sc=sl_bt_sm_passkey_confirm(ble_Data->Connection_Handle,1);
              // Check if passkey confirmation was successful
              if(sc != SL_STATUS_OK)
              {
                  // Log error if passkey confirmation fails
                  LOG_ERROR("sl_bt_sm_passkey_confirm() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
              }

          }
          // If device is bonded, send button status
          if(ble_Data->Bonded)
          {
              SendButtonState_ble(0x01);
          }
      }
      // Check if the external signal is for button released event
      else if(evt->data.evt_system_external_signal.extsignals==evt_Button_Released)
      {
          // Display message indicating button is released
          displayPrintf(DISPLAY_ROW_9, "Button Released");

          // If device is bonded, send button status
          if(ble_Data->Bonded)
          {
              SendButtonState_ble(0x00);
          }
      }

      break;

      // Handle GATT server events for BLE server
#if DEVICE_IS_BLE_SERVER

      // Handle GATT server indication timeout event
    case sl_bt_evt_gatt_server_indication_timeout_id:

    //     LOG_ERROR("server indication timeout\n\r");
      // Reset indication flag
         ble_Data->Indication = false;
         ble_Data->Button_Indication=false;
         break;

         // Handle GATT server characteristic status event
    case sl_bt_evt_gatt_server_characteristic_status_id:

      // Check if it's a temperature measurement characteristic
       if(evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_temperature_measurement)
         {
           // Check client configuration flags
          if (sl_bt_gatt_server_client_config == (sl_bt_gatt_server_characteristic_status_flag_t)
              evt->data.evt_gatt_server_characteristic_status.status_flags)
          {
              // Disable indication if necessary
              if(evt->data.evt_gatt_server_characteristic_status.client_config_flags == gatt_disable)
              {
                  ble_Data->Indication = false;
                  gpioLed0SetOff();
                  displayPrintf(DISPLAY_ROW_TEMPVALUE, "");
              }

              else if(evt->data.evt_gatt_server_characteristic_status.client_config_flags == gatt_indication)
              {
                  // Enable indication
                  ble_Data->Indication = true;
                  gpioLed0SetOn();
              }

          }
         }
       if(evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_button_state)
       {

         if (sl_bt_gatt_server_client_config == (sl_bt_gatt_server_characteristic_status_flag_t)
               evt->data.evt_gatt_server_characteristic_status.status_flags)
         {
             // Disable indication if necessary
            if(evt->data.evt_gatt_server_characteristic_status.client_config_flags == gatt_disable)
            {
               ble_Data->Indication = false;
               gpioLed1SetOff();

            }

            else if(evt->data.evt_gatt_server_characteristic_status.client_config_flags == gatt_indication)
            {
                // Enable indication
                ble_Data->Indication = true;
                gpioLed1SetOn();
            }

         }
       }

          // Check confirmation status
           if (sl_bt_gatt_server_confirmation == (sl_bt_gatt_server_characteristic_status_flag_t)
              evt->data.evt_gatt_server_characteristic_status.status_flags) {
              ble_Data->Indication_InFlight = false;
          }

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
              (evt->data.evt_scanner_scan_report.address_type==0)) {

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
           //       LOG_ERROR("sl_bt_connection_open() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
              }
          }
        }
      break;

      // Handle GATT client events
    case sl_bt_evt_gatt_procedure_completed_id:

      // Reset GATT procedure flag
      bleData->gatt_procedure=false;

      break;

    case sl_bt_evt_gatt_service_id:
      // Store service handle
      bleData->service_handle=evt->data.evt_gatt_service.service;

      break;

    case sl_bt_evt_gatt_characteristic_id:
      // Store characteristic handle
      bleData->char_handle=evt->data.evt_gatt_characteristic.characteristic;

      break;

    case sl_bt_evt_gatt_characteristic_value_id:
      // Send characteristic confirmation
      sc=sl_bt_gatt_send_characteristic_confirmation(bleData->connection_handle);
      if(sc != SL_STATUS_OK)
      {
     //     LOG_ERROR("sl_bt_gatt_set_characteristic_notification() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
      }
      // Store characteristic value
      bleData->char_value = &(evt->data.evt_gatt_characteristic_value.value.data[0]);
      // Convert temperature value and display it
      temp_in_c = FLOAT_TO_INT32((bleData->char_value));
      displayPrintf(DISPLAY_ROW_TEMPVALUE, "Temp=%d", temp_in_c);

      break;

     #endif

      // Handle system soft timer event
      case sl_bt_evt_system_soft_timer_id:

        // Update display
          displayUpdate();

          // Check if there are queued indications and no indication is currently in flight
          if(ble_Data->Queued_Indication!=0 && !ble_Data->Indication_InFlight)
          {

              // Read from the queue
              qc=read_queue();
              // Log error if indication dequeue fails
              if(qc != 0)
              {
                  LOG_ERROR("Indication dequeue failed\n\r");
              }

          }

       break;

       // Handle confirm bonding event
      case sl_bt_evt_sm_confirm_bonding_id:

        // Confirm bonding
        sc=sl_bt_sm_bonding_confirm(ble_Data->Connection_Handle,1);
        // Log error if bonding confirmation fails
        if(sc != SL_STATUS_OK)
        {
            LOG_ERROR("sl_bt_sm_bonding_confirm() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
        }

      break;

      // Handle confirm passkey event
      case sl_bt_evt_sm_confirm_passkey_id:

        // Save passkey from event data
        ble_Data->passkey=evt->data.evt_sm_confirm_passkey.passkey;
        // Display passkey on the appropriate display row
        displayPrintf(DISPLAY_ROW_PASSKEY, "%d", ble_Data->passkey);
        // Display instructions for passkey confirmation
        displayPrintf(DISPLAY_ROW_ACTION, "Confirm with PB0");

      break;

      // Handle bonded event
      case sl_bt_evt_sm_bonded_id:

        // Display bonded status
        displayPrintf(DISPLAY_ROW_CONNECTION,"Bonded");
        // Set bonded flag to true
        ble_Data->Bonded=true;
        // Clear passkey and action display rows
        displayPrintf(DISPLAY_ROW_PASSKEY,"");
        displayPrintf(DISPLAY_ROW_ACTION,"");

        break;

        // Handle bonding failed event
      case sl_bt_evt_sm_bonding_failed_id:

        // Log bonding failure reason
        LOG_ERROR("Bonding failed reason=0x%04x\n\r", evt->data.evt_sm_bonding_failed.reason);

        // Close the connection
        sc=sl_bt_connection_close(ble_Data->Connection_Handle);
        // Log error if connection close fails
        if(sc != SL_STATUS_OK)
        {
            LOG_ERROR("sl_bt_connection_close() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
        }
      break;

  }
}

#if DEVICE_IS_BLE_SERVER

void SendTemp_ble()
{
  // Declaration of local variables
  uint8_t htm_temperature_buffer[5]; // Buffer to hold temperature data
  uint8_t *p = htm_temperature_buffer; // Pointer to the buffer
  uint32_t htm_temperature_flt; // Variable to hold the temperature in floating point format
  uint8_t flags = 0x00; // Flags variable initialized to 0

  // Get a pointer to the BLE data structure
  ble_data_struct_t *ble_Data = get_ble_DataPtr();

  // Check if the device is connected to a BLE client
  if(ble_Data->Connected == true)
  {
    // Convert temperature value to Celsius
    int32_t temperature_in_celcius = ConvertValueToCelcius();

    // Pack flags into the buffer
    UINT8_TO_BITSTREAM(p, flags);

    // Convert temperature to floating point with 3 decimal places
    htm_temperature_flt = INT32_TO_FLOAT(temperature_in_celcius*1000, -3);

    // Pack the floating point temperature value into the buffer
    UINT32_TO_BITSTREAM(p, htm_temperature_flt);

    // Write the temperature value to the GATT database
    sl_status_t sc = sl_bt_gatt_server_write_attribute_value(gattdb_temperature_measurement,
                                                             0,
                                                             5,
                                                             &htm_temperature_buffer[0]);

    // Check if writing to the GATT database was successful
    if(sc != SL_STATUS_OK)
    {
      LOG_ERROR("sl_bt_gatt_server_write_attribute_value() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
    }

    // Check if indication is enabled and there is no indication already in flight
    if (ble_Data->Indication == true && ble_Data->Indication_InFlight == false)
    {
      // Send indication to the BLE client
      sc = sl_bt_gatt_server_send_indication(ble_Data->Connection_Handle,
                                             gattdb_temperature_measurement,
                                             5,
                                             &htm_temperature_buffer[0]);

      // Check if sending indication was successful
      if(sc != SL_STATUS_OK)
      {
        LOG_ERROR("sl_bt_gatt_server_send_indication() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
      }
      else
      {
        // Mark indication as in flight
        ble_Data->Indication_InFlight = true;
        LOG_INFO("Sent indication to get Temp=%d\n\r", temperature_in_celcius);

      }
      displayPrintf(DISPLAY_ROW_TEMPVALUE, "Temp=%d", temperature_in_celcius);
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
void SendButtonState_ble(uint8_t value)
{
  // Get pointer to BLE data structure
  ble_data_struct_t *ble_Data=get_ble_DataPtr();

  // Buffer to hold button status value
  uint8_t button_value_buffer[2];
  // Structure to hold indication data
  queue_struct_t Indication_Data;

  // Set button status value in the buffer
  button_value_buffer[0]=value;
  button_value_buffer[1]=0;

  // Check if device is connected
  if(ble_Data->Connected==true)
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
     if(ble_Data->Button_Indication==true && ble_Data->Bonded==true)
     {
         // Check if an indication is already in flight
        if(ble_Data->Indication_InFlight)
          {
            // Prepare indication data
            Indication_Data.charHandle=gattdb_button_state;
            Indication_Data.bufLength=2;

            // Copy button status value to indication buffer
            for(int i=0;i<2;i++)
              {
                Indication_Data.buffer[i]=button_value_buffer[i];
              }

            // Write indication data to queue
            qc=write_queue(Indication_Data);

            // Increment queued indications count if write was successful
            if(qc == 0)
            {
               ble_Data->Queued_Indication++;
            }
            else
            {
              LOG_ERROR("Indication enqueue failed\n\r");
            }
          }
        else
        {
            // Send indication
            sc=sl_bt_gatt_server_send_indication(ble_Data->Connection_Handle,
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
                ble_Data->Indication_InFlight=true;
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
