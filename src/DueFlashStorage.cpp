#include "DueFlashStorage.h"

DueFlashStorage::DueFlashStorage() {
  uint32_t retCode;

  /* Initialize flash: 6 wait states for flash writing. */
  retCode = flash_init(FLASH_ACCESS_MODE_128, 6);
  if (retCode != FLASH_RC_OK) {
    flash_debug(2, "Flash init failed\n");
  }
}

byte DueFlashStorage::read(uint32_t address) {
  return FLASH_START[address];
}

byte* DueFlashStorage::readAddress(uint32_t address) {
  return FLASH_START + address;
}

uint32_t DueFlashStorage::getOffset(byte* address) {
  return address - FLASH_START;
}

extern "C" unsigned char _etext;
extern "C" unsigned char _srelocate;
extern "C" unsigned char _erelocate;

// See https://arduino.stackexchange.com/questions/83911/how-do-i-get-the-size-of-my-program-at-runtime/83916#83916 for
// an explanation of this function
byte* DueFlashStorage::getFirstFreeBlock() {
  byte* rom_end = &_etext + (&_erelocate - &_srelocate);
  rom_end = (byte*)(((uint32_t)rom_end + 255) & ~0xFF);  // Align to next free flash block (even if the memory ends right on a boundary)
  return rom_end;
}

bool DueFlashStorage::validateAddress(uint32_t address, uint32_t dataLength) const {

  if (FLASH_START + address < getFirstFreeBlock()) {
    flash_debug(2, "Flash write address too low\n");
    return false;
  }

  if (address >= IFLASH0_SIZE + IFLASH1_SIZE) {
    flash_debug(2, "Flash write address too high\n");
    return false;
  }

  if (address + dataLength > IFLASH0_SIZE + IFLASH1_SIZE) {
    flash_debug(2, "Attempt to write past Flash boundary.\n");
    return false;
  }

  return true;
}

boolean DueFlashStorage::write(uint32_t address, byte* data, uint32_t dataLength, bool with_locking) {
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
  const bool needToDisableInterrupts = ((address < IFLASH0_SIZE) || (address < uint32_t(getFirstFreeBlock() - FLASH_START)));
  if (needToDisableInterrupts) {
    noInterrupts();
  }

  bool result = true;
  if (with_locking) {
    retCode = flash_unlock((uint32_t)FLASH_START + address, (uint32_t)FLASH_START + address + dataLength - 1, 0, 0);
    if (retCode != FLASH_RC_OK) {
      flash_debug(2, "Failed to unlock flash for write\n");
      result = false;
    }
  }

  // write data
  if (result) {
    // always try a write WITHOUT erase first:
    retCode = flash_write((uint32_t)FLASH_START + address, data, dataLength, 0);
    if (retCode != FLASH_RC_OK) {
      flash_debug(1, "Flash write sans erase failed; retrying with erase.\n");

      retCode = flash_write((uint32_t)FLASH_START + address, data, dataLength, 1);
    }

    if (retCode != FLASH_RC_OK) {
      flash_debug(2, "Flash write failed\n");
      result = false;
    }
  }

  // Lock page
  if (with_locking) {
    // always re-lock page when previously unlock was attempted above, whether that one succeeded or failed is irrelevant.
    retCode = flash_lock((uint32_t)FLASH_START + address, (uint32_t)FLASH_START + address + dataLength - 1, 0, 0);
    if (retCode != FLASH_RC_OK) {
      flash_debug(2, "Failed to lock flash page\n");
      result = false;
    }
  }

  if (needToDisableInterrupts) {
    interrupts();
  }

  return result;
}

