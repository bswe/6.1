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
#include "wireless_packet.h"

#define RED_LED_PIN PIN_INFO(PORT_C, PIN_1, OUTPUT_MODE, ACTIVE_LOW)    // jeenode - port2 red LED
#define GREEN_LED_PIN PIN_INFO(PORT_D, PIN_5, OUTPUT_MODE, ACTIVE_LOW)  // jeenode - port2 green LED

uint8_t StrBfr[50], PacketBfr[MAX_MAC_PACKET_SIZE];
uint8_t Input[MAX_MAC_PACKET_SIZE];
Rfm12b Radio;
Mac MacLayer;


void DisplayInput () {
	uint8_t Length, i;
		
	Length = Input[0];
	sprintf ((char*) StrBfr, "l=%d, Rcvd= ", Length);
	SendString (StrBfr);
	for (i=0; i < Length; i++) {
		sprintf ((char*) StrBfr, " %x", Input[i]);
		SendString (StrBfr);
		}
	SendString (UI8_P("\r\n"));
    //Input[Length++] = '\r';
    //Input[Length++] = '\n';
    //Input[Length] = 0;
    //SendString (Input);
	}


uint8_t SendPacket (uint8_t* Packet, uint8_t SequenceNumber) {
    #define MAX_ATTEMPTS 50
    int8_t AttemptCount = 0, BytesSent;
    
    while (MAX_ATTEMPTS > AttemptCount++) {
        if ((BytesSent = Radio.Send (Packet))) {
    	    SendStringAndInt (UI8_P("Sent data "), SequenceNumber, UI8_P("\r\n"));
            return BytesSent;
            }            
        _delay_ms (100);    // TODO: change this to a sleep
        }        
    SendStringAndInt (UI8_P("Could not send data "), SequenceNumber, UI8_P("\r\n"));
    return 0;
    }
    

int main (void) {
    uint8_t SequenceNumber = 0;
	uint8_t Char;
    int8_t NumberSent = 0, NumberToSend = 0, LoopCount = 0, UnexpectedPacketCount = 0, NoAckCount = 0;
    		
	InitPinInActive (RED_LED_PIN);
	InitPinInActive (GREEN_LED_PIN);
	
	SerialInit (57600);

	SendString (UI8_P("JeeNodeRfm12B (1.8 wcb)\r\n"));
	SendString (UI8_P("a(uto-test), e(nd-test), (r)eset, [n](s)end\r\n"));
	
	Radio.Initialize ();
	
	while(1) {
		if ((Char = ReadChar())) 
			switch (Char) {
                case 'a':
                    if (NumberToSend == -1)
                        break;
                    SendString (UI8_P("Starting auto-test\r\n"));
                    NumberToSend = -1;
                    SequenceNumber = MacLayer.MakeRequestPacket (PacketBfr, 1, 2, UI8_P("123"), 3);
                    SendPacket (PacketBfr, SequenceNumber);
                    LoopCount = UnexpectedPacketCount = NoAckCount = 0;
	                SetPinActive (GREEN_LED_PIN);
                    break;
                case 'e':
                    if (NumberToSend == 0)
                        break;
                    SendString (UI8_P("Ending auto-test\r\n"));
                    sprintf ((char*) StrBfr, "unexpected # = %d, no_ack # =  %d\r\n", UnexpectedPacketCount, NoAckCount);
                    SendString (StrBfr);
                    NumberToSend = 0;
                    break;
                case 'r': 
				    SendString (UI8_P("RESETING!\r\n"));
				    cli();                  // irq's off
				    wdt_enable (WDTO_15MS); // watch dog on, 15ms
				    while(1);               // loop until watch dog fires
			    case 's':
                    if (NumberToSend <= 0)
                        NumberToSend = 1;
                    SendStringAndInt (UI8_P("Sending "), NumberToSend, UI8_P(" packets\r\n"));
                    NumberSent = -1;
                    while (NumberToSend > ++NumberSent) {
                        SequenceNumber = MacLayer.MakeRequestPacket (PacketBfr, 1, 2, UI8_P("123"), 3);
			            if (0 == SendPacket (PacketBfr, SequenceNumber)) {
                            NumberSent = -1;
    			            break;
			                }
                        }
                    if (NumberToSend == NumberSent) {
    			        SetPinActive (GREEN_LED_PIN);
    			        _delay_ms (100);
    			        SetPinInActive (GREEN_LED_PIN);
                        } 
                    NumberToSend = 0;
                    break;
                default :
                    if ((0x30 <= Char) && (Char <= 0x39))
                        NumberToSend = (NumberToSend * 10) + Char - 0x30;
                    else {
    			        sprintf ((char*) StrBfr, "Unexpected input character %c\r\n", Char);
    			        SendString (StrBfr);
                        }                        
                }                    
		if (Radio.Recv (&Input[0]) > 0) {
		    DisplayInput ();
            if ((Input[PACKET_TYPE_INDEX] == MAC_RESPONSE_PACKET_TYPE) &&
                (Input[PACKET_SEQUENCE_INDEX] == SequenceNumber)) {
    			sprintf ((char*) StrBfr, "Got ack response for %d\r\n", SequenceNumber);
    			SendString (StrBfr);
                if (NumberToSend < 0) {
                    _delay_ms (100);
                    SequenceNumber = MacLayer.MakeRequestPacket (PacketBfr, 1, 2, UI8_P("123"), 3);
                    SendPacket (PacketBfr, SequenceNumber);
	                SetPinActive (GREEN_LED_PIN);
                    LoopCount = 0;
                    }
		        //Radio.DisplayStatus ();
                //Radio.ResetStatus();
                //NumberSent = 0;
                }
            else if (Input[PACKET_TYPE_INDEX] == MAC_REQUEST_PACKET_TYPE) {
                sprintf ((char*) StrBfr, "Got request packet with sequence %d\r\n", Input[PACKET_SEQUENCE_INDEX]);
                SendString (StrBfr);
	            SetPinActive (GREEN_LED_PIN);
                LoopCount = 0;
                SequenceNumber = Input[PACKET_SEQUENCE_INDEX];
                MacLayer.MakeResponsePacket (PacketBfr, 1, 2, SequenceNumber, UI8_P("123"), 3);
			    if (0 == SendPacket (PacketBfr, SequenceNumber)) 
    			    SendStringAndInt (UI8_P("Could not send response for "), Input[PACKET_SEQUENCE_INDEX], UI8_P("\r\n"));
                }
            else {
    			sprintf ((char*) StrBfr, "\r\nGot unexpected packet !!!!!!!!!!!!!!!!!!!!!!\r\n\n");
    			SendString (StrBfr);
                UnexpectedPacketCount++;
                }                                
            }
        _delay_ms (10);
        LoopCount++;
        if (LoopCount == 10)
            SetPinInActive (GREEN_LED_PIN);
        else if ((NumberToSend < 0) && (LoopCount == 100)) {
    	    SendStringAndInt (UI8_P("!!!!!!!!!!!!!!!!!!!!!!! Did not get Ack for "), 
                              SequenceNumber, 
                              UI8_P(" !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n"));
            NoAckCount++;
            SendPacket (PacketBfr, SequenceNumber);
            LoopCount = 0;
	        SetPinActive (GREEN_LED_PIN);
            }
        //else if (NumberSent > 0) {
		    //Radio.DisplayStatus ();
            //Radio.ResetStatus();
            //NumberSent = 0;
            //}            
		}
	}