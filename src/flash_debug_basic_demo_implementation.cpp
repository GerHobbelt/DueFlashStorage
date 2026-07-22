#include "DueFlashStorage.h"

#if !defined(DFS_IGNORE_DEMO_FLASH_DEBUG_IMPL)

extern "C"
WEAK 
void flash_debug(int level, const char *message) {
  Serial.print("  DueFlashDebug: level ");
  Serial.print(level);
  Serial.print(": ");
  Serial.print(message);
  Serial.println();
}

#endif
