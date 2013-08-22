/*
 * JeeNodeRfm12B.cpp
 * 
 * Demo program for testing the RFM12B library on a Jeenode
 *
 * This software is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 * 
 * Created: 8/20/2013 
 * @author: Bollenbacher Software Engineering
 */ 

#define F_CPU 16000000L

#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>		
#include <stdio.h>
#include <string.h>
#include <avr/interrupt.h>
#include "pin.h"
#include "serial.h"
#include "rfm12b.h"

#define RED_LED_PIN PIN_INFO(PORT_C, PIN_1, OUTPUT_MODE, ACTIVE_LOW)    // jeenode - port2 red LED
#define GREEN_LED_PIN PIN_INFO(PORT_D, PIN_5, OUTPUT_MODE, ACTIVE_LOW)  // jeenode - port2 green LED

uint8_t StrBfr[50];
unsigned char Input[MAX_PACKET_SIZE+1];
Rfm12b Radio;

void DisplayInput () {
	unsigned char Length, i;
		
	Length = Radio.Recv (&Input[0]);
	if (Length > 0) {
		sprintf ((char*) StrBfr, "l=%d, Rcvd= ", Length);
		SendString (StrBfr);
		for (i=0; i < Length; i++) {
			sprintf ((char*) StrBfr, " %x", Input[i]);
			SendString (StrBfr);
			}
		SendString (UI8_P("\r\n"));
		SetPinActive (GREEN_LED_PIN);
		_delay_ms (250);
		SetPinInActive (GREEN_LED_PIN);
		}
	}

int main (void) {
	int Count = 0;
	unsigned char Char;
		
	InitPinInActive (RED_LED_PIN);
	InitPinInActive (GREEN_LED_PIN);
	
	SerialInit (57600);

	SendString (UI8_P("JeeNodeRfm12B (1.1 wcb)\r\n"));
	SendString (UI8_P("(s)end, (r)eset\r\n"));
	
	Radio.Initialize ();
	
	while(1) {
		if ((Char = ReadChar())) {
			if (Char == 'r') {
				SendString (UI8_P("RESETING!\r\n"));
				cli();                  // irq's off
				wdt_enable (WDTO_15MS); // watch dog on, 15ms
				while(1);               // loop until watch dog fires
				}
			else if (Char == 's') {
			    sprintf ((char*) StrBfr, "%3d", Count);
                if (Radio.Send (StrBfr, strlen((char*) StrBfr))) 
				    SendStringAndInt (UI8_P("Sent data "), Count++, UI8_P("\r\n"));
                else 
				    SendStringAndInt (UI8_P("Could not send data "), Count++, UI8_P("\r\n"));
				SetPinActive (GREEN_LED_PIN);
				_delay_ms (100);
				SetPinInActive (GREEN_LED_PIN);
				}
			}
		//Radio.DisplayStatus ();
		//_delay_ms (1000);
		DisplayInput ();
		}
	}