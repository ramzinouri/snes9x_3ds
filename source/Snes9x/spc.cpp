#include "copyright.h"


void S9xSPCDump (const char *filename)
{
    FILE *fs;

    bool8 read_only = FALSE;
    char def [PATH_MAX];
    char title [PATH_MAX];
    char drive [_MAX_DRIVE];
    char dir [_MAX_DIR];
    char ext [_MAX_EXT];

    S9xSetSoundMute (TRUE);

#if 0
    _splitpath (Memory.ROMFilename, drive, dir, def, ext);
    strcat (def, ".spc");
    sprintf (title, "%s SPC filename",
		read_only ? "Select load" : "Choose save");
    const char *filename;

    filename = S9xSelectFilename (def, ".", "spc", title);
#endif

    fs = fopen (filename, "wb");
    fputs ("SNES-SPC700 Sound File Data 0.10", fs);
    fseek (fs, 37, SEEK_SET);
    fwrite (&APURegisters.PC, 2, 1, fs);

    fputc (APURegisters.YA.B.A, fs);
    fputc (APURegisters.X, fs);
    fputc (APURegisters.YA.B.Y, fs);
    fputc (APURegisters.P, fs);
    fputc (APURegisters.S - 0x100, fs); // ???
    fseek (fs, 256, SEEK_SET);

    fwrite (IAPU.RAM, 1, 65536, fs);

    fwrite (APU.DSP, 1, 192, fs);
    fwrite (APU.ExtraRAM, 1, 64, fs);

    fclose (fs);

    S9xSetSoundMute (FALSE);
}

