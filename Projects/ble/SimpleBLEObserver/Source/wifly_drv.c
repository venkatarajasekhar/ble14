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

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "wifly_drv.h"
#include "delay_ms.h"
#include "console.h"


// *****************************************************************************
// *****************************************************************************
// Section: File Scope or Global Constants
// *****************************************************************************
// *****************************************************************************
#define BAUDRATE    9600    // baud rate setting
#define BACKSPACE   0x8     // ASCII backspace character code

// init the PIC I/O and UART (UART1, BAUDRATE, 8, N, 1, RTS/CTS )
void initWiFly(uint32_t Fcyc)
{
        // initialize wifly RESET output
        // enable open drain configuration and drive high
        // required since module RESET has internal 100kOhm PU
        // we will perform a software-driven module reset before sending commands
        TRIS_RN_RESET = 0;
        OD_RN_RESET = 1;        // enable RF0 output as OD
        RN_RESET = 1;           // release module from reset (let the pull-up work)
               
        // initialize GPIO9 output
        RN_GPIO9 = 0;
        TRIS_RN_GPIO9 = 0;

        // initialize GPIO4 input
        TRIS_RN_GPIO4 = 1;

        // initialize GPIO6 input
        TRIS_RN_GPIO6 = 1;

        // initialize GPIO5 output
        // enable open-drain config on GPIO5 and drive low
        // required since PICTail has 100k pull-up on GPIO5
        // as soon as the ALT GPIO function is enabled, this pin becomes input
        // and module will interpret as "connect to remote Host", unless you
        // drive it low.
        TRIS_RN_GPIO5 = 0;
        OD_RN_GPIO5 = 1;        // enable RF7 output as OD
        RN_GPIO5 = 0;           // OD driven so output is low
        
        // initialize wifly UART flow control
        PIC_RTS = 1;
        TRIS_PIC_RTS = 0;
        TRIS_PIC_CTS = 1;

        // initialize RN PICTAIL LEDY
        ADCFG_RN_PICTAIL_LEDY = 1;  // make the pin digital
        RN_PICTAIL_LEDY = 0;
        TRIS_RN_PICTAIL_LEDY = 0;
        

        // initialize RN PICTAIL LEDR
        ADCFG_RN_PICTAIL_LEDR = 1;  // make the pin digital
        RN_PICTAIL_LEDR = 0;
        TRIS_RN_PICTAIL_LEDR = 0;


        // initialize PIC UART settings
        // Turn the UART off
        U1MODEbits.UARTEN = 0;
	U1STAbits.UTXEN = 0;
	
	// Disable U1 Interrupts
	IFS0bits.U1TXIF = 0;	          // Clear the Transmit Interrupt Flag
	IEC0bits.U1TXIE = 0;	          // Disable Transmit Interrupts
	IFS0bits.U1RXIF = 0;	          // Clear the Receive Interrupt Flag
	IEC0bits.U1RXIE = 0;	          // Disable Receive Interrupts
	
	// Configure U1MODE SFR
	U1MODEbits.UARTEN = 0;	        // Bit15 TX, RX DISABLED, ENABLE at end of func
	//U1MODEbits.notimplemented;	  // Bit14
	U1MODEbits.USIDL = 0;	          // Bit13 Continue in Idle
	U1MODEbits.IREN = 0;	          // Bit12 No IR translation
	U1MODEbits.RTSMD = 0;	          // Bit11 RTS Flow Ctrl Mode
	//U1MODEbits.notimplemented;	  // Bit10
	U1MODEbits.UEN = 0;		          // Bits8,9 TX,RX enabled
	U1MODEbits.WAKE = 0;	          // Bit7 No Wake up (since we don't sleep here)
	U1MODEbits.LPBACK = 0;	        // Bit6 No Loop Back
	U1MODEbits.ABAUD = 0;	          // Bit5 No Autobaud (would require sending '55')
	U1MODEbits.RXINV = 0;	          // Bit4 IdleState = 1  (for dsPIC)
	//U1MODEbits.BRGH = 1;	          // Bit3 16 clocks per bit period
        U1MODEbits.BRGH = 0;	          // Bit3 16 clocks per bit period
	U1MODEbits.PDSEL = 0;	          // Bits1,2 8bit, No Parity
	U1MODEbits.STSEL = 0;	          // Bit0 One Stop Bit
	
	// Load a value into the Baud Rate generator
	//U1BRG 	= ((Fcyc/BAUDRATE)/4)-1; // (BRGH=1 setting)
	U1BRG 	= ((Fcyc/BAUDRATE)/16)-1; // (BRGH=1 setting)
	// Configure U1STA SFR
	U1STAbits.UTXISEL1 = 0;	        // Bit15 Int when Char is transferred (1/2 config!)
	U1STAbits.UTXINV = 0;	          // Bit14 N/A, IRDA config
	U1STAbits.UTXISEL0 = 0;	        // Bit13 Other half of Bit15
	//U1STAbits.notimplemented = 0;	// Bit12
	U1STAbits.UTXBRK = 0;	          // Bit11 Disabled
	U1STAbits.UTXEN = 0;	          // Bit10 TX pins controlled by periph
	U1STAbits.UTXBF = 0;	          // Bit9 *Read Only Bit*
	U1STAbits.TRMT = 0;	            // Bit8 *Read Only bit*
	U1STAbits.URXISEL = 0;	        // Bits6,7 Int. on character recieved
	U1STAbits.ADDEN = 0;	          // Bit5 Address Detect Disabled
	U1STAbits.RIDLE = 0;	          // Bit4 *Read Only Bit*
	U1STAbits.PERR = 0;		          // Bit3 *Read Only Bit*
	U1STAbits.FERR = 0;		          // Bit2 *Read Only Bit*
	U1STAbits.OERR = 0;		          // Bit1 *Read Only Bit*
	U1STAbits.URXDA = 0;	          // Bit0 *Read Only Bit*
	
	// Set U1RX Interrupt Priority
        // ...default setting used (4)
        // Configure U1 Interrupt Flags (U1 RX is interrupt-driven)
        // IFS0bits.U1TXIF = 0;	          // Clear the Transmit Interrupt Flag
        // IEC0bits.U1TXIE = 0;	          // Disable Transmit Interrupts
        // IFS0bits.U1RXIF = 0;	          // Clear the Receive Interrupt Flag
        // IEC0bits.U1RXIE = 1;	          // Enable Receive Interrupts

        // ...And turn the UART on
        U1MODEbits.UARTEN = 1;
	U1STAbits.UTXEN = 1;

        //while(PIC_CTS);
        // dummy read of NULL character Rx'd from wifly on reset
        //getWiFly();
	
} // initWiFly

// send a character to the UART1 serial port
void putWiFly(char c)
{
    while(PIC_CTS); // wait for !CTS, clear to send
    while (U1STAbits.UTXBF);   // wait while Tx buffer full
    U1TXREG = c;
} // putWiFly


// wait for a new character to arrive to the UART1 serial port
char getWiFly(void)
{
    int8_t rxChar;

    PIC_RTS = 0;    // let WiFly know we're ready
    while (!U1STAbits.URXDA);	// wait for a new character to arrive
    PIC_RTS = 1;    // hold off the module

    rxChar = U1RXREG;
    return(rxChar);

    // return U1RXREG; // read the character from the receive buffer
}// getWiFly


// send a null terminated string to the UART1 serial port
void putsWiFly(char *s)
{
    // loop until *s == '\0' the  end of the string
    while(*s)
    {
        putWiFly(*s++);	// send the character and point to the next one
    }
//    putWiFly('\r');       // terminate with a cr / line feed
} // putsWiFly


char *getsnWiFly(char *s, uint16_t len)
{
    char *p = s;            // copy the buffer pointer 
    do{
        *s = getWiFly();       // wait for a new character
        /* derrick DEBUG */
        putConsole(*s);
        //putWiFly(*s);         // echo character
        if ((*s==BACKSPACE)&&(s>p))
        {
            putWiFly(' ');     // overwrite the last character
            putWiFly( BACKSPACE);
            len++;
            s--;            // back the pointer
            continue;
        }
        if (*s=='\n')      // line feed, ignore it
            continue;
        if (*s=='\r')      // end of line, end loop
            break;          
        s++;                // increment buffer pointer
        len--;
    } while (len>1 );      // until buffer full
 
    *s = '\0';              // null terminate the string 
    
    return p;               // return buffer pointer
} // getsnWiFly

char *getsWiFly(char *s, uint16_t len)
{
    char *p = s;            // copy the buffer pointer
    do{
        *s = getWiFly();       // wait for a new character
        //putWiFly(*s);         // echo character

        if ((*s==BACKSPACE)&&(s>p))
        {
            putWiFly(' ');     // overwrite the last character
            putWiFly( BACKSPACE);
            len++;
            s--;            // back the pointer
            continue;
        }
        s++;                // increment buffer pointer
        len--;
    } while (len>1 );      // until buffer full

    *s = '\0';              // null terminate the string

    return p;               // return buffer pointer
} // getsnWiFly




    
