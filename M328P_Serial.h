

#include <avr/io.h>

#ifndef _M328P_SERIAL_H_
#define _M328P_SERIAL_H_

#define F_CPU 1000000ul

void SerialInit (uint16_t baudrate);
uint8_t SerialRXReady (void);
uint8_t SerialGrabByte (void);
uint8_t SerialRX (void);

uint8_t SerialTXReady (void);
void SerialTX (uint8_t data);
void SerialPrint (const char *stringtoprint);
void SerialPrint (char byte);
void SerialPrint (uint8_t byte);
void SerialPrint (uint16_t twobytes);
void SerialPrint (uint32_t fourbytes);
void SerialPrintln (const char *stringtoprint);

#endif