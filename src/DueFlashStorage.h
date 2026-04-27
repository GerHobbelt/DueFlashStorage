/* 
DueFlashStorage saves non-volatile data for Arduino Due.
The library is made to be similar to EEPROM library
Uses flash block 1 per default.

Note: uploading new software will erase all flash so data written to flash
using this library will not survive a new software upload. 

Inspiration from Pansenti at https://github.com/Pansenti/DueFlash
Rewritten and modified by Sebastian Nilsson
*/


#ifndef DUEFLASHSTORAGE_H
#define DUEFLASHSTORAGE_H

#include <Arduino.h>
#include "flash_efc.h"
#include "efc.h"

// 1Kb of data
#define DATA_LENGTH   ((IFLASH1_PAGE_SIZE / sizeof(byte)) * 4)

// choose a start address that's offset to show that it doesn't have to be on a page boundary
#define FLASH_START   ((byte*)IFLASH0_ADDR)

//  DueFlash is the main class for flash functions
class DueFlashStorage {
public:
	DueFlashStorage();

	// write() writes the specified amount of data into flash.
	// address is the offset from the flash start where the write should start
	// data is a pointer to the data to be written
	// dataLength is length of data in bytes

	byte read(uint32_t address);

	// This returns the physical address of the given flash offset. 0 returns the start of the flash (which is 0x80000 on the Due)
	const byte* readAddress(uint32_t address);

	// Return the flash offset for the given physical address.
	uint32_t getOffset(const byte* address);

	// This returns the physical address of the free flash memory after the program. It is retrieved from the linker map.
	// Writing to any address below the value returned by this function is likely going to corrupt the program memory and
	// then crash the CPU.
	static const byte* getFirstFreeBlock();

	static inline uint32_t getAvailableFlashSize() {
		return IFLASH0_SIZE + IFLASH1_SIZE - (getFirstFreeBlock() - FLASH_START);
	}

	// Test if the given offset is within the freely available space in the Flash.
	//
	// We DO NOT permit overwriting any application code or data, so the first available
	// address offset would be (getFirstFreeBlock() - FLASH_START).
	bool validateAddress(uint32_t address, uint32_t dataLength = 1) const;

	// These write methods write a byte or a block to the given offset.
	inline boolean write(uint32_t address, byte value) {
		return write(address, &value, 1);
	}
	inline boolean write(uint32_t address, const byte* data, uint32_t dataLength) {
		return write(address, data, dataLength, true);
	}

	boolean write(uint32_t address, const byte* data, uint32_t dataLength, bool with_locking);

	inline boolean write_unlocked(uint32_t address, byte value) {
		return write_unlocked(address, &value, 1);
	}
	inline boolean write_unlocked(uint32_t address, const byte* data, uint32_t dataLength) {
		return write(address, data, dataLength, false);
	}

	// This writes directly to the given address. It must be in flash.
	inline boolean write(byte* address, const byte* data, uint32_t dataLength) {
		uint32_t offset = getOffset(address);
		return write(offset, data, dataLength);
	}
};

#endif
