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
  rom_end = (byte*)(((uint32_t)rom_end + 256) & ~0xFF);  // Align to next free flash block (even if the memory ends right on a boundary)
  return rom_end;
}

boolean DueFlashStorage::write(uint32_t address, byte value) {
  uint32_t retCode;
  uint32_t byteLength = 1;

  retCode = flash_unlock((uint32_t)FLASH_START + address, (uint32_t)FLASH_START + address + byteLength - 1, 0, 0);
  if (retCode != FLASH_RC_OK) {
    flash_debug(2, "Failed to unlock flash for write\n");
    return false;
  }

  // write data
  retCode = flash_write((uint32_t)FLASH_START + address, &value, byteLength, 1);

  if (retCode != FLASH_RC_OK) {
    flash_debug(2, "Flash write failed\n");
    return false;
  }

  // Lock page
  retCode = flash_lock((uint32_t)FLASH_START + address, (uint32_t)FLASH_START + address + byteLength - 1, 0, 0);
  if (retCode != FLASH_RC_OK) {
    flash_debug(2, "Failed to lock flash page\n");
    return false;
  }
  return true;
}

boolean DueFlashStorage::write(uint32_t address, byte* data, uint32_t dataLength) {
  uint32_t retCode;

  if (address < IFLASH0_SIZE && address + dataLength > IFLASH0_SIZE) {
    // A write across the boundary of the flash pages requires two calls
    const uint32_t lowerSize = IFLASH0_SIZE - address;
    boolean ret = write(address, data, lowerSize);
    ret &= write(IFLASH0_SIZE, data + lowerSize, dataLength - lowerSize);
    return ret;
  }

  if ((uint32_t)FLASH_START + address < IFLASH0_ADDR) {
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

  if ((address & 3) != 0) {
    flash_debug(2, "Flash start address must be on four byte boundary\n");
    return false;
  }

  // Unlock page
  bool needToDisableInterrupts = (address < IFLASH0_SIZE);
  if (needToDisableInterrupts) {
    noInterrupts();
  }

  bool result = true;
  retCode = flash_unlock((uint32_t)FLASH_START + address, (uint32_t)FLASH_START + address + dataLength - 1, 0, 0);
  if (retCode != FLASH_RC_OK) {
    flash_debug(2, "Failed to unlock flash for write\n");
    result = false;
  }

  // write data
  if (result) {
    retCode = flash_write((uint32_t)FLASH_START + address, data, dataLength, 1);

    if (retCode != FLASH_RC_OK) {
      flash_debug(2, "Flash write failed\n");
      result = false;
    }
  }

  // Lock page
  if (result) {
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

boolean DueFlashStorage::write_unlocked(uint32_t address, byte value) {
  uint32_t retCode;
  uint32_t byteLength = 1;

  // write data
  retCode = flash_write((uint32_t)FLASH_START + address, &value, byteLength, 1);

  if (retCode != FLASH_RC_OK) {
    flash_debug(2, "Flash write failed\n");
    return false;
  }

  return true;
}

boolean DueFlashStorage::write_unlocked(uint32_t address, byte* data, uint32_t dataLength) {
  uint32_t retCode;

  if ((uint32_t)FLASH_START + address < IFLASH0_ADDR) {
    flash_debug(2, "Flash write address too low\n");
    return false;
  }

  if ((uint32_t)FLASH_START + address >= IFLASH1_ADDR + IFLASH1_SIZE) {
    flash_debug(2, "Flash write address too high\n");
    return false;
  }

  if ((((uint32_t)FLASH_START + address) & 3) != 0) {
    flash_debug(2, "Flash start address must be on four byte boundary\n");
    return false;
  }

  // If the flash address we're going to write is in the same segment as the code the CPU is executing, we need to disable interrupts,
  // or the CPU will come to a grinding halt here. Of course, we must be sure we're not to overwrite the code with data.
  bool needToDisableInterrupts = (address < IFLASH0_SIZE);
  if (needToDisableInterrupts) {
    noInterrupts();
  }

  // write data
  retCode = flash_write((uint32_t)FLASH_START + address, data, dataLength, 1);

  if (needToDisableInterrupts) {
    interrupts();
  }

  if (retCode != FLASH_RC_OK) {
    flash_debug(2, "Flash write failed\n");
    return false;
  }

  return true;
}

boolean DueFlashStorage::write(byte* address, byte* data, uint32_t dataLength) {
  uint32_t offset = getOffset(address);
  return write(offset, data, dataLength);
}
