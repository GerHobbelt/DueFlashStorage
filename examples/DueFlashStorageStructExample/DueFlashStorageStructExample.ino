/* This example will write a struct to memory which is a very convinient way of storing configuration parameters.
 Try resetting the Arduino Due or unplug the power to it. The values will stay stored. */

#if 0
#include <DueFlashStorage.h>
#else
#include "src/DueFlashStorage.h"
#endif

DueFlashStorage dueFlashStorage;

// The struct of the configuration.
struct Configuration {
  uint32_t a;
  uint32_t b;
  int32_t bigInteger;
  const char* message;
  char c;
};

// initialize one struct
Configuration configuration;

void setup() {
  Serial.begin(250000 /* was: 115200 */ );
  delay(500);

  /* Flash is erased every time new code is uploaded. Write the default configuration to flash if first time */
  // running for the first time?
  uint8_t codeRunningForTheFirstTime = dueFlashStorage.read(0); // flash bytes will be 255 at first run
  Serial.print("Flash start: 0x");
  Serial.println(codeRunningForTheFirstTime, HEX);

  const byte* startAddr = dueFlashStorage.readAddress(0);
  Serial.println((unsigned int)startAddr, HEX);
  Serial.println();

  int bytes = 0;
  const uint32_t* section_start_address = NULL;
  uint32_t section_count = 0;
  bool section_is_free = false;
  for (;;)
  {
    bool the_end = (bytes >= 512 * 1024);
    const uint32_t* data = (const uint32_t*)(startAddr + bytes);
    bool is_free_block = !the_end && (data[0] == 0xFFFFFFFF && data[1] == 0xFFFFFFFF && data[2] == 0xFFFFFFFF && data[3] == 0xFFFFFFFF);
    if (section_is_free != is_free_block || section_count == 0 || the_end) {
      if (section_count > 0) {
        Serial.print(section_count);
        Serial.print(is_free_block ? " used" : " free");
        Serial.print(" blocks at 0x");
        Serial.print((intptr_t)section_start_address, HEX);
        // Flash page is locked
        Serial.print("..0x");
        Serial.println((intptr_t)(data - 1), HEX);
      }
      section_is_free = is_free_block;
      section_count = 1;
      section_start_address = data;
    }
    else {
      section_count++;
    }

    if (the_end)
      break;

    bytes += 512;
  }
  Serial.println();

  const byte* firstDataAddress = dueFlashStorage.getFirstFreeBlock();
  const byte* flashStartAddress = dueFlashStorage.readAddress(0);

  Serial.print("First free block is at: 0x");
  Serial.print((intptr_t)firstDataAddress, HEX);
  Serial.print(", address @ offset 0 = 0x");
  Serial.println((intptr_t)flashStartAddress, HEX);
  
  delay(2000);
  const uint32_t* firstCall = (const uint32_t*)firstDataAddress;
  uint32_t startContent = *firstCall;
  Serial.print("Found value at first data address: 0x");
  Serial.println(startContent, HEX);
  Serial.print("Address is : 0x");
  Serial.println((intptr_t)firstDataAddress, HEX);
  Serial.print("Is first start: ");
  if (*firstCall == 0xFFFFFFFF) 
  {
    Serial.println("yes");
    delay(1000);
    /* OK first time running, set defaults */
    configuration.a = 22;
    configuration.b = 0;
    configuration.bigInteger = 1147483647; // my lucky number
    configuration.message = "Hello world! (1147483647)";
    configuration.c = 's';

    // write configuration struct to flash at adress 4
    Serial.print("Writing data to 0x");
    Serial.println((intptr_t)firstDataAddress, HEX);
    dueFlashStorage.write2addr(const_cast<byte*>(firstDataAddress) + 4, configuration); // write config struct content to flash

    // write 0 to address 0 to indicate that it is not the first time running anymore
    dueFlashStorage.write(0, 0, sizeof(uint32_t)); 
  }
  else {
    Serial.println("no");
  }
}

static bool streq(const char *s1, const char *s2) {
  if (!s1 || !s2)
    return false;
  return strcmp(s1, s2) == 0;
}

void loop() {
  const byte* firstDataAddress = dueFlashStorage.getFirstFreeBlock();
  Configuration* cfg = (Configuration*)(firstDataAddress + 4);

  // print the content
  Serial.print("a:");
  Serial.print(cfg->a);

  Serial.print(" b:");
  Serial.print(cfg->b);

  Serial.print(" bigInteger:");
  Serial.print(cfg->bigInteger);

  Serial.print(" message:");
  Serial.print(cfg->message);

  Serial.print(" c:");
  Serial.print(cfg->c);
  Serial.println();

  /* change some values in the struct and write them back */

  // increment b by 1 (modulus 100 to start over at 0 when 100 is reached)
  cfg->b = (cfg->b + 1) % 100;

  // change the message
  if (streq(cfg->message, "Hello world!"))
    cfg->message = "Hello Arduino Due!";
  else
    cfg->message = "Hello world!";

  // write configuration struct to flash at adress 4
  dueFlashStorage.write(4, (const byte *)cfg, sizeof(Configuration));

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

