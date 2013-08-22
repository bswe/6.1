/*
 * Hello_World.cpp
 *
 * Author: Bollenbacher Software Engineering
 * Created: 7/14/13
 */ 

#define F_CPU 16000000L

#include <avr/io.h>
#include <util/delay.h>		// Including the delay lib
#include "pin.h"			// Header file for pin defines and functions
#include "serial.h"			// Header for Serial class

int main(void) {
	int Count = 0;
	Pin LedPin = {PORT_B, PIN_5, OUTPUT_MODE, HIGH};  // arduino - board LED
	//Pin LedPin = {PORT_C, PIN_1, OUTPUT_MODE, LOW};   // jeenode - port2 red LED
	//Pin LedPin = {PORT_D, PIN_5, OUTPUT_MODE, LOW};   // jeenode - port2 green LED
	
	SetPinMode (&LedPin);
	
	SerialInit (57600);
	
	SendString ("Hello World (2.6 wcb)\n");

    while(1) {
		ActivatePin (&LedPin);
        SendString ("OH, HELLO WORLD for the ");
        SendInt (++Count);
        SendString ("th time!\n");
		_delay_ms (2000);	
		DeactivatePin (&LedPin);
		_delay_ms (500);
        }
    }