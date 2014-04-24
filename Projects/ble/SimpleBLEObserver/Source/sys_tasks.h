/*******************************************************************************
  The main application tasks state machines definitions

  Company:
    Microchip Technology Inc.

  File Name:
    sys_tasks.h

  Summary:
    Function prototypes for  main applications tasks

  Description:
     The function prototypes that make up the main application tasks, as well
     as the state machine states for the tasks are defined in this file.
*******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
Copyright (c) 2013 released Microchip Technology Inc.  All rights reserved.

Microchip licenses to you the right to use, modify, copy and distribute
Software only when embedded on a Microchip microcontroller or digital signal
controller that is integrated into your product or third party product
(pursuant to the sublicense terms in the accompanying license agreement).

You should refer to the license agreement accompanying this Software for
additional information regarding your rights and obligations.

SOFTWARE AND DOCUMENTATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF
MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE.
IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER
CONTRACT, NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR
OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE OR
CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT OF
SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
(INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.
*******************************************************************************/
// DOM-IGNORE-END

#ifndef __SYSTASKS_H__
#define __SYSTASKS_H__

#if 0

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#ifdef __cplusplus  // Provide C++ Compatability

    extern "C" {

#endif

#include "delay_ms.h"
#include "indicators.h"
#include "switches.h"
#include "lcd.h"
#include "console.h"
#include "tick.h"
#include "wifly_drv.h"
#include "wifly_util.h"
#include "adc.h"
#include "fifo.h"

// *****************************************************************************
// *****************************************************************************
// Section: Data Types
// *****************************************************************************
// *****************************************************************************

// *****************************************************************************
// *****************************************************************************
// Section: Helper Macros
// *****************************************************************************
// *****************************************************************************

/*  TCPTasks() states  */

enum  smStates
{
    SM_TCP_IDLE = 0,
    SM_TCP_INIT,                    // evaluate module WiFi/IP parms and re-init using updates from code
    SM_TCP_WAIT_ASSOCIATE,          // wait for module to associate with AP; get an IP addr
    SM_TCP_WAIT_CONNECT,            // wait for a connection to the target host
    SM_TCP_HTTP_GET,                 // send HTTP request to the server
    SM_TCP_HTTP_GET_REPLY,          // collect & parse HTTP response from server
    SM_TCP_HTTP_GET_PROCESS,        // process the HTTP response "entity body"
    SM_TCP_DISCONNECT,              // disconnect from server
    SM_TCP_DONE,                    // do nothing until the next communication cycle
    SM_TCP_PREPARE_SOFT_AP,
    SM_TCP_PREPARE_EZCONFIG_RESOURCES,
    SM_TCP_EZCONFIG_PROCESS,
};



enum ledStates {LED_OFF, LED_ON};

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

#define MAX_ARGS                        10

#define TIME_GIVEN_TO_ASSOC_IN_SECS     10
#define BLANKLINE               "                "


// *****************************************************************************
// *****************************************************************************
// Section: Interface Routines Group
// *****************************************************************************
// *****************************************************************************
void sys_Tasks(void);
void AppTasks(void);

#endif

#endif // #if 0