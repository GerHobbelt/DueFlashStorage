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

#include <type_traits>

// 1Kb of data
#define DATA_LENGTH   ((IFLASH0_PAGE_SIZE / sizeof(byte)) * 4)

//  DueFlash is the main class for flash functions
class DueFlashStorage {
public:
	DueFlashStorage();

	// write() writes the specified amount of data into flash.
	// address is the offset from the flash start where the write should start
	// data is a pointer to the data to be written
	// dataLength is length of data in bytes

	byte read(uint32_t address) const;
	uint16_t read16(uint32_t address) const;
	uint32_t read32(uint32_t address) const;
	uint64_t read64(uint32_t address) const;

	// return dest_address on success or nullptr on failure.
	byte *read(byte *dest_address, uint32_t dataLength, uint32_t flash_address) const;
	byte *read(byte *dest_address, uint32_t dataLength, const byte *flash_address) const {
		uint32_t offset = getOffset(flash_address);
		return read(dest_address, dataLength, offset);
	}

	// This returns the physical address of the given flash offset. 0 returns the start of the flash (which is 0x80000 on the Due)
	const byte* readAddress(uint32_t address) const;

	// Return the flash offset for the given physical address.
	uint32_t getOffset(const byte* address) const;

	// This returns the physical address of the free flash memory after the program. It is retrieved from the linker map.
	// Writing to any address below the value returned by this function is likely going to corrupt the program memory and
	// then crash the CPU.
	const byte* getFirstFreeBlock();

	uint32_t getAvailableFlashSize();

	// Test if the given offset is within the freely available space in the Flash.
	//
	// We DO NOT permit overwriting any application code or data, so the first available
	// address offset would be (getFirstFreeBlock() - FLASH_START).
	bool validateAddress(uint32_t address, uint32_t dataLength = 1);

	// write a byte or a block to the given offset.
    template <typename T,
          typename std::enable_if<
		    std::is_arithmetic<T>::value
            && !(   std::is_reference<T>::value
                 || std::is_pointer<T>::value),
            bool
          >::type = true
      >
	inline bool write(uint32_t address, T value) {
		return write(address, (const byte *)&value, sizeof(T));
	}
    template <typename T,
          typename std::enable_if<
		      !std::is_arithmetic<T>::value
            && std::is_reference<T>::value,
            bool
          >::type = true
      >
	inline bool write(uint32_t address, const T &value) {
		return write(address, (const byte *)&value, sizeof(T));
	}
    template <typename T,
          typename std::enable_if<
            std::is_pointer<T>::value,
            bool
          >::type = true
      >
	inline bool write(uint32_t address, const T *value) {
		return write(address, (const byte *)value, sizeof(T));
	}
	bool write(uint32_t address, const byte* data, uint32_t dataLength, bool with_locking = true);

	template<typename T>
	inline bool write_unlocked(uint32_t address, T value) {
		return write_unlocked(address, &value, sizeof(T));
	}
	inline bool write_unlocked(uint32_t address, const byte* data, uint32_t dataLength) {
		return write(address, data, dataLength, false);
	}

    template <typename T,
          typename std::enable_if<
            !(   std::is_reference<T>::value
              || std::is_pointer<T>::value),
            bool
          >::type = true
      >
	inline bool write2addr(byte* address, T value) {
		return write2addr(address, (const byte *)&value, sizeof(T));
	}
    template <typename T,
          typename std::enable_if<
            std::is_reference<T>::value,
            bool
          >::type = true
      >
	inline bool write2addr(byte* address, const T &value) {
		return write2addr(address, (const byte *)&value, sizeof(T));
	}
    template <typename T,
          typename std::enable_if<
            std::is_pointer<T>::value,
            bool
          >::type = true
      >
	inline bool write2addr(byte* address, const T *value) {
		return write2addr(address, (const byte *)value, sizeof(T));
	}
	inline bool write2addr(byte* address, const byte* data, uint32_t dataLength, bool with_locking = true) {
		uint32_t offset = getOffset(address);
		return write(offset, data, dataLength, with_locking);
	}

	template<typename T>
	inline bool write2addr_unlocked(byte *address, T value) {
		return write2addr_unlocked(address, &value, sizeof(T));
	}
	inline bool write2addr_unlocked(byte* address, const byte* data, uint32_t dataLength) {
		uint32_t offset = getOffset(address);
		return write_unlocked(offset, data, dataLength);
	}
};

#endif
