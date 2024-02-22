/*
 * ble.c
 *
 *  Created on: Feb 18, 2024
 *      Author: Tharuni Gelli
 */
#include "src/ble.h"
#include "app.h"

#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"

#include "src/ble.h"
#include "sl_bt_api.h"

#define LOG_PARAMETER_VALUE 1

uint32_t advertising_interval=0x190;   //Given advertisement interval is 250msecs
uint16_t connection_interval = 0x3c;   //Given connection interval is 70msecs
uint16_t slave_latency = 0x03;         //slave can skip upto 3 connection events by following (3*70 = 210msecs < 250msecs ), Hence slave latency value is 3
uint16_t supervision_timeout = 0x50;   // Supervision timeout is always one more than slave latency which is 800msecs (1+slave_latency)

sl_status_t sc=0;

ble_data_struct_t ble_Data;

ble_data_struct_t * get_ble_DataPtr()
{
  return (&ble_Data);
}

void handle_ble_event(sl_bt_msg_t *evt)
{
   ble_data_struct_t *ble_Data = get_ble_DataPtr();

   switch (SL_BT_MSG_ID(evt->header))
   {
         case sl_bt_evt_system_boot_id:
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
              if(sc != SL_STATUS_OK)
              {
                  LOG_ERROR("sl_bt_advertiser_start() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
              }

              //initialize connection and indication flags
              ble_Data->Connected           = false;
              ble_Data->Indication          = false;
              ble_Data->Indication_InFlight = false;
            break;
         case sl_bt_evt_connection_opened_id:
            // handle open event
           //set connection flag as true
           ble_Data->Connected = true;

           //get value of connection handle
           ble_Data->Connection_Handle = evt->data.evt_connection_opened.connection;

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
           break;
         case sl_bt_evt_connection_closed_id:
            // handle close event
           //turn off all connection and indication flags
           ble_Data->Connected           = false;
           ble_Data->Indication          = false;
           ble_Data->Indication_InFlight = false;

           //Start advertising of a given advertising set with specified discoverable and connectable modes.
           sc = sl_bt_advertiser_start(ble_Data->Advertising_Set_Handle,
                                            sl_bt_advertiser_general_discoverable,
                                            sl_bt_advertiser_connectable_scannable);
           if(sc != SL_STATUS_OK)
           {
                LOG_ERROR("sl_bt_advertiser_start() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
           }
           break;

            //Triggered whenever the connection parameters are changed and at any time a connection is established
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

            //Indicates that the external signals have been received
            case sl_bt_evt_system_external_signal_id:

            break;


            case sl_bt_evt_gatt_server_characteristic_status_id:

            //check if temperature measurement characteristic is changed

            if(evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_temperature_measurement)
              {

                   //check if any status flag has been changed by client
                   if (sl_bt_gatt_server_client_config == (sl_bt_gatt_server_characteristic_status_flag_t)
                          evt->data.evt_gatt_server_characteristic_status.status_flags)
                   {

                   //check if indication flag is disabled
                   if(evt->data.evt_gatt_server_characteristic_status.client_config_flags == gatt_disable)
                   {
                         ble_Data->Indication = false;

                   }

                   //check if indication flag is enabled
                   else if(evt->data.evt_gatt_server_characteristic_status.client_config_flags == gatt_indication)
                   {
                         ble_Data->Indication = true;
                   }

                   }

                  //check if indication confirmation has been received from client
                  if (sl_bt_gatt_server_confirmation == (sl_bt_gatt_server_characteristic_status_flag_t)
                          evt->data.evt_gatt_server_characteristic_status.status_flags)
                  {
                         ble_Data->Indication_InFlight = false;
                  }
                      //track indication bool
              }
             break;

             //event indicates confirmation from the remote GATT client has not been received within 30 seconds after an indication was sent
             case sl_bt_evt_gatt_server_indication_timeout_id:

             LOG_INFO("server indication timeout\n\r");
             ble_Data->Indication = false;
             break;
    } // end - switch
} // handle_ble_event()



void SendTemp_ble()
{

  uint8_t htm_temperature_buffer[5];
  uint8_t *p = htm_temperature_buffer;
  uint32_t htm_temperature_flt;
  uint8_t flags = 0x00;

  ble_data_struct_t *ble_Data = get_ble_DataPtr();

  //check if bluetooth is connected
  if(ble_Data->Connected == true)
  {

      //get temperature value from sensor in celcius
      int32_t temperature_in_celcius = ConvertValueToCelcius();

      UINT8_TO_BITSTREAM(p, flags);

      htm_temperature_flt = INT32_TO_FLOAT(temperature_in_celcius*1000, -3);

      UINT32_TO_BITSTREAM(p, htm_temperature_flt);

      //write temperature value in gatt database
      sl_status_t sc = sl_bt_gatt_server_write_attribute_value(gattdb_temperature_measurement,
                                                               0,
                                                               5,
                                                               &htm_temperature_buffer[0]);

      if(sc != SL_STATUS_OK)
      {
          LOG_ERROR("sl_bt_gatt_server_write_attribute_value() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
      }

      //check if indication is on
      if (ble_Data->Indication == true && ble_Data->Indication_InFlight == false)
      {

      sc = sl_bt_gatt_server_send_indication(ble_Data->Connection_Handle,
                                                 gattdb_temperature_measurement,
                                                 5,
                                                 &htm_temperature_buffer[0]);
      if(sc != SL_STATUS_OK)
      {
          LOG_ERROR("sl_bt_gatt_server_send_indication() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
      }
      else
      {
          ble_Data->Indication_InFlight = true;
          LOG_INFO("Sent HTM indication, temperature=%d\n\r", temperature_in_celcius);
      }
      }
  }

}
