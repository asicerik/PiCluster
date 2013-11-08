//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
void PUT32 ( unsigned int, unsigned int );
unsigned int GET32 ( unsigned int );
extern void start_l1cache ( void );
extern void stop_l1cache ( void );
extern void start_mmu ( unsigned int, unsigned int );

#include "uart0.h"

//Need a top level entry for every 1MB section, 20 bits.
//memory base addresses 0x100000, 0x200000, 0x300000, etc.
//making a one to one virtual to physical map
//virtual 0x20201234 -> 0x20201234 physical
//
//bits 31:20 of the virtual address index into the top level table
//1<<((31-20)+1)  4096 entries. 0x1000 32 bit entries, 0x4000 bytes.
//
//Bits 31:10 of the top level table point at the course page table
//bits 19:12 of the address index into this table.
//1<<((19-12)+1)  256 entries, 0x100 entries 0x400 bytes per entry
//
//Using a course entry
//
//the hardware looks in the first table and gets an entry.  Bits in the
//entry determine what kind it is, course, section, super section,
//just using course here as it doesnt save any space using the others.
//You can put the coarse entry anywhere.  Going to pack it in next to
//the top level table.  going to limit the size of the table so only
//so many entries will be allowed
//
//Using small pages (4096 byte)
//
//bits 31:12 of the small page descriptor in the course table are the
//physical address in memory.  bits 11:0 of the physical address come
//from the virtual address.

#define MMUTABLEBASE 0x01000000		// 16MB - needs to be above any code/stack

#define MMUTABLESIZE (0x10000)
#define MMUTABLEMASK ((MMUTABLESIZE-1)>>2)

#define TOP_LEVEL_WORDS (1<<((31-20)+1))
#define COARSE_TABLE_WORDS (1<<((19-12)+1))
#define SMALL_TABLE_WORDS (1<<((11-0)+1))

unsigned int nextfree;

//-------------------------------------------------------------------
unsigned int next_coarse_offset ( unsigned int x )
{
    unsigned int mask;

    mask=(~0)<<(10-2);
    mask=~mask;
    while(x&mask) x++; //lazy brute force
    return(x);
}
//-------------------------------------------------------------------
unsigned int add_one ( unsigned int add, unsigned int flags )
{
    unsigned int ra;
    unsigned int rb;
    unsigned int rc;

    //bits 31:20 index into the top level table
    ra=add>>20;
    rc=MMUTABLEBASE+(ra<<2);
    rb=GET32(rc);
    if(rb)
    {
        UartPrintf("Address %p already allocated\n",add);
        return(1);
    }
    add=ra<<20;

    rb=next_coarse_offset(nextfree);
    rc=rb+COARSE_TABLE_WORDS;
    if(rc>=MMUTABLESIZE)
    {
        UartPrintf("Not enough room\n");
        return(1);
    }
    nextfree=rc;
    //use course page table pointer on top level table
    PUT32(MMUTABLEBASE+(ra<<2),(MMUTABLEBASE+(rb<<2))|0x00000001);
    //fill in the course page table. with small entries
    for(ra=0;ra<COARSE_TABLE_WORDS;ra++)
    {
        PUT32(MMUTABLEBASE+(rb<<2)+(ra<<2),(add+(ra<<12))|0x00000032|flags);
    }
    return(0);
}

//-------------------------------------------------------------------------
int mmuInit ( void )
{
    unsigned int ra,rb,rc;
    unsigned int addr;

    for(nextfree=0;nextfree<TOP_LEVEL_WORDS;nextfree++) PUT32(MMUTABLEBASE+(nextfree<<2),0);

    //ram used by the stack and the program
    for (addr=0;addr<MMUTABLEBASE;addr += 0x100000)
    {
    	if(add_one(addr,0x0000|8|4)) return(1);
    }

    //Memory mapped I/O used by the uart, etc, not cached
    if(add_one(0x20000000,0x0000)) return(1);
    if(add_one(0x20100000,0x0000)) return(1);
    if(add_one(0x20200000,0x0000)) return(1);
    if(add_one(0x20600000,0x0000)) return(1);		// SMI

    // Frame buffer (1280x480*4bytes ~= 2.5MB
    if(add_one(0x1c000000,0x0000)) return(1);
    if(add_one(0x1c100000,0x0000)) return(1);
    if(add_one(0x1c200000,0x0000)) return(1);

    // GPU non-cached mailbox access
    if(add_one(0x40000000,0x0000)) return(1);

    UartPrintf("Starting MMU\n");
    start_mmu(MMUTABLEBASE,0x00800001);
    UartPrintf("Starting L1 cache\n");
	start_l1cache(); 
    UartPrintf("Done\n");
	return 0;
}

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------


//-------------------------------------------------------------------------
//
// Copyright (c) 2012 David Welch dwelch@dwelch.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//-------------------------------------------------------------------------
