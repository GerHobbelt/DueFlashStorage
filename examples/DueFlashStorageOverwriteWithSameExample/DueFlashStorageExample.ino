/* This example will write a chunk of data, then intermittantly attempts to rewrite/overwrite 
   that chunk with the very same or minimally updated data: only in the latter scenario should
   DueFlashStorage trigger a flash page erase action. */

#if 0
#include <DueFlashStorage.h>
#else
#include "src/DueFlashStorage.h"
#endif
#include <string>

DueFlashStorage dueFlashStorage;

struct TestData {
  byte b0{0};
  byte b1{42};
  uint32_t i1{1};
  uint64_t li2{12345678901234567890ULL};
  int i3{-555};
};
  

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(250000 /* was: 115200 */ );
  Serial.print("DueFlashStorage: example ");
  Serial.print(__FILE__);
  Serial.println();
  Serial.println();

  TestData cfg;
  Serial.print("Attempt to read CFG data from flash:");
  Serial.print(dueFlashStorage.read(0));
  
  byte b1 = 3;
  uint8_t b2 = 1;
  dueFlashStorage.write(0, b1);
  dueFlashStorage.write(1, b2);
  dueFlashStorage.write(2, b2);
}

void loop() {
  // read from flash at address 0 and 1 and print them
  Serial.print("0:");
  Serial.print(dueFlashStorage.read(0));
  Serial.print(" 1:");
  Serial.print(dueFlashStorage.read(1));  
  
  // read from address 2, increment it, print and then write incremented value back to flash storage
  uint8_t i = dueFlashStorage.read(2) + 1;
  Serial.print(" 2:");
  Serial.print(dueFlashStorage.read(2)); 
  dueFlashStorage.write(2, i);
  
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

