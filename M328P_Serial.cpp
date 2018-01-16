
#include "M328P_Serial.h"
#include <avr/io.h>
#include <stdlib.h>
#include <stdio.h>

// Asynchronous Serial UART Functions for ATMEGA328P

void SerialInit (uint16_t baudrate) {
// 	uint16_t prescaler = ((F_CPU >> 4) / baudrate) - 1;
// 	UBRR0H = (uint8_t) (prescaler >> 8);		// sets baud rate, no multiplier
// 	UBRR0L = (uint8_t) prescaler;
	UBRR0H = 0;		// sets baud rate, no multiplier
	UBRR0L = 12;
	UCSR0A |= (1 << U2X0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);	// async no parity 1 stop bit (default), 8 data bits
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);	// enables RX and TX
}

uint8_t SerialRXReady (void) {
	return (UCSR0A & (1 << RXC0));	// 1 if there's unread data in receive buffer
}

uint8_t SerialRX (void) {
	return UDR0;
}

uint8_t SerialTXReady (void) {
	return (UCSR0A & (1 << TXC0));	// 1 if the transmit buffer is empty
}

void SerialTX (uint8_t data) {
	while (bit_is_clear(UCSR0A, UDRE0));	// waits until transmit buffer is ready for new data
	UDR0 = data;
}

//unfuck below pointers

void SerialPrint (const char *stringtoprint) {
	uint8_t i = 0;
	while (stringtoprint[i]) {
		SerialTX (stringtoprint[i]);
		i++;
	}
}

void SerialPrint (uint8_t byte) {
	char theString[4];
	sprintf(theString, "%u", byte);
	SerialPrint(theString);
}

void SerialPrint (uint16_t twobytes) {
	char theString[6];
	sprintf(theString, "%u", twobytes);
	SerialPrint(theString);
}

void SerialPrint (uint32_t fourbytes) {
	char theString[11];
	sprintf(theString, "%lu", fourbytes);
	SerialPrint(theString);
}