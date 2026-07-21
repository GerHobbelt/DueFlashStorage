/* This example will write a struct to memory which is a very convinient way of storing configuration parameters.
 Try resetting the Arduino Due or unplug the power to it. The values will stay stored. */

#if 0
#include <DueFlashStorage.h>
#else
#include "src/DueFlashStorage.h"
#endif

#define CFG_ADDR_OFFSET    16   /* was: 4, which is fine too, but we use 16 to show that the first write doesn't need an erase beforehand due to the 128-bit distance from the header word chunk now (16 = 128/8). */

DueFlashStorage dueFlashStorage;

// The struct of the configuration.
struct Configuration {
  uint32_t a{0};
  uint32_t b{0};
  int32_t bigInteger{0};
  const char* message{nullptr};
  char c{'?'};
};

void setup() {
  // initialize one struct
  Configuration configuration;
  
  Serial.begin(250000 /* was: 115200 */ );
  while (!Serial)
    ;

  Serial.print("\n\n\n\n\nDueFlashStorage: reading/writing a config struct from/to flash example ");
  Serial.println(__FILE__);

  /* Flash is erased every time new code is uploaded. Write the default configuration to flash if first time */
  
  // running for the first time?
  uint32_t codeRunningForTheFirstTime = dueFlashStorage.read32(0); // flash bytes will be 255 at first run
  Serial.print("\n\nFlash start: 0x");
  Serial.println(codeRunningForTheFirstTime, HEX);

  const byte* startAddr = dueFlashStorage.readAddress(0);
  Serial.print("Flash free storage area start address: 0x");
  Serial.println((unsigned int)startAddr, HEX);
  Serial.println();

  unsigned int bytes = 0;
  const uint32_t* section_start_address = NULL;
  uint32_t section_count = 0;
  bool section_is_free = false;
  for (;;)
  {
    bool the_end = (bytes >= dueFlashStorage.getAvailableFlashSize());
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
  
  delay(1000);

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
    dueFlashStorage.write_at_addr(const_cast<byte*>(firstDataAddress) + CFG_ADDR_OFFSET, configuration); // write config struct content to flash

    // write 0 to address 0 to indicate that it is not the first time running anymore
    dueFlashStorage.write32(0, 0); 
  }
  else {
    Serial.println("no");
  }

  Serial.println("Reading CFG from flash: ");
  Configuration cfg;
  Serial.println(dueFlashStorage.read(cfg, CFG_ADDR_OFFSET) != nullptr);

  // print the content
  Serial.print("a:");
  Serial.print(cfg.a);

  Serial.print(" b:");
  Serial.print(cfg.b);

  Serial.print(" bigInteger:");
  Serial.print(cfg.bigInteger);

  Serial.print(" message:");
  Serial.print(cfg.message);

  Serial.print(" c:");
  Serial.print(cfg.c);
  Serial.println();

  Serial.println("===========================================");
}

static bool streq(const char *s1, const char *s2) {
  if (!s1 || !s2)
    return false;
  return strcmp(s1, s2) == 0;
}

void loop() {
  Serial.println(sizeof(dueFlashStorage));

#if 0
  const byte* firstDataAddress = dueFlashStorage.getFirstFreeBlock();
  Configuration cfg = *((const Configuration*)(firstDataAddress + CFG_ADDR_OFFSET));
#else
  Configuration cfg;
  Serial.println(dueFlashStorage.read(&cfg, CFG_ADDR_OFFSET) != nullptr);
#endif

  // print the content
  Serial.print("a:");
  Serial.print(cfg.a);

  Serial.print(" b:");
  Serial.print(cfg.b);

  Serial.print(" bigInteger:");
  Serial.print(cfg.bigInteger);

  Serial.print(" message:");
  Serial.print(cfg.message);

  Serial.print(" c:");
  Serial.print(cfg.c);
  Serial.println();

  /* change some values in the struct and write them back */
  /* only do these edits on EVEN rounds: the ODD rounds still will attempt to write, but the flash library should recognize the fact that nothing has changed and thus optimize out that write action, reducing flash wear ==> longer hardware life/MTBF! */
  static int cnt = 0;
  cnt++;

  if (cnt % 2 == 0) {
    // increment b by 1 (modulus 100 to start over at 0 when 100 is reached)
    cfg.b = (cfg.b + 1) % 100;

    // change the message
    if (streq(cfg.message, "Hello world!"))
      cfg.message = "Hello Arduino Due!";
    else
      cfg.message = "Hello world!";
  }

  // write configuration struct to flash at adress CFG_ADDR_OFFSET
  dueFlashStorage.write(CFG_ADDR_OFFSET, &cfg);

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

// non-weak: this one overrides the default debug output function in the library
extern "C"
void flash_debug(int level, const char *message) {
  Serial.print("  debug level ");
  Serial.print(level);
  Serial.print(": ");
  Serial.print(message);
  Serial.println();
}

