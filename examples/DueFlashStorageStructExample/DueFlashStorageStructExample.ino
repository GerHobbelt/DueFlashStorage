/* This example will write a struct to memory which is a very convinient way of storing configuration parameters.
 Try resetting the Arduino Due or unplug the power to it. The values will stay stored. */

#include <DueFlashStorage.h>
DueFlashStorage dueFlashStorage;

// The struct of the configuration.
struct Configuration {
  uint32_t a;
  uint32_t b;
  int32_t bigInteger;
  char* message;
  char c;
};

// initialize one struct
Configuration configuration;

void setup() {
  Serial.begin(115200);
  delay(500);

  /* Flash is erased every time new code is uploaded. Write the default configuration to flash if first time */
  // running for the first time?
  uint8_t codeRunningForTheFirstTime = dueFlashStorage.read(0); // flash bytes will be 255 at first run
  Serial.print("Flash start: 0x");

  byte* startAddr = dueFlashStorage.readAddress(0);
  Serial.println((unsigned int)startAddr, HEX);
  int bytes = 0;
  while (bytes < 512 * 1024)
  {
    int* data = (int*)(startAddr + bytes);
    if (*data == 0xFFFFFFFF)
     {
     Serial.print("Free block at 0x");
     Serial.println((unsigned int)data, HEX);
     }
     else
     {
      Serial.print("Used block at 0x");
     Serial.println((unsigned int)data, HEX);
     }
    bytes+=512;
  }

  byte* firstDataAddress = dueFlashStorage.getFirstFreeBlock();
  byte* flashStartAddress = dueFlashStorage.readAddress(0);

  Serial.print("First free block is at: 0x");
  Serial.println((uint32_t)firstDataAddress, HEX);
  
  delay(2000);
  int* firstCall = (int*)firstDataAddress;
  int startContent = *firstCall;
  Serial.print("Found value at first data address: ");
  Serial.println(startContent);
  Serial.print("Address is : ");
  Serial.println((uint32_t)firstDataAddress, HEX);
  Serial.print("Is first start: ");
  if (*firstCall == -1) 
  {
    Serial.println("yes");
    delay(1000);
    /* OK first time running, set defaults */
    configuration.a = 22;
    configuration.b = 0;
    configuration.bigInteger = 1147483647; // my lucky number
    configuration.message = "Hello world!";
    configuration.c = 's';

    // write configuration struct to flash at adress 4
    byte b2[sizeof(Configuration)]; // create byte array to store the struct
    memcpy(b2, &configuration, sizeof(Configuration)); // copy the struct to the byte array
    Serial.print("Writing data to 0x");
    Serial.println((uint32_t)firstDataAddress, HEX);
    dueFlashStorage.write(firstDataAddress + 4, b2, sizeof(Configuration)); // write byte array to flash

    // write 0 to address 0 to indicate that it is not the first time running anymore
    dueFlashStorage.write(firstDataAddress, 0, sizeof(uint32_t)); 
  }
  else {
    Serial.println("no");
  }
}

void loop() {
  byte* firstDataAddress = dueFlashStorage.getFirstFreeBlock();
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

  delay(1000);
}
