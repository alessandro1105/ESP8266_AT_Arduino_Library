/******************************************************************************
SparkFunESP8266WiFi.cpp
ESP8266 WiFi Shield Library Main Source File
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

#include <ESP8266WiFi.h>
#include <Arduino.h>
#include "util/ESP8266_AT.h"
#include "ESP8266Client.h"

#define ESP8266_DISABLE_ECHO

////////////////////////
// Buffer Definitions //
////////////////////////
#define ESP8266_RX_BUFFER_LEN 64 // Number of bytes in the serial receive buffer (ORGINAL WAS 128)
char esp8266RxBuffer[ESP8266_RX_BUFFER_LEN];
unsigned int bufferHead; // Holds position of latest byte placed in buffer.

////////////////////
// Initialization //
////////////////////

ESP8266Class::ESP8266Class()
{
	for (int i=0; i<ESP8266_MAX_SOCK_NUM; i++)
		_state[i] = AVAILABLE;
}

bool ESP8266Class::begin(unsigned long baudRate)
{
	//salvo la serial da utilizzare
	swSerial.begin(baudRate);
	_serial = &swSerial;
	_baud = baudRate;
	
	if (test())
	{
		if (!reset())
			return false;
		if (!setMode(ESP8266_MODE_STA))
			return false;
		if (!setTransferMode(1))
			return false;
		if (!setMux(1))
			return false;
#ifdef ESP8266_DISABLE_ECHO
		if (!echo(false))
			return false;
#endif
		return true;
	}
	
	return false;

}

bool ESP8266Class::begin(Stream &serial, unsigned long baudRate)
{

	//salvo la serial da utilizzare
	_serial = &serial;
	_baud = baudRate;
	
	if (test())
	{
		if (!reset())
			return false;
		if (!setMode(ESP8266_MODE_STA))
			return false;
		if (!setTransferMode(1))
			return false;
		if (!setMux(1))
			return false;
#ifdef ESP8266_DISABLE_ECHO
		if (!echo(false))
			return false;
#endif
		return true;
	}
	
	return false;
}

///////////////////////
// Basic AT Commands //
///////////////////////

bool ESP8266Class::test()
{
	sendCommand(ESP8266_TEST); // Send AT

	if (readForResponsePROGMEM(RESPONSE_OK, COMMAND_RESPONSE_TIMEOUT) > 0)
		return true;
	
	return false;
}

bool ESP8266Class::reset()
{
	sendCommand(ESP8266_RESET); // Send AT+RST
	
	delay(2000);

	if (readForResponsePROGMEM(RESPONSE_OK, COMMAND_RESET_TIMEOUT) > 0) {
		return true;
	}
	
	return false;
}

bool ESP8266Class::echo(bool enable)
{
	if (enable)
		sendCommand(ESP8266_ECHO_ENABLE);
	else
		sendCommand(ESP8266_ECHO_DISABLE);
	
	if (readForResponsePROGMEM(RESPONSE_OK, COMMAND_RESPONSE_TIMEOUT) > 0)
		return true;
	
	return false;
}

bool ESP8266Class::setBaud(unsigned long baud)
{
	char parameters[16];
	memset(parameters, 0, 16);
	// Constrain parameters:
	baud = constrain(baud, 110, 115200);
	
	// Put parameters into string
	sprintf(parameters, "%d,8,1,0,0", baud);
	
	// Send AT+UART=baud,databits,stopbits,parity,flowcontrol
	sendCommand(ESP8266_UART, ESP8266_CMD_SETUP, parameters);
	
	if (readForResponsePROGMEM(RESPONSE_OK, COMMAND_RESPONSE_TIMEOUT) > 0)
		return true;
	
	return false;
}

int16_t ESP8266Class::getVersion(char * ATversion)
{
	sendCommand(ESP8266_VERSION); // Send AT+GMR
	// Example Response: 0018000902\r\n
	//                   \r\n
	//                   OK\r\n
	// (~101 characters)
	// Look for "OK":
	int16_t rsp = (readForResponsePROGMEM(RESPONSE_OK, COMMAND_RESPONSE_TIMEOUT) > 0);
	if (rsp > 0)
	{
		char *p = esp8266RxBuffer;
		char *q = strchr(p, '\r');
		if (q == NULL) return ESP8266_RSP_UNKNOWN;
		strncpy(ATversion, p, q-p);
		ATversion[q-p] = 0; //terminatore di stringa
	}
	
	return rsp;
}

////////////////////
// WiFi Functions //
////////////////////

// getMode()
// Input: None
// Output:
//    - Success: 1, 2, 3 (ESP8266_MODE_STA, ESP8266_MODE_AP, ESP8266_MODE_STAAP)
//    - Fail: <0 (esp8266_cmd_rsp)
int16_t ESP8266Class::getMode()
{
	sendCommand(ESP8266_WIFI_MODE, ESP8266_CMD_QUERY);
	
	// Example response: \r\nAT+CWMODE_CUR?\r+CWMODE_CUR:2\r\n\r\nOK\r\n
	// Look for the OK:
	int16_t rsp = readForResponsePROGMEM(RESPONSE_OK, COMMAND_RESPONSE_TIMEOUT);
	if (rsp > 0)
	{
		// Then get the number after ':':
		char * p = strchr(esp8266RxBuffer, ':');
		if (p != NULL)
		{
			char mode = *(p+1);
			if ((mode >= '1') && (mode <= '3'))
				return (mode - 48); // Convert ASCII to decimal
		}
		
		return ESP8266_RSP_UNKNOWN;
	}
	
	return rsp;
}

// setMode()
// Input: 1, 2, 3 (ESP8266_MODE_STA, ESP8266_MODE_AP, ESP8266_MODE_STAAP)
// Output:
//    - Success: >0
//    - Fail: <0 (esp8266_cmd_rsp)
int16_t ESP8266Class::setMode(esp8266_wifi_mode mode)
{
	char modeChar[2] = {0, 0};
	sprintf(modeChar, "%d", mode);
	sendCommand(ESP8266_WIFI_MODE, ESP8266_CMD_SETUP, modeChar);
	
	return readForResponsePROGMEM(RESPONSE_OK, COMMAND_RESPONSE_TIMEOUT);
}

int16_t ESP8266Class::connect(const char * ssid)
{
	connect(ssid, "");
}

// connect()
// Input: ssid and pwd const char's
// Output:
//    - Success: >0
//    - Fail: <0 (esp8266_cmd_rsp)
int16_t ESP8266Class::connect(const char * ssid, const char * pwd)
{
	_serial->print("AT");

	//---PATCH PROGMEM
	int len = strlen_P(ESP8266_CONNECT_AP);
	for (int i = 0; i < len; i++)
	{
		char c =  pgm_read_byte_near(ESP8266_CONNECT_AP + i);
		_serial->print(c);
	}
	//---PATCH PROGMEM
	
	_serial->print("=\"");
	_serial->print(ssid);
	_serial->print("\"");
	if (pwd != NULL)
	{
		_serial->print(",");
		_serial->print("\"");
		_serial->print(pwd);
		_serial->print("\"");
	}
	_serial->print("\r\n");
	
	return readForResponsesPROGMEM(RESPONSE_OK, RESPONSE_FAIL, WIFI_CONNECT_TIMEOUT);
}

int16_t ESP8266Class::getAP(char * ssid)
{
	sendCommand(ESP8266_CONNECT_AP, ESP8266_CMD_QUERY); // Send "AT+CWJAP?"
	
	int16_t rsp = readForResponsePROGMEM(RESPONSE_OK, COMMAND_RESPONSE_TIMEOUT);
	// Example Responses: No AP\r\n\r\nOK\r\n
	// - or -
	// +CWJAP:"WiFiSSID","00:aa:bb:cc:dd:ee",6,-45\r\n\r\nOK\r\n
	if (rsp > 0)
	{
		// Look for "No AP"
		if (strstr(esp8266RxBuffer, "No AP") != NULL)
			return 0;
		
		
		//---PATCH PROGMEM
		int len = strlen_P(ESP8266_CONNECT_AP);
		//creo buffer
		char cmd[len +1]; //compreso terminatore

		for (int i = 0; i <= len; i++)
		{
			char c =  pgm_read_byte_near(ESP8266_CONNECT_AP + i);
			cmd[i] = c;
		}
		//---PATCH PROGMEM

		// Look for "+CWJAP"
		char * p = strstr(esp8266RxBuffer, cmd);
		if (p != NULL)
		{
			p += strlen(cmd) + 2;
			char * q = strchr(p, '"');
			if (q == NULL) return ESP8266_RSP_UNKNOWN;
			strncpy(ssid, p, q-p); // Copy string to temp char array:

			//---FIX
			ssid[q-p] = 0; //inserisco il terminatore di stringa

			return 1;
		}
	}
	
	return rsp;
}

int16_t ESP8266Class::disconnect()
{
	sendCommand(ESP8266_DISCONNECT); // Send AT+CWQAP
	// Example response: \r\n\r\nOK\r\nWIFI DISCONNECT\r\n
	// "WIFI DISCONNECT" comes up to 500ms _after_ OK. 
	int16_t rsp = readForResponsePROGMEM(RESPONSE_OK, COMMAND_RESPONSE_TIMEOUT);
	if (rsp > 0)
	{
		rsp = readForResponsePROGMEM(RESPONSE_OK, COMMAND_RESPONSE_TIMEOUT);
		if (rsp > 0)
			return rsp;
		return 1;
	}
	
	return rsp;
}

// status()
// Input: none
// Output:
//    - Success: 2, 3, 4, or 5 (ESP8266_STATUS_GOTIP, ESP8266_STATUS_CONNECTED, ESP8266_STATUS_DISCONNECTED, ESP8266_STATUS_NOWIFI)
//    - Fail: <0 (esp8266_cmd_rsp)
int16_t ESP8266Class::status()
{
	int16_t statusRet = updateStatus();
	if (statusRet > 0)
	{
		switch (_status.stat)
		{
		case ESP8266_STATUS_GOTIP: // 3
		case ESP8266_STATUS_DISCONNECTED: // 4 - "Client" disconnected, not wifi
			return 1;
			break;
		case ESP8266_STATUS_CONNECTED: // Connected, but haven't gotten an IP
		case ESP8266_STATUS_NOWIFI: // No WiFi configured
			return 0;
			break;
		}
	}
	return statusRet;
}

int16_t ESP8266Class::updateStatus()
{
	sendCommand(ESP8266_TCP_STATUS); // Send AT+CIPSTATUS\r\n
	// Example response: (connected as client)
	// STATUS:3\r\n
	// +CIPSTATUS:0,"TCP","93.184.216.34",80,0\r\n\r\nOK\r\n 
	// - or - (clients connected to ESP8266 server)
	// STATUS:3\r\n
	// +CIPSTATUS:0,"TCP","192.168.0.100",54723,1\r\n
	// +CIPSTATUS:1,"TCP","192.168.0.101",54724,1\r\n\r\nOK\r\n 
	int16_t rsp = readForResponsePROGMEM(RESPONSE_OK, COMMAND_RESPONSE_TIMEOUT);
	if (rsp > 0)
	{
		char * p = searchBuffer("STATUS:");
		if (p == NULL)
			return ESP8266_RSP_UNKNOWN;
		
		p += strlen("STATUS:");
		_status.stat = (esp8266_connect_status)(*p - 48);
		
		for (int i=0; i<ESP8266_MAX_SOCK_NUM; i++)
		{
			p = strstr(p, "+CIPSTATUS:");
			if (p == NULL)
			{
				// Didn't find any IPSTATUS'. Set linkID to 255.
				for (int j=i; j<ESP8266_MAX_SOCK_NUM; j++)
					_status.ipstatus[j].linkID = 255;
				return rsp;
			}
			else
			{
				p += strlen("+CIPSTATUS:");
				// Find linkID:
				uint8_t linkId = *p - 48;
				if (linkId >= ESP8266_MAX_SOCK_NUM)
					return rsp;
				_status.ipstatus[linkId].linkID = linkId;
				
				// Find type (p pointing at linkID):
				p += 3; // Move p to either "T" or "U"
				if (*p == 'T')
					_status.ipstatus[linkId].type = ESP8266_TCP;
				else if (*p == 'U')
					_status.ipstatus[linkId].type = ESP8266_UDP;
				else
					_status.ipstatus[linkId].type = ESP8266_TYPE_UNDEFINED;
				
				// Find remoteIP (p pointing at first letter or type):
				p += 6; // Move p to first digit of first octet.
				for (uint8_t j = 0; j < 4; j++)
				{
					char tempOctet[4];
					memset(tempOctet, 0, 4); // Clear tempOctet
					
					size_t octetLength = strspn(p, "0123456789"); // Find length of numerical string:
					
					strncpy(tempOctet, p, octetLength); // Copy string to temp char array:
					_status.ipstatus[linkId].remoteIP[j] = atoi(tempOctet); // Move the temp char into IP Address octet
					
					p += (octetLength + 1); // Increment p to next octet
				}
				
				// Find port (p pointing at ',' between IP and port:
				p += 1; // Move p to first digit of port
				char tempPort[6];
				memset(tempPort, 0, 6);
				size_t portLen = strspn(p, "0123456789"); // Find length of numerical string:
				strncpy(tempPort, p, portLen);
				_status.ipstatus[linkId].port = atoi(tempPort);
				p += portLen + 1;
				
				// Find tetype (p pointing at tetype)
				if (*p == '0')
					_status.ipstatus[linkId].tetype = ESP8266_CLIENT;
				else if (*p == '1')
					_status.ipstatus[linkId].tetype = ESP8266_SERVER;
			}
		}
	}
	
	return rsp;
}

// localIP()
// Input: none
// Output:
//    - Success: Device's local IPAddress
//    - Fail: 0
IPAddress ESP8266Class::localIP()
{
	sendCommand(ESP8266_GET_LOCAL_IP); // Send AT+CIFSR\r\n
	// Example Response: 192.168.0.114\r\n
	//                   \r\n
	//                   OK\r\n
	// Look for the OK:
	int16_t rsp = readForResponsePROGMEM(RESPONSE_OK, COMMAND_RESPONSE_TIMEOUT);
	if (rsp > 0)
	{

		char * p = esp8266RxBuffer;

		IPAddress returnIP;
			
		//p += 7; // Move p seven places. (skip STAIP,")
		for (uint8_t i = 0; i < 4; i++)
		{
			char tempOctet[4];
			memset(tempOctet, 0, 4); // Clear tempOctet
			
			size_t octetLength = strspn(p, "0123456789"); // Find length of numerical string:
			if (octetLength >= 4) // If it's too big, return an error
				return ESP8266_RSP_UNKNOWN;
			
			strncpy(tempOctet, p, octetLength); // Copy string to temp char array:
			returnIP[i] = atoi(tempOctet); // Move the temp char into IP Address octet
			
			p += (octetLength + 1); // Increment p to next octet
		}
		
		return returnIP;
	}
	
	return rsp;
}

/////////////////////
// TCP/IP Commands //
/////////////////////

int16_t ESP8266Class::tcpConnect(uint8_t linkID, const char * destination, uint16_t port)
{

	//---PATCH PROGMEM
	int len = strlen_P(ESP8266_TCP_CONNECT);

	char cmd[len+1];

	for (int i = 0; i <= len; i++)
	{
		char c = pgm_read_byte_near(ESP8266_TCP_CONNECT + i);
		cmd[i] = c;
	}
	//---PATCH PROGMEM

	_serial->print("AT");
	_serial->print(cmd);
	_serial->print("=");
	_serial->print(linkID);
	_serial->print(",");
	_serial->print("\"TCP\",");
	_serial->print("\"");
	_serial->print(destination);
	_serial->print("\",");
	_serial->print(port);
	_serial->print("\r\n");
	// Example good: CONNECT\r\n\r\nOK\r\n
	// Example bad: DNS Fail\r\n\r\nERROR\r\n
	// Example meh: ALREADY CONNECTED\r\n\r\nERROR\r\n
	int16_t rsp = readForResponsesPROGMEM(RESPONSE_OK, RESPONSE_ERROR, CLIENT_CONNECT_TIMEOUT);
	
	if (rsp < 0)
	{
		// We may see "ERROR", but be "ALREADY CONNECTED".
		// Search for "ALREADY", and return success if we see it.
		char * p = searchBuffer("ALREADY");
		if (p == NULL) {
			return rsp;
		}
		
	}


	//invio il comando di invio
	sendCommand(ESP8266_TCP_SEND);

	int16_t rspSend = readForResponse(">", CLIENT_CONNECT_TIMEOUT);

	if (rspSend > 0) {
		return 1;
	} else {
		return rspSend;
	}

	return 1;
}

int16_t ESP8266Class::tcpSend(uint8_t linkID, const uint8_t *buf, size_t size)
{	

	//delay(1000);

	_serial->print((const char *)buf);
	return size;

	// if (size > 2048)
	// 	return ESP8266_CMD_BAD;
	// char params[8];
	// sprintf(params, "%d,%d", linkID, size);
	// sendCommand(ESP8266_TCP_SEND, ESP8266_CMD_SETUP, params);
	
	// int16_t rsp = readForResponsesPROGMEM(RESPONSE_OK, RESPONSE_ERROR, COMMAND_RESPONSE_TIMEOUT);
	// //if (rsp > 0)
	// if (rsp != ESP8266_RSP_FAIL)
	// {
	// 	_serial->print((const char *)buf);
		
	// 	rsp = readForResponse("SEND OK", COMMAND_RESPONSE_TIMEOUT);
		
	// 	if (rsp > 0)
	// 		return size;
	// }
	
	// return rsp;
}

int16_t ESP8266Class::close(uint8_t linkID)
{
	char params[2];
	sprintf(params, "%d", linkID);
	sendCommand(ESP8266_TCP_CLOSE, ESP8266_CMD_SETUP, params);
	
	// Eh, client virtual function doesn't have a return value.
	// We'll wait for the OK or timeout anyway.
	return readForResponsePROGMEM(RESPONSE_OK, COMMAND_RESPONSE_TIMEOUT);
}

int16_t ESP8266Class::setTransferMode(uint8_t mode)
{
	char params[2] = {0, 0};
	params[0] = (mode > 0) ? '1' : '0';
	sendCommand(ESP8266_TRANSMISSION_MODE, ESP8266_CMD_SETUP, params);
	
	return readForResponsePROGMEM(RESPONSE_OK, COMMAND_RESPONSE_TIMEOUT);
}

int16_t ESP8266Class::setMux(uint8_t mux)
{
	char params[2] = {0, 0};
	params[0] = (mux > 0) ? '1' : '0';
	sendCommand(ESP8266_TCP_MULTIPLE, ESP8266_CMD_SETUP, params);
	
	return readForResponsePROGMEM(RESPONSE_OK, COMMAND_RESPONSE_TIMEOUT);
}

int16_t ESP8266Class::configureTCPServer(uint16_t port, uint8_t create)
{
	char params[10];
	if (create > 1) create = 1;
	sprintf(params, "%d,%d", create, port);
	sendCommand(ESP8266_SERVER_CONFIG, ESP8266_CMD_SETUP, params);
	
	return readForResponsePROGMEM(RESPONSE_OK, COMMAND_RESPONSE_TIMEOUT);	
}

//////////////////////////////
// Stream Virtual Functions //
//////////////////////////////

size_t ESP8266Class::write(uint8_t c)
{
	_serial->write(c);
}

int ESP8266Class::available()
{
	return _serial->available();
}

int ESP8266Class::read()
{
	return _serial->read();
}

int ESP8266Class::peek()
{
	return _serial->peek();
}

void ESP8266Class::flush()
{
	_serial->flush();
}

//////////////////////////////////////////////////
// Private, Low-Level, Ugly, Hardware Functions //
//////////////////////////////////////////////////

void ESP8266Class::sendCommand(const char * cmdPROGMEM, enum esp8266_command_type type, const char * params)
{
	//---PATCH PROGMEM
	int len = strlen_P(cmdPROGMEM);

	char cmd[len+1];

	for (int i = 0; i <= len; i++)
	{
		char c =  pgm_read_byte_near(cmdPROGMEM + i);
		cmd[i] = c;
	}
	//---PATCH PROGMEM

	_serial->print("AT");
	_serial->print(cmd);
	if (type == ESP8266_CMD_QUERY) {
		_serial->print("?");
	} else if (type == ESP8266_CMD_SETUP) {
		_serial->print("=");
		_serial->print(params);		
	}
	_serial->print("\r\n");

}

int16_t ESP8266Class::readForResponse(const char * rsp, unsigned int timeout)
{
	unsigned long timeIn = millis();	// Timestamp coming into function
	unsigned int received = 0; // received keeps track of number of chars read
	
	clearBuffer();	// Clear the class receive buffer (esp8266RxBuffer)
	while (timeIn + timeout > millis()) // While we haven't timed out
	{
		if (_serial->available()) // If data is available on UART RX
		{
			received += readByteToBuffer();
			if (searchBuffer(rsp))	// Search the buffer for goodRsp
				return received;	// Return how number of chars read
		}
	}
	
	if (received > 0) // If we received any characters
		return ESP8266_RSP_UNKNOWN; // Return unkown response error code
	else // If we haven't received any characters
		return ESP8266_RSP_TIMEOUT; // Return the timeout error code
}

int16_t ESP8266Class::readForResponses(const char * pass, const char * fail, unsigned int timeout)
{
	unsigned long timeIn = millis();	// Timestamp coming into function
	unsigned int received = 0; // received keeps track of number of chars read
	
	clearBuffer();	// Clear the class receive buffer (esp8266RxBuffer)
	while (timeIn + timeout > millis()) // While we haven't timed out
	{
		if (_serial->available()) // If data is available on UART RX
		{
			received += readByteToBuffer();
			if (searchBuffer(pass))	// Search the buffer for goodRsp
				return received;	// Return how number of chars read
			if (searchBuffer(fail))
				return ESP8266_RSP_FAIL;
		}
	}
	
	if (received > 0) // If we received any characters
		return ESP8266_RSP_UNKNOWN; // Return unkown response error code
	else // If we haven't received any characters
		return ESP8266_RSP_TIMEOUT; // Return the timeout error code
}

int16_t ESP8266Class::readForResponsePROGMEM(const char * rspPROGMEM, unsigned int timeout)
{
	//---PATCH PROGMEM
	int len = strlen_P(rspPROGMEM);
	//creo buffer
	char rsp[len +1]; //compreso terminatore

	for (int i = 0; i <= len; i++)
	{
		char c =  pgm_read_byte_near(rspPROGMEM + i);
		rsp[i] = c;
	}
	//---PATCH PROGMEM

	return readForResponse(rsp, timeout);
}

//---PROGMEM VARIANT
int16_t ESP8266Class::readForResponsesPROGMEM(const char * passPROGMEM, const char * failPROGMEM, unsigned int timeout)
{
	//---PATCH PROGMEM
	int len = strlen_P(passPROGMEM);
	//creo buffer
	char pass[len +1]; //compreso terminatore

	for (int i = 0; i <= len; i++)
	{
		char c =  pgm_read_byte_near(passPROGMEM + i);
		pass[i] = c;
	}

	len = strlen_P(failPROGMEM);
	//creo buffer
	char fail[len +1]; //compreso terminatore

	for (int i = 0; i <= len; i++)
	{
		char c =  pgm_read_byte_near(failPROGMEM + i);
		fail[i] = c;
	}
	//---PATCH PROGMEM

	return readForResponses(pass, fail, timeout);
}

//////////////////
// Buffer Stuff //
//////////////////
void ESP8266Class::clearBuffer()
{
	memset(esp8266RxBuffer, '\0', ESP8266_RX_BUFFER_LEN);
	bufferHead = 0;
}	

unsigned int ESP8266Class::readByteToBuffer()
{
	// Read the data in
	char c = _serial->read();
	// Store the data in the buffer
	esp8266RxBuffer[bufferHead] = c;
	//! TODO: Don't care if we overflow. Should we? Set a flag or something?
	bufferHead = (bufferHead + 1) % ESP8266_RX_BUFFER_LEN;
	
	return 1;
}

char * ESP8266Class::searchBuffer(const char * test)
{

	int bufferLen = strlen((const char *)esp8266RxBuffer);
	// If our buffer isn't full, just do an strstr
	if (bufferLen < ESP8266_RX_BUFFER_LEN)
		return strstr((const char *)esp8266RxBuffer, test);
	else
	{	//! TODO
		// If the buffer is full, we need to search from the end of the 
		// buffer back to the beginning.
		int testLen = strlen(test);
		for (int i=0; i<ESP8266_RX_BUFFER_LEN; i++)
		{
			
		}
	}
}

ESP8266Class esp8266;
