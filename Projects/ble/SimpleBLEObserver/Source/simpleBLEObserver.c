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
#include <stdio.h>


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


/*********************************

 switch off this macro for demo or production

**********************************/
#define STATE_DEBUG				FALSE
#define DEBUG_BUF_SIZE			256
char debug_buf[DEBUG_BUF_SIZE];

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


//the period time 1000ms
#define PERIODTIME        10000

//the count of no ping at disconnect state
#define DIS_NP_LIMIT      30   
//the count of no ping at connect state
#define CONN_NP_LIMIT     10

#define PONG      1
#define NOPONG    0

#define SENDPING "PING\r\n"

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

uint8 tcp_connected = 0;

///** infrastructure mode **/
//char ssid[32]   = "UnwiredGrain";
//char passwd[32] = "3211238976";
//char chan[32]   = "0";
//char iphost[32] = "192.168.1.124";

/** infrastructure mode **/
//char ssid[32]   = "OVERTECH";
//char passwd[32] = "overtechoffice2013";
//char chan[32]   = "0";
//char iphost[32] = "192.168.1.171";

//char ssid[32]   = "MTC-OFFICE-TEST";
char ssid[32]   = "MTC-OFFICE-10";
char passwd[32] = "MATON123";
char chan[32]   = "0";
char iphost[32] = "192.168.10.20";


//char ssid[32]   = "AndroidAP";
//char passwd[32] = "oumd9531";
//char chan[32]   = "0";
//char iphost[32] = "192.168.43.99";

/** SoftAP mode **/
char ssidOfAP[20]         = "Microchip";
char moduleMacAddress[25] = {'\0'};
char macFormData[150]     = {'\0'};
char ssidFormData[150]    = {'\0'};

volatile int lll;

/* The Webserver Definitions  - Derrick */
static uint8 simpleBuf[512];
static uint8 fileName[64];

static uint16		sbidx = 0;
volatile int    	parms;
static uint16      	httpLine = 0;

static uint8 		tinyBuf[32];
static uint16      	contentLen = 0;

static uint16 		fillingPostData = 0;

static uint16 NoPongCount = 0;
static bool ConnectState =  false;


enum WIFISTATE
{
    DISCONNECT,
    CONNECT,
};

enum WIFISTATE  WifiState= DISCONNECT;

/* http parser states */
enum HttpState
{
    HTTP_CONN_WAIT,
    HTTP_CONN_WAIT1,
    HTTP_CONN_WAIT2,
    HTTP_CONN_OPEN,
    HTTP_CONN_CLOSE,
    HTTP_CONN_CLOSE1,
};

enum HttpMethod {
    HTTP_METHOD_GET,
    HTTP_METHOD_POST,
    HTTP_METHOD_READ_POST_DATA,
    HTTP_METHOD_GET_XML,
    HTTP_METHOD_FORCE_STARTOVER,
    HTTP_METHOD_ADVANCED_FEATURES
};

enum HttpState  httpState = HTTP_CONN_WAIT;
enum HttpMethod httpMethod;

// static void sendPage(unsigned char *s);
// static void sendPageXML(unsigned char *s);
// static void sendPageChunks(unsigned char *fmt, ...);
   
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

unsigned char 		wiflyInBuffer[512];
uint16 				wiflyInBufferIndex;
static bool			discardUartRX = TRUE;
static uint16		debugCount = 0;
static uint16 NP_Counter = 0;

// typedef void (*halUARTCBack_t) (uint8 port, uint8 event);
static void uartCB(uint8 port, uint8 event) {
	
	uint8 bin[16];
	
	switch (event) {
		case HAL_UART_RX_ABOUT_FULL:
		
			//if (discardUartRX) {			
			//	HalUARTRead(port, bin, 16);
			//}
		
			break;
		default:
			break;
	}
}

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void simpleBLEObserverEventCB( gapObserverRoleEvent_t *pEvent );
static void simpleBLEObserver_HandleKeys( uint8 shift, uint8 keys );
static void simpleBLEObserver_ProcessOSALMsg( osal_event_hdr_t *pMsg );
static void simpleBLEAddDeviceInfo( uint8 *pAddr, uint8 addrType );
char *bdAddr2Str ( uint8 *pAddr );

static void hang(){ /** do nothing **/ };

char *DCtoHEX( uint8 *pAddr );
void DisconnectEntry(void);

#include "pages.c"

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

typedef struct {
	unsigned char *s;
} wifly_sendPage_param_t;

static wifly_sendPage_param_t wifly_sendPage_param;

static
PT_THREAD(wifly_sendPage_pt(struct pt * pt) )
{
	static uint32 timestamp;
 /*
    sendString(httpHeaders);
    sprintf(tinyBuf, "%d\r\n\r\n", strlen(s));
    sendString(tinyBuf);
    sendString(s);
*/
	PT_BEGIN(pt);
/**	
	DELAY_MS(30);
	putsWiFly((char *)httpHeaders);
	DELAY_MS(15);

	sprintf((char *)tinyBuf, "%d\r\n\r\n", strlen((const char *)s));
	putsWiFly((char *)tinyBuf);
	DELAY_MS(15);

	putsWiFly((char *)s); **/
	PT_END(pt);
}


void sendPageXML(unsigned char *s)
{
 /*
    sendString(httpHeaders);
    sprintf(tinyBuf, "%d\r\n\r\n", strlen(s));
    sendString(tinyBuf);
    sendString(s);
*/
	/**
      DELAY_MS(30);
      putsWiFly((char *)xmlHeaders);
      //ConsolePutString(httpHeaders);
      putsConsole((char *)xmlHeaders);
      DELAY_MS(15);

      sprintf((char *)tinyBuf, "%d\r\n\r\n", strlen((const char *)s));
      putsWiFly((char *)tinyBuf);
      //ConsolePutString(tinyBuf);
      putsConsole((char *)tinyBuf);
      DELAY_MS(15);

      putsWiFly((char *)s);
      //ConsolePutString(s);
      putsConsole((char *)s); **/
}

#if 0
void sendPageChunks(unsigned char *fmt, ...)
{
    va_list ptrToArguments;
    unsigned char *ptrToArgChars, *sval[MAX_ARGS] = {NULL};
    int index = 0;
    int pageLength = 0;

    /* extract each argument one at a time and match with sval pointer array */
    va_start(ptrToArguments, fmt);
    
    for(ptrToArgChars = fmt; *ptrToArgChars; ptrToArgChars++)
    {
        if(*ptrToArgChars != '%')
        {
            continue;
        }

        /* fill the array to pointers with the pointers that are passed as args */
        switch(*++ptrToArgChars)
        {
            case 's':
                sval[index] = va_arg(ptrToArguments, unsigned char *);
                index++;
                break;
            default:
                break;
        }
    }
    va_end(ap);

    /* preceed each hmtl body with the html standard header */
    DELAY_MS(30);
    putsWiFly((char *)httpHeaders);
    putsConsole((char *)httpHeaders);
    DELAY_MS(15);

    /* calculate the total size of the hmtl page to be sent */
    index = 0;
    while(sval[index] != NULL)
    {
        pageLength = pageLength + strlen((const char *)sval[index]);
        index++;
    }

    /* calculate the total size of the hmtl page to be sent 
    sprintf((char *)tinyBuf, "%d\r\n\r\n",
               strlen((const char *)sval[0]) + strlen((const char *)sval[1]) +
               strlen((const char *)sval[2]) + strlen((const char *)sval[3]) +
               strlen((const char *)sval[4]) );  */
    sprintf((char *)tinyBuf, "%d\r\n\r\n",pageLength);
    putsWiFly((char *)tinyBuf);
    putsConsole((char *)tinyBuf);
    DELAY_MS(15);

    /* send all the individual pieces that make up the page */
    index = 0;
    while(sval[index] != NULL)
    {
         putsWiFly((char *)sval[index]);
         putsConsole((char *)sval[index]);
         index++;
    }

}
#endif


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
  
  discardUartRX = TRUE;

  P1_2 = 0;                                            // Drive Low
  P1DIR |= (1 << 2);        
  DELAY_MS(100);
  P1DIR &= ~(1 << 2);                                   // High Z
  DELAY_MS(500);
  
  discardUartRX = FALSE;
  debugCount = 0;
  
  error = CTS_HIGH ? SUCCESS : FAILURE;
  
  PT_END(pt);
}

static 
PT_THREAD(wifly_enter_command_mode_pt(struct pt * pt))
{
  static uint32 timestamp;
  static uint16 len1, len2;
  
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

  len1 = Hal_UART_RxBufLen(HAL_UART_PORT_0);
  len2 = 
  HalUARTRead(HAL_UART_PORT_0,                  // read rx buffer
              wiflyInBuffer, 
              len1);

  
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
  memset(wiflyInBuffer, '\0', sizeof(wiflyInBuffer));
  DELAY_MS(param->delay);
  
  HalUARTRead(HAL_UART_PORT_0, (uint8*)wiflyInBuffer, Hal_UART_RxBufLen(HAL_UART_PORT_0));
    
  // search for instance of "command_response" within the Rx buffer
  error = (strstr(wiflyInBuffer, param -> response)) ? SUCCESS : FAILURE;

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
  // if (error) PT_EXIT(pt);

  // leave any auto-joined network
  wifly_send_command_prepare(ACTION_LEAVE, NULL, NULL, NULL, NULL, 0);
  PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
 // if (error) PT_EXIT(pt);
  
  // re-program SSID, passphrase, channel from user input
  wifly_send_command_prepare(SET, WLAN, SSID, ssid, STD_RESPONSE, 500);
  PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
  //if (error) PT_EXIT(pt);  

  wifly_send_command_prepare(SET, WLAN, PHRASE, passwd, STD_RESPONSE, 500);
  PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
  //if (error) PT_EXIT(pt);   

  wifly_send_command_prepare(SET, WLAN, CHANNEL, chan, STD_RESPONSE, 500);
  PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
  //if (error) PT_EXIT(pt);  

  // restore ip parameters
  wifly_send_command_prepare(SET, IP, DHCP, DHCP_MODE_ON, STD_RESPONSE, 500);
  PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
  //if (error) PT_EXIT(pt);  

  // sound like non-sense
  /* Settng host ip to 0.0.0.0 is a MUST or else the DNS client process
   * WILL NOT START !!!!!, and the http client demo is dependent on the
   * dns client process auto starting.
   */
  wifly_send_command_prepare(SET, IP, HOST, iphost, STD_RESPONSE, 500);
  PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
  //if (error) PT_EXIT(pt); 
  
  // tcp client only
  wifly_send_command_prepare(SET, IP, "protocol", "0x08", STD_RESPONSE, 500);
  PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
  //if (error) PT_EXIT(pt);   

  wifly_send_command_prepare(SET, SYS, "autoconn", "10", STD_RESPONSE, 500);
  PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
  //if (error) PT_EXIT(pt); 
  
  // set ip flags 0x6, disable tcp retry if ap lost
//  wifly_send_command_prepare(SET, IP, "flags", "0x6", STD_RESPONSE, 500);
//  PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
  //if (error) PT_EXIT(pt);   

  
  // restore remaining module parameters to defaults (those changed by entry into EZConfig mode)
  // set comm close 0
  wifly_send_command_prepare(SET, COMM, CLOSE, CLOSE_VALUE, STD_RESPONSE, 500);
  PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
  // if (error) PT_EXIT(pt);   
  // set comm open 0
  wifly_send_command_prepare(SET, COMM, OPEN, OPEN_VALUE, STD_RESPONSE, 500);
  PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
  // if (error) PT_EXIT(pt);  
  // set comm remote 0
  wifly_send_command_prepare(SET, COMM, REMOTE, COMM_REMOTE_VALUE, STD_RESPONSE, 500);
  PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
  // if (error) PT_EXIT(pt); 
  // set wlan join 1
  wifly_send_command_prepare(SET, WLAN, JOIN, JOIN_VALUE, STD_RESPONSE, 500);
  PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
  
    // set the length of time to wait for join ap
  wifly_send_command_prepare(SET, "opt", "jointmr", "3000", STD_RESPONSE, 500);
  PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
  // if (error) PT_EXIT(pt); 
  
  // set sys iofunc 0x50
  wifly_send_command_prepare(SET, SYS, IOFUNC, "0x50", STD_RESPONSE, 500);
  PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
  // if (error) PT_EXIT(pt); 

  // save the changes
  wifly_send_command_prepare(FILEIO_SAVE, NULL, NULL, NULL, STD_RESPONSE, 500);
  PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
  
  
  // if (error) PT_EXIT(pt); 
  
  // (hard-)reset the module to apply the new settings
  PT_SPAWN(pt, &child, wifly_hard_reset_pt(&child));
  
  // delay after module reset/reboot to allow GPIO4 toggle low-high-low
  DELAY_MS(250);

  PT_END(pt);
}


static
PT_THREAD(wifly_configureAP_pt(struct pt * pt))
{
	static struct pt child;
	static uint32 timestamp;
	
	// no idea what is this.
	// leave PIC24 open-drain pin as output and turn off
	// let PICTail pull-up resistor pull GPIO5 high
	// open-drain configuration should be ok while GPIO5 transitions back to default function during SoftAP mode
    // RN_GPIO5 = 1;
	
	PT_BEGIN(pt);
	
	// enter command mode
	PT_SPAWN(pt, &child, wifly_enter_command_mode_pt(&child));
	if (error) PT_EXIT(pt);
	
	// leave the network
	wifly_send_command_prepare(ACTION_LEAVE, NULL, NULL, NULL, NULL, 500);
	PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
	if (error) PT_EXIT(pt);
	
	wifly_send_command_prepare(SET, WLAN, JOIN, SAP_NETWORK_AP_MODE, STD_RESPONSE, 500);
	PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
	if (error) PT_EXIT(pt);

	// set the wlan channel
	wifly_send_command_prepare(SET, WLAN, CHANNEL, SAP_NETWORK_CHANNEL, STD_RESPONSE, 500);
	PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
	if (error) PT_EXIT(pt);
	
	// set the wlan ssid
	wifly_send_command_prepare(SET, WLAN, SSID, (const char *)ssidOfAP, STD_RESPONSE, 500);
	PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
	if (error) PT_EXIT(pt);	
	
    // set the wlan passphrase
	wifly_send_command_prepare(SET, WLAN, PHRASE, SAP_NETWORK_PASS, STD_RESPONSE, 500);
	PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
	if (error) PT_EXIT(pt);	
    
    // set dhcp mode
	wifly_send_command_prepare(SET, IP, DHCP, SAP_NETWORK_DHCP_MODE, STD_RESPONSE, 500);
	PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
	if (error) PT_EXIT(pt);	
	
    // set ip mode
	wifly_send_command_prepare(SET, IP, ADDRESS, SAP_NETWORK_IP_ADDRESS, STD_RESPONSE, 500);
	PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
	if (error) PT_EXIT(pt);	

    // set netmaske
	wifly_send_command_prepare(SET, IP, NETMASK, SAP_NETWORK_SUBNET_MASK, STD_RESPONSE, 500);
	PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
	if (error) PT_EXIT(pt);	
	
    // set gateway
	wifly_send_command_prepare(SET, IP, GATEWAY, SAP_NETWORK_IP_ADDRESS, STD_RESPONSE, 500);
	PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
	if (error) PT_EXIT(pt);	

    // set tcp port open string, what is this?
	wifly_send_command_prepare(SET, COMM, OPEN, TCPPORT_OPEN_STRING, STD_RESPONSE, 500);
	PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
	if (error) PT_EXIT(pt);	

    // set tcp port close string, what is this?
	wifly_send_command_prepare(SET, COMM, CLOSE, TCPPORT_CLOSE_STRING, STD_RESPONSE, 500);
	PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
	if (error) PT_EXIT(pt);		

	// set tcp port hello string
	wifly_send_command_prepare(SET, COMM, REMOTE, TCPPORT_HELLO_STRING, STD_RESPONSE, 500);
	PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
	if (error) PT_EXIT(pt);	

	// set io func (0x50? may be different from common mode, what is supposed to be 0x70)
	wifly_send_command_prepare(SET, "sys", "iofunc", "0x50", STD_RESPONSE, 500);
	PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
	if (error) PT_EXIT(pt);	

	// save the settings
	wifly_send_command_prepare(FILEIO_SAVE, NULL, NULL, NULL, STD_RESPONSE, 500);
	PT_SPAWN(pt, &child, wifly_send_command_pt(&child));
	if (error) PT_EXIT(pt);	
	
    PT_SPAWN(pt, &child, wifly_hard_reset_pt(&child));
	if (error) PT_EXIT(pt);
	
	DELAY_MS(1000);
	
	flushUARTRxBuffer(HAL_UART_PORT_0);
			 
	PT_END(pt);
}



uint8 httpProcessPostString (char *sourceString, char *patternString, char *stringValue)
{
    uint8 indx;
    char  *partialString;

    if( (partialString = (char *)strstr((const char *)sourceString, (const char *)patternString)) )
    {
        while(*partialString != '=')
        {
            ++partialString;
        }
        ++partialString;
        indx = 0;
        while(*partialString != '&')
        {
            stringValue[indx++] = *partialString++;
        }
        stringValue[indx] = '\0';
        return(1);
    }
    else
    {
        return(0);
    }
}



static 
PT_THREAD(wifly_httpParseLine_pt(struct pt * pt))
{
	static struct pt child;
	static uint32 timestamp;
	
	PT_BEGIN(pt);
	
    if (fillingPostData)
    {
        httpProcessPostString ((char *)simpleBuf, (char *)"SSID=", ssid);
		httpProcessPostString ((char *)simpleBuf, (char *)"password=", passwd);
		httpProcessPostString ((char *)simpleBuf, (char *)"Channel=", chan);

        /* send the close-out page back to the browser */
        sendPage(page2);
        DELAY_MS(1000);

        /* the work of the EZConfig process is completed and what remains is to
         * reconfigure the module back to its normal state and let it try to
         * associate to its freashly savd network parameters
         */
         
        /* The HTTP client operates in polling mode so restore that mode by
         * disabling the UART interrupts. No more data into fifo */
        // IEC0bits.U1RXIE = 0;
        // IEC4bits.U1ERIE = 0;

		flushUARTRxBuffer(HAL_UART_PORT_0);
        // flushWiflyRxUart(10);  // flush out uart for 40 milliseconds
        // IFS0bits.U1RXIF = 0;
        // IFS4bits.U1ERIF = 0;
        // fifoInit();           // reset fifo indexes.


        /* because the device will not operate in Polling mode until it rebooted
         * the normal wifly driver functions cannot be used here.    Must just send
         * commands directly to the uart Tx, before switching back to  polling mode
         * after module reset
         */
        // RN_GPIO5 = 0;
        
        // need to do these at a minimum before using wifly_util_xxx_APIs
		DELAY_MS(350);
		HalUARTWrite(HAL_UART_PORT_0, "$$$", strlen("$$$"));
		DELAY_MS(1000);

		HalUARTWrite(HAL_UART_PORT_0, "set comm close 0\r\n", strlen("set comm close 0\r\n"));
        putsWiFly("set comm close 0");
        putWiFly('\r');
        DELAY_MS(40);
        putsWiFly("set comm open 0");
        putWiFly('\r');
        DELAY_MS(40);
        putsWiFly("set sys iofunc 0x70");
        putWiFly('\r');
        DELAY_MS(40);
        putsWiFly("save");
        putWiFly('\r');
        DELAY_MS(40);
        putsWiFly("reboot");
        putWiFly('\r');
        DELAY_MS(40);
        
        /* flush out the uart because bytes from previous communications are
         * still residing in it (up to 4 bytes according to specs
         */

        DELAY_MS(500);
        // U1STAbits.OERR  = 0;
        // flushWiflyRxUart(10);  // flush out uart for 100 milliseconds
		flushUARTRxBuffer(HAL_UART_PORT_0);
		
		

        /* tell the module it can now send data */
        // PIC_RTS = 1;
        // U1STAbits.OERR  = 0;

		PT_SPAWN(pt, &child, wifly_reconfigure_pt(&child));

    }


    if (httpLine == 0)
    {
        if(strstr( (const char *)simpleBuf, "GET /scanresults"))
        {
            httpMethod = HTTP_METHOD_GET_XML;
        }

        else if(strstr( (const char *)simpleBuf, "GET /index"))
        {
            httpMethod = HTTP_METHOD_GET;
        }

        else if(strstr( (const char *)simpleBuf, "GET /advance"))
        {
            httpMethod = HTTP_METHOD_ADVANCED_FEATURES;
        }

        else if (sscanf((char *)simpleBuf, "GET %63s", (char *)fileName))
        {
            httpMethod = HTTP_METHOD_FORCE_STARTOVER;
        }
        else if (sscanf((char *)simpleBuf, "POST %63s", (char *)fileName))
        {
            httpMethod = HTTP_METHOD_POST;
        }
    }
    ++httpLine;     // Increment the line number

    lll = strlen((char *)simpleBuf);
    if (lll == 0)               // End of http block
    {
        httpLine = 0;           // reset line at end of block
        if(httpMethod == HTTP_METHOD_GET_XML )
        {
            // sendPageXML((unsigned char *)&xmlLines);
            httpMethod = HTTP_METHOD_FORCE_STARTOVER;
        }

        else if (httpMethod == HTTP_METHOD_ADVANCED_FEATURES)
        {
            sendPageChunks((unsigned char *)"%s%s%s%s%s",advanced_part1,macFormData,advanced_part2,ssidFormData,advanced_part3);
            httpMethod = HTTP_METHOD_FORCE_STARTOVER;
        }

        else if (httpMethod == HTTP_METHOD_GET)
        {
            sendPage((unsigned char *)page4);
            httpMethod = HTTP_METHOD_FORCE_STARTOVER;
        }
        else if (httpMethod == HTTP_METHOD_POST)
        {
            fillingPostData = 1;
        }
    }
    else
    {
        sscanf((char *)simpleBuf,"Content-Length: %d", &contentLen);
    }
	
	PT_END(pt);
}



PT_THREAD(wifly_EZConfigTasks_pt(struct pt * pt))
{
	static struct pt child;
	static uint32 timestamp;
    
	static uint8 c;

	PT_BEGIN(pt);
	
    while (Hal_UART_RxBufLen(HAL_UART_PORT_0) && HalUARTRead(HAL_UART_PORT_0, &c, 1))
	{		
        if (HTTP_CONN_WAIT == httpState) {
        	if (c == '^') {
            	httpState = HTTP_CONN_WAIT1; // received ^
            }
        } 
		else if ( HTTP_CONN_WAIT1 == httpState ) {
            if (c == 'o') {
            	httpState = HTTP_CONN_WAIT2; // received ^o
            }
			else
				httpState = HTTP_CONN_WAIT;  // received ^?
		}
		else if ( HTTP_CONN_WAIT2 == httpState ) {
        	if (c == '^')
            {
				httpState = HTTP_CONN_OPEN;  // received ^o^
				sbidx = 0;
				httpLine = 0;
			}
            else
            	httpState = HTTP_CONN_WAIT;  // received ^o?
		}
		else if ( HTTP_CONN_OPEN == httpState ) {
                if (c == '^')
                {
                    httpState = HTTP_CONN_CLOSE;  // received ^
                }
                else if (c == '\n')  /* the end of each line received after open */
                {
                    simpleBuf[sbidx-1] = '\0';
					
                    PT_SPAWN(pt, &child, wifly_httpParseLine_pt(&child));
					
                    sbidx = 0;                  // Get ready for next line
                }
                else
                {
                    /* in the middle of each line received after open */
                    if (sbidx < (int)sizeof(simpleBuf)-1)
                    {
                        simpleBuf[sbidx++] = c;
                    }

                    if (fillingPostData && (sbidx >= contentLen) )
                    {
                        simpleBuf[sbidx] = '\0';
						
                        PT_SPAWN(pt, &child, wifly_httpParseLine_pt(&child));
						
                        fillingPostData = 0;
                        sbidx = 0;
                    }
                }
		}
		else if ( HTTP_CONN_CLOSE == httpState ) {
                if (c == 'c')
                    httpState = HTTP_CONN_CLOSE1; // received ^c
                else
                    httpState = HTTP_CONN_OPEN;  // received ^?
        }
		else if ( HTTP_CONN_CLOSE1 == httpState ) {
			
                if (c == '^')
                {
                    httpState = HTTP_CONN_WAIT; // received ^c^

                    /* derrick added */
                    sbidx           = 0;
                    httpLine        = 0;
                    fillingPostData = 0;
                }
                else
                    httpState = HTTP_CONN_OPEN;  // received ^c?
		}

    }
	
	PT_END(pt);
}



static struct pt pt_test1_pt;
static 
PT_THREAD(pt_test1(struct pt * pt) ) {

  
  	static struct pt child;
  	static uint32 timestamp;
  
  	PT_BEGIN(pt);
  
	// Power WiFi Module
	P1SEL &= ~(1 << 3);		// enable GPIO on P1.3
  	P1_3 = 1;				// drive high
  	P1DIR |= (1 << 3);		// output
	
	// FORCE WAKEUP
	P1SEL &= ~(1 << 1); 	// enable GPIO on P1.1
	P1_1 = 1;				// drive high
	P1DIR |= (1 << 1);		// output
	
	// WiFi MODULE GPIO 4, P0.0
	P0SEL &= ~(1 << 0);
	P0DIR &= ~(1 << 0);		// input
	
	// WiFi MODULE GPIO 5, P0.6 not really used yet
	P0SEL &= ~(1 << 6);
	P0_6 = 0;				// drive low
	P0DIR |= (1 << 6);		// output
	
	// WiFi MODULE GPIO 6, P0.1
	P0SEL &= ~(1 << 1);
	P0DIR &= ~(1 << 1);		// input
	
	
	// p1_0 drive led1 deafaut 1 turn off led1
	P1SEL &= ~(1 << 0);
	P1_0 = 1;				// drive high
	P1DIR |= (1 << 0);		// output
	
	
	// p0_7 drive led2 deafaut 1 turn off led2
	P0SEL &= ~(1 << 7);
	P0_7 = 1;				// drive high
	P0DIR |= (1 << 7);		// output
	
	
	DELAY_MS(1000);
			
	PT_SPAWN(pt, &child, wifly_hard_reset_pt(&child));
	DELAY_MS(1000);

	/**
			PT_SPAWN(pt, &child, wifly_hard_reset_pt(&child));
			DELAY_MS(1000);
			
			PT_SPAWN(pt, &child, wifly_hard_reset_pt(&child));
			DELAY_MS(1000);	 **/

	PT_SPAWN(pt, &child, wifly_reconfigure_pt(&child));  
	DELAY_MS(1000);		
	
	//	start time
	osal_start_reload_timer( simpleBLETaskId, TIMER1_EVT, PERIODTIME );	
	flushUARTRxBuffer(HAL_UART_PORT_0);
	DisconnectEntry();
		
//		while (1 == P0_0) { // wait for tcp connection
//			
//			
//			P1_0 = 1;				// turn off led1
//			P0_7 = 0;				// turn on led2
//			
//			PT_WAIT_UNTIL(pt, 1 == P0_1);
//			
//			P1_0 = 0;				// turn on led1
//			P0_7 = 0;				// turn on led1
//			
//			
//			GAPObserverRole_StartDiscovery( DEFAULT_DISCOVERY_MODE,
//            	DEFAULT_DISCOVERY_ACTIVE_SCAN,
//                DEFAULT_DISCOVERY_WHITE_LIST);  
//			
//			PT_WAIT_UNTIL(pt, (0 == P0_1) || (0 == P0_0));
//			
//			P1_0 = 0;				// turn on led1
//			P0_7 = 1;				// turn on led1
//			
//			GAPObserverRole_CancelDiscovery();
//		}
//	} 
	
	
	

	// PT_SPAWN(pt, &child, wifly_hard_reset_pt(&child));
	// DELAY_MS(1000);
	
  	// PT_SPAWN(pt, &child, wifly_configureAP_pt(&child));
  	// DELAY_MS(1000);	// for 10s

	// PT_SPAWN(pt, &child, wifly_hard_reset_pt(&child));
	// DELAY_MS(1000);
  	
	// PT_SPAWN(pt, &child, wifly_reconfigure_pt(&child));	
	// DELAY_MS(1000);
  
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
  uartConfig.callBackFunc = uartCB;
  
  HalUARTOpen(HAL_UART_PORT_0, &uartConfig);
  
  // Setup a delayed profile startup
  osal_set_event( simpleBLETaskId, START_DEVICE_EVT );
  osal_set_event( simpleBLETaskId, SELF_MESSAGE_EVT );
  
  
  
  PT_INIT(&pt_test1_pt);
}

void ConnectEntry(void)
{
	NP_Counter = 0;
	GAPObserverRole_StartDiscovery( DEFAULT_DISCOVERY_MODE,
                                      DEFAULT_DISCOVERY_ACTIVE_SCAN,
                                      DEFAULT_DISCOVERY_WHITE_LIST );

	if (STATE_DEBUG) {
		sprintf(debug_buf, "C Enter, state is %d\r\n", WifiState);
		HalUARTWrite(HAL_UART_PORT_0, (uint8*)debug_buf, strlen(debug_buf));
	}	
}

void ConnectExit(void)
{
	GAPObserverRole_CancelDiscovery();	
	
	if (STATE_DEBUG) {
		sprintf(debug_buf, "C Exit, state is %d\r\n", WifiState);
		HalUARTWrite(HAL_UART_PORT_0, (uint8*)debug_buf, strlen(debug_buf));
	}	
}

void DisconnectEntry(void)
{
	NP_Counter = 0;
	
	if (STATE_DEBUG) {
		sprintf(debug_buf, "D Enter\r\n");
		HalUARTWrite(HAL_UART_PORT_0, (uint8*)debug_buf, strlen(debug_buf));
	}		
}
void DisconnectExit(void)
{

	if (STATE_DEBUG) {
		sprintf(debug_buf, "D Exit, state is %d \r\n", WifiState);
		HalUARTWrite(HAL_UART_PORT_0, (uint8*)debug_buf, strlen(debug_buf));		
	}		
}



void ProcessTimeEvent(void)
{
    uint8 response = NOPONG;
	uint16 len = 0;
	
	if(WifiState==CONNECT)
	{
			P1_0 = 0;				// turn on led1
    		P0_7 = 0;				// turn on led1	
	}
	else
	{
			P1_0 = 1;				// turn on led1
			P0_7 = 0;				// turn on led1
	}
	
	HalUARTWrite(HAL_UART_PORT_0, SENDPING, strlen(SENDPING));
	
	len = Hal_UART_RxBufLen(HAL_UART_PORT_0);
    if (len) {
		memset(wiflyInBuffer, '\0', sizeof(wiflyInBuffer));
    	len = HalUARTRead(HAL_UART_PORT_0, (uint8*)wiflyInBuffer, len);
	
    	if( len && strstr((char const *)wiflyInBuffer, "PONG") ) {
			response = PONG;
		}
	}

	if (STATE_DEBUG) {	
		sprintf(debug_buf, "before: e: %s, st: %d, cnt: %d\r\n", (response == PONG ? "PONG" : "NOPONG"), WifiState, NP_Counter);
		HalUARTWrite(HAL_UART_PORT_0, debug_buf, strlen(debug_buf));
	}
	
	switch(response)
	{
		case PONG:
			
			if(WifiState==DISCONNECT)
			{
				DisconnectExit();  
				WifiState = CONNECT;
				ConnectEntry();
			}
			else /* CONNECT */
			{
				NP_Counter = 0;
			}
			
			break;
			
		case NOPONG:
			
			if(WifiState==CONNECT)
			{
				if(NP_Counter > CONN_NP_LIMIT)
				{
					ConnectExit();
					WifiState = DISCONNECT;
					DisconnectEntry();
					
				}
				else
					NP_Counter++;
			}
			else if(WifiState==DISCONNECT)
			{
				if(NP_Counter > DIS_NP_LIMIT)
				{
					HAL_SYSTEM_RESET();//REBOOT SYSTEM
				}
				else
					NP_Counter++;
			}
			break;
		
		default:
			HAL_SYSTEM_RESET();//REBOOT SYSTEM
			break;
		
	}
  
	if (STATE_DEBUG) {
		
		sprintf(debug_buf, "after : e: %s, st: %d, cnt: %d\r\n\r\n", (response == PONG ? "PONG" : "NOPONG"), WifiState, NP_Counter);
		HalUARTWrite(HAL_UART_PORT_0, debug_buf, strlen(debug_buf));
	}
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
  
  if ( events & TIMER1_EVT )
  {
    // process the envent 
	ProcessTimeEvent();
    return ( events ^ TIMER1_EVT );
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
      // simpleBLEObserver_HandleKeys( ((keyChange_t *)pMsg)->state, ((keyChange_t *)pMsg)->keys );
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
		
		  if (STATE_DEBUG) {
			  HalUARTWrite(HAL_UART_PORT_0, ".", 1);
		  }
		  else {
			HalUARTWrite(HAL_UART_PORT_0, bdAddr2Str( pEvent->deviceInfo.addr ), 14);
			//HalUARTWrite(HAL_UART_PORT_0, "\r\n", 2);
			// simpleBLEAddDeviceInfo( pEvent->deviceInfo.addr, pEvent->deviceInfo.addrType );
			HalUARTWrite(HAL_UART_PORT_0,(unsigned char*)DCtoHEX((uint8*) &pEvent->deviceInfo.rssi ), 4);
			  
			// HalUARTWrite(HAL_UART_PORT_0, (unsigned char*)&pEvent->deviceInfo.rssi, 1);
			HalUARTWrite(HAL_UART_PORT_0, "\r\n", 2);
		  }
      }
      break;
      
    case GAP_DEVICE_DISCOVERY_EVENT:
      {
		  	// do it again
		  if(WifiState==CONNECT)
		  {
			GAPObserverRole_StartDiscovery( DEFAULT_DISCOVERY_MODE,
            	DEFAULT_DISCOVERY_ACTIVE_SCAN,
            	DEFAULT_DISCOVERY_WHITE_LIST);  	
		  }
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


char *DCtoHEX( uint8 *pAddr )
{
  uint8       i;
  char        hex[] = "0123456789ABCDEF";
  static char str[5];
  char        *pStr = str;
  
  *pStr++ = '0';
  *pStr++ = 'x';
  
  // Start from end of addr
  pAddr += 1;
  
  for ( i = 1; i > 0; i-- )
  {
    *pStr++ = hex[*--pAddr >> 4];
    *pStr++ = hex[*pAddr & 0x0F];
  }
  
  *pStr = 0;
  
  return str;
}
