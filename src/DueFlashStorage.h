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
	// address is the offset from the start of 'available space' 
	// as specified by `getFirstFreeBlock()`.
	// data is a pointer to the data to be written
	// dataLength is length of data in bytes

	byte read8(uint32_t address);
	uint16_t read16(uint32_t address);
	uint32_t read32(uint32_t address);
	uint64_t read64(uint32_t address);

	byte read8_at_addr(const byte *flash_address);
	uint16_t read16_at_addr(const byte *flash_address);
	uint32_t read32_at_addr(const byte *flash_address);
	uint64_t read64_at_addr(const byte *flash_address);

    template <typename T>
	inline byte *read(T *dest, uint32_t flash_address) {
	  const byte *ptr = readAddress(flash_address);
  	  return read_at_addr((byte *)dest, sizeof(T), ptr);
	}

    template <typename T>
	inline byte *read(T &dest, uint32_t flash_address) {
	  const byte *ptr = readAddress(flash_address);
  	  return read_at_addr((byte *)&dest, sizeof(T), ptr);
	}

    template <typename T>
	inline byte *read_at_addr(T *dest, const byte* flash_address) {
  	  return read_at_addr((byte *)dest, sizeof(T), flash_address);
	}

    template <typename T>
	inline byte *read_at_addr(T &dest, const byte* flash_address) {
  	  return read_at_addr((byte *)&dest, sizeof(T), flash_address);
	}

	// return dest_address on success or nullptr on failure.
	byte *read(byte *dest_address, uint32_t dataLength, uint32_t flash_address) {
	  const byte *ptr = readAddress(flash_address);
  	  return read_at_addr(dest_address, dataLength, ptr);
	}
	byte *read_at_addr(byte *dest_address, uint32_t dataLength, const byte *flash_address);

	// This returns the physical address of the given flash offset. 
	// 0 returns the start of the available data space in the flash, as produced by
	// `getFirstFreeBlock()`.
	const byte* readAddress(uint32_t address);

	// Return the flash offset for the given physical address.
	uint32_t getOffset(const byte* address);

	// This returns the physical address of the free flash memory after the program. It is retrieved from the linker map.
	// Writing to any address below the value returned by this function is likely going to corrupt the program memory and
	// then crash the CPU.
	const byte* getFirstFreeBlock();

	uint32_t getAvailableFlashSize();

	// Test if the given offset is within the freely available space in the Flash.
	//
	// We DO NOT permit overwriting any application code or data, so the first available
	// address offset would be (getFirstFreeBlock() i.e. 0).
	bool validateAddress_at_addr(const byte* address, uint32_t dataLength = 1);
	bool validateAddress(uint32_t address, uint32_t dataLength = 1) {
	  const byte *ptr = readAddress(address);
	  return validateAddress_at_addr(ptr, dataLength);
	}

	inline bool write8(uint32_t address, byte value) {
	  return write(address, value);
	}
	inline bool write16(uint32_t address, uint16_t value) {
	  return write(address, value);
	}
	inline bool write32(uint32_t address, uint32_t value) {
	  return write(address, value);
	}
	inline bool write64(uint32_t address, uint64_t value) {
	  return write(address, value);
	}

	inline bool write8_at_addr(byte *address, byte value) {
	  return write_at_addr(address, value);
	}
	inline bool write16_at_addr(byte *address, uint16_t value) {
	  return write_at_addr(address, value);
	}
	inline bool write32_at_addr(byte *address, uint32_t value) {
	  return write_at_addr(address, value);
	}
	inline bool write64_at_addr(byte *address, uint64_t value) {
	  return write_at_addr(address, value);
	}

	// write a byte or a block to the given offset.
    template <typename T,
          typename std::enable_if<
		    std::is_arithmetic<T>::value
		    && !std::is_pointer<T>::value,
            bool
          >::type = true
      >
	inline bool write(uint32_t address, const T value) {
		return write(address, (const byte *)&value, sizeof(T));
	}
    template <typename T,
          typename std::enable_if<
		    !std::is_arithmetic<T>::value
		    && !std::is_pointer<T>::value,
            bool
          >::type = true
      >
	inline bool write(uint32_t address, const T &value) {
	  return write(address, (const byte *)&value, sizeof(T));
	}
    template <typename T>
	inline bool write(uint32_t address, const T *value) {
	  return write(address, (const byte *)value, sizeof(T));
	}
	inline bool write(uint32_t address, const byte* data, uint32_t dataLength, bool with_locking = true) {
	  byte *ptr = const_cast<byte *>(readAddress(address));
	  return write_at_addr(ptr, data, dataLength, with_locking);
	}

    template <typename T,
          typename std::enable_if<
		    std::is_arithmetic<T>::value
		    && !std::is_pointer<T>::value,
            bool
          >::type = true
      >
	inline bool write_unlocked(uint32_t address, const T value) {
		return write_unlocked(address, (const byte *)&value, sizeof(T));
	}
    template <typename T,
          typename std::enable_if<
		    !std::is_arithmetic<T>::value
		    && !std::is_pointer<T>::value,
            bool
          >::type = true
      >
	inline bool write_unlocked(uint32_t address, const T &value) {
		return write_unlocked(address, (const byte *)&value, sizeof(T));
	}
    template <typename T>
	inline bool write_unlocked(uint32_t address, const T *value) {
		return write_unlocked(address, (const byte *)value, sizeof(T));
	}
	inline bool write_unlocked(uint32_t address, const byte* data, uint32_t dataLength) {
		return write(address, data, dataLength, false);
	}

    template <typename T,
          typename std::enable_if<
		    std::is_arithmetic<T>::value
		    && !std::is_pointer<T>::value,
            bool
          >::type = true
      >
	inline bool write_at_addr(byte* address, T value) {
		return write_at_addr(address, (const byte *)&value, sizeof(T));
	}
    template <typename T,
          typename std::enable_if<
		    !std::is_arithmetic<T>::value
		    && !std::is_pointer<T>::value,
            bool
          >::type = true
      >
	inline bool write_at_addr(byte* address, const T &value) {
		return write_at_addr(address, (const byte *)&value, sizeof(T));
	}
    template <typename T>
	inline bool write_at_addr(byte* address, const T *value) {
		return write_at_addr(address, (const byte *)value, sizeof(T));
	}
	bool write_at_addr(byte* address, const byte* data, uint32_t dataLength, bool with_locking = true);

    template <typename T,
          typename std::enable_if<
		    std::is_arithmetic<T>::value
		    && !std::is_pointer<T>::value,
            bool
          >::type = true
      >
	inline bool write_unlocked_at_addr(byte* address, T value) {
		return write_unlocked_at_addr(address, (const byte *)&value, sizeof(T));
	}
    template <typename T,
          typename std::enable_if<
		    !std::is_arithmetic<T>::value
		    && !std::is_pointer<T>::value,
            bool
          >::type = true
      >
	inline bool write_unlocked_at_addr(byte* address, const T &value) {
		return write_unlocked_at_addr(address, (const byte *)&value, sizeof(T));
	}
    template <typename T>
	inline bool write_unlocked_at_addr(byte* address, const T *value) {
		return write_unlocked_at_addr(address, (const byte *)value, sizeof(T));
	}
	inline bool write_unlocked_at_addr(byte* address, const byte* data, uint32_t dataLength) {
		return write_at_addr(address, data, dataLength, false);
	}
};

#endif
