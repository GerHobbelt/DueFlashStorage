/* This example will write 3 bytes to 3 different addresses and print them to the serial monitor.
   Try resetting the Arduino Due or unplug the power to it. The values will stay stored. */

#if 0
#include <DueFlashStorage.h>
#else
#include "src/DueFlashStorage.h"
#endif
#include <string>

#if 01
#define CHUNK_ADDR_OFFSET    16   /* we use 16 to show that the first writes don't need an erase beforehand due to the 128-bit distance from the previous value chunk now (16 = 128/8). */
#else
#define CHUNK_ADDR_OFFSET    1
#endif

DueFlashStorage dueFlashStorage;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(250000 /* was: 115200 */ );
  while (!Serial)
    ;

  Serial.print("\n\n\n\n\nDueFlashStorage: basic flash read/write API usage example ");
  Serial.println(__FILE__);
  Serial.println();
  Serial.println();

  byte b1 = dueFlashStorage.read8(0);
  if (b1 == 0xFF) {
    b1 = 1;
    uint8_t b2 = 100;
    dueFlashStorage.write8(0 * CHUNK_ADDR_OFFSET, b1);
    dueFlashStorage.write8(1 * CHUNK_ADDR_OFFSET, b2);
    dueFlashStorage.write8(2 * CHUNK_ADDR_OFFSET, b2);
  }
  else {
    b1++;
    dueFlashStorage.write8(0 * CHUNK_ADDR_OFFSET, b1);
  }

  display_flash_debug_messages();
}

void loop() {
  // read from flash at 'data space' address 0 and 1 and print them
  Serial.print("0:");
  Serial.print(dueFlashStorage.read8(0));
  Serial.print(" 1:");
  Serial.print(dueFlashStorage.read8(1 * CHUNK_ADDR_OFFSET));  

  /* only do the next edit on EVEN rounds: the ODD rounds still will attempt to write, but the flash library should recognize the fact that nothing has changed and thus optimize out that write action, reducing flash wear ==> longer hardware life/MTBF! */
  static int cnt = 0;
  cnt++;

  // read from address 2, increment it, print and then write incremented value back to flash storage
  uint8_t i = dueFlashStorage.read8(2 * CHUNK_ADDR_OFFSET);
  if (cnt % 2 == 0) {
    i++;
  }
  Serial.print(" 2:");
  Serial.print(dueFlashStorage.read8(2 * CHUNK_ADDR_OFFSET)); 
  dueFlashStorage.write8(2 * CHUNK_ADDR_OFFSET, i);
  
  Serial.println();

  display_flash_debug_messages();

  static bool led_state = false;
  digitalWrite(LED_BUILTIN, led_state); 
  led_state = !led_state;

  // halt after 5 rounds to prevent wearing out your flash quickly during these experiments...
  if (cnt < 5) {
    delay(1000);
  }
  else {
    Serial.println("Halting... (press RESET button to observe the last written CFG being kept intact in flash storage)\n");
    while (1) ;
  }

  Serial.println("-----------------------------------------");
}

// --------------------

#pragma pack(push, 1)
struct FlashDebugMessageStore {
  uint8_t depth;
  int8_t level[3];
  const char *message[3];
} flash_debug_msg_store{0, {}, {}};
#pragma pack(pop)

// non-weak: this one overrides the default debug output function in the library
extern "C"
void flash_debug(int level, const char *message) {
  uint8_t d = flash_debug_msg_store.depth;
  if (d < 3) {
    flash_debug_msg_store.level[d] = level;
    flash_debug_msg_store.message[d] = message;
    flash_debug_msg_store.depth = d + 1;
  }
}

void display_flash_debug_messages() {
  uint8_t d = flash_debug_msg_store.depth;
  for (uint8_t i = 0; i < d; i++) {
    Serial.print("  flash [");
    Serial.print((int)i);
    Serial.print("] level ");
    Serial.print((int)flash_debug_msg_store.level[i]);
    Serial.print(": ");
    Serial.print(flash_debug_msg_store.message[i]);
    Serial.println();
  }
  flash_debug_msg_store.depth = 0;
}
