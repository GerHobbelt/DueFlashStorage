/* This example will write a chunk of data, then intermittantly attempts to rewrite/overwrite 
   that chunk with the very same or minimally updated data: only in the latter scenario should
   DueFlashStorage trigger a flash page erase action.
   
   This code also reads directly from flash instead of using the read APIs; after all, 
   it's just another chunk of memory/ROM!
*/

#if 0
#include <DueFlashStorage.h>
#else
#include "src/DueFlashStorage.h"
#endif

DueFlashStorage dueFlashStorage;

struct TestData {
  byte b0{0};
  byte b1{42};
  uint32_t i1{1};
  uint64_t li2{0x1234567890123456ULL};
  int i3{-555};
  uint32_t iter{0};

  void display() const {
    Serial.print("(");
    Serial.print("b0:");
    Serial.print(b0);
    Serial.print(",b1:");
    Serial.print(b1);
    Serial.print(",i1:");
    Serial.print(i1);
    Serial.print(",li2:");
    uint32_t v1 = (uint32_t)(li2 >> 32);
    Serial.print(v1,HEX);
    uint32_t v2 = (uint32_t)(li2);
    Serial.print(v2,HEX);
    Serial.print(",i3:");
    Serial.print(i3);
    Serial.print(",iter:");
    Serial.print(iter);
    Serial.println(")");
  }

  bool is_valid() const {
    return (iter != 0xFFFFFFFFU);
  }

  void increment_iter() {
    iter++;
    // prevent iter becoming 0xFFFFFFFF, which is indiscernible from erased flash page data: wrap before we hit that number!
    if (iter == 0xFFFFFFFFU)
      iter = 0;
  }

  // > comparison tells us if this TestData instance is the latest one!
  // See also the comments elsewhere in this example about the boundary conditions
  // and how we deal with those (in here).
  bool operator > (const TestData *alt) const {
    if (!alt)
      return true;

    uint32_t ours = this->iter;
    uint32_t theirs = alt->iter;

    if ((ours | theirs) & 0x80000000) {
      // one or both have bit31 set, so we're approaching the boundary condition...
      if ((ours ^ theirs) & 0x80000000) {
        // only one of us has bit31 set, so we're looking at the boundary condition...
        if (ours & 0x80000000) {
          // theirs does not have bit31 set, hence THEY are younger than us!
          return false;
        }
        else {
          // ours does not have bit31 set, hence WE are younger than them!
          return true;
        }
      }
      else {
        // both US and THEM have our bit31 set.
        // We may be close to the boundary condition, but we have wrapped yet, so it's just comparing regular numbers now...
        return ours > theirs;
      }
    }
    else {
      // clean
      return ours > theirs;
    }
  }

  bool is_more_recent_than(const TestData *them) const {
    if (!them)
      return true;
    return this->operator > (them);
  }
  bool is_older_than(const TestData *them) const {
    if (!them)
      return true;
    return !is_more_recent_than(them);
  }

  static const byte *get_next_flash_slot_address(const byte *flash_data_address) {
    // assume each TestData item is stored at a chunk (128 bit) boundary: that's the boundary for non-clashing SAM3X flash writes, i.e. writes which don't require page erase while keeping neighbouring data intact.
    // hence we align every test at this 128 bit boundary:
    flash_data_address += (sizeof(TestData) + 15) & ~0x0F;
    return flash_data_address;
  }
  static const TestData *get_next_flash_slot_address(const TestData *flash_data_address) {
    return (const TestData *)get_next_flash_slot_address((const byte *)flash_data_address);
  }
};

TestData cfg;
const TestData *cfg_flash_slot_address = nullptr; // stores a pointer to the current CFG stored in flash

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(250000 /* was: 115200 */ );
  while (!Serial)
    ;

  Serial.print("\n\n\n\n\nDueFlashStorage: reading/writing a struct from/to flash example (with optimized write: only when change has happened)\n");
  Serial.println(__FILE__);
  Serial.println();
  Serial.println();

  /* Flash is erased every time new code is uploaded. Write the default configuration to flash if first time */

  const byte* const flash_data_address = dueFlashStorage.getFirstFreeBlock();

  // running for the first time?
  const TestData& test_cfg = *((const TestData *)flash_data_address);
  bool is_very_first_run = (0xFF == test_cfg.b0); // flash bytes will be 255 at first run
  if (is_very_first_run) {
    TestData fresh_cfg;
    Serial.print("writing a fresh CFG at index 0: ");
    Serial.println(dueFlashStorage.write(0, fresh_cfg));  // address 0
    
    fresh_cfg.display();
  
    cfg = fresh_cfg;
    // and the slot/address where we're supposed to have written this chunk
    cfg_flash_slot_address = (const TestData *)dueFlashStorage.readAddress(0);
  }
  else {
    // now we're going to do something different from the other examples:
    // we 'know' we keep multiple CFG instances, cycling through them,
    // so we're going to look for the last used one in the series,
    // then copy it into the next slot.
    //
    // We allocated the entire data space in the flash for this ring,
    // so we wrap only once we've hit the end of the flash!
    //
    // Theoretically/practically, there's a boundary condition when 
    // the TestData.iter wraps around, but we 'cover' for that by the 
    // following theory:
    // the `iter` type is larger than the number of TestData instances 
    // we are ever able to store in the data zone of the flash, so
    // we have been overwriting the oldest instances already before
    // this happens.
    // Hence, we're good when we simply track the highest and lowest
    // iter numbers: when we have a wrap/boundary condition, the
    // lowest iter number will be high in the uint32_t range, i.e.
    // have its bit31 set. When that happens, the 'is this higher' 
    // check must decide that values without bit31 set are ABOVE/LATER THAN
    // those very high numbers.
    //
    // In fact, our code assumes we cannot store 2^29 TestData instances,
    // which is correct. :-)
    //
    Serial.println("scanning the flash data zone...");
    uint32_t valid_cnt = 0;
    uint32_t full_cnt = 0;
    const TestData *current = (const TestData *)flash_data_address;
    const TestData *last_used = nullptr;
    const TestData *oldest_used = nullptr;
    const byte* const flash_end_address = dueFlashStorage.getFirstFreeBlock() + dueFlashStorage.getAvailableFlashSize();
    // assume each TestData item is stored at a chunk (128 bit) boundary: that's the boundary for non-clashing SAM3X flash writes, i.e. writes which don't require page erase while keeping neighbouring data intact.
    // hence we align every test at this 128 bit boundary:
#if 0
    current = TestData::get_next_flash_slot_address(current);
#endif
    while (((const byte *)(current + 1)) <= flash_end_address) {
      // test:
      // is it a valid/legal slot?
      if (current->is_valid()) {
        if (current->is_more_recent_than(last_used)) {
          last_used = current;
        }
        else if (current->is_older_than(oldest_used)) {
          oldest_used = current;
        }
        valid_cnt++;
      }
      full_cnt++;

      current = TestData::get_next_flash_slot_address(current);
    } 

    Serial.println("Scan results:");

    Serial.print("last used @ 0x");
    Serial.print((intptr_t)last_used, HEX);
    Serial.print(": ");
    last_used->display();

    Serial.print("oldest used @ 0x");
    Serial.print((intptr_t)oldest_used, HEX);
    Serial.print(": ");
    oldest_used->display();

    Serial.print("Inspected ");
    Serial.print(valid_cnt);
    Serial.print(" slots, out of a total of ");
    Serial.print(full_cnt);
    Serial.println(" potential slots.");

    // now we load the latest into RAM:
    cfg = *last_used;
    cfg.increment_iter();
    // and the slot/address where we're supposed to write this chunk
    cfg_flash_slot_address = TestData::get_next_flash_slot_address(last_used);
    if (((const byte *)(cfg_flash_slot_address + 1)) > flash_end_address) {
      // wrap! prevent out-of-bounds writing of the CFG
      cfg_flash_slot_address = (const TestData *)dueFlashStorage.readAddress(0);
    }

    Serial.print("writing an updated CFG at address 0x");
    Serial.print((intptr_t)cfg_flash_slot_address, HEX);
    Serial.print(": ");
    Serial.println(dueFlashStorage.write_at_addr((byte *)cfg_flash_slot_address, cfg));  // address XXXXX
    
    cfg.display();
  }

  Serial.println("========================================================");
}

void loop() {
  Serial.print("(re)writing active CFG (unchanged) -- this should report that there's nothing to write as the CFG data has not changed -- at 0x");
  Serial.print((intptr_t)cfg_flash_slot_address, HEX);
  Serial.print(": ");
  Serial.println(dueFlashStorage.write_at_addr((byte *)cfg_flash_slot_address, cfg));  // address XXXXX

  /* only do the next edit on EVEN rounds: the ODD rounds still will attempt to write, but the flash library should recognize the fact that nothing has changed and thus optimize out that write action, reducing flash wear ==> longer hardware life/MTBF! */
  static int cnt = 0;
  cnt++;

  // every 3 rounds we UPDATE the CFG and write it to a fresh slot: the effect of this should be that
  // there are no flash page erase reports from the debug function UNTIL we've wrapped around the flash
  // data zone, which is about 15K CFG sloits large, so that'll take quite some time...
  //
  //
  // TODO: we should be able to tell the flash to simply erase the next page, because the *implicit*
  // page erase inside the write() method handler will attempt very hard to keep all surrounding data
  // intact and that's not what we need/want here, now that we're demonstrating a very basic wear-leveled
  // flash storage approach...
  //
  if (cnt % 3 == 3 - 1) {
    const TestData* const flash_end_address = (const TestData*)(dueFlashStorage.getFirstFreeBlock() + dueFlashStorage.getAvailableFlashSize());

    // update CFG and write to the next slot:
    cfg.increment_iter();
    // and the slot/address where we're supposed to write this chunk
    cfg_flash_slot_address = TestData::get_next_flash_slot_address(cfg_flash_slot_address);
    if (cfg_flash_slot_address + 1 > flash_end_address) {
      // wrap! prevent out-of-bounds writing of the CFG:
      cfg_flash_slot_address = (const TestData *)dueFlashStorage.readAddress(0);
    }

    Serial.print("writing an updated CFG at address 0x");
    Serial.print((intptr_t)cfg_flash_slot_address, HEX);
    Serial.print(": ");
    Serial.println(dueFlashStorage.write_at_addr((byte *)cfg_flash_slot_address, cfg));  // address XXXXX
    
    cfg.display();
  }

  static bool led_state = false;
  digitalWrite(LED_BUILTIN, led_state); 
  led_state = !led_state;

  // halt after 5 rounds to prevent wearing out your flash quickly during these experiments...
  if (cnt < 500) {
    delay(10);      // YEAH... we're speeding this one up a tad. I assume all other examples work flawlessly and we already provisionally tested the flash rewrite optimization so we don't waer out the flash with this 500/3 write actions (no page erasures expected).
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

