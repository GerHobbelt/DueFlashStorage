#include "DueFlashStorage.h"

// choose a start address that's offset to show that it doesn't have to be on a page boundary
#define FLASH_START   ((byte*)IFLASH0_ADDR)

DueFlashStorage::DueFlashStorage() {
  uint32_t retCode;

  /* Initialize flash: 6 wait states for flash writing. */
  retCode = flash_init(FLASH_ACCESS_MODE_128, 6);
  if (retCode != FLASH_RC_OK) {
    flash_debug(2, "Flash init failed.");
  }
}

byte DueFlashStorage::read8(uint32_t address) {
  return *readAddress(address);
}

uint16_t DueFlashStorage::read16(uint32_t address) {
  return *((const uint16_t *)readAddress(address));
}
uint32_t DueFlashStorage::read32(uint32_t address) {
  return *((const uint32_t *)readAddress(address));
}
uint64_t DueFlashStorage::read64(uint32_t address) {
  return *((const uint64_t *)readAddress(address));
}

// return dest_address on success or nullptr on failure.
byte *DueFlashStorage::read(byte *dest_address, uint32_t dataLength, const byte* flash_address) {

  Serial.print("DueFlashStorage::");
  Serial.print(__FUNCTION__);
  Serial.print("(");
  Serial.print((intptr_t)dest_address, HEX);
  Serial.print(",");
  Serial.print(dataLength);
  Serial.print(",");
  Serial.print((intptr_t)flash_address, HEX);
  Serial.println(")");

  memcpy(dest_address, flash_address, dataLength);
  return dest_address;
}

const byte* DueFlashStorage::readAddress(uint32_t address) {
  return getFirstFreeBlock() + address;
}

uint32_t DueFlashStorage::getOffset(const byte* address) {
  return address - getFirstFreeBlock();
}

extern "C" unsigned char _etext;
extern "C" unsigned char _srelocate;
extern "C" unsigned char _erelocate;

// See https://arduino.stackexchange.com/questions/83911/how-do-i-get-the-size-of-my-program-at-runtime/83916#83916 for
// an explanation of this function
const byte* DueFlashStorage::getFirstFreeBlock() {
  const byte* rom_end = &_etext + (&_erelocate - &_srelocate);
  constexpr const uint32_t page_size_minus_1 = IFLASH1_PAGE_SIZE - 1;
  rom_end = (const byte*)(((intptr_t)rom_end + page_size_minus_1) & ~page_size_minus_1);  // Align to next free flash block (even if the memory ends right on a boundary)
  return rom_end;
}

uint32_t DueFlashStorage::getAvailableFlashSize() {
    return IFLASH0_SIZE + IFLASH1_SIZE - (getFirstFreeBlock() - FLASH_START);
}

bool DueFlashStorage::validateAddress(const byte* address, uint32_t dataLength) {

#if 01
  if (address < getFirstFreeBlock()) {
    flash_debug(2, "Flash write address too low: negative address offsets not allowed.");
    return false;
  }
#else
  if (address < FLASH_START) {
    flash_debug(2, "Flash write address too low: outside flash memory space.");
    return false;
  }
#endif

  if (address >= FLASH_START + IFLASH0_SIZE + IFLASH1_SIZE) {
    flash_debug(2, "Flash write address too high.");
    return false;
  }

  if (address + dataLength > FLASH_START + IFLASH0_SIZE + IFLASH1_SIZE) {
    flash_debug(2, "Attempt to write past Flash boundary.");
    return false;
  }

  return true;
}

bool DueFlashStorage::write2addr(byte* address, const byte* data, uint32_t dataLength, bool with_locking) {
  uint32_t retCode;

  Serial.print("DueFlashStorage::");
  Serial.print(__FUNCTION__);
  Serial.print("(");
  Serial.print((intptr_t)address, HEX);
  Serial.print(",");
  Serial.print((intptr_t)data, HEX);
  Serial.print(",");
  Serial.print(dataLength);
  Serial.print(",");
  Serial.print(with_locking);
  Serial.println(")");

  if (!validateAddress(address, dataLength)) {
    return false;
  }

  if (address < FLASH_START + IFLASH0_SIZE && address + dataLength > FLASH_START + IFLASH0_SIZE) {
    // A write across the boundary of the flash pages requires two calls
    const uint32_t lowerSize = FLASH_START + IFLASH0_SIZE - address;
    bool ret = write2addr(address, data, lowerSize, with_locking);
    ret &= write2addr(FLASH_START + IFLASH0_SIZE, data + lowerSize, dataLength - lowerSize, with_locking);
    return ret;
  }

  // Unlock page
  const bool applicationCodeExtendsIntoSecondFlash = (getFirstFreeBlock() >= FLASH_START + IFLASH0_SIZE);
  const bool needToDisableInterrupts = ((address < FLASH_START + IFLASH0_SIZE) || applicationCodeExtendsIntoSecondFlash);
  if (needToDisableInterrupts) {
    flash_debug(0, "Must disable interrupts when writing to flash; this is done automatically.");
    noInterrupts();
  }

  bool result = true;
  if (with_locking) {
    retCode = flash_unlock((uint32_t)address, (uint32_t)address + dataLength - 1, 0, 0);
    if (retCode != FLASH_RC_OK) {
      flash_debug(2, "Failed to unlock flash for write.");
      result = false;
    }
  }

  // write data
  if (result) {
    // always try a write WITHOUT erase first:
    retCode = flash_write((uint32_t)address, data, dataLength, 0);
    if (retCode != FLASH_RC_OK) {
      flash_debug(1, "Flash write sans erase failed; retrying with erase.");

      retCode = flash_write((uint32_t)address, data, dataLength, 1);
    }

    if (retCode != FLASH_RC_OK) {
      flash_debug(2, "Flash write failed.");
      result = false;
    }
  }

  // Lock page
  if (with_locking) {
    // always re-lock page when previously unlock was attempted above, whether that one succeeded or failed is irrelevant.
    retCode = flash_lock((uint32_t)address, (uint32_t)address + dataLength - 1, 0, 0);
    if (retCode != FLASH_RC_OK) {
      flash_debug(2, "Failed to lock flash page.");
      result = false;
    }
  }

  if (needToDisableInterrupts) {
    interrupts();
  }

  return result;
}

// ----------------------------------------------------------------------------------------------------

WEAK 
void flash_debug(int level, const char *message) {
  Serial.print("DueFlashDebug: level ");
  Serial.print(level);
  Serial.print(": ");
  Serial.print(message);
  Serial.println();
}

