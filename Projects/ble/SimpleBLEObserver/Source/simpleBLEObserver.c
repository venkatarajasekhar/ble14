/**************************************************************************************************
  Filename:       simpleBLEObserver.c
  Revised:        $Date: 2011-06-20 11:57:59 -0700 (Mon, 20 Jun 2011) $
  Revision:       $Revision: 28 $

  Description:    This file contains the Simple BLE Observer sample application 
                  for use with the CC2540 Bluetooth Low Energy Protocol Stack.

  Copyright 2011 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
**************************************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include <string.h>

#include "bcomdef.h"
#include "OSAL.h"
#include "OSAL_PwrMgr.h"
#include "OnBoard.h"
#include "hal_led.h"
#include "hal_key.h"
#include "hal_lcd.h"
#include "ll.h"
#include "hci.h"
#include "hal_uart.h"
#include "observer.h"

#include "simpleBLEObserver.h"

#include "pt.h"
#include "wifly_util.h"

#define DELAY_MS(milli)         timestamp = osal_GetSystemClock(); \
                                PT_WAIT_UNTIL(pt, osal_GetSystemClock() - timestamp > milli);

/*********************************************************************
 * MACROS
 */

// Length of bd addr as a string
#define B_ADDR_STR_LEN                        15

/*********************************************************************
 * CONSTANTS
 */

// Maximum number of scan responses
#define DEFAULT_MAX_SCAN_RES                  8

// Scan duration in ms
#define DEFAULT_SCAN_DURATION                 4000

// Discovey mode (limited, general, all)
#define DEFAULT_DISCOVERY_MODE                DEVDISC_MODE_ALL

// TRUE to use active scan
#define DEFAULT_DISCOVERY_ACTIVE_SCAN         FALSE

// TRUE to use white list during discovery
#define DEFAULT_DISCOVERY_WHITE_LIST          FALSE

#define AFUNC_UNAVAIL                           ((uint8)-2)
#define AFUNC_BLOCKING                          ((uint8)-1)
#define ASTATE_FINAL                            (-1)


#define PEEK            0
#define KICK            1

#define OP_AVAIL        0                        
#define OP_INIT         1
#define OP_KICK         2

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
char  WiflyInBuffer[512];           // input buffer used to receive data from WiFly
int   WiflyInBufferIndex;           // index to wifly input buffer array


char ssid[32]   = "UnwiredGrain";
char passwd[32] = "3211238976";
char chan[32]   = "0";
   
/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

// Task ID for internal task/event processing
static uint8 simpleBLETaskId;

// GAP GATT Attributes
//static const uint8 simpleBLEDeviceName[GAP_DEVICE_NAME_LEN] = "Simple BLE Observer";

// Number of scan results and scan result index
static uint8 simpleBLEScanRes;
static uint8 simpleBLEScanIdx;

// Scan result list
static gapDevRec_t simpleBLEDevList[DEFAULT_MAX_SCAN_RES];

// Scanning state
static uint8 simpleBLEScanning = FALSE;

// global error code for peek-kick
static uint8 error;

/*********************************************************************/

static halUARTCfg_t uartConfig;

unsigned char wiflyInBuffer[512];
uint16 wiflyInBufferIndex;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void simpleBLEObserverEventCB( gapObserverRoleEvent_t *pEvent );
static void simpleBLEObserver_HandleKeys( uint8 shift, uint8 keys );
static void simpleBLEObserver_ProcessOSALMsg( osal_event_hdr_t *pMsg );
static void simpleBLEAddDeviceInfo( uint8 *pAddr, uint8 addrType );
char *bdAddr2Str ( uint8 *pAddr );

static void hang(){ /** do nothing **/ };

/*********************************************************************
 * PROFILE CALLBACKS
 */

// GAP Role Callbacks
static const gapObserverRoleCB_t simpleBLERoleCB =
{
  NULL,                     // RSSI callback
  simpleBLEObserverEventCB  // Event callback
};

/*********************************************************************
*/

static void flushUARTRxBuffer(uint8 port) {
  
  uint8 c;
  
  while (Hal_UART_RxBufLen(port)) {
    HalUARTRead(port, &c, 1);
  }
}

/*********************************************************************
* state machine for asynchronous function: wifly_hardware_reset
*/
#define CTS_HIGH        1
#define CTS_NOT_HIGH    0                               // quick and dirty placeholder

static 
PT_THREAD(wifly_hard_reset_pt(struct pt *pt))
{
  static uint32 timestamp;
    
  PT_BEGIN(pt);

  P1_2 = 0;                                            // Drive Low
  P1DIR |= (1 << 2);        
  DELAY_MS(50);
  P1DIR &= ~(1 << 2);                                   // High Z
  DELAY_MS(100);
  error = CTS_HIGH ? SUCCESS : FAILURE;
  
  PT_END(pt);
}

static 
PT_THREAD(wifly_enter_command_mode_pt(struct pt * pt))
{
  static uint32 timestamp;
  
  PT_BEGIN(pt);
  
  flushUARTRxBuffer(HAL_UART_PORT_0);           // flush rx
  
  DELAY_MS(350);                                // 250ms at least
  HalUARTWrite(HAL_UART_PORT_0, "$$$", 3);      // write token
  DELAY_MS(350);                                // 250ms at least
  
  unsigned char* temp_ptr;                      // locals NOT continued.
  int j, len;                                   // don't stop
       
  osal_memset(wiflyInBuffer, '\0', sizeof(wiflyInBuffer));
  len = Hal_UART_RxBufLen(HAL_UART_PORT_0);
  if (0 == len) {
    error = FAILURE;
    PT_EXIT(pt);
  }

  HalUARTRead(HAL_UART_PORT_0,                  // read rx buffer
              wiflyInBuffer, 
              Hal_UART_RxBufLen(HAL_UART_PORT_0));

  temp_ptr = wiflyInBuffer;                     // trim leading null
  for (j = 0; j < 16; j++) {                    // assuming at most 16 leading null
    if (*temp_ptr == '\0') 
      temp_ptr++;
    else 
      break;
  }    
  error = (strstr((char const*)&wiflyInBuffer[j], "CMD")) ? SUCCESS : FAILURE;
  
  PT_END(pt);
}

typedef struct {
  const char *type;
  const char *category;
  const char *option;
  const char *value;
  const char *response;
  uint16      delay;
} wifly_send_command_param_t;

static wifly_send_command_param_t wifly_send_command_params;

static void wifly_send_command_prepare(const char *type,
                                       const char *category,
                                       const char *option,
                                       const char *value,
                                       const char *response,
                                       uint16 delay) 
{
  wifly_send_command_params.type = type;
  wifly_send_command_params.category = category;
  wifly_send_command_params.option = option;
  wifly_send_command_params.value = value;
  wifly_send_command_params.response = response;
  wifly_send_command_params.delay = delay;    
}

static 
PT_THREAD(wifly_send_command_pt(struct pt * pt))
{
  static uint32 timestamp;
  static wifly_send_command_param_t* param = &wifly_send_command_params;
  
  PT_BEGIN(pt);
  
  flushUARTRxBuffer(HAL_UART_PORT_0);

  if(param->type != NULL)
  {
    HalUARTWrite(HAL_UART_PORT_0, (uint8*)param->type, strlen(param->type));
  }
  if(param->category != NULL)
  {
    HalUARTWrite(HAL_UART_PORT_0, " ", 1);
    HalUARTWrite(HAL_UART_PORT_0, (uint8*)param->category, strlen(param->category));
  }
  if(param->option != NULL)
  {
    HalUARTWrite(HAL_UART_PORT_0, " ", 1);
    HalUARTWrite(HAL_UART_PORT_0, (uint8*)param->option, strlen(param->option));
  }
  if(param->value != NULL)
  {
    HalUARTWrite(HAL_UART_PORT_0, " ", 1);
    HalUARTWrite(HAL_UART_PORT_0, (uint8*)param->value, strlen(param->value));
  }
  
  HalUARTWrite(HAL_UART_PORT_0, "\r", 1);

  if (param->response == NULL) {
    
    DELAY_MS(250);      // wait for tx finish and rx flush
                        // if we do this too soon, the next command may 
                        // get response of command.
    error = SUCCESS;
    PT_EXIT(pt);
  }
  
  // initialize the receiving buffer
  memset(WiflyInBuffer, '\0', sizeof(WiflyInBuffer));
  DELAY_MS(param->delay);
  
  HalUARTRead(HAL_UART_PORT_0, (uint8*)WiflyInBuffer, Hal_UART_RxBufLen(HAL_UART_PORT_0));
    
  // search for instance of "command_response" within the Rx buffer
  error = (strstr(WiflyInBuffer, param -> response)) ? SUCCESS : FAILURE;

  PT_END(pt);
}


static 
PT_THREAD(wifly_reconfigure_pt(struct pt * pt))
{
  static uint32 timestamp;
  static struct pt child;
  
  PT_BEGIN(pt);
  
  /* put module in infrastructure mode using the configuration parameters */
  PT_SPAWN(pt, &child, wifly_enter_command_mode_pt(&child));
  if (error) PT_EXIT(pt);

  // leave any auto-joined network
  wifly_send_command_prepare(ACTION_LEAVE, NULL, NULL, NULL, NULL, 0);
  PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
  // if (error) PT_EXIT(pt);
  
  // re-program SSID, passphrase, channel from user input
  wifly_send_command_prepare(SET, WLAN, SSID, ssid, GENERIC_RESPONSE, 500);
  PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
  if (error) PT_EXIT(pt);  

  wifly_send_command_prepare(SET, WLAN, PHRASE, passwd, GENERIC_RESPONSE, 500);
  PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
  if (error) PT_EXIT(pt);   

  wifly_send_command_prepare(SET, WLAN, CHANNEL, chan, STD_RESPONSE, 500);
  PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
  if (error) PT_EXIT(pt);  

  // restore ip parameters
  wifly_send_command_prepare(SET, IP, DHCP, DHCP_MODE_ON, STD_RESPONSE, 500);
  PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
  if (error) PT_EXIT(pt);  

  /* Settng host ip to 0.0.0.0 is a MUST or else the DNS client process
   * WILL NOT START !!!!!, and the http client demo is dependent on the
   * dns client process auto starting.
   */
  wifly_send_command_prepare(SET, IP, HOST, "0.0.0.0", STD_RESPONSE, 500);
  PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
  if (error) PT_EXIT(pt); 
  
  // restore remaining module parameters to defaults (those changed by entry into EZConfig mode)
  // set comm close 0
  wifly_send_command_prepare(SET, COMM, CLOSE, CLOSE_VALUE, STD_RESPONSE, 500);
  PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
  if (error) PT_EXIT(pt);   
  // set comm open 0
  wifly_send_command_prepare(SET, COMM, OPEN, OPEN_VALUE, STD_RESPONSE, 500);
  PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
  if (error) PT_EXIT(pt);  
  // set comm remote 0
  wifly_send_command_prepare(SET, COMM, REMOTE, COMM_REMOTE_VALUE, STD_RESPONSE, 500);
  PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
  if (error) PT_EXIT(pt); 
  // set wlan join 1
  wifly_send_command_prepare(SET, WLAN, JOIN, JOIN_VALUE, STD_RESPONSE, 500);
  PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
  if (error) PT_EXIT(pt); 
  
  // set sys iofunc 0x70
  wifly_send_command_prepare(SET, SYS, IOFUNC, IOFUNC_VALUE, STD_RESPONSE, 500);
  PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
  if (error) PT_EXIT(pt); 

  // save the changes
  wifly_send_command_prepare(FILEIO_SAVE, NULL, NULL, NULL, STD_RESPONSE, 100);
  PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
  if (error) PT_EXIT(pt); 
  
  // (hard-)reset the module to apply the new settings
  PT_SPAWN(pt, &child, wifly_hard_reset_pt(&child));
  
  // delay after module reset/reboot to allow GPIO4 toggle low-high-low
  DELAY_MS(250);

  PT_END(pt);
}

static struct pt pt_test1_pt;
static 
PT_THREAD(pt_test1(struct pt * pt) ) {
  
  static struct pt child;
  
  PT_BEGIN(pt);
  
  PT_SPAWN(pt, &child, wifly_hard_reset_pt(&child));
  if (error) PT_EXIT(pt);
  
  /**
  PT_SPAWN(pt, &child, wifly_enter_command_mode_pt(&child));
  if (error) PT_EXIT(pt);
  
  osal_memset(&wifly_send_command_params, 0, sizeof(wifly_send_command_params));
  wifly_send_command_params.type = "factory RESET\r";
  wifly_send_command_params.timeout = 100;
  // wifly_send_command_prepare("factory RESET\r", NULL, NULL, NULL, STD_RESPONSE, 100);
  PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
  if (error) PT_EXIT(pt); **/
  
  PT_SPAWN(pt, &child, wifly_reconfigure_pt(&child));
  
  PT_END(pt);
}


/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      SimpleBLEObserver_Init
 *
 * @brief   Initialization function for the Simple BLE Observer App Task.
 *          This is called during initialization and should contain
 *          any application specific initialization (ie. hardware
 *          initialization/setup, table initialization, power up
 *          notification).
 *
 * @param   task_id - the ID assigned by OSAL.  This ID should be
 *                    used to send messages and set timers.
 *
 * @return  none
 */
void SimpleBLEObserver_Init( uint8 task_id )
{
  simpleBLETaskId = task_id;

  // Setup Observer Profile
  {
    uint8 scanRes = DEFAULT_MAX_SCAN_RES;
    GAPObserverRole_SetParameter ( GAPOBSERVERROLE_MAX_SCAN_RES, sizeof( uint8 ), &scanRes );
  }
  
  // Setup GAP
  GAP_SetParamValue( TGAP_GEN_DISC_SCAN, DEFAULT_SCAN_DURATION );
  GAP_SetParamValue( TGAP_LIM_DISC_SCAN, DEFAULT_SCAN_DURATION );

  // Register for all key events - This app will handle all key events
  RegisterForKeys( simpleBLETaskId );
  
  // makes sure LEDs are off
  HalLedSet( (HAL_LED_1 | HAL_LED_2), HAL_LED_MODE_OFF );
  
  uartConfig.configured = TRUE;
  uartConfig.baudRate = HAL_UART_BR_9600;
  uartConfig.flowControl = FALSE;
  uartConfig.flowControlThreshold = 0;
  uartConfig.rx.maxBufSize = 256;
  uartConfig.tx.maxBufSize = 256;
  uartConfig.idleTimeout = 6;
  uartConfig.intEnable = TRUE;
  uartConfig.callBackFunc = 0;
  
  HalUARTOpen(HAL_UART_PORT_0, &uartConfig);
  
  // Setup a delayed profile startup
  osal_set_event( simpleBLETaskId, START_DEVICE_EVT );
  osal_set_event( simpleBLETaskId, SELF_MESSAGE_EVT );
  
  PT_INIT(&pt_test1_pt);
}

/*********************************************************************
 * @fn      SimpleBLEObserver_ProcessEvent
 *
 * @brief   Simple BLE Observer Application Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id  - The OSAL assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  events not processed
 */
uint16 SimpleBLEObserver_ProcessEvent( uint8 task_id, uint16 events )
{
  
  VOID task_id; // OSAL required parameter that isn't used in this function
  
  if ( events & SYS_EVENT_MSG )
  {
    uint8 *pMsg;

    if ( (pMsg = osal_msg_receive( simpleBLETaskId )) != NULL )
    {
      simpleBLEObserver_ProcessOSALMsg( (osal_event_hdr_t *)pMsg );

      // Release the OSAL message
      VOID osal_msg_deallocate( pMsg );
    }

    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }

  if ( events & START_DEVICE_EVT )
  {
    // Start the Device
    VOID GAPObserverRole_StartDevice( (gapObserverRoleCB_t *) &simpleBLERoleCB );

    return ( events ^ START_DEVICE_EVT );
  }
  if ( events & SELF_MESSAGE_EVT )
  {
    if ( pt_test1(&pt_test1_pt) ) {
      events ^= SELF_MESSAGE_EVT;       // stop busy loop
    }
    return events;      // don't clear bit.
  }
  
  // return test1_ProcessEvent( events );
  // Discard unknown events
  return 0;
}

/*********************************************************************
 * @fn      simpleBLEObserver_ProcessOSALMsg
 *
 * @brief   Process an incoming task message.
 *
 * @param   pMsg - message to process
 *
 * @return  none
 */
static void simpleBLEObserver_ProcessOSALMsg( osal_event_hdr_t *pMsg )
{
  switch ( pMsg->event )
  {
    case KEY_CHANGE:
      simpleBLEObserver_HandleKeys( ((keyChange_t *)pMsg)->state, ((keyChange_t *)pMsg)->keys );
      break;

    case GATT_MSG_EVENT:
      break;
  }
}

/*********************************************************************
 * @fn      simpleBLEObserver_HandleKeys
 *
 * @brief   Handles all key events for this device.
 *
 * @param   shift - true if in shift/alt.
 * @param   keys - bit field for key events. Valid entries:
 *                 HAL_KEY_SW_2
 *                 HAL_KEY_SW_1
 *
 * @return  none
 */
uint8 gStatus;
static void simpleBLEObserver_HandleKeys( uint8 shift, uint8 keys )
{
  (void)shift;  // Intentionally unreferenced parameter

  if ( keys & HAL_KEY_UP )
  {
    // Start or stop discovery
    if ( !simpleBLEScanning )
    {
      simpleBLEScanning = TRUE;
      simpleBLEScanRes = 0;
      
      LCD_WRITE_STRING( "Discovering...", HAL_LCD_LINE_1 );
      LCD_WRITE_STRING( "", HAL_LCD_LINE_2 );
      
      GAPObserverRole_StartDiscovery( DEFAULT_DISCOVERY_MODE,
                                      DEFAULT_DISCOVERY_ACTIVE_SCAN,
                                      DEFAULT_DISCOVERY_WHITE_LIST );      
    }
    else
    {
      GAPObserverRole_CancelDiscovery();
    }
  }

  if ( keys & HAL_KEY_LEFT )
  {
    // Display discovery results
    if ( !simpleBLEScanning && simpleBLEScanRes > 0 )
    {
        // Increment index of current result (with wraparound)
        simpleBLEScanIdx++;
        if ( simpleBLEScanIdx >= simpleBLEScanRes )
        {
          simpleBLEScanIdx = 0;
        }
        
        LCD_WRITE_STRING_VALUE( "Device", simpleBLEScanIdx + 1,
                                10, HAL_LCD_LINE_1 );
        LCD_WRITE_STRING( bdAddr2Str( simpleBLEDevList[simpleBLEScanIdx].addr ),
                          HAL_LCD_LINE_2 );
    }
  }

  if ( keys & HAL_KEY_RIGHT )
  {
  }
  
  if ( keys & HAL_KEY_CENTER )
  {
  }
  
  if ( keys & HAL_KEY_DOWN )
  {
  }
}

/*********************************************************************
 * @fn      simpleBLEObserverEventCB
 *
 * @brief   Observer event callback function.
 *
 * @param   pEvent - pointer to event structure
 *
 * @return  none
 */
static void simpleBLEObserverEventCB( gapObserverRoleEvent_t *pEvent )
{
  switch ( pEvent->gap.opcode )
  {
    case GAP_DEVICE_INIT_DONE_EVENT:  
      {
        LCD_WRITE_STRING( "BLE Observer", HAL_LCD_LINE_1 );
        LCD_WRITE_STRING( bdAddr2Str( pEvent->initDone.devAddr ),  HAL_LCD_LINE_2 );
      }
      break;

    case GAP_DEVICE_INFO_EVENT:
      {
        simpleBLEAddDeviceInfo( pEvent->deviceInfo.addr, pEvent->deviceInfo.addrType );
      }
      break;
      
    case GAP_DEVICE_DISCOVERY_EVENT:
      {
        // discovery complete
        simpleBLEScanning = FALSE;

        // Copy results
        simpleBLEScanRes = pEvent->discCmpl.numDevs;
        osal_memcpy( simpleBLEDevList, pEvent->discCmpl.pDevList,
                     (sizeof( gapDevRec_t ) * pEvent->discCmpl.numDevs) );
        
        LCD_WRITE_STRING_VALUE( "Devices Found", simpleBLEScanRes,
                                10, HAL_LCD_LINE_1 );
        if ( simpleBLEScanRes > 0 )
        {
          LCD_WRITE_STRING( "<- To Select", HAL_LCD_LINE_2 );
        }

        // initialize scan index to last device
        simpleBLEScanIdx = simpleBLEScanRes;
      }
      break;
      
    default:
      break;
  }
}

/*********************************************************************
 * @fn      simpleBLEAddDeviceInfo
 *
 * @brief   Add a device to the device discovery result list
 *
 * @return  none
 */
static void simpleBLEAddDeviceInfo( uint8 *pAddr, uint8 addrType )
{
  uint8 i;
  
  // If result count not at max
  if ( simpleBLEScanRes < DEFAULT_MAX_SCAN_RES )
  {
    // Check if device is already in scan results
    for ( i = 0; i < simpleBLEScanRes; i++ )
    {
      if ( osal_memcmp( pAddr, simpleBLEDevList[i].addr , B_ADDR_LEN ) )
      {
        return;
      }
    }
    
    // Add addr to scan result list
    osal_memcpy( simpleBLEDevList[simpleBLEScanRes].addr, pAddr, B_ADDR_LEN );
    simpleBLEDevList[simpleBLEScanRes].addrType = addrType;
    
    // Increment scan result count
    simpleBLEScanRes++;
  }
}

/*********************************************************************
 * @fn      bdAddr2Str
 *
 * @brief   Convert Bluetooth address to string
 *
 * @return  none
 */
char *bdAddr2Str( uint8 *pAddr )
{
  uint8       i;
  char        hex[] = "0123456789ABCDEF";
  static char str[B_ADDR_STR_LEN];
  char        *pStr = str;
  
  *pStr++ = '0';
  *pStr++ = 'x';
  
  // Start from end of addr
  pAddr += B_ADDR_LEN;
  
  for ( i = B_ADDR_LEN; i > 0; i-- )
  {
    *pStr++ = hex[*--pAddr >> 4];
    *pStr++ = hex[*pAddr & 0x0F];
  }
  
  *pStr = 0;
  
  return str;
}

/*********************************************************************
*********************************************************************/
