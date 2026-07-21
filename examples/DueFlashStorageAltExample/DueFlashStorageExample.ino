/* This example will write 3 bytes to 3 different addresses and print them to the serial monitor.
   Try resetting the Arduino Due or unplug the power to it. The values will stay stored. */

#if 0
#include <DueFlashStorage.h>
#else
#include "src/DueFlashStorage.h"
#endif
#include <string>

DueFlashStorage dueFlashStorage;

static uint32_t flash_base_address = 0;



extern "C" unsigned char _etext;
extern "C" unsigned char _srelocate;
extern "C" unsigned char _erelocate;

static void printPointer(const void *ptr) {
  Serial.print("0x");
  Serial.print((intptr_t)ptr, 16);
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

#if 0
  // wait for USB serial port to be connected - wait for pc program to open the serial port
  SerialUSB.begin(250000 /* was: 115200 */ );    // Initialize Native USB port
  while(!SerialUSB);
#endif  

  Serial.begin(250000 /* was: 115200 */ );
  Serial.print("DueFlashStorage: example ");
  Serial.print(__FILE__);
  Serial.println();
  Serial.println();

  auto p1 = dueFlashStorage.getFirstFreeBlock();
  flash_base_address = dueFlashStorage.getOffset(p1);

  byte b1 = 3;
  uint8_t b2 = 1;
#if 01
  dueFlashStorage.write(flash_base_address + 0, b1);
  dueFlashStorage.write(flash_base_address + 1, b2);
  dueFlashStorage.write(flash_base_address + 2, b2);
#else
  Serial.print("p1 = ");
  printPointer(p1);
  Serial.print(", base_addr = 0x");
  Serial.print(flash_base_address, 16);
  Serial.println();


  Serial.print("_etext = ");
  printPointer(&_etext);
  Serial.println();
  Serial.print("_srelocate = ");
  printPointer(&_srelocate);
  Serial.println();
  Serial.print("_erelocate = ");
  printPointer(&_erelocate);
  Serial.println();
  Serial.println();

  for (int i = -500; i <= 2; ) {
    char buf[12];
    snprintf(buf, sizeof(buf), "%4d:", i);
    Serial.print(buf);
    for (int j = 0; j < 10; j++, i++) {
      Serial.print(' ');
      Serial.print(dueFlashStorage.read(flash_base_address + i), 16);
    }
    Serial.println();
  }
  Serial.println();
#endif
}

void loop() {
  // read from flash at address 0 and 1 and print them
  Serial.print("0:");
  Serial.print(dueFlashStorage.read(flash_base_address + 0));
  Serial.print(" 1:");
  Serial.print(dueFlashStorage.read(flash_base_address + 1));  
  
  // read from address 2, increment it, print and then write incremented value back to flash storage
  uint8_t i = dueFlashStorage.read(flash_base_address + 2) + 1;
  Serial.print(" 2:");
  Serial.print(dueFlashStorage.read(flash_base_address + 2)); 
  //dueFlashStorage.write(flash_base_address + 2, i);
  
  Serial.println();

  static bool led_state = false;
  digitalWrite(LED_BUILTIN, led_state); 
  led_state = !led_state;

  delay(1000);
}

// --------------------

// non-weak: this one overrides the default debug output function in the library
void flash_debug(int level, const char *message) {
  Serial.print("level = ");
  Serial.print(level);
  Serial.print(": ");
  Serial.print(message);
  Serial.println();
}

