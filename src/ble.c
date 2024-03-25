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

int32_t temperature_in_celsius;

static int32_t FLOAT_TO_INT32(const uint8_t *buffer_ptr);  //FLT_TO_UINT32 macro you used to write the value to the buffer in your Server code

// Definition for the UUID of the Health Thermometer service as specified by the Bluetooth Special Interest Group (SIG)
static const uint8_t thermometer_service[2] = { 0x09, 0x18 };

// UUID for the Temperature Measurement characteristic as specified by the Bluetooth Special Interest Group (SIG)
static const uint8_t thermometer_char[2] = { 0x1c, 0x2a };

//  UUID defined by Bluetooth SIG - Button State Service
// 00000001-38c8-433e-87ec-652a2d136289
static const uint8_t button_service[16] = { 0x89, 0x62, 0x13, 0x2d, 0x2a, 0x65, 0xec, 0x87, 0x3e, 0x43, 0xc8, 0x38, 0x01, 0x00, 0x00, 0x00 };

//UUID defined by Bluetooth SIG - Button state characteristic
// 00000002-38c8-433e-87ec-652a2d136289
static const uint8_t button_char[16] = { 0x89, 0x62, 0x13, 0x2d, 0x2a, 0x65, 0xec, 0x87, 0x3e, 0x43, 0xc8, 0x38, 0x02, 0x00, 0x00, 0x00 };

#endif


uint8_t server_address[6] = SERVER_BT_ADDRESS; // array declared for storing server address

uint32_t advertising_interval=0x190;   //Given advertisement interval is 250msecs
uint16_t connection_interval = 0x3c;   //Given connection interval is 70msecs
uint16_t slave_latency = 0x03;         //slave can skip upto 3 connection events by following (3*70 = 210msecs < 250msecs ), Hence slave latency value is 3
uint16_t supervision_timeout = 0x50;   // Supervision timeout is always one more than slave latency which is 800msecs (1+slave_latency)
uint16_t scan_interval = 0x50;         //scanning interval of 50 ms
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

#if DEVICE_IS_BLE_SERVER
/**
 * @brief   Get the next pointer value, considering QUEUE_DEPTH.
 * @param   ptr Current pointer value.
 * @return  uint32_t Updated pointer value.
 * @reference  From the professor lecture
 */
uint32_t next_ptr(uint32_t ptr)
{
  // Check if incrementing ptr by 1 would reach QUEUE_DEPTH
  if(ptr+1==QUEUE_DEPTH)
  {
    // If true, wrap the pointer to the beginning (set to 0)
    ptr=0;
  }
  else
  {
    // If false, simply increment the pointer
  ptr=ptr+1;
  }
  return(ptr);
} // nextPtr()

/**
 * @brief   Write data to the queue.
 * @details This function writes data to the queue based on the given parameters.
 * @param   charHandle   Identifier for the character handle.
 * @param   bufLength    Length of the buffer.
 * @param   buffer       Pointer to the buffer containing data to be written.
 * @return  bool         True if the write operation failed, false otherwise.
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
        ble_Data->full=1;
    }

    // Return false to indicate a successful write operation
    return 0;

} // write_queue()


/**
 * @brief   Read data from the queue.
 * @details This function reads data from the queue and updates the provided parameters.
 * @param   charHandle   Pointer to store the character handle read from the queue.
 * @param   bufLength    Pointer to store the buffer length read from the queue.
 * @param   buffer       Pointer to store the buffer data read from the queue.
 * @return  bool         True if the read operation failed, false otherwise.
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


void SendTemp_ble()
{
  // Declaration of local variables
  uint8_t htm_temperature_buffer[5]; // Buffer to hold temperature data
  uint8_t *p = htm_temperature_buffer; // Pointer to the buffer
  uint32_t htm_temperature_flt; // Variable to hold the temperature in floating point format
  uint8_t flags = 0x00; // Flags variable initialized to 0

  // Get a pointer to the BLE data structure
  ble_data_struct_t *ble_Data = get_ble_DataPtr();

  queue_struct_t Indication_Data;

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
    if (ble_Data->Indication == true )
    {
      if (ble_Data->Indication_InFlight)
      {
          //save the characteristic values in a buffer_entry
          Indication_Data.charHandle = gattdb_temperature_measurement;
          Indication_Data.bufLength = 5;
          for(int i=0; i<5; i++)
              Indication_Data.buffer[i] = htm_temperature_buffer[i];

          //store indication data in circular buffer
          qc = write_queue(Indication_Data);

          //increase number of queued indications
          if(qc == 0)
            ble_Data->Queued_Indication++;
          else
            LOG_ERROR("Indication enqueue failed\n\r");
      }

      else
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
        LOG_INFO("Sent indication to get Temperature=%d\n\r", temperature_in_celcius);
        displayPrintf(DISPLAY_ROW_TEMPVALUE, "Temp=%d", temperature_in_celcius);

      }

   }
  }
 }
}

void SendButtonState_ble(uint8_t value)
{
  // Get pointer to BLE data structure
  ble_data_struct_t *ble_Data=get_ble_DataPtr();
  // Structure to hold indication data
  queue_struct_t Indication_Data;
  // Buffer to hold button status value
  uint8_t button_value_buffer[2];


  // Set button status value in the buffer
  button_value_buffer[0]=value; // Assign the function's input value as the button state
  button_value_buffer[1]=0; // Set the second byte of the buffer to 0, completing the buffer

  // Check if device is connected
  if(ble_Data->Connected==true) // If BLE device is connected
  {
      // Write button status to GATT server
     sl_status_t sc=sl_bt_gatt_server_write_attribute_value(gattdb_button_state,
                                                            0, // Attribute offset
                                                            1, // Length of data to be written
                                                            &button_value_buffer[0]); // Data buffer pointer
     // Check if write operation was successful
     if(sc != SL_STATUS_OK) // If there was an error writing the attribute value
     {
        LOG_ERROR("sl_bt_gatt_server_write_attribute_value() returned != 0 status=0x%04x\n\r", (unsigned int)sc); // Log an error message
     }

     // Check if button indication is enabled and the device is bonded
     if(ble_Data->Button_Indication==true && ble_Data->Bonded==true) // If indications are enabled and device is bonded
     {
         // Check if an indication is already in flight
        if(ble_Data->Indication_InFlight) // If an indication is already being processed
          {
            // Prepare indication data
            Indication_Data.charHandle=gattdb_button_state; // Set characteristic handle for the indication
            Indication_Data.bufLength=2; // Set buffer length for the indication

            // Copy button status value to indication buffer
            for(int i=0;i<2;i++) // Loop to copy data into indication buffer
              {
                Indication_Data.buffer[i]=button_value_buffer[i]; // Copy each byte
              }

            // Write indication data to queue
            qc=write_queue(Indication_Data); // Attempt to queue the indication data

            // Increment queued indications count if write was successful
            if(qc == 0) // If the queue write operation was successful
            {
               ble_Data->Queued_Indication++; // Increment the count of queued indications
            }
            else
            {
              LOG_ERROR("Indication enqueue failed\n\r"); // Log an error if queueing fails
            }
          }
        else // If no indication is currently in flight
        {
            // Send indication
            sc=sl_bt_gatt_server_send_indication(ble_Data->Connection_Handle, // Connection handle
                                                 gattdb_button_state, // Characteristic handle
                                                 2, // Length of the data
                                                 &button_value_buffer[0]); // Data buffer pointer
            // Check if indication send was successful
            if(sc!=SL_STATUS_OK) // If there was an error sending the indication
            {
                LOG_ERROR("sl_bt_gatt_server_send_indication() returned != 0 status=0x%04x\n\r", (unsigned int)sc); // Log an error message
            }
            else
            {
                // Set indication in flight flag
                ble_Data->Indication_InFlight=true; // Indicate that an indication is currently being processed
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



void handle_ble_event(sl_bt_msg_t *evt)
{

  // Obtain a pointer to the BLE data structure
  ble_data_struct_t *ble_Data = get_ble_DataPtr();

  // Use a switch statement to manage various BLE events
  switch(SL_BT_MSG_ID(evt->header))
  {
    // Applicable for both server and client modes

    // Handle the system boot event
     case sl_bt_evt_system_boot_id:

       // Set up the display interface
       displayInit();

       // Retrieve the device's identity address
       sc = sl_bt_system_get_identity_address(&(ble_Data->My_Address),
                                             &(ble_Data->My_AddressType));
       // Verify if the retrieval was successful
      if(sc != SL_STATUS_OK)
       {
          LOG_ERROR("Failed to get identity address, status=0x%04x\n\r", (unsigned int)sc);
      }

#if DEVICE_IS_BLE_SERVER
      // Initialize BLE server

      // Attempt to create an advertising set
      sc = sl_bt_advertiser_create_set(&(ble_Data->Advertising_Set_Handle));
      // Verify creation success
      if (sc != SL_STATUS_OK)
      {
        LOG_ERROR("Failed to create advertiser set, status=0x%04x", (unsigned int)sc);
      }

      // Configure advertising timing with specified intervals
      sc = sl_bt_advertiser_set_timing(ble_Data->Advertising_Set_Handle,
                                       advertising_interval,
                                       advertising_interval,
                                       0,  // Duration (0 = infinite)
                                       0); // Max events (0 = no limit)
      // Verify timing configuration success
      if (sc != SL_STATUS_OK)
      {
        LOG_ERROR("Failed to set advertising timing, status=0x%04x", (unsigned int)sc);
      }


      // Start advertising as general discoverable and connectable
      sc = sl_bt_advertiser_start(ble_Data->Advertising_Set_Handle,
                                  sl_bt_advertiser_general_discoverable,
                                  sl_bt_advertiser_connectable_scannable);
      // Verify advertising start success
      if (sc != SL_STATUS_OK)
      {
        LOG_ERROR("Failed to start advertising, status=0x%04x", (unsigned int)sc);
      }

      // Update display with server status
      displayPrintf(DISPLAY_ROW_NAME, "Server");
      displayPrintf(DISPLAY_ROW_CONNECTION, "Advertising");

#else
      // Set the scanning mode to passive for the BLE scanner
      sc = sl_bt_scanner_set_mode(sl_bt_gap_1m_phy, SCAN_PASSIVE);
      if (sc != SL_STATUS_OK)
      {
          LOG_ERROR("sl_bt_scanner_set_mode() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
      }

      // Configure the timing of the BLE scanner
      sc = sl_bt_scanner_set_timing(sl_bt_gap_1m_phy, scan_interval, scan_window);
      if (sc != SL_STATUS_OK)
      {
          LOG_ERROR("sl_bt_scanner_set_timing() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
      }

      // Set the default parameters for BLE connections
      sc = sl_bt_connection_set_default_parameters(connection_interval,
                                                   connection_interval,
                                                   slave_latency,
                                                   supervision_timeout,
                                                   0,
                                                   4);
      if (sc != SL_STATUS_OK)
      {
          LOG_ERROR("sl_bt_scanner_set_default_parameters() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
      }

      // Begin scanning for BLE devices
      sc = sl_bt_scanner_start(sl_bt_gap_1m_phy, sl_bt_scanner_discover_generic);
      if (sc != SL_STATUS_OK)
      {
          LOG_ERROR("sl_bt_scanner_start() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
      }

      // Update the display with client information and current action
      displayPrintf(DISPLAY_ROW_NAME, "Client");
      displayPrintf(DISPLAY_ROW_CONNECTION, "Discovering");
#endif

      // Configure security manager with specified IO capabilities
       sc = sl_bt_sm_configure(FLAG, sl_bt_sm_io_capability_displayyesno);
       if (sc != SL_STATUS_OK)
       {
           LOG_ERROR("Security manager configuration failed, status=0x%04x", (unsigned int)sc);
       }

       // Attempt to delete all existing bondings
       sc = sl_bt_sm_delete_bondings();
       if (sc != SL_STATUS_OK) {
           LOG_ERROR("Failed to delete bondings, status=0x%04x", (unsigned int)sc);
       }
        // Display server address and the assignment number
        displayPrintf(DISPLAY_ROW_BTADDR, "%02X:%02X:%02X:%02X:%02X:%02X",
                            ble_Data->My_Address.addr[0],
                            ble_Data->My_Address.addr[1],
                            ble_Data->My_Address.addr[2],
                            ble_Data->My_Address.addr[3],
                            ble_Data->My_Address.addr[4],
                            ble_Data->My_Address.addr[5]);
        displayPrintf(DISPLAY_ROW_ASSIGNMENT, "A9");

        // Initialize all the required flags
      ble_Data->Indication          = false;
      ble_Data->Connected           = false;
      ble_Data->Bonded              = false;
      ble_Data->Indication_InFlight = false;
      ble_Data->Button_Indication   = false;
      ble_Data->Button_Pressed      = false;
      ble_Data->Gatt_Procedure      = false;
      ble_Data->PB1_Button_Pressed = false;
      ble_Data->w_ptr                = 0;
      ble_Data->r_ptr                = 0;
      ble_Data->Queued_Indication  = 0;
      ble_Data->Press_Seq = 0;

      break;

      // This section handles the event when a BLE connection is established
      case sl_bt_evt_connection_opened_id:

        // Mark the device as connected
        ble_Data->Connected = true;

        // Save the handle for the established connection
        ble_Data->Connection_Handle = evt->data.evt_connection_opened.connection;

      // This preprocessor directive checks if the device is configured as a BLE server
      #if DEVICE_IS_BLE_SERVER

        // If the device is a BLE server, stop advertising to save power and prevent further connections
        sc = sl_bt_advertiser_stop(ble_Data->Advertising_Set_Handle);

        if(sc != SL_STATUS_OK)
        {
                  LOG_ERROR("sl_bt_advertiser_stop() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
        }

        // After a connection is established, set the connection parameters such as intervals and timeouts
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
      // Display the server's bluetooth's address
      displayPrintf(DISPLAY_ROW_BTADDR2,"%02X:%02X:%02X:%02X:%02X:%02X",
                    server_address[0],
                    server_address[1],
                    server_address[2],
                    server_address[3],
                    server_address[4],
                    server_address[5]);
#endif

      // Display connection status as conneted
      displayPrintf(DISPLAY_ROW_CONNECTION, "Connected");
      break;

    // Handle connection closed event by using connection close id
    case sl_bt_evt_connection_closed_id:

      gpioLed0SetOff();
      gpioLed1SetOff();

      displayPrintf(DISPLAY_ROW_9, "");
      displayPrintf(DISPLAY_ROW_TEMPVALUE, "");
      displayPrintf(DISPLAY_ROW_PASSKEY, "");
      displayPrintf(DISPLAY_ROW_ACTION, "");
      // Reset all the necessary flags
      ble_Data->Indication          = false;
      ble_Data->Connected           = false;
      ble_Data->Bonded              = false;
      ble_Data->Indication_InFlight = false;
      ble_Data->Button_Indication   = false;
      ble_Data->Button_Pressed      = false;
      ble_Data->Gatt_Procedure      = false;
      ble_Data->PB1_Button_Pressed = false;
      ble_Data->w_ptr                = 0;
      ble_Data->r_ptr                = 0;
      ble_Data->Queued_Indication  = 0;
      ble_Data->Press_Seq = 0;

      // Initiates the process of deleting all existing bondings from the BLE server's bonding table
      sc = sl_bt_sm_delete_bondings();
      if (sc != SL_STATUS_OK) {
         // If there's an error in deleting bondings, log the error status
         LOG_ERROR("Failed to delete bondings with status=0x%04x\n\r", (unsigned int)sc);
      }

#if DEVICE_IS_BLE_SERVER

      // Starts advertising to make the BLE server discoverable and connectable by BLE clients
      sc = sl_bt_advertiser_start(ble_Data->Advertising_Set_Handle,
                                  sl_bt_advertiser_general_discoverable,
                                  sl_bt_advertiser_connectable_scannable);
      // Checks if there was an error in starting the advertiser
      if (sc != SL_STATUS_OK)
      {
          LOG_ERROR("sl_bt_advertiser_start() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
          // LOG_ERROR("Failed to start advertising with status=0x%04x\n\r", (unsigned int)sc);
      }

      // Updates the display to indicate that the device is now advertising
      displayPrintf(DISPLAY_ROW_CONNECTION, "Advertising");


#else
      // Initiate scanning for BLE devices using the 1M PHY, and set the discovery mode to generic
      sc = sl_bt_scanner_start(sl_bt_gap_1m_phy, sl_bt_scanner_discover_generic);

      // Check if the scanner start function returns an error
      if (sc != SL_STATUS_OK)
      {
        LOG_ERROR("sl_bt_scanner_start() returned !=0 status=0x%04x\n\r", (unsigned int)sc);
      }

      // Update the display to indicate that the device is currently discovering other BLE devices
      displayPrintf(DISPLAY_ROW_CONNECTION, "Discovering");

#endif

      //Clear the temperature value and server Bluetooth address from the display.
      displayPrintf(DISPLAY_ROW_TEMPVALUE, "");
      displayPrintf(DISPLAY_ROW_BTADDR2,"");

      break;

      // Handle confirm bonding event
      case sl_bt_evt_sm_confirm_bonding_id:

          // Confirm bonding
          sc = sl_bt_sm_bonding_confirm(ble_Data->Connection_Handle, 1);

          // Log error if bonding confirmation fails
          if (sc != SL_STATUS_OK)
          {
              LOG_ERROR("sl_bt_sm_bonding_confirm() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
          }

      break;

      // Handle confirm passkey event
      case sl_bt_evt_sm_confirm_passkey_id:

         // Save passkey from event data
         ble_Data->passkey = evt->data.evt_sm_confirm_passkey.passkey;

         // Display passkey on the appropriate display row
         displayPrintf(DISPLAY_ROW_PASSKEY, "%d", ble_Data->passkey);

         // Display instructions for passkey confirmation
         displayPrintf(DISPLAY_ROW_ACTION, "Confirm with PB0");

         break;


         // Handle bonded event
         case sl_bt_evt_sm_bonded_id:

            // Display bonded status
            displayPrintf(DISPLAY_ROW_CONNECTION, "Bonded");

            // Set bonded flag to true
            ble_Data->Bonded = true;

            // Clear passkey and action display rows
            displayPrintf(DISPLAY_ROW_PASSKEY, "");
            displayPrintf(DISPLAY_ROW_ACTION, "");

            break;


            // Handle bonding failed event
            case sl_bt_evt_sm_bonding_failed_id:

               // Log bonding failure reason
               LOG_ERROR("Bonding failed. Reason: 0x%04x\n\r", evt->data.evt_sm_bonding_failed.reason);

               // Close the connection
               sc = sl_bt_connection_close(ble_Data->Connection_Handle);

                // Log error if connection close fails
                if (sc != SL_STATUS_OK) {
                    LOG_ERROR("Failed to close connection. Status: 0x%04x\n\r", (unsigned int)sc);
                }

                break;
     // Handle connection parameters event by using this function
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
   // Handle external signal event
   case sl_bt_evt_system_external_signal_id:
#if !DEVICE_IS_BLE_SERVER
     //detect button press sequence to change indication status for button state service
     //check if PB0 is pressed once
     if(evt->data.evt_system_external_signal.extsignals == evt_Button_Pressed && ble_Data->Button_Pressed)
         ble_Data->Press_Seq = 1;

          //check if PB1 is pressed while PB0 is pressed
          if((ble_Data->Press_Seq == 1) && ble_Data->PB1_Button_Pressed && evt->data.evt_system_external_signal.extsignals == evt_Button_Pressed)
          {
              ble_Data->Press_Seq = 2;
          }

          //check if PB1 is released in sequence
          if((ble_Data->Press_Seq == 2) && !ble_Data->PB1_Button_Pressed && evt->data.evt_system_external_signal.extsignals == evt_Button_Released)
          {
              ble_Data->Press_Seq = 3;
              break;
          }

          //Check if PB0 is released. If so, toggle the indication status to reflect the change, then break from the loop to prevent further actions until the next condition check.
          if((ble_Data->Press_Seq== 3) && !ble_Data->Button_Pressed && evt->data.evt_system_external_signal.extsignals == evt_Button_Released)
          {
              if(ble_Data->Button_Indication)
                {
                  sc = sl_bt_gatt_set_characteristic_notification(ble_Data->Connection_Handle,
                                                                  ble_Data->Button_Char_Handle,
                                                                  sl_bt_gatt_disable);
                  if(sc != SL_STATUS_OK) {
                      LOG_ERROR("sl_bt_gatt_set_characteristic_notification() 1 returned != 0 status=0x%04x\n\r", (unsigned int)sc);
                  }
                  else
                  {
                      // LOG_INFO("Disables indications from client\n\r");
                  }
              }
              else
              {
                  sc = sl_bt_gatt_set_characteristic_notification(ble_Data->Connection_Handle,
                                                                  ble_Data->Button_Char_Handle,
                                                                  sl_bt_gatt_indication);
                  if(sc != SL_STATUS_OK) {
                      LOG_ERROR("sl_bt_gatt_set_characteristic_notification() 2 returned != 0 status=0x%04x\n\r", (unsigned int)sc);
                  }
                  else {
                      // LOG_INFO("Enables indications from client\n\r");
                  }
              }
              break;

          }

#endif
       // Check if the external signal is for button pressed event
       if (evt->data.evt_system_external_signal.extsignals == evt_Button_Pressed)
       {
#if DEVICE_IS_BLE_SERVER
           // Display message indicating button is pressed
           displayPrintf(DISPLAY_ROW_9, "Button Pressed");

           // send button status if device bonded
           if (ble_Data->Bonded)
           {
               SendButtonState_ble(0x01);
           }
#endif

#if !DEVICE_IS_BLE_SERVER
       if(ble_Data->PB1_Button_Pressed)
       {
              sc = sl_bt_gatt_read_characteristic_value(ble_Data->Connection_Handle,
                                                       ble_Data->Button_Char_Handle);
              if(sc != SL_STATUS_OK) {
                    LOG_ERROR("sl_bt_gatt_read_characteristic_value() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
              }
       }

#endif

      // If device is not bonded
      if (ble_Data->Button_Pressed && ble_Data->Bonded == false)
      {
          // Confirm passkey for pairing
          sc = sl_bt_sm_passkey_confirm(ble_Data->Connection_Handle, 1);
          // Check if passkey confirmation was successful
          if (sc != SL_STATUS_OK)
          {
               // Log error if passkey confirmation fails
               LOG_ERROR("sl_bt_sm_passkey_confirm() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
          }
       }
    }
#if DEVICE_IS_BLE_SERVER
       // Check if the external signal is for button released event
       if (evt->data.evt_system_external_signal.extsignals == evt_Button_Released)
       {
           // Display message indicating button is released
           displayPrintf(DISPLAY_ROW_9, "Button Released");

           // If device is bonded, send button status
           if (ble_Data->Bonded)
           {
               SendButtonState_ble(0x00);
           }
       }
#endif

      break;
      // Handle system soft timer event
      case sl_bt_evt_system_soft_timer_id:

     // Update display
     displayUpdate();
#if DEVICE_IS_BLE_SERVER

     // Check if there are queued indications and no indication is currently in flight
     if (ble_Data->Queued_Indication != 0 && !ble_Data->Indication_InFlight)
     {

         // Read from the queue
         qc = read_queue();

         // Log error if indication dequeue fails
         if (qc != 0)
            {
                  LOG_ERROR("Indication dequeue failed\n\r");
            }

     }
#endif
     break;

      // Handle GATT server events for BLE server
#if DEVICE_IS_BLE_SERVER

       // Handle GATT server characteristic status event
       case sl_bt_evt_gatt_server_characteristic_status_id:
           // Check if it's a temperature measurement characteristic
           if (evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_temperature_measurement)
           {
               // Check client configuration flags
               if (sl_bt_gatt_server_client_config == (sl_bt_gatt_server_characteristic_status_flag_t)evt->data.evt_gatt_server_characteristic_status.status_flags)
               {
                   // Disable indication if necessary
                   if (evt->data.evt_gatt_server_characteristic_status.client_config_flags == gatt_disable)
                   {
                       ble_Data->Indication = false; // Disable temperature indication
                       gpioLed0SetOff(); // Turn off LED indicating temperature indication
                       displayPrintf(DISPLAY_ROW_TEMPVALUE, ""); // Clear temperature display
                   }
                   else if (evt->data.evt_gatt_server_characteristic_status.client_config_flags == gatt_indication)
                   {
                       // Enable indication
                       ble_Data->Indication = true; // Enable temperature indication
                       gpioLed0SetOn(); // Turn on LED indicating temperature indication
                   }
               }
           }
           // Check if it's a button state characteristic
           if (evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_button_state)
           {
               if (sl_bt_gatt_server_client_config == (sl_bt_gatt_server_characteristic_status_flag_t)evt->data.evt_gatt_server_characteristic_status.status_flags)
               {
                   // Disable indication if necessary
                   if (evt->data.evt_gatt_server_characteristic_status.client_config_flags == gatt_disable)
                   {
                       ble_Data->Indication = false; // Disable button indication
                       gpioLed1SetOff(); // Turn off LED indicating button indication
                   }
                   else if (evt->data.evt_gatt_server_characteristic_status.client_config_flags == gatt_indication)
                   {
                       // Enable indication
                       ble_Data->Indication = true; // Enable button indication
                       gpioLed1SetOn(); // Turn on LED indicating button indication
                   }
               }
           }
           // Check confirmation status
           if (sl_bt_gatt_server_confirmation == (sl_bt_gatt_server_characteristic_status_flag_t)evt->data.evt_gatt_server_characteristic_status.status_flags)
           {
               ble_Data->Indication_InFlight = false; // Confirmation received, reset indication in-flight flag
           }
           break;


           // Handle GATT server indication timeout event
           case sl_bt_evt_gatt_server_indication_timeout_id:
               LOG_ERROR("server indication timeout\n\r");
               // Reset indication flag
               ble_Data->Indication = false; // Reset indication flag when indication timeout occurs
               ble_Data->Button_Indication = false; // Reset button indication flag as well
               break;


#else
           // Handle scanner scan report event
              case sl_bt_evt_scanner_scan_report_id:
                  // Check if the scanned device is the server
                  if (evt->data.evt_scanner_scan_report.packet_type == 0) {
                      // Compare scanned device address with server address
                      if ((evt->data.evt_scanner_scan_report.address.addr[0] == server_address[0]) &&
                          (evt->data.evt_scanner_scan_report.address.addr[1] == server_address[1]) &&
                          (evt->data.evt_scanner_scan_report.address.addr[2] == server_address[2]) &&
                          (evt->data.evt_scanner_scan_report.address.addr[3] == server_address[3]) &&
                          (evt->data.evt_scanner_scan_report.address.addr[4] == server_address[4]) &&
                          (evt->data.evt_scanner_scan_report.address.addr[5] == server_address[5]) &&
                          (evt->data.evt_scanner_scan_report.address_type == 0)) {

                          sc = sl_bt_scanner_stop();
                          if (sc != SL_STATUS_OK) {

                              LOG_ERROR("sl_bt_scanner_stop() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
                          }

                          sl_bt_connection_open(evt->data.evt_scanner_scan_report.address,
                                                evt->data.evt_scanner_scan_report.address_type, sl_bt_gap_1m_phy, NULL);
                          if (sc != SL_STATUS_OK) {

                              LOG_ERROR("sl_bt_connection_open() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
                          }
                      }
                  }
                  break;



              case sl_bt_evt_gatt_procedure_completed_id:

                if(evt->data.evt_gatt_procedure_completed.result == 0x110F)
                {

                       sc = sl_bt_sm_increase_security(ble_Data->Connection_Handle);
                       if(sc != SL_STATUS_OK) {
                           LOG_ERROR("sl_bt_sm_increase_security() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
                       }
                }


                if(ble_Data->Press_Seq == 3)
                {
                    ble_Data->Press_Seq = 0;
                    ble_Data->Button_Indication = !ble_Data->Button_Indication;
                }

                 ble_Data->Gatt_Procedure = false;
                 break;

              case sl_bt_evt_gatt_service_id:

                if(memcmp(evt->data.evt_gatt_service.uuid.data, thermometer_service, sizeof(thermometer_service)) == 0)
                       ble_Data->Service_Handle = evt->data.evt_gatt_service.service;
                else if(memcmp(evt->data.evt_gatt_service.uuid.data, button_service, sizeof(button_service)) == 0)
                       ble_Data->Button_Service_Handle = evt->data.evt_gatt_service.service;
                  break;

              case sl_bt_evt_gatt_characteristic_id:

                if(memcmp(evt->data.evt_gatt_characteristic.uuid.data, thermometer_char, sizeof(thermometer_char)) == 0)
                       ble_Data->Char_Handle = evt->data.evt_gatt_characteristic.characteristic;
                else if(memcmp(evt->data.evt_gatt_characteristic.uuid.data, button_char, sizeof(button_char)) == 0)
                       ble_Data->Button_Char_Handle = evt->data.evt_gatt_characteristic.characteristic;

                break;

              case sl_bt_evt_gatt_characteristic_value_id:

                  if(evt->data.evt_gatt_characteristic_value.att_opcode == sl_bt_gatt_handle_value_indication)
                  {
                  sc = sl_bt_gatt_send_characteristic_confirmation(ble_Data->Connection_Handle);
                  if (sc != SL_STATUS_OK) {
                      LOG_ERROR("sl_bt_gatt_set_characteristic_notification() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
                  }
                  }

                  if(evt->data.evt_gatt_characteristic_value.characteristic == ble_Data->Char_Handle)
                  {

                  ble_Data->Char_Value = &(evt->data.evt_gatt_characteristic_value.value.data[0]);

                  temperature_in_celsius = FLOAT_TO_INT32((ble_Data->Char_Value));
                  displayPrintf(DISPLAY_ROW_TEMPVALUE, "Temp=%d", temperature_in_celsius);
                  }

                  if(ble_Data->Bonded && ble_Data->Button_Indication && (evt->data.evt_gatt_characteristic_value.characteristic == ble_Data->Button_Char_Handle))
                  {
                       if(evt->data.evt_gatt_characteristic_value.value.data[0] == 0x01)
                       {
                           displayPrintf(DISPLAY_ROW_9, "Button Pressed");
                       }
                       else if(evt->data.evt_gatt_characteristic_value.value.data[0] == 0x00)
                       {
                            displayPrintf(DISPLAY_ROW_9, "Button Released");
                       }
                  }

                  if(evt->data.evt_gatt_characteristic_value.att_opcode == sl_bt_gatt_read_response)
                  {
                       if(evt->data.evt_gatt_characteristic_value.value.data[0] == 0x01)
                       {
                           displayPrintf(DISPLAY_ROW_9, "Button Pressed");
                       }
                       else if(evt->data.evt_gatt_characteristic_value.value.data[0] == 0x00)
                       {
                              displayPrintf(DISPLAY_ROW_9, "Button Released");
                       }
                 }


                 break;
     #endif

  }
}

