/******************************************************************************
SparkFunESP8266Client.h
ESP8266 WiFi Shield Library Client Header File
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

#ifndef ESP8266CLIENT_H
#define ESP8266CLIENT_H

#include <Arduino.h>
#include <SoftwareSerial.h>
#include <IPAddress.h>
#include "Client.h"
#include "ESP8266WiFi.h"

#define BUFFER_IPD_SIZE 15
#define BUFFER_PRINT_SIZE 16

class ESP8266Client : public Client {
	
public:
	ESP8266Client();
	ESP8266Client(uint8_t sock);

	uint8_t status();
	
	virtual int connect(IPAddress ip, uint16_t port);
	virtual int connect(const char *host, uint16_t port);
	
	int connect(IPAddress ip, uint16_t port, uint32_t keepAlive);
	int connect(String host, uint16_t port, uint32_t keepAlive = 0);
	int connect(const char *host, uint16_t port, uint32_t keepAlive);
	
	virtual size_t write(uint8_t);
	virtual size_t write(const uint8_t *buf, size_t size);
	virtual int available();
	virtual int read();
	virtual int read(uint8_t *buf, size_t size);
	virtual int peek();
	virtual void flush();
	virtual void stop();
	virtual uint8_t connected();
	virtual operator bool();

	friend class WiFiServer;

	using Print::write;
	using Print::print;
	using Print::println;

	size_t print(const __FlashStringHelper *s);
	size_t println(const __FlashStringHelper *s);

private:
	static uint16_t _srcport;
	uint16_t  _socket;
	bool ipMuxEn;

	//variable to clean output
	bool _receivedIPD; //indica se ho già ricevuto l'header IPD
	char _bufferIPD[BUFFER_IPD_SIZE]; //contine l'header IPD
	int _bufferIPDIndex; //indice del buffer IPD

	int _bodyLen; //lunghezza del body della risposta
	int _bodyPos; //posizione di lettura del buffer di IPD
	

	uint8_t getFirstSocket();
};

#endif