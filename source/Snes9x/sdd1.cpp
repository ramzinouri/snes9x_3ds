#include "copyright.h"


#include "snes9x.h"
#include "memmap.h"
#include "ppu.h"
#include "sdd1.h"
#include "3dsimpl.h"

#ifdef __linux
#include <unistd.h>
#endif

void S9xSetSDD1MemoryMap (uint32 bank, uint32 value)
{
    bank = 0xc00 + bank * 0x100;
    value = value * 1024 * 1024;

    int c;

    for (c = 0; c < 0x100; c += 16)
    {
	uint8 *block = &Memory.ROM [value + (c << 12)];
	int i;

	for (i = c; i < c + 16; i++)
	    Memory.Map [i + bank] = block;
    }
}

void S9xResetSDD1 ()
{
    memset (&Memory.FillRAM [0x4800], 0, 4);
    for (int i = 0; i < 4; i++)
    {
	Memory.FillRAM [0x4804 + i] = i;
	S9xSetSDD1MemoryMap (i, i);
    }
}

void S9xSDD1PostLoadState ()
{
    for (int i = 0; i < 4; i++)
	S9xSetSDD1MemoryMap (i, Memory.FillRAM [0x4804 + i]);
}

static int S9xCompareSDD1LoggedDataEntries (const void *p1, const void *p2)
{
    uint8 *b1 = (uint8 *) p1;
    uint8 *b2 = (uint8 *) p2;
    uint32 a1 = (*b1 << 16) + (*(b1 + 1) << 8) + *(b1 + 2);
    uint32 a2 = (*b2 << 16) + (*(b2 + 1) << 8) + *(b2 + 2);

    return (a1 - a2);
}

void S9xSDD1SaveLoggedData ()
{
    if (Memory.SDD1LoggedDataCount != Memory.SDD1LoggedDataCountPrev)
    {
	qsort (Memory.SDD1LoggedData, Memory.SDD1LoggedDataCount, 8,
	       S9xCompareSDD1LoggedDataEntries);

	FILE *fs = fopen (S9xGetFilename (".dat"), "wb");

	if (fs)
	{
	    fwrite (Memory.SDD1LoggedData, 8,
		    Memory.SDD1LoggedDataCount, fs);
	    fclose (fs);
#if defined(__linux)
	    chown (S9xGetFilename (".dat"), getuid (), getgid ());
#endif
	}
	Memory.SDD1LoggedDataCountPrev = Memory.SDD1LoggedDataCount;
    }
}

void S9xSDD1LoadLoggedData ()
{
    FILE *fs = fopen (S9xGetFilename (".dat"), "rb");

    Memory.SDD1LoggedDataCount = Memory.SDD1LoggedDataCountPrev = 0;

    if (fs)
    {
	int c = fread (Memory.SDD1LoggedData, 8, 
		       MEMMAP_MAX_SDD1_LOGGED_ENTRIES, fs);

	if (c != EOF)
	    Memory.SDD1LoggedDataCount = Memory.SDD1LoggedDataCountPrev = c;
	fclose (fs);
    }
}

