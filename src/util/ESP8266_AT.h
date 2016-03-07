/******************************************************************************
ESP8266_AT.h
ESP8266 AT Command Definitions
Jim Lindblom @ SparkFun Electronics
Original Creation Date: June 20, 2015
http://github.com/sparkfun/SparkFun_ESP8266_AT_Arduino_Library

!!! Description Here !!!

Development environment specifics:
	IDE: Arduino 1.6.5
	Hardware Platform: Arduino Uno
	ESP8266 WiFi Shield Version: 1.0

This code is beerware; if you see me (or any other SparkFun employee) at the
local, and you've found our code helpful, please buy us a round!

Distributed as-is; no warranty is given.
******************************************************************************/

//////////////////////
// Common Responses //
//////////////////////
const char RESPONSE_OK[] PROGMEM = "OK\r\n";
const char RESPONSE_ERROR[] PROGMEM = "ERROR\r\n";
const char RESPONSE_FAIL[] PROGMEM = "FAIL";
const char RESPONSE_READY[] PROGMEM = "READY!";

///////////////////////
// Basic AT Commands //
///////////////////////
const char ESP8266_TEST[] PROGMEM = "";	// Test AT startup
const char ESP8266_RESET[] PROGMEM = "+RST"; // Restart module
const char ESP8266_VERSION[] PROGMEM = "+GMR"; // View version info
const char ESP8266_ECHO_ENABLE[] PROGMEM = "E1"; // AT commands echo
const char ESP8266_ECHO_DISABLE[] PROGMEM = "E0"; // AT commands echo
const char ESP8266_UART[] PROGMEM = "+UART"; // UART configuration

////////////////////
// WiFi Functions //
////////////////////
const char ESP8266_WIFI_MODE[] PROGMEM = "+CWMODE"; // WiFi mode (sta/AP/sta+AP)
const char ESP8266_CONNECT_AP[] PROGMEM = "+CWJAP"; // Connect to AP
const char ESP8266_DISCONNECT[] PROGMEM = "+CWQAP"; // Disconnect from AP

/////////////////////
// TCP/IP Commands //
/////////////////////
const char ESP8266_TCP_STATUS[] PROGMEM = "+CIPSTATUS"; // Get connection status
const char ESP8266_TCP_CONNECT[] PROGMEM = "+CIPSTART"; // Establish TCP connection or register UDP port
const char ESP8266_TCP_SEND[] PROGMEM = "+CIPSEND"; // Send Data
const char ESP8266_TCP_CLOSE[] PROGMEM = "+CIPCLOSE"; // Close TCP/UDP connection
const char ESP8266_GET_LOCAL_IP[] PROGMEM = "+CIFSR"; // Get local IP address
const char ESP8266_TCP_MULTIPLE[] PROGMEM = "+CIPMUX"; // Set multiple connections mode
const char ESP8266_SERVER_CONFIG[] PROGMEM = "+CIPSERVER"; // Configure as server
const char ESP8266_TRANSMISSION_MODE[] PROGMEM = "+CIPMODE"; // Set transmission mode

