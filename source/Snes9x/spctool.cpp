#include "copyright.h"


#include "snes9x.h"
#include "spctool/dsp.h"
#include "spctool/spc700.h"
#include "spctool/soundmod.h"
#include "apu.h"

bool8 S9xOpenSoundDevice (int, bool8, int);

void S9xSetPlaybackRate (uint32 rate)
{
    DOpt		SmpOpt;
    
    SmpOpt.Smp8bit=false;
    SmpOpt.SmpMono=false;
    SmpOpt.IntType=IntC;
    SmpOpt.OldBRE=false;
    SmpOpt.MixRout=1;
    SetSPUOpt (rate, SmpOpt);

//    so.playback_rate = playback_rate;
//    so.err_rate = (uint32) (SNES_SCANLINE_TIME * FIXED_POINT / (1.0 / (double) so.playback_rate));
 }

bool8 S9xSetSoundMute (bool8 mute)
{
    return (TRUE);
}

START_EXTERN_C
bool8 S9xInitSound (int mode, bool8 stereo, int buffer_size)
{
    if (!(mode & 7))
	return (TRUE);

    S9xSetSoundMute (TRUE);
    if (!S9xOpenSoundDevice (mode, stereo, buffer_size))
    {
	S9xMessage (S9X_ERROR, S9X_SOUND_DEVICE_OPEN_FAILED,
		    "Sound device open failed");
	return (FALSE);
    }
    return (TRUE);
}

void TraceSPC (unsigned char *PC, unsigned short YA, unsigned char X,
	       SPCFlags PS, unsigned char *SP)
{
    APURegisters.YA.W = YA;
    APURegisters.X = X;
    APURegisters.S = SP - IAPU.RAM;
    IAPU.PC = PC;
    IAPU._Carry = PS.C;
    IAPU._Zero = !PS.Z | (PS.N << 7);
    IAPU._Overflow = PS.V;
    APURegisters.P = *(uint8 *) &PS;
    S9xTraceAPU ();
}

bool8 S9xInitAPU ()
{
    void		*SPCBase;               //Base pointer and aligned pointer to SPC RAM
    
    SPCBase=malloc(131072);                  //Allocate memory for SPC RAM
	memset(SPCBase, 0, 131072);

    IAPU.RAM=(uint8 *) InitSPU(SPCBase);                 //Initialize SPU w/ ptr to SPC RAM (Call only once)

    S9xSetPlaybackRate (22000);
    ResetSPU(20);                            //Reset SPU with pre-amp level of 30
//    _SetSPCDbg(TraceSPC);                   //Install debug handler
   return (TRUE);
}

void S9xResetAPU ()
{
    ResetSPU(20);
    IAPU.RAM [0xf1] = 0x80;
    _FixSPC (0xffc0, 0, 0, 0, 0, 0xff);
//    FixDSP ();
}

extern "C" void EDSP (uint8 *, int32);

void S9xMixSamplesO (uint8 *buffer, int sample_count, int byte_offset)
{
//    _EmuDSP (buffer + byte_offset, sample_count / 2);

    EDSP (buffer + byte_offset, sample_count / 2);
#if 0
for (int i = 0; i < 32; i++)
printf ("%02x,", *(buffer + byte_offset + i));
printf ("\n");
#endif
}
END_EXTERN_C

void S9xFixSoundAfterSnapshotLoad ()
{
}

void S9xSetSoundControl (int)
{
}

#ifdef DEBUGGER
START_EXTERN_C
void S9xDeinitAPU ()
{
}

END_EXTERN_C
#endif

