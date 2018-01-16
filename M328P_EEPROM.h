#include <avr/io.h>

#ifndef _M328P_EEPROM_H_
#define _M328P_EEPROM_H_

#define ADDR_CURRENTADDY 0x0000

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

#endif

// SkeinTwister EEPROM structure
// 0x00 - 0x01: 2: Address of current block
// 0x02 - 0x03: 2: Block A, Counter
// 0x04 - 0x07: 4: Block A, Time
// 0x08 - 0x0D: 6: Block B....
// 0x0E - 0x13: 7: Block C....