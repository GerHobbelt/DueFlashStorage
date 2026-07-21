
## This is Arduino Due (or rather: Atmel/Microchip SAM3X MCU specific)

From the SAM3X datasheet it follows that the internal Flash has a few features and limitations (which are quite different from other microprocessor series, such as the STM32).

### Page Erase

SAM3X, as all other MCUs, erases the flash **per page**. Page size is specified per model as `FL_PAGE_SIZE`, which for the Due SAM3X8E is specified as

```
#define IFLASH0_PAGE_SIZE        (256u)
#define IFLASH1_PAGE_SIZE        (256u)
```

which, incidentally, is the same number as for all other SAM3 MCU models.

This determines what we can do in terms of ensuring ‘database stability’: we will always need at least 1(one) completely empty page available to store the next edit(s).

> Fortunately this is quite different from the STM32F0,1,4,7 chips, where page (a.k.a. *sector*) size is very large. Ref: [Just a moment...](https://electronics.stackexchange.com/questions/278437/stm32f74x-flash-page-size-and-sectors) , [Flash memory: Does the entire page need to be erased before writing just a few bytes? - Electrical Engineering Stack Exchange](https://electronics.stackexchange.com/questions/122550/flash-memory-does-the-entire-page-need-to-be-erased-before-writing-just-a-few-b?rq=1), ….
> > 

See also: [arduino - Datalogging on serial flash memory - Electrical Engineering Stack Exchange](https://electronics.stackexchange.com/questions/120809/datalogging-on-serial-flash-memory?rq=1)


### Cannot overwrite written bytes

Contrary to the STM32 (referenced above) and others, the SAM3 datasheet states that bytes, once written, **cannot** be overwritten, i.e. it is not possible to ‘fuse’ individual lingering 1-bits to 0’s!

In fact, the **minimum write size granularity** is 64/128 bits, depending on the (18.5.1) *EEFC Flash Mode Register* `EEFC_FMR`: FAM: Flash Access Mode bit. Since we want fast Flash access, the write width is to be 128 bits, i.e. **16 bytes**.


### Write granularity: writing individual bytes or words is illegal

See the (18) *Enhanced Embedded Flash Controller (EEFC)* chapter in the SAM3 datasheet: all flash writes must happen with the previously mentioned 128 bit granularity.

Meanwhile the datasheet states that the flash write buffer (RAM) is one sector/page large and ‘wraps around’, i.e. you must fill only one sector/page at a time as addressing any other flash page will write into that *wrapped* buffer and consequently corrupt your first flash page write.



---

The above limitations imply that we can, at best, write 256/16 = 16 ‘data chunks’ in a single flash page. With a single flash available to us in the Arduino Due (the other is assumed occupied by firmware) at 128K, we thus have a best case of 128K/16 = 8K data chunks available.
*However*, we’ll need at least 1 empty page to write/update the next chunk if we want to ensure our stored chunks stay valid even under power loss / write failure / erase failure conditions: that’s 256/16 = 16 chunks less than 8192 
–> no problem then to have some ‘element index numbers’ serve as ‘magic numbers’ for those will be impossible to use given our own demands re data integrity under power loss conditions.

Now do go and the 8K count limitation as is or do we ‘chunk’ multiple smaller items together into a single 16 byte write?

## Layout of a single (userland) storage unit

Let’s see what a *valid* storage unit would look like. We’ll need these at least:

- an element identifier (`uint16_t`?)
- a `bool` flag indicating whether this one is *the active version* –> given the parceled write behaviour of the flash, we *should* probably defer this to a separate *administration chunk* or other means?
- the length of the element data (`uint16_t`?)
- the actual data itself… (1 byte or more…)

Can we use a fast lookup/index for our userland elements?
First, then, is the question: what’s the best case count for our ‘chunked’ tiny element storage approach?
Assuming we have a special construct where we don’t need the length, are able to fold the validation into the identifier space and the data itself is a single byte, then our overhead is 2 bytes for each element, hence arriving at a sum of 3, hence 128K/3 = 43690 elements. 
A lookup index would then need to map any 16 bit ID to an address in the 128K address space, which would cost 17 bits per address offset, given our modulo 3 scenario here.

Better then to assume 16 bit (word) alignment & size, thus minimum userland element cost is 4 bytes (2 bytes for the identifier, 2 bytes for the data), giving us 12K/4 = 32K elements count: this implies we can either use an identifier that serves as the flash address offset at the same time and costs 15 bits a pop, or we need a 32K lookup table, if we don’t want to spend our time searching.

Can we do a kind of compromise? 
I.e. where we use that “identifier equals flash offset” idea, *but* take care of the elements that get moved due to the flash being progressively filled and rewritten, by using a (smaller!) lookup table for the *moved items only*: if yours is in the lookup table, it has been moved but has not been ‘rewritten’ yet (which would be a more costly operation for the userland as all parents/referencers should update their reference to these) and if yours is **not** in the lookup table, then it’s available at the indicated location in flash!

The nice thing about that 32K = 15 bit identifier is that we use the other 15 bit range for those elements currently stored in SRAM, or other purposes.

Also note that the “identifier equals flash (qword-level) address offset” scheme ‘automatically’ provides a ‘validation check’: if the chunk indicated by the identifier-equals-address-offset **does not** mention that very same identifier, than the indicated chunk is invalid/erased/corrupt.
Or… should we assume validity and discard that identifier slot entirely? 
Can we reconstruct all userland elements then, once we reboot the machine, i.e. when we restart the software? Not unless we have some very specific ‘marker’ blocks in there that start everything off, so it’s safer to keep that identifier in there.
Plus a type byte, so we can decode what kind of userland data (or administrative chunk) this actually is…

The preliminary conclusion:

it’s smarter to have larger userland single chunks than storing tiny userland bits in there: the overhead is quite large.










