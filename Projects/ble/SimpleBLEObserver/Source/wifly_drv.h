/*******************************************************************************
  The Wifly module driver function prototypes

  Company:
    Microchip Technology Inc.

  File Name:
    wifly_drv.h

  Summary:
     The functions prototyes that are used to drive UART 1 and GPIO pins that interface
     with the wifly module are included in this file.

  Description:
    The functions prototyes that are used to drive UART 1 and GPIO pins that interface
     with the wifly module are included in this file.
*******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
Copyright (c) <year> released Microchip Technology Inc.  All rights reserved.

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

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#ifndef WIFLY_DRV_H
#define WIFLY_DRV_H

// *****************************************************************************
// *****************************************************************************
// Section: Helper Macros
// *****************************************************************************
// *****************************************************************************

// WiFly PicTail I/O Definitions (Refer to RN-171/131 PicTail Schematic)
// requires v2.45 firmware with the following module pre-configuration:
//
// set comm close 0         // suppress all output when connection opened/closed
// set comm open 0
// set comm remote 0
// set sys print 0          // suppress all debug/status print level output messages
// set sys iofunc 0x70      // enable ALT GPIO functions
// set wlan join 1          // auto-associate with saved SSID on reboot
// set uart flow 1          // enable UART flow control
// save                     // save these settings
// reboot                   // reboot the module
//
// (remaining parameters are left at defaults)

// wifly RESET input (PIC output)
// module has internal 100kOhm pull-up
// configure PIC pin as OD output
#define RN_RESET                PORTFbits.RF0
#define TRIS_RN_RESET           TRISFbits.TRISF0
#define OD_RN_RESET             ODCFbits.ODF0


// wifly GPIO9 input (PIC output)
#define RN_GPIO9                PORTFbits.RF6
#define TRIS_RN_GPIO9           TRISFbits.TRISF6

// wifly GPIO4 output (PIC input)
// used to indicate association status
#define RN_GPIO4                PORTFbits.RF8
#define TRIS_RN_GPIO4           TRISFbits.TRISF8

// wifly GPIO5 input (PIC output)
// Used to trigger remote IP host connection
// Configured as open drain O/P
#define RN_GPIO5                PORTFbits.RF7
#define TRIS_RN_GPIO5           TRISFbits.TRISF7
#define OD_RN_GPIO5             ODCFbits.ODF7

// wifly GPIO6 output (PIC input)
// used to indicate TCP connection status
#define RN_GPIO6                PORTEbits.RE8
#define TRIS_RN_GPIO6           TRISEbits.TRISE8

// wifly RTS output (PIC input (CTS))
#define PIC_CTS                 PORTDbits.RD14
#define TRIS_PIC_CTS            TRISDbits.TRISD14

// wifly CTS input (PIC output (RTS))
#define PIC_RTS                 PORTDbits.RD15
#define TRIS_PIC_RTS            TRISDbits.TRISD15

// wifly PICTail LEDY
#define RN_PICTAIL_LEDY         PORTBbits.RB3
#define ADCFG_RN_PICTAIL_LEDY   AD1PCFGbits.PCFG3
#define TRIS_RN_PICTAIL_LEDY    TRISBbits.TRISB3

// wifly PICTAIL LEDR
#define RN_PICTAIL_LEDR         PORTBbits.RB4
#define ADCFG_RN_PICTAIL_LEDR   AD1PCFGbits.PCFG4
#define TRIS_RN_PICTAIL_LEDR    TRISBbits.TRISB4

// *****************************************************************************
// *****************************************************************************
// Section: Interface Routines
// *****************************************************************************
// *****************************************************************************
// init the PIC I/O and UART settings
void initWiFly(uint32_t Fcyc);

// send a character to the serial port
void putWiFly(char c);

// wait for a new character to arrive to the serial port
char getWiFly(void);

// send a null terminated string to the serial port
void putsWiFly(char *s);

// receive a null terminated string in a buffer of len char
char *getsnWiFly(char *s, uint16_t n);

// receive a n-length string in a buffer
char *getsWiFly(char *s, uint16_t n);

#endif // WIFLY_DRV_H
