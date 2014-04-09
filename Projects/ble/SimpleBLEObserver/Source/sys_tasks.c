/*******************************************************************************
 The Wifly configuration and http client application tasks

  Company:
    Microchip Technology Inc.

  File Name:
    sys_tasks.c

  Summary:
      The function that make up the main application tasks are defined in this file.

  Description:
     The functions that make up the main http client application are defined in this file.
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

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include <xc.h>
#include <sys_config.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "sys_tasks.h"


// *****************************************************************************
// *****************************************************************************
// Section: File Scope or Global Constants
// *****************************************************************************
// *****************************************************************************
char  OutString[128];               // console/lcd display output buffer
char  WiflyInBuffer[512];           // input buffer used to receive data from WiFly
int   WiflyInBufferIndex;           // index to wifly input buffer array
char  WiflyOutBuffer[128];          // output buffer used to send data to WiFly

uint8_t  accessPoints[20][25];         // APs found during scanning
uint8_t  xmlLines[1024];



TICK StatusLedDelay;  // Status LED time delay variable



TICK    AssociateWaitDelay;            // wait 10secs in assciate state before moving onto Config

TICK AssociateLedToggleDelay;       // used to set Associate LED (LEDY) blink delays
TICK TCPConnectLedToggleDelay;      // used to set TCP Connect LED (LEDR) blink delays
TICK configureLedToggleDelay;       // the configure the module LED blink delay
TICK HTTPGetReplyDelay;             // used to set max delay for receiving the HTTP reply

NODE_INFO myNodeInfo;               // contains MAC address and IP address

int numHTTPGetAttempts;             // track number of HTTP GET attempts for which there was no reply
                                    // after some number, close the socket and restart the TCP state machine
int j;

unsigned char *LedValue;            // used to store start location of "ledset="
                                    // in the received response
unsigned char temp[32];
volatile unsigned int ToggleLed;    // used to store the current toggle LED value

unsigned int HTTPReplyLineCnt;      // used to indicate the number of HTTP response lines received
unsigned char HTTPReplyVersion[32]; // used to hold the version information in HTTP reply status line
unsigned char HTTPReplyStatusCode[32];    // used to hold the HTTP response status code
unsigned char HTTPReplyStatusPhrase[32]; // used to hold the HTTP response status phrase


void sendPage(unsigned char *s);
void sendPageXML(unsigned char *s);
void sendPageChunks(unsigned char *fmt, ...);

/* The Webserver Definitions  - Derrick */
unsigned char   simpleBuf[512];
unsigned char   fileName[64];

int             sbidx;
volatile int    parms;
int             httpLine;

unsigned char   tinyBuf[32];
int             contentLen = 0;

int fillingPostData = 0;
extern unsigned char fifo[FIFO_MAX_ENTRIES];
extern int8_t   ssidOfAP[20];
extern int8_t   macFormData[150];
extern int8_t   ssidFormData[150];
extern int8_t   moduleMacAddress[25];


unsigned char httpHeaders[] = "HTTP/1.1 200 OK\r\n"
"Connection: keep-alive\r\n"
"Content-Type: text/html\r\n"
"Content-Length: ";

unsigned char xmlHeaders[] = "HTTP/1.1 200 OK\r\n"
"Connection: keep-alive\r\n"
"Content-Type: text/xml\r\n"
"Content-Length: ";

unsigned char page2[] =
"<html>\r\n"
"<body>\r\n"
"Access Point has been reconfigured"
"</body>\r\n"
"</html>";


const unsigned char page4[] =
"<!DOCTYPE html>\r\n"
"<html lang=\"en\"> \r\n"
"<head> \r\n"
"<title>Microchip EZConfig Application</title>\r\n"
"<script>\r\n"
"var xmlhttp;\r\n"
"function loadXMLDoc(url, cfunc, data)\r\n"
"{\r\n"
    "if (window.XMLHttpRequest)\r\n"
    "{\r\n"
        "xmlhttp = new XMLHttpRequest();\r\n"
        "xmlhttp.open((data==null)?\"GET\":\"POST\", url, true);\r\n"
        "xmlhttp.send(data);\r\n"
    "}\r\n"
    "else\r\n"
    "{\r\n"
        "xmlhttp = new ActiveXObject(\"Microsoft.XMLHTTP\");\r\n"
        "xmlhttp.open((data==null)?\"GET\":\"POST\", url, true);\r\n"
        "xmlhttp.send(data);\r\n"
    "}\r\n"
    "xmlhttp.onreadystatechange = cfunc;\r\n"
"}\r\n"
"function getXMLValue(xmlData, field, child) \r\n"
"{\r\n"
	"child = typeof(child) == \"undefined\" ? 0 : child;\r\n"

	"try {\r\n"
		"if(xmlData.getElementsByTagName(field)[child].firstChild.nodeValue)\r\n"
			"return xmlData.getElementsByTagName(field)[child].firstChild.nodeValue;\r\n"
		"else\r\n"
			"return null;\r\n"
	"} catch(err) { return null; }\r\n"
"}\r\n"
"function switchNetwork(id)\r\n"
"{\r\n"
	"var name = id;\r\n"
	"if (name == \"\")\r\n"
	"{\r\n"
		"alert('SSID cannot be left blank!');\r\n"
		"return;\r\n"
	"}\r\n"
	"document.getElementById(\"SSID2\").value = name;\r\n"
"}\r\n"

"function deleteWiFiScanTable()\r\n"
"{\r\n"
    "var table = document.getElementById('scanTable').getElementsByTagName('tbody')[0];\r\n"
    "var rows = table.rows;\r\n"

    "while (rows.length - 1)\r\n"
        "table.deleteRow(rows.length - 1);\r\n"
"}\r\n"
"function addScanRow(ssid)\r\n"
"{\r\n"
	"var tbody = document.getElementById('scanTable').getElementsByTagName(\"tbody\")[0];\r\n"
	"var row = document.createElement(\"tr\");\r\n"

	"row.setAttribute('id', ssid);\r\n"
	"row.setAttribute('onmouseover', \"this.style.cursor='pointer'\");\r\n"
        "row.setAttribute('onclick', 'switchNetwork(id);');\r\n"
	"var data1 = document.createElement(\"td\");\r\n"
	"data1.setAttribute('style', 'width:10em');\r\n"
	"data1.appendChild(document.createTextNode(ssid));\r\n"

	"row.appendChild(data1);\r\n"
	"tbody.appendChild(row);\r\n"
"}\r\n"
"function scanWiFiNetwork()\r\n"
"{\r\n"
    "deleteWiFiScanTable();\r\n"
    "loadXMLDoc('scanresults.xml?scan=1',function()\r\n"
    "{\r\n"
        "if (xmlhttp.readyState==4 && xmlhttp.status==200)\r\n"
        "{\r\n"
            "var i,x;\r\n"
             "xmlData = xmlhttp.responseXML;\r\n"
             "x = xmlhttp.responseXML.documentElement.getElementsByTagName(\"bss\");\r\n"
             "txt=\"\";\r\n"
             "for(i=0; i < x.length; i++)\r\n"
             "{\r\n"
                     "addScanRow( getXMLValue(xmlData, 'ssid', i) );\r\n"
             "}\r\n"
        "}\r\n"
   "});\r\n"
"}\r\n"
"</script>\r\n"
"<style>\r\n"
"body { \r\n"
"background-color: #312e1d;\r\n"
"font-family: tahoma, sans-serif;\r\n"
"}\r\n"
"#content {\r\n"
"margin: 20px auto;\r\n"
"width: 600px;\r\n"
"background-color: #f8f8f8;\r\n"
"padding: 20px 10px;\r\n"
"border: solid 3px red;\r\n"
"}\r\n"

"h1 {\r\n"
"font-size: 120%;\r\n"
"}\r\n"
"input[type=text] {\r\n"
"width: 400px;\r\n"
"}\r\n"
"tr.label {\r\n"
"vertical-align: top;\r\n"
"}\r\n"
"#menu {\r\n"
	"float: left;\r\n"
	"width: 150px;\r\n"
	"padding-right: 5px;\r\n"
"}\r\n"
"#menu a {\r\n"
	"width: 145px;\r\n"
	"display: block;\r\n"
	"background: #c00;\r\n"
	"color: white;\r\n"
	"padding: 5px;\r\n"
	"font-weight: bold;\r\n"
	"border-bottom: 2px solid #fff;\r\n"
	"text-decoration: none;\r\n"
"}\r\n"
"#menu a:hover {\r\n"
	"background: #d33;\r\n"
"}\r\n"
"</style>\r\n"
"</head>\r\n"
"<body>\r\n"
"<div id=\"content\">\r\n"
"<h1 style=\"text-align:center\"> Device Configuration Form </h1>\r\n"
"<div id=\"menu\">\r\n"
    "<a href=\"/index.htm\">Configure</a>\r\n"
    "<a href=\"/advance.htm\">Advanced Features</a>\r\n"
"</div>\r\n"
"<form method=\"post\" action=\"http://5.16.71.1:2000\">\r\n"
"<table>\r\n"
"<tr>\r\n"
"<td height=\"20\"></td>\r\n"
"</tr>\r\n"
"<tr>\r\n"
"<td class=\"label\">SSID</td>\r\n"
"<td class=\"form\"><input name=\"SSID\" type=\"text\" id=\"SSID2\" /></td>\r\n"
"</tr>\r\n"
"<tr>\r\n"
"<td class=\"label\">Network Password</td>\r\n"
"<td class=\"form\"><input name=\"password\" type=\"text\" /></td>\r\n"
"</tr>\r\n"
"<tr>\r\n"
"<td class=\"label\">Channel</td>\r\n"
"<td class=\"form\">\r\n"
"<select name=\"Channel\">\r\n"
"<option value=\"\">-- Select channel --</option>\r\n"
"<option value=\"0\">Search On All Channels</option>\r\n"
"<option value=\"1\">Channel 1</option>\r\n"
"<option value=\"2\">Channel 2</option>\r\n"
"<option value=\"3\">Channel 3</option>\r\n"
"<option value=\"4\">Channel 4</option>\r\n"
"<option value=\"5\">Channel 5</option>\r\n"
"<option value=\"6\">Channel 6</option>\r\n"
"<option value=\"7\">Channel 7</option>\r\n"
"<option value=\"8\">Channel 8</option>\r\n"
"<option value=\"9\">Channel 9</option>\r\n"
"<option value=\"10\">Channel 10</option>\r\n"
"<option value=\"11\">Channel 11</option>\r\n"
"</select>\r\n"
"</td>"
"</tr>"
"<tr>\r\n"
"<td class=\"label\">Auto-Join</td>\r\n"
"<td class=\"form\">\r\n"
"Enable <input type=\"radio\" name=\"AutoJoin\" value=\"Yes\"  /> &nbsp;\r\n"
"Disable <input type=\"radio\" name=\"AutoJoin\" value=\"No\" checked/> &nbsp;\r\n"
"</td>\r\n"
"</tr>\r\n"
"<tr>\r\n"
"<td height=\"20\"></td>\r\n"
"</tr>\r\n"
"<tr>\r\n"
"<td></td>\r\n"
"<td><input type=\"submit\" name=\"submit\" value=\" Enter \" /></td>\r\n"
"</tr>\r\n"
"<tr>\r\n"
"<td height=\"40\"></td>\r\n"
"</tr>\r\n"
"</table>"
        "<table id=\"scanTable\">\r\n"
    	"<tbody>\r\n"
    		"<tr>\r\n"
    			"<td colspan=\"4\">\r\n"
    				"<input id=\"rescan\" type=\"button\" style=\"width:18em\" \r\n"
    				  "value=\"View Available Wireless Networks...\" onclick=\"scanWiFiNetwork();\"></input>\r\n"
    			"</td>\r\n"
    		"</tr>\r\n"
    	"</tbody>\r\n"
        "</table>\r\n"
"</form>\r\n"
"</div>\r\n"
"</body>\r\n"
"</html>";


const unsigned char advanced1[] =
"<!DOCTYPE html>\r\n"
"<html lang=\"en\"> \r\n"
"<head> \r\n"
"<title>Microchip EZConfig Application</title>\r\n"

"<style>\r\n"
"body { \r\n"
"background-color: #312e1d;\r\n"
"font-family: tahoma, sans-serif;\r\n"
"}\r\n"
"#content {\r\n"
"margin: 20px auto;\r\n"
"width: 600px;\r\n"
"background-color: #f8f8f8;\r\n"
"padding: 20px 10px;\r\n"
"border: solid 3px red;\r\n"
"}\r\n"

"h1 {\r\n"
"font-size: 120%;\r\n"
"}\r\n"
"input[type=text] {\r\n"
"width: 400px;\r\n"
"}\r\n"
"tr.label {\r\n"
"vertical-align: top;\r\n"
"}\r\n"
"#menu {\r\n"
	"float: left;\r\n"
	"width: 150px;\r\n"
	"padding-right: 5px;\r\n"
"}\r\n"
"#menu a {\r\n"
	"width: 145px;\r\n"
	"display: block;\r\n"
	"background: #c00;\r\n"
	"color: white;\r\n"
	"padding: 5px;\r\n"
	"font-weight: bold;\r\n"
	"border-bottom: 2px solid #fff;\r\n"
	"text-decoration: none;\r\n"
"}\r\n"
"#menu a:hover {\r\n"
	"background: #d33;\r\n"
"}\r\n"
"</style>\r\n"
"</head>\r\n"
"<body>\r\n"
"<div id=\"content\">\r\n"
"<h1 style=\"text-align:center\"> Module Advanced Features </h1>\r\n"
"<div id=\"menu\">\r\n"
    "<a href=\"/index.htm\">Configure</a>\r\n"
    "<a href=\"/advance.htm\">Advanced Features</a>\r\n"
"</div>\r\n"
"<form>\r\n"
    "<table>\r\n"
        "<tr>\r\n"
        "<td height=\"20\"></td>\r\n"
        "</tr>\r\n"
        "<tr>\r\n"
            "<td class=\"label\">Mac Address:</td>\r\n"
            "<td class=\"form\"><input name=\"MAC\" type=\"text\" value=\"00:06:66:81:06:03\" disabled /></td>\r\n"
        "</tr>\r\n"

        "<tr>\r\n"
            "<td class=\"label\">Network SSID:</td>\r\n"
            "<td class=\"form\"><input name=\"ssid3\" type=\"text\" value=\"mttSoftAP_03_06\" disabled /></td>\r\n"
        "</tr>\r\n"

        "<tr>\r\n"
            "<td class=\"label\">Firmware Version:</td>\r\n"
            "<td class=\"form\"><input name=\"frmversion\" type=\"text\" value=\"2.45\" disabled /></td>\r\n"
        "</tr>\r\n"

        "<tr>\r\n"
            "<td height=\"20\"></td>\r\n"
        "</tr>\r\n"

    "</table>\r\n"
  "</form>\r\n"
"</div>\r\n"
"</body>\r\n"
"</html>";


const unsigned char advanced_part1[] =
"<!DOCTYPE html>\r\n"
"<html lang=\"en\"> \r\n"
"<head> \r\n"
"<title>Microchip EZConfig Application</title>\r\n"

"<style>\r\n"
"body { \r\n"
"background-color: #312e1d;\r\n"
"font-family: tahoma, sans-serif;\r\n"
"}\r\n"
"#content {\r\n"
"margin: 20px auto;\r\n"
"width: 600px;\r\n"
"background-color: #f8f8f8;\r\n"
"padding: 20px 10px;\r\n"
"border: solid 3px red;\r\n"
"}\r\n"

"h1 {\r\n"
"font-size: 120%;\r\n"
"}\r\n"
"input[type=text] {\r\n"
"width: 400px;\r\n"
"}\r\n"
"tr.label {\r\n"
"vertical-align: top;\r\n"
"}\r\n"
"#menu {\r\n"
	"float: left;\r\n"
	"width: 150px;\r\n"
	"padding-right: 5px;\r\n"
"}\r\n"
"#menu a {\r\n"
	"width: 145px;\r\n"
	"display: block;\r\n"
	"background: #c00;\r\n"
	"color: white;\r\n"
	"padding: 5px;\r\n"
	"font-weight: bold;\r\n"
	"border-bottom: 2px solid #fff;\r\n"
	"text-decoration: none;\r\n"
"}\r\n"
"#menu a:hover {\r\n"
	"background: #d33;\r\n"
"}\r\n"
"</style>\r\n"
"</head>\r\n"
"<body>\r\n"
"<div id=\"content\">\r\n"
"<h1 style=\"text-align:center\"> Module Advanced Features </h1>\r\n"
"<div id=\"menu\">\r\n"
    "<a href=\"/index.htm\">Configure</a>\r\n"
    "<a href=\"/advance.htm\">Advanced Features</a>\r\n"
"</div>\r\n"
"<form>\r\n"
    "<table>\r\n"
        "<tr>\r\n"
        "<td height=\"20\"></td>\r\n"
        "</tr>\r\n"
        "<tr>\r\n"
            "<td class=\"label\">Mac Address:</td>\r\n";


const unsigned char advanced_part2[] =
        "</tr>\r\n"
        "<tr>\r\n"
        "<td class=\"label\">Network SSID:</td>\r\n";
         

const unsigned char advanced_part3[] =
        "</tr>\r\n"
        "<tr>\r\n"
            "<td class=\"label\">Firmware Version:</td>\r\n"
            "<td class=\"form\"><input name=\"frmversion\" type=\"text\" value=\"2.45\" disabled /></td>\r\n"
         "</tr>\r\n"
        "<tr>\r\n"
            "<td height=\"20\"></td>\r\n"
        "</tr>\r\n"
    "</table>\r\n"
  "</form>\r\n"
"</div>\r\n"
"</body>\r\n"
"</html>";


volatile int sendResponse=0;

char ssid[32]   = {'\0'};
char passwd[32] = {'\0'};
char chan[32]   = {'\0'};


volatile int lll;

enum smStates   smTCPTasks;
enum ledStates  smStatusLed;
enum HttpState  httpState;
enum HttpMethod httpMethod;


char  *ConnectionValue;

/*************End of WebServer Definitions****************/

/** LOCAL PROTOTYPES ********************************************************/

void StatusLedTasks(void);      // Toggle LED_D3 every second - indicates blocking code
void AppTasks(void);            // Application Tasks
void TCPTasks(void);            // TCPIP Tasks (WiFly interface)


void EZConfigTasks(void);      // HTTP Server to configure the module
void configWiFlyAsSoftAP(void);



/** sysTasks() ********************************************************************/

void sys_Tasks(void)
{
    /* the leds shows the status of the module, the TCPTask runs the http client */
    while(1)
    {
        StatusLedTasks();
        AppTasks();
        TCPTasks();
    }
    
} 



/******************************************************************************
 * Function:        void StatusLedTasks(void)
 *
 * PreCondition:    LedDelay initialized for 0.5 second delay
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This routing toggles LED_D3 every 0.5 second.
 *                  Can be used to identify blocking code in the while(1) loop
 *
 * Note:
 *
 *****************************************************************************/
void StatusLedTasks(void)
{
   switch(smStatusLed)
   {
       case LED_OFF:
           if((int32_t)(TickGet() - StatusLedDelay) > (int32_t)0)
           {
               LED_D3 = 1;  // turn on LED
               smStatusLed = LED_ON;  // update state machine
               StatusLedDelay = TickGet() + TICK_SECOND; // reset timer
           }
           break;

       case LED_ON:
           if((int32_t)(TickGet() - StatusLedDelay) > (int32_t)0)
           {
               LED_D3 = 0;  // turn off LED
               smStatusLed = LED_OFF;  // update state machine
               StatusLedDelay = TickGet() + TICK_SECOND; // reset timer
           }
    	   break;

       default:
           break;
    }

} // StatusLedTasks()

/******************************************************************************
 * Function:        void AppTasks(void)
 *
 * PreCondition:    sys_Initialize() executed
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This routine runs the application-specific state machine
 *
 * Note:
 *
 *****************************************************************************/
void AppTasks(void)
{
    /* Users can put their general purpose application code here */


} // AppTasks()

/******************************************************************************
 * Function:        void TCPTasks(void)
 *
 * PreCondition:    sys_Initialize() executed
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This routine runs the TCP communications state machine for
 *                  this application
 *
 *                  After associating with an internet-connected AP, the station
 *                  opens a TCP connection to a pre-defined IP:Port and
 *                  implements a HTTP-client protocol with a set of PHP scripts
 *                  running on the server machine (see TCP0501 for details).
 *
 *                  These scripts get/set the explorer16 LED/Switch/Pot datum
 *                  from a database stored on the server. Other clients (web browsers)
 *                  are also able to interact with this data view the state of the
 *                  LEDS/Switches/Pot, as well as toggle the explorer16's LEDs.
 *
 *
 * Note:
 *
 *****************************************************************************/
void TCPTasks(void)
{
    switch(smTCPTasks)
    {
        case SM_TCP_IDLE:

            // Idle state - nothing to do here yet, so go to initialize state
            smTCPTasks = SM_TCP_INIT;
            break;

        case SM_TCP_INIT:

            // (hard-)reset the module
            sprintf(OutString,"\r\n\r\nResetting WiFly...\r\n");
            putsConsole(OutString);
            wifly_util_hardware_reset();

            // delay after module reset/reboot to allow GPIO4 toggle low-high-low
            Delayms(250);

            // initialize default PIC wifly interface pin levels
            // pins initialized by "wifly_util_hardware_reset()":
            // - RN_RESET = 1; (module firmware is running)
            // - PIC_RTS = 1; (hold off the module)
            // returns only after the module is ready (PIC_CTS = 0)
            RN_GPIO9 = 0;       // set default state of Adhoc/Factory reset jumper input
            RN_GPIO5 = 0;       // disable remote TCP connection

            // Turn off the yellow "Associated" LED on the PICTail (PIC LEDY)
            RN_PICTAIL_LEDY = 0;
            // Turn off the green "TCP Connected Opened" LED on teh PICTAIL (PIC LEDR)
            RN_PICTAIL_LEDR = 0;

            // begin transition to next TCP state (AP Association)
            sprintf(OutString,"\r\n\r\n...associating with Access Point\r\n");
            putsConsole(OutString);

            // set up Associate LED (WiFly PICTAil LEDY) toggle delay
            // toggle every 100mS while waiting to associate
            AssociateLedToggleDelay = TickGet() + TICK_SECOND/10;

            // how long to wait for association before triggering EZConfig
            AssociateWaitDelay = TickGet() + TICK_SECOND*TIME_GIVEN_TO_ASSOC_IN_SECS;

            // Advance to next state
            smTCPTasks = SM_TCP_WAIT_ASSOCIATE;
            
            // Update LCD message
            clrLCD();
            putsLCD("Associating...");
            
            break;

        case SM_TCP_WAIT_ASSOCIATE:
            
            // check to see if it's time to toggle the associate LED
            if((int32_t)(TickGet() - AssociateLedToggleDelay) > (int32_t)0)
            {
                RN_PICTAIL_LEDY ^= 1;  // toggle the LED
                AssociateLedToggleDelay = TickGet() + TICK_SECOND/10; // reset timer
            }
            // check for GPIO4 line high - indicating successful association
            if(1 == RN_GPIO4)
            {
                sprintf(OutString,"\r\n\r\n...Associated!..getting IP Addr & MAC Addr\r\n\r\n");
                putsConsole(OutString);
                // Turn on the Associated LED
                RN_PICTAIL_LEDY = 1;

                // get the module's IP address and MAC address
                // enter command mode
                if(!wifly_util_enter_command_mode())
                {
                    sprintf(OutString,"\r\ncould not enter command mode...halting\r\n");
                    putsConsole(OutString);
                    while(1);
                }
                
                // send a "get ip address" command to retrieve the IP address
                // result is in WiflyInBuffer[]
                wifly_util_send_command(GET, IP, ADDRESS, NULL, STD_RESPONSE);

                // save IP address in our NODE_INFO structure
                sscanf(WiflyInBuffer, "%3d.%3d.%3d.%3d", &myNodeInfo.IPAddr.v[0],
                &myNodeInfo.IPAddr.v[1], &myNodeInfo.IPAddr.v[2],
                &myNodeInfo.IPAddr.v[3]);

                // clear the LCD display
                clrLCD();

                // write IP address to LCD line 2
                setLCDC(0x40);
                sprintf(OutString, "%d.%d.%d.%d",myNodeInfo.IPAddr.v[0],
                myNodeInfo.IPAddr.v[1], myNodeInfo.IPAddr.v[2],
                myNodeInfo.IPAddr.v[3]);
                putsLCD(OutString);

                // send a "get mac" command to retrieve the MAC address
                // result is in WiflyInBuffer[]
                wifly_util_send_command(GET, MAC, NULL, NULL, STD_RESPONSE);

                // the following scanf conversion *DOES NOT WORK* with MAC bytes having leading zeros
                // i.e. 00:06:66:81:06:23 saved as 00:66:66:81:66:23 (?)
                // save MAC address in our NODE_INFO structure
                //j = sscanf(WiflyInBuffer, "\r\r\nMac Addr=%2x:%2x:%2x:%2x:%2x:%2x", &myNodeInfo.MACAddr.v[0],
                //&myNodeInfo.MACAddr.v[1], &myNodeInfo.MACAddr.v[2],
                //&myNodeInfo.MACAddr.v[3], &myNodeInfo.MACAddr.v[4],
                //&myNodeInfo.MACAddr.v[5]);

                // "brute force" conversion/storage of MAC address return string
                // save each MAC byte as a string
                myNodeInfo.MACAddr.raw[0][0] = WiflyInBuffer[12];
                myNodeInfo.MACAddr.raw[0][1] = WiflyInBuffer[13];

                myNodeInfo.MACAddr.raw[1][0] = WiflyInBuffer[15];
                myNodeInfo.MACAddr.raw[1][1] = WiflyInBuffer[16];

                myNodeInfo.MACAddr.raw[2][0] = WiflyInBuffer[18];
                myNodeInfo.MACAddr.raw[2][1] = WiflyInBuffer[19];

                myNodeInfo.MACAddr.raw[3][0] = WiflyInBuffer[21];
                myNodeInfo.MACAddr.raw[3][1] = WiflyInBuffer[22];

                myNodeInfo.MACAddr.raw[4][0] = WiflyInBuffer[24];
                myNodeInfo.MACAddr.raw[4][1] = WiflyInBuffer[25];

                myNodeInfo.MACAddr.raw[5][0] = WiflyInBuffer[27];
                myNodeInfo.MACAddr.raw[5][1] = WiflyInBuffer[28];

                // now save each MAC byte as an int
                for(j=0;j<=5;j++)
                {
                    myNodeInfo.MACAddr.v[j] = (int)strtol(myNodeInfo.MACAddr.raw[j], (char **)NULL, 16);
                }
                
                // Display MAC Address to line 1 of LCD
                setLCDC(0x00);
                sprintf(OutString, "MAC:%s%s%s%s%s%s",(char *)&myNodeInfo.MACAddr.raw[0],
                (char *)&myNodeInfo.MACAddr.raw[1], (char *)&myNodeInfo.MACAddr.raw[2],
                (char *)&myNodeInfo.MACAddr.raw[3], (char *)&myNodeInfo.MACAddr.raw[4],
                (char *)&myNodeInfo.MACAddr.raw[5]);
                putsLCD(OutString);

                // exit command mode
                wifly_util_exit_command_mode();
                               
                // open TCP connection to server
                sprintf(OutString,"\r\n\r\n...connecting to Server...\r\n");
                putsConsole(OutString);

                // trigger wifly to create a TCP connection to the remote server
                // using stored settings in the module
                RN_GPIO5 = 1;

                // set up TCP Connect LED (WiFly PICTAil LEDR) toggle delay
                // toggle every 100mS while waiting for a TCP connection
                TCPConnectLedToggleDelay = TickGet() + TICK_SECOND/10;

                smTCPTasks = SM_TCP_WAIT_CONNECT;
            }
            
            /* THE EZCONFIG PROCESS STATES:
             * if module cannot associate after waiting for 10 seconds the
             * very first time module is powered up, then pass control over to
             * EZConfiguration process.
             * EZ config mode 
            */
            else
            {
                
              if((int32_t)(TickGet() - AssociateWaitDelay) > (int32_t)0)
              {
                  smTCPTasks = SM_TCP_PREPARE_SOFT_AP;
              }
              
            }
            break;

        /* set up the module in Soft AP mode, which is the first step towards
         * configuring the device via a browser
         */
        case SM_TCP_PREPARE_SOFT_AP:
        {
            // begin EZConfig process
            sprintf(OutString,"\r\n\r\n..3. Association Failed so moving to  EZConfig\r\n\r\n");
            putsConsole(OutString);
            // set status LEDs to 0 so both will toggle at the same time
            RN_PICTAIL_LEDR = 0;
            RN_PICTAIL_LEDY = 0;
            configWiFlyAsSoftAP();
            smTCPTasks = SM_TCP_PREPARE_EZCONFIG_RESOURCES;
        }
        break;

        case SM_TCP_PREPARE_EZCONFIG_RESOURCES:
        {
            // begin EZConfig process
            sprintf(OutString,"\r\n\r\n...4. Please Use Browser  to configure module\r\n\r\n");
            putsConsole(OutString);

            sprintf(OutString,"http://%s:2000\r\n\r\n", SAP_NETWORK_IP_ADDRESS);
            putsConsole(OutString);

            /* put up the IP address of softAP on LCD as well*/
            setLCDC(0x00);
            putsLCD(BLANKLINE);
            setLCDC(0x00);
            sprintf(OutString, "Confg w/Browser");
            putsLCD(OutString);
            setLCDC(0x40);
            putsLCD(BLANKLINE);
            setLCDC(0x40);
            putsLCD(SAP_NETWORK_IP_ADDRESS);

            /* the webserver will use this circular buffer to handle postings
             * from the web browser
             */
            fifoInit();

            /* flush the old uart and switch to the ISR feeding the fifo via uart Rx's */
            flushWiflyRxUart(4);
            IEC0bits.U1RXIE = 1;
            IEC4bits.U1ERIE = 1;

            PIC_RTS = 0;

            // set up EZConfig LED (WiFly PICTAil LEDR) toggle delay
            // toggle every 100mS while waiting to be configured
            configureLedToggleDelay = TickGet() + TICK_SECOND/10;

            smTCPTasks = SM_TCP_EZCONFIG_PROCESS;
        }
        break;

        /* handle the browser while it configures the ssid, passphrase and channel of device */
        case SM_TCP_EZCONFIG_PROCESS:
        {
            // check to see if it's time to toggle the configuring LEDs
            if((int32_t)(TickGet() - configureLedToggleDelay) > (int32_t)0)
            {
                RN_PICTAIL_LEDR ^= 1;  // toggle the LEDs
                RN_PICTAIL_LEDY ^= 1;
                configureLedToggleDelay = TickGet() + TICK_SECOND/10; // reset timer
            }

            /* run the configure task until the browser is used to configure the
             * the device.   Inside the EZconfigTasks() function, the state machine
             * will advance back to SM_TCP_WAIT_ASSOCIATE after the device has
             * been configured to the new AP is which to join.
            */
            EZConfigTasks();
        }
        break;

        case SM_TCP_WAIT_CONNECT:
            // check to see if it's time to toggle the TCP Connect LED
            if((int32_t)(TickGet() - TCPConnectLedToggleDelay) > (int32_t)0)
            {
                RN_PICTAIL_LEDR ^= 1;  // toggle the LED
                TCPConnectLedToggleDelay = TickGet() + TICK_SECOND/10; // reset timer
            }
            // check for GPIO6 line to go high - indicating successful TCP connection
            if(1 == RN_GPIO6)
            {
                sprintf(OutString,"\r\n\r\n...Connected to server..exchanging data\r\n\r\n");
                putsConsole(OutString);
                
                // turn on the TCP Connected LED
                RN_PICTAIL_LEDR = 1;
                
                // initialize HTTP GET request counter (for TCP failure detection)
                // since we are using wifly's GPIO5 TCP control input, our application
                // protocol must monitor messages and determine when TCP connection should be terminated
                // (i.e. "set sys idle xx" function is disabled)
                numHTTPGetAttempts = 0;

                // transition to next state: send the HTTP GET request to the server
                smTCPTasks = SM_TCP_HTTP_GET;
            }

            break;
       
        case SM_TCP_HTTP_GET:
            
            // (for readability) insert 3 lines on Console between HTTP request and response messages
            putsConsole("\r\n\r\n\r\n");
            
            // send HTTP GET request line
            sprintf(WiflyOutBuffer, "GET /luc/UpdateSQL.php?mac=%s%s%s%s%s%s&Led=%d&Button=%d&Pot=%d HTTP/1.0\r\n",
                    (char *)&myNodeInfo.MACAddr.raw[0], (char *)&myNodeInfo.MACAddr.raw[1], (char *)&myNodeInfo.MACAddr.raw[2],
                    (char *)&myNodeInfo.MACAddr.raw[3], (char *)&myNodeInfo.MACAddr.raw[4], (char *)&myNodeInfo.MACAddr.raw[5],
                    (*((volatile unsigned char*)(&LATA))),
                    (SW_S4)|(SW_S5<<1)|(SW_S6<<2)|(SW_S3<<3),
                    ADC1BUF0);
            putsWiFly(WiflyOutBuffer);
            putsConsole(WiflyOutBuffer);
            
            // send HTTP Header-line 1
            //sprintf(WiflyOutBuffer, "Host: %d.%d.%d.%d\r\n", (unsigned char*)&myNodeInfo.IPAddr.v[1], &myNodeInfo.IPAddr.v[2],
            //        &myNodeInfo.IPAddr.v[3]);
            sprintf(WiflyOutBuffer, "Host: mtt.mchpcloud.com\r\n");
            putsWiFly(WiflyOutBuffer);
            putsConsole(WiflyOutBuffer);
            
            // send HTTP Header-line 2 + Blank Line
            putsWiFly("Connection: keep-alive\r\n\r\n");
            putsConsole("Connection: keep-alive\r\n\r\n");
                      
            // initialize the wifly receive buffer
            memset(WiflyInBuffer, '\0', sizeof(WiflyInBuffer));
            // initialize the wifly receive buffer index
            WiflyInBufferIndex = 0;

            // initialize the HTTP response status line fields and line counter

            // initialize HTTP reply line counter
            HTTPReplyLineCnt = 0;

            // increment HTTP GET request counter (for TCP failure detection)
            numHTTPGetAttempts++;

            // check for TCP failure
            if(numHTTPGetAttempts > 3)
            {
                // TCP Communications Error - Reset TCP State Machine
                sprintf(OutString,"\r\n\r\n...TCP Error (SM_TCP_HTTP_GET)..resetting TCP State Machine\r\n\r\n");
                putsConsole(OutString);
                // disable remote TCP connection
                RN_GPIO5 = 0;       
                Delayms(10);
                smTCPTasks = SM_TCP_INIT;
                break;
            }

            // prepare to collect the HTTP response
            PIC_RTS = 0;    // let WiFly know we're ready to Rx
            
            // set the time limit on collection (wait up to 5 seconds for reply to be received)
            HTTPGetReplyDelay = TickGet() + TICK_SECOND*5;

            // transition to the next state
            smTCPTasks = SM_TCP_HTTP_GET_REPLY;
            
            break;

        case SM_TCP_HTTP_GET_REPLY:
            
            // after 5 seconds,  process received data (if any) & update the LEDs
            
            if((int32_t)(TickGet() - HTTPGetReplyDelay) > (int32_t)0)
            {
                // hold off on any more rx
                PIC_RTS = 1;
                // process HTTP server reply
                smTCPTasks = SM_TCP_HTTP_GET_PROCESS;
                
            }
            
            // process received character
            if(U1STAbits.URXDA)
            {
                // reset TCP fail detect variable
                numHTTPGetAttempts = 0;
                
                // process received character
                PIC_RTS = 1;    
                WiflyInBuffer[WiflyInBufferIndex] = U1RXREG;

                if(WiflyInBuffer[WiflyInBufferIndex] == '\n')
                {
                    HTTPReplyLineCnt++;
                }
                // check for reception of first complete line of HTTP reply
                // evaluate for phrase "HTTP/1.1 200 OK" to continue reception
                // otherwise reset TCP state machine
                if(HTTPReplyLineCnt == 2)
                {
                    Nop();
                    Nop();
                    Nop();
                    if(!strstr(WiflyInBuffer, "HTTP/1.1 200 OK"))
                    {
                        // TCP Communications Error - Reset TCP State Machine
                        sprintf(OutString,"\r\n\r\n...TCP Error (SM_TCP_HTTP_GET_REPLY)..resetting TCP State Machine\r\n\r\n");
                        putsConsole(OutString);
                        // disable remote TCP connection
                        RN_GPIO5 = 0;
                        Delayms(10);
                        smTCPTasks = SM_TCP_INIT;
                        break;
                    }
                }
                // look for EOT character (0x04) (or > 512 bytes rx'ed) to switch to next state
                if((WiflyInBuffer[WiflyInBufferIndex] == '\04') || (WiflyInBufferIndex > sizeof(WiflyInBuffer)))
                {
                    smTCPTasks = SM_TCP_HTTP_GET_PROCESS;
                }
                else
                {
                    putConsole(WiflyInBuffer[WiflyInBufferIndex]);
                    WiflyInBufferIndex++;
                    PIC_RTS = 0;
                }
            }
            
            break;
            
        case SM_TCP_HTTP_GET_PROCESS:

            // look for string "Connection: close" in the HTTP reply
            // indicates that the server is going to close the socket after the payload is send

            // find the location/existence of "Connection: close" field in Rx'd data
            ConnectionValue = strstr(WiflyInBuffer, "Connection: close");

            if(ConnectionValue)
            {
                // Remote host closing socket - close TCP connection and try to re-connect
                sprintf(OutString,"\r\n\r\n...TCP Error (SM_TCP_HTTP_GET_PROCESS)..remote host closing connection\r\n\r\n");
                putsConsole(OutString);
                // disable remote TCP connection
                RN_GPIO5 = 0;
                // Turn off the green "TCP Connected Opened" LED on teh PICTAIL (PIC LEDR)
                RN_PICTAIL_LEDR = 0;
                Delayms(10);
                // re-open TCP connection to server
                sprintf(OutString,"\r\n\r\n...re-connecting to Server...\r\n");
                putsConsole(OutString);

                // trigger wifly to create a TCP connection to the remote server
                // using stored settings in the module
                RN_GPIO5 = 1;

                // set up TCP Connect LED (WiFly PICTAil LEDR) toggle delay
                // toggle every 100mS while waiting for a TCP connection
                TCPConnectLedToggleDelay = TickGet() + TICK_SECOND/10;

                // transition to TCP wait connect
                smTCPTasks = SM_TCP_WAIT_CONNECT;
                break;
            }

            // look for string "ledset=" in received data - it will contain any
            // required updates to the explorer16 LEDs
            // received field contains information as to which LED to toggle
            // explorer Leds D10 thru D4 are able to be toggled
            // i.e. "ledset"=128 should toggle D10, "ledset=64" should toggle D9 etc
            
            // find the location of "ledset=" field in Rx'd data
            LedValue = (unsigned char *)strstr(WiflyInBuffer, "ledset=");
            
            if(LedValue)
            {
                j = 0;
                // point to the ledset value
                LedValue += 7;
                // get the ledset value string
                while(*LedValue != ';')
                {
                    temp[j] = *LedValue;
                    LedValue++;
                    j++;
                }
                // terminate string with NULL char
                temp[j] = '\0';

                // convert it to an unsigned int
                ToggleLed = (volatile unsigned int)strtol((const char *)temp, (char **)NULL, 10);
                
                // apply it
                LATA ^= ToggleLed;

                // re-initialize HTTP GET request counter (for TCP failure detection)
                numHTTPGetAttempts = 0;

                // continut sending the HTTP GET request to the server
                smTCPTasks = SM_TCP_HTTP_GET;

            }
            else
            {
                // TCP Communications Error - Reset TCP State Machine
                sprintf(OutString,"\r\n\r\n...TCP Error (SM_TCP_HTTP_GET_PROCESS)..error in reply payload...resetting TCP State Machine\r\n\r\n");
                putsConsole(OutString);
                putsConsole(WiflyInBuffer);
                putsConsole("\r\n\r\n");
                // disable remote TCP connection
                RN_GPIO5 = 0;
                Delayms(10);
                smTCPTasks = SM_TCP_INIT;
                break;
            }

           break;

        default:
            break;
    } // switch(smTCPTasks)

} // TCPTasks()


void sendPage(unsigned char *s)
{
 /*
    sendString(httpHeaders);
    sprintf(tinyBuf, "%d\r\n\r\n", strlen(s));
    sendString(tinyBuf);
    sendString(s);
*/
      Delayms(30);
      putsWiFly((char *)httpHeaders);
      Delayms(15);

      sprintf((char *)tinyBuf, "%d\r\n\r\n", strlen((const char *)s));
      putsWiFly((char *)tinyBuf);
      Delayms(15);

      putsWiFly((char *)s);
}


void sendPageXML(unsigned char *s)
{
 /*
    sendString(httpHeaders);
    sprintf(tinyBuf, "%d\r\n\r\n", strlen(s));
    sendString(tinyBuf);
    sendString(s);
*/
      Delayms(30);
      putsWiFly((char *)xmlHeaders);
      //ConsolePutString(httpHeaders);
      putsConsole((char *)xmlHeaders);
      Delayms(15);

      sprintf((char *)tinyBuf, "%d\r\n\r\n", strlen((const char *)s));
      putsWiFly((char *)tinyBuf);
      //ConsolePutString(tinyBuf);
      putsConsole((char *)tinyBuf);
      Delayms(15);

      putsWiFly((char *)s);
      //ConsolePutString(s);
      putsConsole((char *)s);
}

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
    Delayms(30);
    putsWiFly((char *)httpHeaders);
    putsConsole((char *)httpHeaders);
    Delayms(15);

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
    Delayms(15);

    /* send all the individual pieces that make up the page */
    index = 0;
    while(sval[index] != NULL)
    {
         putsWiFly((char *)sval[index]);
         putsConsole((char *)sval[index]);
         index++;
    }

}

uint8_t httpProcessPostString (char *sourceString, char *patternString, char *stringValue)
{
    uint8_t indx;
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

void httpParseLine()
{
    if (fillingPostData)
    {
        if( (httpProcessPostString ((char *)simpleBuf, (char *)"SSID=", ssid))  )
        {
            sprintf(OutString,"\r\nSSID: ");
            putsConsole(OutString);
            sprintf(OutString,ssid);
            putsConsole(OutString);
        }

        if( (httpProcessPostString ((char *)simpleBuf, (char *)"password=", passwd)) )
        {
            sprintf(OutString,"\r\nPassword: ");
            putsConsole(OutString);
            sprintf(OutString,passwd);
            putsConsole(OutString);
        }

        if( (httpProcessPostString ((char *)simpleBuf, (char *)"Channel=", chan)) )
        {
            sprintf(OutString,"\r\nChannel: ");
            putsConsole(OutString);
            sprintf(OutString,chan);
            putsConsole(OutString);
        }

        /* send the close-out page back to the browser */
        sendPage(page2);
        Delayms(1000);

        /* the work of the EZConfig process is completed and what remains is to
         * reconfigure the module back to its normal state and let it try to
         * associate to its freashly savd network parameters
         */
         
        /* The HTTP client operates in polling mode so restore that mode by
         * disabling the UART interrupts. No more data into fifo */
        IEC0bits.U1RXIE = 0;
        IEC4bits.U1ERIE = 0;

        flushWiflyRxUart(10);  // flush out uart for 40 milliseconds
        IFS0bits.U1RXIF = 0;
        IFS4bits.U1ERIF = 0;
        fifoInit();           // reset fifo indexes.


        /* because the device will not operate in Polling mode until it rebooted
         * the normal wifly driver functions cannot be used here.    Must just send
         * commands directly to the uart Tx, before switching back to  polling mode
         * after module reset
         */
        RN_GPIO5 = 0;
        
        // need to do these at a minimum before using wifly_util_xxx_APIs
        putsWiFly("$$$");
        Delayms(1000);
        putsWiFly("set comm close 0");
        putWiFly('\r');
        Delayms(40);
        putsWiFly("set comm open 0");
        putWiFly('\r');
        Delayms(40);
        putsWiFly("set sys iofunc 0x70");
        putWiFly('\r');
        Delayms(40);
        putsWiFly("save");
        putWiFly('\r');
        Delayms(40);
        putsWiFly("reboot");
        putWiFly('\r');
        Delayms(40);
        
        /* flush out the uart because bytes from previous communications are
         * still residing in it (up to 4 bytes according to specs
         */

        Delayms(500);
        U1STAbits.OERR  = 0;
        flushWiflyRxUart(10);  // flush out uart for 100 milliseconds

        /* tell the module it can now send data */
        PIC_RTS = 1;
        U1STAbits.OERR  = 0;

        /* put module in infrastructure mode using the configuration parameters */
        if(!wifly_util_enter_command_mode())
        {
            sprintf(OutString,"\r\ncould not enter command mode...halting\r\n");
            putsConsole(OutString);
            while(1);
        }
         
        // leave any auto-joined network
        if(!wifly_util_send_command(ACTION_LEAVE, NULL, NULL, NULL, STD_RESPONSE))
        {
            sprintf(OutString,"\r\nleave...failed\r\n");
            putsConsole(OutString);
            while(1);
        }
        // re-program SSID, passphrase, channel from user input
        
        if(!wifly_util_send_command(SET, WLAN, SSID, ssid, STD_RESPONSE))
        {
            sprintf(OutString,"\r\nEZC1: set wlan ssid...failed\r\n");
            putsConsole(OutString);
            while(1);
        }

        if(!wifly_util_send_command(SET, WLAN, PHRASE, passwd, STD_RESPONSE))
        {
            sprintf(OutString,"\r\nEZC1:set wlan phrase...failed\r\n");
            putsConsole(OutString);
            while(1);
        }

        if(!wifly_util_send_command(SET, WLAN, CHANNEL, chan, STD_RESPONSE))
        {
            sprintf(OutString,"\r\nEZC1:set wlan channel...failed\r\n");
            putsConsole(OutString);
            while(1);
        }

        // restore ip parameters
        if(!wifly_util_send_command(SET, IP, DHCP, DHCP_MODE_ON, STD_RESPONSE))
        {
            sprintf(OutString,"\r\nEZC1:set ip dhcp...failed\r\n");
            putsConsole(OutString);
            while(1);
        }

        /* Settng host ip to 0.0.0.0 is a MUST or else the DNS client process
         * WILL NOT START !!!!!, and the http client demo is dependent on the
         * dns client process auto starting.
        */

        if(!wifly_util_send_command(SET, IP, HOST, "0.0.0.0", STD_RESPONSE))
        {
             sprintf(OutString,"\r\nset ip host...failed\r\n");
             putsConsole(OutString);
             while(1);
        }

        // restore remaining module parameters to defaults (those changed by entry into EZConfig mode)

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

        // set wlan join 1
        if(!wifly_util_send_command(SET, WLAN, JOIN, JOIN_VALUE, STD_RESPONSE))
        {
            sprintf(OutString,"\r\nset wlan join 1...failed\r\n");
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

        // save the changes
        if(!wifly_util_send_command(FILEIO_SAVE, NULL, NULL, NULL, STD_RESPONSE))
        {
            sprintf(OutString,"\r\nEZC1: save...failed\r\n");
            putsConsole(OutString);
            while(1);
        }
        
        // (hard-)reset the module to apply the new settings
        sprintf(OutString,"\r\n\r\n...6. Browser Configuration Done\r\n\r\n");
        putsConsole(OutString);
        wifly_util_hardware_reset();

        // delay after module reset/reboot to allow GPIO4 toggle low-high-low
        Delayms(250);

        // set up Associate LED (WiFly PICTAil LEDY) toggle delay
        // toggle every 100mS while waiting to associate
        AssociateLedToggleDelay = TickGet() + TICK_SECOND/10;

        // how long to wait for association before triggering EZConfig
        AssociateWaitDelay = TickGet() + TICK_SECOND*TIME_GIVEN_TO_ASSOC_IN_SECS;

        //Update the console message to indicate SM_TCP_WAIT_ASSOCIATE state
        sprintf(OutString,"\r\n\r\n...associating with Access Point\r\n");
        putsConsole(OutString);
        // Update LCD message per SM_TCP_WAIT_ASSOCIATE state
        clrLCD();
        putsLCD("Associating...");

        smTCPTasks = SM_TCP_WAIT_ASSOCIATE;

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
            sendPageXML((unsigned char *)&xmlLines);
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
}

void EZConfigTasks()
{
    unsigned char c;


    while (fifoCount())
    {
        c = fifoRd();       // grab a char
        switch (httpState)
        {
            case HTTP_CONN_WAIT:
                if (c == '^')
                {
                    httpState = HTTP_CONN_WAIT1; // received ^
                }
                break;
            case HTTP_CONN_WAIT1:
                if (c == 'o')
                {
                    httpState = HTTP_CONN_WAIT2; // received ^o

                }
                else
                    httpState = HTTP_CONN_WAIT;  // received ^?
                break;
            case HTTP_CONN_WAIT2:
                if (c == '^')
                {
                    httpState = HTTP_CONN_OPEN;  // received ^o^
                    sbidx = 0;
                    httpLine = 0;
                }
                else
                    httpState = HTTP_CONN_WAIT;  // received ^o?
                break;
            case HTTP_CONN_OPEN:
                if (c == '^')
                {
                    httpState = HTTP_CONN_CLOSE;  // received ^
                }
                else if (c == '\n')  /* the end of each line received after open */
                {
                    simpleBuf[sbidx-1] = '\0';
                    httpParseLine();
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
                        httpParseLine();
                        fillingPostData = 0;
                        sbidx = 0;
                    }
                }
                break;
            case HTTP_CONN_CLOSE:
                if (c == 'c')
                    httpState = HTTP_CONN_CLOSE1; // received ^c
                else
                    httpState = HTTP_CONN_OPEN;  // received ^?
                break;
            case HTTP_CONN_CLOSE1:
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
                break;
            default:
                break;


        }
    }
}


void configWiFlyAsSoftAP()
{

    // leave PIC24 open-drain pin as output and turn off
    // let PICTail pull-up resistor pull GPIO5 high
    // open-drain configuration should be ok while GPIO5 transitions back to default function during SoftAP mode
    RN_GPIO5 = 1;

    // enter command mode
    if(!wifly_util_enter_command_mode())
    {
        sprintf(OutString,"\r\nEZC: could not enter command mode...halting\r\n");
        putsConsole(OutString);
        while(1);
    }
    
    // leave the network
    if(!wifly_util_send_command(ACTION_LEAVE, NULL, NULL, NULL, STD_RESPONSE))
    {
        sprintf(OutString,"\r\nleave...failed\r\n");
        putsConsole(OutString);
        while(1);
    }

    if(!wifly_util_send_command(SET, WLAN, JOIN, SAP_NETWORK_AP_MODE, STD_RESPONSE))
    {
        sprintf(OutString,"\r\nEZC: set wlan join 7...failed\r\n");
        putsConsole(OutString);
        while(1);
    }

    // set the wlan channel
    if(!wifly_util_send_command(SET, WLAN, CHANNEL, SAP_NETWORK_CHANNEL, STD_RESPONSE))
    {
        sprintf(OutString,"\r\nset wlan channel...failed\r\n");
        putsConsole(OutString);
        while(1);
    }

    // set the wlan ssid
    if(!wifly_util_send_command(SET, WLAN, SSID, (const char *)ssidOfAP, STD_RESPONSE))
    {
        sprintf(OutString,"\r\nEZC: set wlan ssid...failed\r\n");
        putsConsole(OutString);
        while(1);
    }

    // set the wlan passphrase
    if(!wifly_util_send_command(SET, WLAN, PHRASE, SAP_NETWORK_PASS, STD_RESPONSE))
    {
        sprintf(OutString,"\r\nEZC: set wlan phrase...failed\r\n");
        putsConsole(OutString);
        while(1);
    }

    // set dhcp mode
    if(!wifly_util_send_command(SET, IP, DHCP, SAP_NETWORK_DHCP_MODE, STD_RESPONSE))
    {
        sprintf(OutString,"\r\nEZC: set ip dhcp...failed\r\n");
        putsConsole(OutString);
        while(1);
    }


    if(!wifly_util_send_command(SET, IP, ADDRESS, SAP_NETWORK_IP_ADDRESS, STD_RESPONSE))
    {
        sprintf(OutString,"\r\nEZC: set ip address...failed\r\n");
        putsConsole(OutString);
        while(1);
    }


    if(!wifly_util_send_command(SET, IP, NETMASK, SAP_NETWORK_SUBNET_MASK, STD_RESPONSE))
    {
        sprintf(OutString,"\r\nEZC: set ip netmask...failed\r\n");
        putsConsole(OutString);
        while(1);
    }

    if(!wifly_util_send_command(SET, IP, GATEWAY, SAP_NETWORK_IP_ADDRESS, STD_RESPONSE))
    {
        sprintf(OutString,"\r\nEZC: set ip gateway...failed\r\n");
        putsConsole(OutString);
        while(1);
    }


    if(!wifly_util_send_command(SET, COMM, OPEN, TCPPORT_OPEN_STRING, STD_RESPONSE))
    {
        sprintf(OutString,"\r\nEZC: set comm open...failed\r\n");
        putsConsole(OutString);
        while(1);
    }

    if(!wifly_util_send_command(SET, COMM, CLOSE, TCPPORT_CLOSE_STRING, STD_RESPONSE))
    {
        sprintf(OutString,"\r\nEZC: set comm close ...failed\r\n");
        putsConsole(OutString);
        while(1);
    }


    if(!wifly_util_send_command(SET, COMM, REMOTE, TCPPORT_HELLO_STRING, STD_RESPONSE))
    {
        sprintf(OutString,"\r\nEZC: set comm remote ...failed\r\n");
        putsConsole(OutString);
        while(1);
    }

    if(!wifly_util_send_command(SET, "sys", "iofunc", "0x50", STD_RESPONSE))
    {
        sprintf(OutString,"\r\nEZC: set sys iofunc ...failed\r\n");
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

    wifly_util_hardware_reset();

}

