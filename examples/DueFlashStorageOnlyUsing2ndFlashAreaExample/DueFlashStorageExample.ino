/* Same simple wear-leveling demo code as DueFlashStorageOverwriteWithSameExample but now we
   start and limit ourselves to using the IFLASH1 area, instead of using all available space
   (i.e. part of IFLASH0 alongside IFLASH1)
   
   The visible effect should be that the DueFlashStorage internals optimize of this and thus
   DO NOT disable interrupts / block everything while writing chunks to flash.
   
   Compare this source file against the same-named one in ../DueFlashStorageOverwriteWithSameExample/
   to observe the edits/diffs required for this change of behaviour.
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

  // N.B.: do NOT use readAddress(IFLASH0_SIZE) method call as that is feeding an *absolute*
  // flash offset into a relative conversion function.
  // Also note that we check if the firmware is so fat that it has filled over into FLASH1 already:
  // in that case, we abort by locking up. Kinda like `assert()` but for embedded applications.
  const byte* const flash_data_address = dueFlashStorage.readAbsoluteAddress(IFLASH0_SIZE);

  // 
  // Of course, this demo app is so small that this check does not fire here, but when you copy-paste
  // this into your own, larger code base, it's good to have seen the check -- which is self-explanatory.
  //
  if (flash_data_address < dueFlashStorage.getFirstFreeBlock()) {
    Serial.println("not enough space left on this device for this demo.\n\nHALTING...");
    Serial.flush();
    while (1) 
      ;   // spin forever...
  }

  // running for the first time?
  const TestData& test_cfg = *((const TestData *)flash_data_address);
  bool is_very_first_run = (0xFF == test_cfg.b0); // flash bytes will be 255 at first run
  if (is_very_first_run) {
    TestData fresh_cfg;
    Serial.print("writing a fresh CFG at address 0x");
    Serial.print((intptr_t)flash_data_address, HEX);
    Serial.print(": ");
    // can't use write(IFLASH0_SIZE, ...) as, again, that's feeding an absolute offset
    // into a relative offset-using function. But no worries: we have the address!
    Serial.println(dueFlashStorage.write_at_addr((byte *)flash_data_address, fresh_cfg));  // address 0
    
    fresh_cfg.display();
  
    cfg = fresh_cfg;
    // and the slot/address where we're supposed to have written this chunk
    cfg_flash_slot_address = (const TestData *)flash_data_address;
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
    const byte* const flash_end_address = dueFlashStorage.getFlashEndAddress();
    // assume each TestData item is stored at a chunk (128 bit) boundary: that's the boundary for non-clashing SAM3X flash writes, i.e. writes which don't require page erase while keeping neighbouring data intact.
    // hence we align every test at this 128 bit boundary:
    while (((const byte *)(current + 1)) <= flash_end_address) {
      // test:
      // is it a valid/legal slot?
      if (current->is_valid()) {
        if (current->is_more_recent_than(last_used)) {
          last_used = current;
        }
        if (current->is_older_than(oldest_used)) {
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
      cfg_flash_slot_address = (const TestData *)dueFlashStorage.readAbsoluteAddress(IFLASH0_SIZE);
    }

    Serial.print("writing an updated CFG at address 0x");
    Serial.print((intptr_t)cfg_flash_slot_address, HEX);
    Serial.print(": ");
    Serial.println(dueFlashStorage.write_at_addr((byte *)cfg_flash_slot_address, cfg));  // address XXXXX
    
    cfg.display();
  }

  display_flash_debug_messages();

  Serial.println("========================================================");
}

void loop() {
  Serial.print("(re)writing active CFG (unchanged) -- this should report that there's nothing to write as the CFG data has not changed -- at 0x");
  Serial.print((intptr_t)cfg_flash_slot_address, HEX);
  Serial.print(": ");
  Serial.println(dueFlashStorage.write_at_addr((byte *)cfg_flash_slot_address, cfg));  // address XXXXX

  display_flash_debug_messages();

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
    const TestData* const flash_end_address = (const TestData*)(dueFlashStorage.getFlashEndAddress());

    // update CFG and write to the next slot:
    cfg.increment_iter();
    // and the slot/address where we're supposed to write this chunk
    cfg_flash_slot_address = TestData::get_next_flash_slot_address(cfg_flash_slot_address);
    if (cfg_flash_slot_address + 1 > flash_end_address) {
      // wrap! prevent out-of-bounds writing of the CFG:
      cfg_flash_slot_address = (const TestData *)dueFlashStorage.readAbsoluteAddress(IFLASH0_SIZE);
    }

    Serial.print("writing an updated CFG at address 0x");
    Serial.print((intptr_t)cfg_flash_slot_address, HEX);
    Serial.print(": ");
    Serial.println(dueFlashStorage.write_at_addr((byte *)cfg_flash_slot_address, cfg));  // address XXXXX
    
    cfg.display();

    display_flash_debug_messages();
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
