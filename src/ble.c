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

static int32_t FLOAT_TO_INT32(const uint8_t *buffer_ptr);  //FLT_TO_UINT32 macro you used to write the value to the buffer in your Server code

uint8_t server_address[6] = SERVER_BT_ADDRESS; // array declared for storing server address

uint32_t advertising_interval=0x190;   //Given advertisement interval is 250msecs
uint16_t connection_interval = 0x3c;   //Given connection interval is 70msecs
uint16_t slave_latency = 0x03;         //slave can skip upto 3 connection events by following (3*70 = 210msecs < 250msecs ), Hence slave latency value is 3
uint16_t supervision_timeout = 0x50;   // Supervision timeout is always one more than slave latency which is 800msecs (1+slave_latency)
uint16_t scan_int = 0x50;         //scanning interval of 50 ms
uint16_t scan_window = 0x28;      //scanning window of 25 ms

// Declaration of a variable 'sc' of type 'sl_status_t' and initialization to 0
sl_status_t sc = 0;

// Declaration of a structure 'ble_data_struct_t' variable named 'ble_Data'
ble_data_struct_t ble_Data;

// Function definition for returning a pointer to a structure of type 'ble_data_struct_t'
ble_data_struct_t * get_ble_DataPtr()
{
  // Return the memory address of the 'ble_Data' structure
  return (&ble_Data);
}


int32_t temperature_in_c; // temperature storing variable used in client event and also to display temp on client

void handle_ble_event(sl_bt_msg_t *evt)
{
   ble_data_struct_t *ble_Data = get_ble_DataPtr();

   switch (SL_BT_MSG_ID(evt->header))
   {
         case sl_bt_evt_system_boot_id:

           displayInit();
            // handle boot event
           /*
           * Read the Bluetooth identity address used by the device, which can be a public
           * or random static device address.
           *
           * @param[out] address Bluetooth public address in little endian format
           * @param[out] type Address type
           *     - <b>0:</b> Public device address
           *     - <b>1:</b> Static random address
           *
           * @return SL_STATUS_OK if successful. Error code otherwise.
           */
            sc = sl_bt_system_get_identity_address(&(ble_Data->My_Address),
                                                        &(ble_Data->My_AddressType));
            if(sc != SL_STATUS_OK)
            {
                LOG_ERROR("sl_bt_system_get_identity_address() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
            }

             #if DEVICE_IS_BLE_SERVER

             // for servers only
             /*
             * Create an advertising set. The handle of the created advertising set is
             * returned in response.
             *
             * @param[out] handle Advertising set handle
             *
             * @return SL_STATUS_OK if successful. Error code otherwise.
             */
             sc = sl_bt_advertiser_create_set(&(ble_Data->Advertising_Set_Handle));
             if(sc != SL_STATUS_OK)
             {
                 LOG_ERROR("sl_bt_advertiser_create_set() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
             }

             /*
             * Set the advertising timing parameters of the given advertising set. This
             * setting will take effect next time that advertising is enabled.
             *
             * @param[in] handle Advertising set handle
             * @param[in] interval_min - Minimum advertising interval. Value in units of 0.625 ms
             * @param[in] interval_max - Maximum advertising interval. Value in units of 0.625 ms
             * @param[in] duration @parblock
             * @param[in] maxevents @parblock
             * @return SL_STATUS_OK if successful. Error code otherwise.
             */
             sc = sl_bt_advertiser_set_timing(ble_Data->Advertising_Set_Handle,
                                                  advertising_interval,
                                                  advertising_interval,
                                                  0,
                                                  0);
             if(sc != SL_STATUS_OK)
             {
                 LOG_ERROR("sl_bt_advertiser_set_timing() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
             }

              /*
              * Start advertising of a given advertising set with specified discoverable and
              * connectable modes.
              * The number of concurrent advertising is limited by MAX_ADVERTISERS configuration.
              * @param[in] handle Advertising set handle
              * @param[in] discover Enum @ref sl_bt_advertiser_discoverable_mode_t. Discoverable mode. sl_bt_advertiser_general_discoverable - Discoverable using general discovery procedure
              * @param[in] connect Enum @ref sl_bt_advertiser_connectable_mode_t. Connectable mode. sl_bt_advertiser_connectable_scannable - Undirected connectable scannable.
              * @return SL_STATUS_OK if successful. Error code otherwise.
              */
             sc = sl_bt_advertiser_start(ble_Data->Advertising_Set_Handle,
                                              sl_bt_advertiser_general_discoverable,
                                              sl_bt_advertiser_connectable_scannable);
                  if(sc != SL_STATUS_OK) {
                      LOG_ERROR("sl_bt_advertiser_start() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
                  }

                  displayPrintf(DISPLAY_ROW_NAME, "Server");
                  displayPrintf(DISPLAY_ROW_CONNECTION, "Advertising");
            #else

                  /* Set the scan mode on the specified PHYs. If the device is currently scanning
                   for advertising devices on PHYs, new parameters will take effect when
                   scanning is restarted.

                   @param[in] phys PHYs for which the parameters are set.
                   @param[in] scan_mode @parblock
                     Scan mode. Values:
                       - <b>0:</b> Passive scanning
                       - <b>1:</b> Active scanning    */
                  sc = sl_bt_scanner_set_mode(sl_bt_gap_1m_phy, SCAN_PASSIVE);
                  if(sc != SL_STATUS_OK) {
                      LOG_ERROR("sl_bt_scanner_set_mode() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
                  }

                  /* Set the scanning timing parameters on the specified PHYs. If the device is
                     currently scanning for advertising devices on PHYs, new parameters will take
                      effect when scanning is restarted.

                    @param[in] phys PHYs for which the parameters are set.
                    @param[in] scan_interval @parblock
                       Scan interval is defined as the time interval when the device starts its
                        last scan until it begins the subsequent scan. In other words, how often to
                        scan

                    @param[in] scan_window @parblock
                    Scan window defines the duration of the scan which must be less than or
                    equal to the @p scan_interval */
                  sc = sl_bt_scanner_set_timing(sl_bt_gap_1m_phy, scan_int, scan_window);
                  if(sc != SL_STATUS_OK) {
                      LOG_ERROR("sl_bt_scanner_set_timing() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
                  }

                  /* Set the default Bluetooth connection parameters. The values are valid for all
                     subsequent connections initiated by this device. To change parameters of an
                   already established connection, use the command @ref
                   sl_bt_connection_set_parameters.   */
                  sc = sl_bt_connection_set_default_parameters(connection_interval,
                                                               connection_interval,
                                                               slave_latency,
                                                               supervision_timeout,
                                                               0,
                                                               4);
                  if(sc != SL_STATUS_OK) {
                      LOG_ERROR("sl_bt_scanner_set_default_parameters() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
                  }

                  /*  Start the GAP discovery procedure to scan for advertising devices on the
                      specified scanning PHYs. To cancel an ongoing discovery procedure, use the
                      @ref sl_bt_scanner_stop command.

                      @param[in] scanning_phy @parblock
                      The scanning PHYs.

                        @param[in] discover_mode Enum @ref sl_bt_scanner_discover_mode_t. Bluetooth
                      discovery Mode. Values:
                  - <b>sl_bt_scanner_discover_limited (0x0):</b> Discover only limited
                    discoverable devices.
                  - <b>sl_bt_scanner_discover_generic (0x1):</b> Discover limited and
                    generic discoverable devices.
                  - <b>sl_bt_scanner_discover_observation (0x2):</b> Discover all devices.  */

                  sc = sl_bt_scanner_start(sl_bt_gap_1m_phy, sl_bt_scanner_discover_generic);
                  if(sc != SL_STATUS_OK) {
                      LOG_ERROR("sl_bt_scanner_start() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
                  }

                  displayPrintf(DISPLAY_ROW_NAME, "Client");
                  displayPrintf(DISPLAY_ROW_CONNECTION, "Discovering");
         #endif

                  displayPrintf(DISPLAY_ROW_BTADDR, "%02X:%02X:%02X:%02X:%02X:%02X",
                                     ble_Data->My_Address.addr[0],
                                     ble_Data->My_Address.addr[1],
                                     ble_Data->My_Address.addr[2],
                                     ble_Data->My_Address.addr[3],
                                     ble_Data->My_Address.addr[4],
                                     ble_Data->My_Address.addr[5]);
                       displayPrintf(DISPLAY_ROW_ASSIGNMENT, "A7");
                       //initialize connection and indication flags
                       ble_Data->Connected           = false;
                       ble_Data->Indication          = false;
                       ble_Data->Indication_InFlight = false;
                       ble_Data->Gatt_Procedure      = false;
                       break;

         case sl_bt_evt_connection_opened_id:

           ble_Data->Connected = true;


           ble_Data->Connection_Handle = evt->data.evt_connection_opened.connection;

           #if DEVICE_IS_BLE_SERVER
            /*
            * Stop the advertising of the given advertising set. Counterpart with @ref
            * sl_bt_advertiser_start.
            *
            * This command does not affect the enable state of the periodic advertising
            * set, i.e., periodic advertising is not stopped.
            *
            * @param[in] handle Advertising set handle
            *
            * @return SL_STATUS_OK if successful. Error code otherwise.
            */
           sc = sl_bt_advertiser_stop(ble_Data->Advertising_Set_Handle);
           if(sc != SL_STATUS_OK)
           {
                 LOG_ERROR("sl_bt_advertiser_stop() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
           }

           sc = sl_bt_connection_set_parameters(ble_Data->Connection_Handle,
                                                      connection_interval,
                                                      connection_interval,
                                                      slave_latency,
                                                      supervision_timeout,
                                                      0,
                                                      0);
           if(sc != SL_STATUS_OK)
           {
               LOG_ERROR("sl_bt_connection_set_parameters() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
           }
           #else

           displayPrintf(DISPLAY_ROW_BTADDR2, "%02X:%02X:%02X:%02X:%02X:%02X",
                         server_address[0],
                         server_address[1],
                         server_address[2],
                         server_address[3],
                         server_address[4],
                         server_address[5]);

           #endif
           displayPrintf(DISPLAY_ROW_CONNECTION, "Connected");
           break;

         case sl_bt_evt_connection_closed_id:
           // handle connection closed event
           ble_Data->Connected           = false;
           ble_Data->Indication          = false;
           ble_Data->Indication_InFlight = false;

           #if DEVICE_IS_BLE_SERVER

           sc = sl_bt_advertiser_start(ble_Data->Advertising_Set_Handle,
                                            sl_bt_advertiser_general_discoverable,
                                            sl_bt_advertiser_connectable_scannable);
           if(sc != SL_STATUS_OK)
           {
                LOG_ERROR("sl_bt_advertiser_start() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
           }
           displayPrintf(DISPLAY_ROW_CONNECTION, "Advertising");
      #else
           sc = sl_bt_scanner_start(sl_bt_gap_1m_phy, sl_bt_scanner_discover_generic);
           if(sc != SL_STATUS_OK)
          {
                LOG_ERROR("sl_bt_scanner_start() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
          }
           displayPrintf(DISPLAY_ROW_CONNECTION, "Discovering");
      #endif

      displayPrintf(DISPLAY_ROW_TEMPVALUE, "");
      displayPrintf(DISPLAY_ROW_BTADDR2, "");
      break;


            case sl_bt_evt_connection_parameters_id:
              // handle connection parameters event
            #if LOG_PARAMETER_VALUE
              LOG_INFO("Connection params: Connection=%d, Interval=%d, Latency=%d, Timeout=%d, securityMode=%d\n\r",
                             (int) (evt->data.evt_connection_parameters.connection),
                             (int) (evt->data.evt_connection_parameters.interval*1.25),
                             (int) (evt->data.evt_connection_parameters.latency),
                             (int) (evt->data.evt_connection_parameters.timeout*10),
                             (int) (evt->data.evt_connection_parameters.security_mode) );
            #endif

            break;


            case sl_bt_evt_system_external_signal_id:

            break;

            case sl_bt_evt_system_soft_timer_id:

                  displayUpdate();

            break;

            #if DEVICE_IS_BLE_SERVER
            case sl_bt_evt_gatt_server_characteristic_status_id:

              // Check if the status flag indicates client configuration
              if(evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_temperature_measurement)
              {

                if (sl_bt_gatt_server_client_config == (sl_bt_gatt_server_characteristic_status_flag_t)
                                        evt->data.evt_gatt_server_characteristic_status.status_flags)
                {

                 // Check if client configuration flags indicate disabling
                 if(evt->data.evt_gatt_server_characteristic_status.client_config_flags == gatt_disable)
                 {
                         ble_Data->Indication = false;
                         displayPrintf(DISPLAY_ROW_TEMPVALUE, "");

                 }
                 // Check if client configuration flags indicate indication
                 else if(evt->data.evt_gatt_server_characteristic_status.client_config_flags == gatt_indication)
                 {
                           ble_Data->Indication = true;
                 }

               }
               // Check if the status flag indicates server confirmation
               if (sl_bt_gatt_server_confirmation == (sl_bt_gatt_server_characteristic_status_flag_t)
                                        evt->data.evt_gatt_server_characteristic_status.status_flags)
               {
                    ble_Data->Indication_InFlight = false;
               }

              }
             break;


             case sl_bt_evt_gatt_server_indication_timeout_id:

             LOG_INFO("server indication timeout\n\r");
             ble_Data->Indication = false;
             break;

#else
      //for clients only

             // Notifies about an advertising or scan response packet captured by the
             // device's radio when it is in scanning mode.

            case sl_bt_evt_scanner_scan_report_id:

              // Extracts and processes data from advertisement packets.
           if (evt->data.evt_scanner_scan_report.packet_type == 0)
           {
               //check  for the desired server device
               if((evt->data.evt_scanner_scan_report.address.addr[0] == server_address[0]) &&
              (evt->data.evt_scanner_scan_report.address.addr[1] == server_address[1]) &&
              (evt->data.evt_scanner_scan_report.address.addr[2] == server_address[2]) &&
              (evt->data.evt_scanner_scan_report.address.addr[3] == server_address[3]) &&
              (evt->data.evt_scanner_scan_report.address.addr[4] == server_address[4]) &&
              (evt->data.evt_scanner_scan_report.address.addr[5] == server_address[5]) &&
              (evt->data.evt_scanner_scan_report.address_type==0)) {

              //scanner - stop
              sc = sl_bt_scanner_stop();
              if(sc != SL_STATUS_OK)
              {
                  LOG_ERROR("sl_bt_scanner_stop() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
              }

              /*  @param[in] address Address of the device to connect to
          @param[in] address_type Enum @ref sl_bt_gap_address_type_t. Address type of
          the device to connect to.
          @param[in] initiating_phy Enum @ref sl_bt_gap_phy_t. The initiating PHY.
          @param[out] connection Handle that will be assigned to the connection after
          the connection is established. This handle is valid only if the result code
            of this response is 0 (zero).  */
              sc = sl_bt_connection_open(evt->data.evt_scanner_scan_report.address,
                                         evt->data.evt_scanner_scan_report.address_type, sl_bt_gap_1m_phy, NULL);
              if(sc != SL_STATUS_OK) {
                  LOG_ERROR("sl_bt_connection_open() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
              }
          }
      }

      break;

      // Indicates the finalization of the current GATT (Generic Attribute Profile) process.
      //This marks the conclusion as either successful or with an error identified during the procedure.
          case sl_bt_evt_gatt_procedure_completed_id:

            ble_Data->Gatt_Procedure = false;

            break;

          // Indicates the identification of a GATT service in the remote device's GATT database.
          case sl_bt_evt_gatt_service_id:

            //save service handle
            ble_Data->Service_Handle = evt->data.evt_gatt_service.service;

            break;

          // Denotes the discovery of a GATT characteristic in the remote device's GATT database.
          case sl_bt_evt_gatt_characteristic_id:

            //save characteristic handle
            ble_Data->Char_Handle = evt->data.evt_gatt_characteristic.characteristic;
            break;

          // Acknowledges the receipt of characteristic value(s) from the remote GATT server.
          case sl_bt_evt_gatt_characteristic_value_id:

            // Confirms the sending of an indication acknowledgment to the server.
            sc = sl_bt_gatt_send_characteristic_confirmation(ble_Data->Connection_Handle);

            if(sc != SL_STATUS_OK) {
                LOG_ERROR("sl_bt_gatt_set_characteristic_notification() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
            }

            //value got from server saved in variable
            ble_Data->Char_Value = &(evt->data.evt_gatt_characteristic_value.value.data[0]);

            temperature_in_c = FLOAT_TO_INT32((ble_Data->Char_Value));

            displayPrintf(DISPLAY_ROW_TEMPVALUE, "Temp=%d", temperature_in_c);

            break;

      #endif

    } // end - switch
} // handle_ble_event()

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

#else



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
