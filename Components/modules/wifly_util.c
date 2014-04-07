/*******************************************************************************
  The wifly module-level command/response API functions

  Company:
    Microchip Technology Inc.

  File Name:
    wifly_util.h

  Summary:
     API functions for the wifly module

  Description:
      The API functions that are used to send commands and receive
      responses from the Wifly module.
******************************************************************************/

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
// *****************************************************************************

// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include <xc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "delay_ms.h"
#include "wifly_drv.h"
#include "wifly_util.h"
#include "console.h"


extern  uint8_t  xmlLines[256];
unsigned char c = 0;

int8_t      ssidOfAP[20]         = {'\0'};
int8_t      moduleMacAddress[25] = {'\0'};
int8_t      macFormData[150]     = {'\0'};
int8_t      ssidFormData[150]    = {'\0'};


void buildXMLFile(uint8_t (*accessPoints)[25], int number);

void wifly_util_hardware_reset(void)
{
    RN_RESET = 0;
    Delayms(10);
    RN_RESET = 1;
    Delayms(10);

    // wait until the module is finished self-initialization
    sprintf(OutString,"waiting for CTS to go low...\r\n\r\n");
    putsConsole(OutString);
    while(PIC_CTS);

    // dummy read of NULL character Rx'd from wifly on reset
    sprintf(OutString,"waiting for data from wifly...\r\n\r\n");
    putsConsole(OutString);
    getWiFly();
}

bool wifly_util_enter_command_mode(void)
{
    char *temp_ptr;
    int j;

    // must insert 250ms before and after the command key entry sequence
    Delayms(350);

    // send the key sequence "$$$"
    while(PIC_CTS); // wait for !CTS, clear to send
    while (U1STAbits.UTXBF);   // wait while Tx buffer full
    U1TXREG = '$';
    while(PIC_CTS); // wait for !CTS, clear to send
    while (U1STAbits.UTXBF);   // wait while Tx buffer full
    U1TXREG = '$';
    while(PIC_CTS); // wait for !CTS, clear to send
    while (U1STAbits.UTXBF);   // wait while Tx buffer full
    U1TXREG = '$';

    // echo to the console
     putsConsole("\r\n$$$");

    // must insert 250ms before and after the command key entry sequence
    Delayms(350);

    // initialize the receiving buffer
    memset(WiflyInBuffer, '\0', sizeof(WiflyInBuffer));

    // get the response string "CMD\r\n"
    getsnWiFly(WiflyInBuffer, (uint16_t )sizeof(WiflyInBuffer));

    // ignore any leading null ('\0') characters
    // sometimes the module emits an initial NULL ascii char on warm restart
    // i.e. WiflyInBuffer[] = "\0CMD\0\0\0\0\0..."

    temp_ptr = WiflyInBuffer;

    for(j=0;j<=sizeof(WiflyInBuffer);j++)
    {
        if(*temp_ptr == '\0')
        {
            temp_ptr++;
        }
        else
        {
            break;
        }
    }

    // search for instance of "CMD" within the Rx buffer, starting at the first
    // non-null character
    if(strstr(&WiflyInBuffer[j], "CMD"))
    {
        // echo reply to console
        putsConsole(WiflyInBuffer);
        putConsole('\r');
        putConsole('\n');
        // return true if string match
        return true;
    }
    else
    {
        // return false if string does not match
        return false;
    }

}

// send command string - collect/evaluate wifly reply string from UART
bool wifly_util_send_command(
        const char *command_type,
        const char *command_category,
        const char *command_option,
        const char *command_value,
        const char *command_response)
{

    int index;

    // construct and send the full output command string
    if(command_type != (const char*)NULL)
    {
        putsWiFly((char *)command_type);
        putsConsole((char *)command_type);
        
    }
    if(command_category != (const char*)NULL)
    {
        putWiFly(' ');
        putsWiFly((char *)command_category);
        putConsole(' ');
        putsConsole((char *)command_category);
    }
    if(command_option != (const char*)NULL)
    {
        putWiFly(' ');
        putsWiFly((char *)command_option);
        putConsole(' ');
        putsConsole((char *)command_option);
    }
    if(command_value != (const char*)NULL)
    {
        putWiFly(' ');
        putsWiFly((char *)command_value);
        putConsole(' ');
        putsConsole((char *)command_value);
    }
        
    // terminate command string with '\r'
    putWiFly('\r');
    // add '\r' to console output for readability
    putConsole('\r');
    
    // evaluate the response for success/failure
    if(command_response != (const char *)NULL)
    {
        // initialize the receiving buffer
        memset(WiflyInBuffer, '\0', sizeof(WiflyInBuffer));
        index = 0;
        // get the response string - up to the end of the command prompt
        while((c = getWiFly()) != '>')         
        {
            WiflyInBuffer[index++] = c;
            putConsole(c);
        }
        putsConsole("> ");
        
        // search for instance of "command_response" within the Rx buffer
        if(strstr(WiflyInBuffer, command_response))
        {
            // match!
            // return success
            return true;
        }
        else
        {
            // return fail if string does not match
            return false;
        }
    }
    // if no reply is expected, return true
    else
    {
        return true;
    }
}

// exit command mode  - send exit string and verify EXIT reply
void wifly_util_exit_command_mode(void)
{
    putsWiFly("exit");
    putWiFly('\r');
    putsConsole("exit\r");
        
    // read back dummy '\r' and '\n' from the module
    c = getWiFly();
    putConsole(c);
    c = getWiFly();
    putConsole(c);
    
    // 'E'
    c = getWiFly();
    putConsole(c);
    // 'X'
    c = getWiFly();
    putConsole(c);
    // 'I'
    c = getWiFly();
    putConsole(c);
    // 'T'
    c = getWiFly();
    putConsole(c);
    // '\r'
    c = getWiFly();
    putConsole(c);
    // '\n'
    c = getWiFly();
    putConsole(c);

}

void flushWiflyRxUart(int8_t numTenMillisecond)
{
    int8_t index, temp;

    for(index = 0; index < numTenMillisecond; index++)
    {
        if(U1STAbits.URXDA)
        {
            temp = U1RXREG;
        }
        Delayms(10);
    }
}

// program all default settings required for the application
void wifly_util_config_module_all(void)
{
    // Initialize all WLAN and IP application-specific wifly configurations
    // Assumes module is in factory reset state (v2.45 fw)
    // Paramters initialized in this function:
    //
    // leave
    // set comm close 0
    // set comm open 0
    // set comm remote 0
    // set sys print 0
    // set sys iofunc 0x70
    // set wlan join 1
    // set ip remote 80
    // set dns name mtt.mchpcloud.com
    // set uart flow 1
    // set uart mode 0
    //
    // SSID, Passphrase and Channel are left at factory defaults
    // On first boot, application will fall into EZConfig and User will set this up
    // Subsequent reboots will thus work.

    sprintf(OutString,"initializing WiFly parameters...\r\n\r\n");
    putsConsole(OutString);

    // reset the module
    wifly_util_hardware_reset();

    // enter command mode
    if(!wifly_util_enter_command_mode())
    {
        sprintf(OutString,"\r\ncould not enter command mode...halting\r\n");
        putsConsole(OutString);
        while(1);
    }

    // leave
    if(!wifly_util_send_command(ACTION_LEAVE, NULL, NULL, NULL, STD_RESPONSE))
    {
        sprintf(OutString,"\r\nleave...failed\r\n");
        putsConsole(OutString);
        while(1);
    }

    // set comm close 0
    if(!wifly_util_send_command(SET, COMM, CLOSE, CLOSE_VALUE, STD_RESPONSE))
    {
        sprintf(OutString,"\r\nset comm close 0...failed\r\n");
        putsConsole(OutString);
        while(1);
    }

    // set comm open 0
    if(!wifly_util_send_command(SET, COMM, OPEN, OPEN_VALUE, STD_RESPONSE))
    {
        sprintf(OutString,"\r\nset comm open 0...failed\r\n");
        putsConsole(OutString);
        while(1);
    }

    // set comm remote 0
    if(!wifly_util_send_command(SET, COMM, REMOTE, COMM_REMOTE_VALUE, STD_RESPONSE))
    {
        sprintf(OutString,"\r\nset comm remote 0...failed\r\n");
        putsConsole(OutString);
        while(1);
    }

    // set sys print 0
    if(!wifly_util_send_command(SET, SYS, PRINT, PRINT_VALUE, STD_RESPONSE))
    {
        sprintf(OutString,"\r\nset sys print 0...failed\r\n");
        putsConsole(OutString);
        while(1);
    }

    // set sys iofunc 0x70
    if(!wifly_util_send_command(SET, SYS, IOFUNC, IOFUNC_VALUE, STD_RESPONSE))
    {
        sprintf(OutString,"\r\nset sys iofunc 0x70...failed\r\n");
        putsConsole(OutString);
        while(1);
    }

    // set wlan join 1
    if(!wifly_util_send_command(SET, WLAN, JOIN, JOIN_VALUE, STD_RESPONSE))
    {
        sprintf(OutString,"\r\nset wlan join 1...failed\r\n");
        putsConsole(OutString);
        while(1);
    }

    // set ip remote 80
    if(!wifly_util_send_command(SET, IP, REMOTE, IP_REMOTE_VALUE, STD_RESPONSE))
    {
        sprintf(OutString,"\r\nset ip remote 80...failed\r\n");
        putsConsole(OutString);
        while(1);
    }

    // set dns name mtt.mchpcloud.com
    if(!wifly_util_send_command(SET, DNS, NAME, NAME_VALUE, STD_RESPONSE))
    {
        sprintf(OutString,"\r\nset dns name mtt.mchpcloud.com...failed\r\n");
        putsConsole(OutString);
        while(1);
    }

    // set uart flow 1
    if(!wifly_util_send_command(SET, UART, FLOW, FLOW_VALUE, STD_RESPONSE))
    {
        sprintf(OutString,"\r\nset uart flow 1...failed\r\n");
        putsConsole(OutString);
        while(1);
    }

    // set uart mode 1
    if(!wifly_util_send_command(SET, UART, MODE, MODE_VALUE, STD_RESPONSE))
    {
        sprintf(OutString,"\r\nset uart mode 1...failed\r\n");
        putsConsole(OutString);
        while(1);
    }

    // save the settings
    if(!wifly_util_send_command(FILEIO_SAVE, NULL, NULL, NULL, STD_RESPONSE))
    {
        sprintf(OutString,"\r\nsave...failed\r\n");
        putsConsole(OutString);
        while(1);
    }

    /* append the last 2 bytes of the macaddress to the ssid name string */
    wifly_util_send_command(GET, MAC, NULL, NULL, STD_RESPONSE);

    strncat((char *)moduleMacAddress, (const char *)&WiflyInBuffer[12], 17);
    sprintf((char *)macFormData, "<td class=\"form\"><input name=\"MAC\" type=\"text\" value=\"%s\" disabled /></td>\r\n", moduleMacAddress);
    putsConsole((char *)macFormData);

    /* replace : with _ in macaddress string */
    WiflyInBuffer[23] = '_';
    WiflyInBuffer[26] = '_';

    /* will resolve to mttSofAP_xx_yy for the ssid of the AP to be create later */
    strcat((char *)ssidOfAP, (const char *)SAP_NETWORK_SSID);
    strncat((char *)ssidOfAP, (const char *)&WiflyInBuffer[23], 6);
    sprintf((char *)ssidFormData, "<td class=\"form\"><input name=\"ssid3\" type=\"text\" value=\"%s\" disabled /></td>\r\n", ssidOfAP);
    putsConsole((char *)ssidFormData);


    // (hard-)reset the module to apply the new settings
    sprintf(OutString,"\r\n\r\nResetting WiFly...\r\n");
    putsConsole(OutString);
    wifly_util_hardware_reset();

    // delay after module reset/reboot to allow GPIO4 toggle low-high-low
    Delayms(250);

} // wifly_util_config_module_all()

void wifly_util_active_scan(void)
{
    int16_t    index, idx;
    uint8_t    accessPointFound[20];
    uint8_t    duplicate = 0;
    int        numAccessPointsFound = 0;
    char    s1[25];

    int8_t  i;
    uint8_t c;

    sprintf(OutString,"received scan request...\r\n\r\n");
    putsConsole(OutString);

    // enter command mode
    if(!wifly_util_enter_command_mode())
    {
        sprintf(OutString,"\r\ncould not enter command mode...halting\r\n");
        putsConsole(OutString);
        while(1);
    }

    sprintf(OutString,"scanning the channels...\r\n\r\n");
    if(!wifly_util_send_command("scan", "500", NULL, NULL, STD_RESPONSE))
    {
        sprintf(OutString,"\r\nscan attempt...failed\r\n");
        putsConsole(OutString);
        while(1);
    }

    
    /* pick off the leading blank lines that are transmitted */
    do {
        c = getWiFly();
        putConsole(c);
    }while (c != 'd');

    // pick off the "SCAN:Found xx" received text pattern
    memset(WiflyInBuffer,    '\0', sizeof(WiflyInBuffer));
    memset(accessPointFound, '\0', sizeof(accessPointFound));
    memset(OutString, '\0', sizeof(OutString) );
    index = 0;

    /* get the space before the number */
    c = getWiFly();

    c = getWiFly();
    putConsole(c);
    WiflyInBuffer[index++] = c;

    c = getWiFly();
    putConsole(c);
    WiflyInBuffer[index] = c;

    numAccessPointsFound = atoi((const char *)WiflyInBuffer);
    sprintf(OutString,"\r\nAP:: %d\r\n", numAccessPointsFound);
    putsConsole(OutString);

    do
    {
        c = getWiFly();
        putConsole(c);
    }while (c != '\n');


    /* prepare for the next line */
    /* pick of the pattern:    05,01,-75,06,3104,14,00,00:0b:86:e0:b8:a0,mchp-peap   */
    memset(WiflyInBuffer,    '\0', sizeof(WiflyInBuffer));
    memset(accessPointFound, '\0', sizeof(accessPointFound));
    index   = 0;
    idx     = 0;

    // get the response string - up to the end of the command prompt
    sprintf(OutString,"collecting the access popints found...\r\n\r\n");
    for(i = 0; i < numAccessPointsFound; i++)
    {
        while((c = getWiFly()) != '\n')
        {
            WiflyInBuffer[index++] = c;
        }

        /* 9 parameters are output for each AP discovered - see users guide */
        if( (sscanf(WiflyInBuffer, "%s,%s,%s,%s,%s,%s,%s,%s,%s", s1,s1,s1,s1,s1,s1,s1,s1,accessPointFound)) == 9)
        {
            sprintf(OutString,"%s\r\n", accessPointFound);
            putsConsole(OutString);

            /* Copy the first found ssid into the arrary */
            if(idx == 0)
            {
                strcpy((char *)&accessPoints[idx%15][0], (const char *)accessPointFound);
                idx++;
            }
            /* get rid of duplicates that are found, max of 15 entries however */
            else
            {
                for(index = 0; index < idx; index++)
                {
                    if( !(strcmp((const char *)&accessPoints[index][0], (const char *)accessPointFound)) )
                    {
                        duplicate = 1;
                        break;     /* get out of this inner for-loop */
                    }
                }
                if(!duplicate)
                {
                    strcpy((char *)&accessPoints[idx%15][0], (const char *)accessPointFound);
                    idx++;
                }

            }
        }
        memset(WiflyInBuffer,    '\0', sizeof(WiflyInBuffer));
        memset(accessPointFound, '\0', sizeof(accessPointFound));
        duplicate   = 0;
        index       = 0;
    }

    /* go until END: is received */
    do
    {
        c = getWiFly();
        putConsole(c);
    }while (c != ':');

    // terminate command string with '\r'
    putWiFly('\r');
    putWiFly('\n');

    /* go until <2.45>: is received */
    do
    {
        c = getWiFly();
        putConsole(c);
    }while (c != '>');
    

    /* limit the number of entries to 10 */
    if(idx > 15)
    {
        idx = 15;
    }

    buildXMLFile(accessPoints, idx);
    /* exit command mode*/
    wifly_util_exit_command_mode();


} // wifly_util_active_scan()

void buildXMLFile(uint8_t (*accPoints)[25], int numAccPoints)
{
    int  index;
    char *linePtr;
    
    linePtr =  (char *)xmlLines;

    memset(xmlLines, '\0', sizeof(xmlLines));

    strcpy((char *)linePtr, (char *)"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
    strcat((char *)linePtr, (char *)"<scanlist>\n");
    

    for(index = 0; index < numAccPoints; index++)
    {
        strcat((char *)linePtr, (char *)"<bss>\n");
            strcat((char *)linePtr, (char *)"<ssid>");
            strcat((char *)linePtr, (char *)accPoints);
            strcat((char *)linePtr, (char *)"</ssid>\n");
        strcat((char *)linePtr,(char *)"</bss>\n");

        accPoints++;
    }

    strcat((char *)linePtr, (char *)"</scanlist>");
}

