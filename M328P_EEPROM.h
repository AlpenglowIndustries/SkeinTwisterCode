#include <avr/io.h>

#ifndef _M328P_EEPROM_H_
#define _M328P_EEPROM_H_

#define ADDR_MODE0		 0
#define ADDR_MODE2		 2
#define ADDR_CURRENTADDY 4

// General EEPROM read/write/clear functions
void eeWriteByte (uint16_t addy, uint8_t eedata);
uint8_t eeReadByte (uint16_t addy);
void eeWrite16 (uint16_t addy, uint16_t eedata);
uint16_t eeRead16 (uint16_t addy);
void eeWrite32 (uint16_t addy, uint32_t eedata);
uint32_t eeRead32 (uint16_t addy);
void eeErase (uint16_t addy);

// Specific functions to SkeinTwister
void eeClearNext (uint16_t addy, uint8_t bytes);
void eeInit (void);
void saveTime (uint32_t time);
void eeDump (void);

#endif

// SkeinTwister EEPROM structure
// 0x00 - 0x01: 2: Cal Value of Mode0
// 0x02 - 0x03: 2: Cal Value of Mode2
// 0x04 - 0x05: 2: Address of current block
// 0x06 - 0x07: 2: Block A, Counter
// 0x08 - 0x0B: 4: Block A, Time
// 0x0C - 0x11: 6: Block B....
// 0x12 - 0x17: 7: Block C....