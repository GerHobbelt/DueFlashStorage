# DueFlashStorage

DueFlashStorage saves non-volatile data for Arduino Due. The library is made to be similar to the EEPROM library. Uses flash block 1 per default.

### Features

- Non-volatile data storage. Resetting or loss of power to the Arduino will not affect the data.
- Similar to the standard EEPROM library
- Write and read byte by byte
- Write and read byte arrays to store arbitrary variable types (strings, structs, integers, floats)

Note: The flash storage is reset every time you upload a new sketch to your Arduino.

Inspiration and some code from Pansenti at https://github.com/Pansenti/DueFlash


## Changes to the original implementation:

- The whole flash space can be addressed. Flash offset 0 is now the start of the first flash block, where the application resides. Therefore one must be **extra careful** not to overwrite the program in memory. Use the new `getFirstFreeBlock` function to retrieve the first block not used by the program. 
- Added a new overload to write: `boolean write(byte* addres, byte* data, uin32_t dataLength)` that allows writing to addresses instead of offsets. 


## Install

Create a new folder in your Arduino sketch folder named DueFlashStorage. 
Download and put all files from this repository into the folder. 

or:

Arduino.cc has some good instructions on how to add libraries:
https://www.arduino.cc/en/Guide/Libraries

The easiest is just to open the library manager and search for "DueFlashStorage". Install the one from Sebastian Nilsson.

Remember that you also need to install the Arduino SAM Boards from the board manager:
https://www.arduino.cc/en/Guide/Cores

## Use

### Basic use

```cpp
// write the value 123 to offset 0x20000
dueFlashStorage.write(0x20000, 123);

// read byte at offset 0
byte b = dueFlashStorage.read(0x20000);
```

### Advanced use to store configuration parameters

```cpp
// say you want to store a struct with parameters:
struct Configuration {
  uint8_t a;
  uint8_t b;
  int32_t bigInteger;
  char* message;
  char c;
};
Configuration configuration;

// then write it to flash like this:
byte b2[sizeof(Configuration)]; // create byte array to store the struct
memcpy(b2, &configuration, sizeof(Configuration)); // copy the struct to the byte array
dueFlashStorage.write(4, b2, sizeof(Configuration)); // write byte array to flash at address 4

// and read from flash like this:
byte* b = dueFlashStorage.readAddress(4); // byte array which is read from flash at adress 4
Configuration configurationFromFlash; // create a temporary struct
memcpy(&configurationFromFlash, b, sizeof(Configuration)); // copy byte array to temporary struct

/* see example code for a working example */
```

## Examples

### DueFlashStorageExample.cpp

This example will write 3 bytes to 3 different addresses and print them to the serial monitor.
Try resetting the Arduino Due or unplug the power to it. The values will stay stored.
   
### DueFlashStorageStructExample.cpp

This example will write a struct to memory which is a very convinient way of storing configuration parameters.
Try resetting the Arduino Due or unplug the power to it. The values will stay stored.


## Support my creation of open source software:

[![Flattr this git repo](http://api.flattr.com/button/flattr-badge-large.png)](https://flattr.com/submit/auto?user_id=sebnil&url=https://github.com/sebnil/DueFlashStorage)

<a href='https://ko-fi.com/A0A2HYRH' target='_blank'><img height='36' style='border:0px;height:36px;' src='https://az743702.vo.msecnd.net/cdn/kofi2.png?v=0' border='0' alt='Buy Me a Coffee at ko-fi.com' /></a>
