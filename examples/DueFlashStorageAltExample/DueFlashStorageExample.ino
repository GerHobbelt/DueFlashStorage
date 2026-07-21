/* This example will write 3 bytes to 3 different addresses and print them to the serial monitor.
   Try resetting the Arduino Due or unplug the power to it. The values will stay stored. */

#if 0
#include <DueFlashStorage.h>
#else
#include "src/DueFlashStorage.h"
#endif
#include <string>

DueFlashStorage dueFlashStorage;

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
  Serial.print("DueFlashStorage: flash address API overloads usage example ");
  Serial.print(__FILE__);
  Serial.println();
  Serial.println();

  auto* flash_data_address = dueFlashStorage.getFirstFreeBlock();

  byte b1 = 3;
  uint8_t b2 = 1;
  dueFlashStorage.write(flash_data_address + 0, b1);
  dueFlashStorage.write(flash_data_address + 1, b2);
  dueFlashStorage.write(flash_data_address + 2, b2);
}

void loop() {
  // read from flash at 'data space' address 0 and 1 and print them
  Serial.print("0:");
  Serial.print(dueFlashStorage.read(flash_data_address + 0));
  Serial.print(" 1:");
  Serial.print(dueFlashStorage.read(flash_data_address + 1));  
  
  // read from address 2, increment it, print and then write incremented value back to flash storage
  uint8_t i = dueFlashStorage.read(flash_data_address + 2) + 1;
  Serial.print(" 2:");
  Serial.print(dueFlashStorage.read(flash_data_address + 2)); 
  dueFlashStorage.write(flash_data_address + 2, i);
  
  Serial.println();

  static bool led_state = false;
  digitalWrite(LED_BUILTIN, led_state); 
  led_state = !led_state;

  // halt after 5 rounds to prevent wearing out your flash quickly during these experiments...
  {
    static int cnt = 0;
    cnt++;

    if (cnt < 5) {
      delay(1000);
    }
    else {
      Serial.println("Halting...");
      while (1) ;
    }
  }
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

