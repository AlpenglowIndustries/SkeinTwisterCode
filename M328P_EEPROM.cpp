
#include "M328P_EEPROM.h"
#include <avr/io.h>
#include <avr/interrupt.h>

#define	ADDR_FIRSTBLOCK		0x0002
#define BLOCKSIZE		6
#define MAXWRITES		65535

void eeWriteByte (uint16_t addy, uint8_t eedata)  {
	cli();
	while (EECR & (1 << EEPE));				// wait for occurring write to end (EEPE will be set)
	while (SPMCSR & (1 << SPMEN));			// wait for any flash programming to end
	EEAR = addy;
	EEDR = eedata;
	EECR &= ~((1 << EEPM1) | (1 << EEPM0));	// atomic erase & write operation
	EECR |= (1 << EEMPE);					// master enable for writing, must be set first
	EECR |= (1 << EEPE);					// triggers write
	sei();
}

uint8_t eeReadByte (uint16_t addy)  {
	cli();
	while (EECR & (1 << EEPE));		// wait for occurring write to end (EEPE will be set)
	EEAR = addy;
	EECR |= (1 << EERE);			// read enable
	sei();
	return EEDR;
}

// Writes a uint16_t to EEPROM, LSbyte first
void eeWrite16 (uint16_t addy, uint16_t eedata) {
	uint8_t i = 0;
	for (i = 0; i < 2; i++) {
		uint8_t eedata8 = (uint8_t) (eedata >> (i*8));
		eeWriteByte(addy+i, eedata8);
	}
}

// Reads a uint16_t from EEPROM, MSbyte first
uint16_t eeRead16 (uint16_t addy) {
	uint16_t eedata = 0;
	uint8_t i = 0;
	for (i = 1; i < 255; i--) {
		uint8_t eedata8 = eeReadByte(addy + i);
		eedata |= eedata8;
		eedata = (eedata << (i*8));
	}
	return eedata;
}

// Writes a uint32_t to EEPROM, LSbyte first
void eeWrite32 (uint16_t addy, uint32_t eedata) {
	uint8_t i = 0;
	for (i = 0; i < 4; i++) {
		uint8_t eedata8 = (uint8_t) (eedata >> (i*8));
		eeWriteByte(addy+i, eedata8);
	}	
}

// Reads a uint32_t from EEPROM, MSbyte first
uint32_t eeRead32 (uint16_t addy) {
	uint32_t eedata = 0;
	uint32_t eedata8 = 0;
	uint8_t i = 0;
	for (i = 3; i < 255; i--) {
		eedata8 = eeReadByte(addy + i);
		eedata |= (eedata8 << (i*8));
	}
	return eedata;
}

// Erases EEPROM from beginning to address specified
// "Erase" means filling with 0xFF
void eeErase (uint16_t addy) {
	uint8_t i = 0;
	for (i = 0; i <= addy; i++) {
		eeWriteByte(i, 0xFF);
	}
}

// Clears EEPROM starting at a certain address, extending through a specific number of bytes
// "Clear" means filling with 0x00
void eeClearNext (uint16_t addy, uint8_t numBytes) {
	uint8_t i = 0;
	for (i = 0; i < numBytes; i++) {
		eeWriteByte(addy, 0x00);
		addy++;		
	}
}

// if completely blank EEPROM, initializes address & first block
void eeInit (void) {
	uint16_t addyvalue = eeRead16(ADDR_CURRENTADDY);
	if (addyvalue == 0xFFFF) {
		eeWrite16 (ADDR_CURRENTADDY, ADDR_FIRSTBLOCK);
		eeClearNext (ADDR_FIRSTBLOCK, BLOCKSIZE);
	}
}

// writes time to EEPROM
// argument: elapsed time of current run in ms
// reads existing accumulated time (stored in tenths of seconds) and adds elapsed time
// checks the count first, increments the block if over the max # of writes
// writes new total time in tenths of seconds
void saveTime (uint32_t time) {
	uint16_t addy = eeRead16(ADDR_CURRENTADDY);
	uint16_t count = eeRead16(addy);
	uint32_t totalTime = eeRead32(addy+2);
	totalTime += time/100;
	if (count < MAXWRITES) {
		eeWrite16(addy, count+1);
		eeWrite32(addy+2, totalTime);
	}
	else {
		count = 1;
		addy += BLOCKSIZE;
		eeWrite16(ADDR_CURRENTADDY, addy);
		eeClearNext(addy, BLOCKSIZE);
		eeWrite16(addy, count);
		eeWrite32(addy+2, totalTime);
	}
}

