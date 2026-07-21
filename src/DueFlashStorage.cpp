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

byte DueFlashStorage::read(uint32_t address) const {
  return FLASH_START[address];
}

uint16_t DueFlashStorage::read16(uint32_t address) const {
  return *((const uint16_t *)(&FLASH_START[address]));
}
uint32_t DueFlashStorage::read32(uint32_t address) const {
  return *((const uint32_t *)(&FLASH_START[address]));
}
uint64_t DueFlashStorage::read64(uint32_t address) const {
  return *((const uint64_t *)(&FLASH_START[address]));
}

// return dest_address on success or nullptr on failure.
byte *DueFlashStorage::read(byte *dest_address, uint32_t dataLength, uint32_t flash_address) const {
  memcpy(dest_address, &FLASH_START[flash_address], dataLength);
  return dest_address;
}

const byte* DueFlashStorage::readAddress(uint32_t address) const {
  return FLASH_START + address;
}

uint32_t DueFlashStorage::getOffset(const byte* address) const {
  return address - FLASH_START;
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

bool DueFlashStorage::validateAddress(uint32_t address, uint32_t dataLength) {

  if (FLASH_START + address < getFirstFreeBlock()) {
    flash_debug(2, "Flash write address too low.");
    return false;
  }

  if (address >= IFLASH0_SIZE + IFLASH1_SIZE) {
    flash_debug(2, "Flash write address too high.");
    return false;
  }

  if (address + dataLength > IFLASH0_SIZE + IFLASH1_SIZE) {
    flash_debug(2, "Attempt to write past Flash boundary.");
    return false;
  }

  return true;
}

boolean DueFlashStorage::write(uint32_t address, const byte* data, uint32_t dataLength, bool with_locking) {
  uint32_t retCode;

  if (!validateAddress(address, dataLength)) {
    return false;
  }

  if (address < IFLASH0_SIZE && address + dataLength > IFLASH0_SIZE) {
    // A write across the boundary of the flash pages requires two calls
    const uint32_t lowerSize = IFLASH0_SIZE - address;
    boolean ret = write(address, data, lowerSize);
    ret &= write(IFLASH0_SIZE, data + lowerSize, dataLength - lowerSize);
    return ret;
  }

  // Unlock page
  const bool applicationCodeExtendsIntoSecondFlash = (getOffset(getFirstFreeBlock()) >= IFLASH0_SIZE);
  const bool needToDisableInterrupts = ((address < IFLASH0_SIZE) || applicationCodeExtendsIntoSecondFlash);
  if (needToDisableInterrupts) {
    flash_debug(0, "Must disable interrupts when writing to flash; this is done automatically.");
    noInterrupts();
  }

  bool result = true;
  if (with_locking) {
    retCode = flash_unlock((uint32_t)FLASH_START + address, (uint32_t)FLASH_START + address + dataLength - 1, 0, 0);
    if (retCode != FLASH_RC_OK) {
      flash_debug(2, "Failed to unlock flash for write.");
      result = false;
    }
  }

  // write data
  if (result) {
    // always try a write WITHOUT erase first:
    retCode = flash_write((uint32_t)FLASH_START + address, data, dataLength, 0);
    if (retCode != FLASH_RC_OK) {
      flash_debug(1, "Flash write sans erase failed; retrying with erase.");

      retCode = flash_write((uint32_t)FLASH_START + address, data, dataLength, 1);
    }

    if (retCode != FLASH_RC_OK) {
      flash_debug(2, "Flash write failed.");
      result = false;
    }
  }

  // Lock page
  if (with_locking) {
    // always re-lock page when previously unlock was attempted above, whether that one succeeded or failed is irrelevant.
    retCode = flash_lock((uint32_t)FLASH_START + address, (uint32_t)FLASH_START + address + dataLength - 1, 0, 0);
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

