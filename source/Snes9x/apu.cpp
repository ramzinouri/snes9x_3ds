#include "copyright.h"


#include "snes9x.h"
#include "spc700.h"
#include "apu.h"
#include "soundux.h"
#include "cpuexec.h"

/* For note-triggered SPC dump support */
#include "snapshot.h"

extern int32 env_counter_table[32];

#include "3ds.h"
#include "3dsgpu.h"
#include "3dssnes9x.h"
#include "3dssound.h"
#include "3dsimpl.h"


int spc_is_dumping=0;
int spc_is_dumping_temp;
uint8 spc_dump_dsp[0x100];

extern int NoiseFreq [32];
#ifdef DEBUGGER
void S9xTraceSoundDSP (const char *s, int i1 = 0, int i2 = 0, int i3 = 0,
					   int i4 = 0, int i5 = 0, int i6 = 0, int i7 = 0);
#endif

bool8 S9xInitAPU ()
{
    IAPU.RAM = (uint8 *) malloc (0x10000);

    if (!IAPU.RAM)
    {
		S9xDeinitAPU ();
		return (FALSE);
    }

	memset(IAPU.RAM, 0, 0x10000);

    return (TRUE);
}

void S9xDeinitAPU ()
{
    if (IAPU.RAM)
    {
		free ((char *) IAPU.RAM);
		IAPU.RAM = NULL;
    }
}


EXTERN_C uint8 APUROM [64];

void S9xResetAPU ()
{

    int i;

    Settings.APUEnabled = Settings.NextAPUEnabled;

	if(Settings.APUEnabled)
		APU.Flags &= ~HALTED_FLAG;

	ZeroMemory(spc_dump_dsp, 0x100);
	ZeroMemory(IAPU.RAM, 0x100);
	memset(IAPU.RAM+0x20, 0xFF, 0x20);
	memset(IAPU.RAM+0x60, 0xFF, 0x20);
	memset(IAPU.RAM+0xA0, 0xFF, 0x20);
	memset(IAPU.RAM+0xE0, 0xFF, 0x20);

	for(i=1;i<256;i++)
	{
		memcpy(IAPU.RAM+(i<<8), IAPU.RAM, 0x100);
	}

    ZeroMemory (APU.OutPorts, 4);
    IAPU.DirectPage = IAPU.RAM;
    memmove (APU.ExtraRAM, &IAPU.RAM [0xffc0], sizeof (APUROM));
    memmove (&IAPU.RAM [0xffc0], APUROM, sizeof (APUROM));
    IAPU.PC = IAPU.RAM + IAPU.RAM [0xfffe] + (IAPU.RAM [0xffff] << 8);
    APU.Cycles = 0;
	APU.OldCycles = -99999999; // For shapshot compatibility
    APURegisters.YA.W = 0;
    APURegisters.X = 0;
    APURegisters.S = 0xef;
    APURegisters.P = 0x02;
    S9xAPUUnpackStatus ();
    APURegisters.PC = 0;
    IAPU.APUExecuting = Settings.APUEnabled;
#ifdef SPC700_SHUTDOWN
    IAPU.WaitAddress1 = NULL;
    IAPU.WaitAddress2 = NULL;
    IAPU.WaitCounter = 0;
#endif
	IAPU.NextAPUTimerPos = 0;
	IAPU.NextAPUTimerPosDiv10000 = 0;
	IAPU.APUTimerCounter = 0;
    APU.ShowROM = TRUE;
    IAPU.RAM [0xf1] = 0x80;

	// Doing this here reduces the possibility of a race condition
	// in the DSP core.
	//		
	IAPU.DSPWriteIndex = 0;
	IAPU.DSPReplayIndex = 0;
	
    for (i = 0; i < 3; i++)
    {
		APU.TimerEnabled [i] = FALSE;
		APU.TimerValueWritten [i] = 0;
		APU.TimerTarget [i] = 0;
		APU.Timer [i] = 0;
    }
    for (int j = 0; j < 0x80; j++)
		APU.DSP [j] = 0;

    IAPU.TwoCycles = IAPU.OneCycle * 2;

    for (i = 0; i < 256; i++)
		APU.S9xAPUCycles [i] = S9xAPUCycleLengths [i] * IAPU.OneCycle;
	
    APU.DSP [APU_ENDX] = 0;
    APU.DSP [APU_KOFF] = 0;
    APU.DSP [APU_KON] = 0;
    APU.DSP [APU_FLG] = APU_MUTE | APU_ECHO_DISABLED;
    APU.KeyedChannels = 0;

	for (int i = 0; i < 0x80; i++)
		IAPU.DSPCopy[i] = APU.DSP[i];

    S9xResetSound (TRUE);
    S9xSetEchoEnable (0);
}




void S9xSetAPUDSPReplay ()
{
    uint8 reg = IAPU.RAM [0xf2];

	int32 targetIndex = IAPU.DSPWriteIndex;
	while (IAPU.DSPReplayIndex != targetIndex)
	{
		S9xSetAPUDSP(
			IAPU.DSPWriteBuffer[IAPU.DSPReplayIndex].byte,
			IAPU.DSPWriteBuffer[IAPU.DSPReplayIndex].reg);

		IAPU.DSPReplayIndex = (IAPU.DSPReplayIndex + 1) & (DSPWRITEBUFFERSIZE - 1);
	}
}


void S9xSetAPUDSPLater (uint8 byte)
{
    uint8 reg = IAPU.RAM [0xf2];

	IAPU.DSPWriteBuffer[IAPU.DSPWriteIndex].reg = reg;	
	IAPU.DSPWriteBuffer[IAPU.DSPWriteIndex].byte = byte;	

	IAPU.DSPWriteIndex = (IAPU.DSPWriteIndex + 1) & (DSPWRITEBUFFERSIZE - 1);

	// Bug fix: Once the SPC code has written into the DSP register,
	// we will have to return the same value immediately upon read by the SPC code,
	// despite us queuing to write into our 3DS DSP core later.
	//
	// So this DSPCopy serves exactly that purpose.
	//
	IAPU.DSPCopy[reg] = byte;

	//if (reg == 0x4c ||reg == 0x5c ||reg == 0x6c ||reg == 0x7c)
	//	printf ("D %02x<%02x  APU.PC %04x\n", reg, byte, IAPU.PC - IAPU.RAM);
}


void S9xSetAPUDSP (uint8 byte, uint8 reg)
{
	static uint8 KeyOn;
	static uint8 KeyOnPrev;
    int i;
	int pitch;

	spc_dump_dsp[reg] = byte;

    switch (reg)
    {
    case APU_FLG:
		if (byte & APU_SOFT_RESET)
		{
			APU.DSP [reg] = APU_MUTE | APU_ECHO_DISABLED | (byte & 0x1f);
			APU.DSP [APU_ENDX] = 0;
			APU.DSP [APU_KOFF] = 0;
			APU.DSP [APU_KON] = 0;
			S9xSetEchoWriteEnable (FALSE);
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("[%d] DSP reset\n", ICPU.Scanline);
#endif
			// Kill sound
			S9xResetSound (FALSE);
		}
		else
		{
			S9xSetEchoWriteEnable (!(byte & APU_ECHO_DISABLED));
			if (byte & APU_MUTE)
			{
#ifdef DEBUGGER
				if (Settings.TraceSoundDSP)
					S9xTraceSoundDSP ("[%d] Mute sound\n", ICPU.Scanline);
#endif
				S9xSetSoundMute (TRUE);
			}
			else
				S9xSetSoundMute (FALSE);
			
			SoundData.noise_rate = env_counter_table[byte & 0x1f];
		}
		break;
    case APU_NON:
		if (byte != APU.DSP [APU_NON])
		{
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("[%d] Noise:", ICPU.Scanline);
#endif
			uint8 mask = 1;
			for (int c = 0; c < 8; c++, mask <<= 1)
			{
				int type;
				if (byte & mask)
				{
					type = SOUND_NOISE;
#ifdef DEBUGGER
					if (Settings.TraceSoundDSP)
					{
						if (APU.DSP [reg] & mask)
							S9xTraceSoundDSP ("%d,", c);
						else
							S9xTraceSoundDSP ("%d(on),", c);
					}
#endif
				}
				else
				{
					type = SOUND_SAMPLE;
#ifdef DEBUGGER
					if (Settings.TraceSoundDSP)
					{
						if (APU.DSP [reg] & mask)
							S9xTraceSoundDSP ("%d(off),", c);
					}
#endif
				}
				S9xSetSoundType (c, type);
			}
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("\n");
#endif
		}
		break;
    case APU_MVOL_LEFT:
		if (byte != APU.DSP [APU_MVOL_LEFT])
		{
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("[%d] Master volume left:%d\n",
				ICPU.Scanline, (signed char) byte);
#endif
			S9xSetMasterVolume ((signed char) byte,
				(signed char) APU.DSP [APU_MVOL_RIGHT]);
		}
		break;
    case APU_MVOL_RIGHT:
		if (byte != APU.DSP [APU_MVOL_RIGHT])
		{
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("[%d] Master volume right:%d\n",
				ICPU.Scanline, (signed char) byte);
#endif
			S9xSetMasterVolume ((signed char) APU.DSP [APU_MVOL_LEFT],
				(signed char) byte);
		}
		break;
    case APU_EVOL_LEFT:
		if (byte != APU.DSP [APU_EVOL_LEFT])
		{
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("[%d] Echo volume left:%d\n",
				ICPU.Scanline, (signed char) byte);
#endif
			S9xSetEchoVolume ((signed char) byte,
				(signed char) APU.DSP [APU_EVOL_RIGHT]);
		}
		break;
    case APU_EVOL_RIGHT:
		if (byte != APU.DSP [APU_EVOL_RIGHT])
		{
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("[%d] Echo volume right:%d\n",
				ICPU.Scanline, (signed char) byte);
#endif
			S9xSetEchoVolume ((signed char) APU.DSP [APU_EVOL_LEFT],
				(signed char) byte);
		}
		break;
    case APU_ENDX:
#ifdef DEBUGGER
		if (Settings.TraceSoundDSP)
			S9xTraceSoundDSP ("[%d] Reset ENDX\n", ICPU.Scanline);
#endif
		byte = 0;
		break;

    case APU_KOFF:
		//		if (byte)
		{
			uint8 mask = 1;
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("[%d] Key off:", ICPU.Scanline);
#endif
			for (int c = 0; c < 8; c++, mask <<= 1)
			{
				if ((byte & mask) != 0)
				{
#ifdef DEBUGGER

					if (Settings.TraceSoundDSP)
						S9xTraceSoundDSP ("%d,", c);
#endif
					if (APU.KeyedChannels & mask)
					{
						{
							KeyOnPrev&=~mask;
							APU.KeyedChannels &= ~mask;
							APU.DSP [APU_KON] &= ~mask;
							//APU.DSP [APU_KOFF] |= mask;
							S9xSetSoundKeyOff (c);
						}
					}
				}
				else if((KeyOnPrev&mask)!=0)
				{
					KeyOnPrev&=~mask;
					APU.KeyedChannels |= mask;
					//APU.DSP [APU_KON] |= mask;
					APU.DSP [APU_KOFF] &= ~mask;
					APU.DSP [APU_ENDX] &= ~mask;
					S9xPlaySample (c);
				}
			}
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("\n");
#endif
		}
		//KeyOnPrev=0;
		APU.DSP [APU_KOFF] = byte;
		return;
    case APU_KON:
		if (spc_is_dumping)
		{
			if (byte & ~spc_is_dumping_temp)
			{
				APURegisters.PC = IAPU.PC - IAPU.RAM;
				S9xAPUPackStatus();
				S9xSPCDump (S9xGetFilenameInc (".spc"));
				spc_is_dumping = 0;
			}
		}
		if (byte)
		{
			uint8 mask = 1;
#ifdef DEBUGGER

			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("[%d] Key on:", ICPU.Scanline);
#endif
			for (int c = 0; c < 8; c++, mask <<= 1)
			{
				if ((byte & mask) != 0)
				{
#ifdef DEBUGGER
					if (Settings.TraceSoundDSP)
						S9xTraceSoundDSP ("%d,", c);
#endif
					// Pac-In-Time requires that channels can be key-on
					// regardeless of their current state.
					if((APU.DSP [APU_KOFF] & mask) ==0)
					{
						KeyOnPrev&=~mask;
						APU.KeyedChannels |= mask;
						//APU.DSP [APU_KON] |= mask;
						//APU.DSP [APU_KOFF] &= ~mask;
						APU.DSP [APU_ENDX] &= ~mask;
						S9xPlaySample (c);
					}
					else KeyOn|=mask;
				}
			}
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("\n");
#endif
		}
		spc_is_dumping_temp = byte;
		return;

    case APU_VOL_LEFT + 0x00:
    case APU_VOL_LEFT + 0x10:
    case APU_VOL_LEFT + 0x20:
    case APU_VOL_LEFT + 0x30:
    case APU_VOL_LEFT + 0x40:
    case APU_VOL_LEFT + 0x50:
    case APU_VOL_LEFT + 0x60:
    case APU_VOL_LEFT + 0x70:
		// At Shin Megami Tensei suggestion 6/11/00
		//	if (byte != APU.DSP [reg])
		{
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("[%d] %d volume left: %d\n",
				ICPU.Scanline, reg>>4, (signed char) byte);
#endif
			S9xSetSoundVolume (reg >> 4, (signed char) byte,
				(signed char) APU.DSP [reg + 1]);
		}
		break;
    case APU_VOL_RIGHT + 0x00:
    case APU_VOL_RIGHT + 0x10:
    case APU_VOL_RIGHT + 0x20:
    case APU_VOL_RIGHT + 0x30:
    case APU_VOL_RIGHT + 0x40:
    case APU_VOL_RIGHT + 0x50:
    case APU_VOL_RIGHT + 0x60:
    case APU_VOL_RIGHT + 0x70:
		// At Shin Megami Tensei suggestion 6/11/00
		//	if (byte != APU.DSP [reg])
		{
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("[%d] %d volume right: %d\n",
				ICPU.Scanline, reg >>4, (signed char) byte);
#endif
			S9xSetSoundVolume (reg >> 4, (signed char) APU.DSP [reg - 1],
				(signed char) byte);
		}
		break;

    case APU_P_LOW + 0x00:
    case APU_P_LOW + 0x10:
    case APU_P_LOW + 0x20:
    case APU_P_LOW + 0x30:
    case APU_P_LOW + 0x40:
    case APU_P_LOW + 0x50:
    case APU_P_LOW + 0x60:
    case APU_P_LOW + 0x70:
#ifdef DEBUGGER
		if (Settings.TraceSoundDSP)
			S9xTraceSoundDSP ("[%d] %d freq low: %d\n",
			ICPU.Scanline, reg>>4, byte);
#endif
		pitch = (((int)byte + ((int)APU.DSP [reg + 1] << 8)) & FREQUENCY_MASK);
		S9xSetSoundHertz (reg >> 4, pitch * 8);
		//S9xSetSoundHertz (reg >> 4, ((byte + (APU.DSP [reg + 1] << 8)) & FREQUENCY_MASK) * 8);
		break;

    case APU_P_HIGH + 0x00:
    case APU_P_HIGH + 0x10:
    case APU_P_HIGH + 0x20:
    case APU_P_HIGH + 0x30:
    case APU_P_HIGH + 0x40:
    case APU_P_HIGH + 0x50:
    case APU_P_HIGH + 0x60:
    case APU_P_HIGH + 0x70:
#ifdef DEBUGGER
		if (Settings.TraceSoundDSP)
			S9xTraceSoundDSP ("[%d] %d freq high: %d\n",
			ICPU.Scanline, reg>>4, byte);
#endif
		pitch = ((((int)byte << 8) + (int)APU.DSP [reg - 1]) & FREQUENCY_MASK);
		S9xSetSoundHertz (reg >> 4, pitch * 8);
		//S9xSetSoundHertz (reg >> 4, (((byte << 8) + APU.DSP [reg - 1]) & FREQUENCY_MASK) * 8);
		break;

    case APU_SRCN + 0x00:
    case APU_SRCN + 0x10:
    case APU_SRCN + 0x20:
    case APU_SRCN + 0x30:
    case APU_SRCN + 0x40:
    case APU_SRCN + 0x50:
    case APU_SRCN + 0x60:
    case APU_SRCN + 0x70:
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("[%d] %d sample number: %d\n",
				ICPU.Scanline, reg>>4, byte);
#endif
		break;

    case APU_ADSR1 + 0x00:
    case APU_ADSR1 + 0x10:
    case APU_ADSR1 + 0x20:
    case APU_ADSR1 + 0x30:
    case APU_ADSR1 + 0x40:
    case APU_ADSR1 + 0x50:
    case APU_ADSR1 + 0x60:
    case APU_ADSR1 + 0x70:
		if (byte != APU.DSP [reg])
		{
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("[%d] %d adsr1: %02x\n",
				ICPU.Scanline, reg>>4, byte);
#endif
			{
				S9xFixEnvelope (reg >> 4, APU.DSP [reg + 2], byte,
					APU.DSP [reg + 1]);
			}
		}
		break;

    case APU_ADSR2 + 0x00:
    case APU_ADSR2 + 0x10:
    case APU_ADSR2 + 0x20:
    case APU_ADSR2 + 0x30:
    case APU_ADSR2 + 0x40:
    case APU_ADSR2 + 0x50:
    case APU_ADSR2 + 0x60:
    case APU_ADSR2 + 0x70:
		if (byte != APU.DSP [reg])
		{
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("[%d] %d adsr2: %02x\n",
				ICPU.Scanline, reg>>4, byte);
#endif
			{
				S9xFixEnvelope (reg >> 4, APU.DSP [reg + 1], APU.DSP [reg - 1],
					byte);
			}
		}
		break;

    case APU_GAIN + 0x00:
    case APU_GAIN + 0x10:
    case APU_GAIN + 0x20:
    case APU_GAIN + 0x30:
    case APU_GAIN + 0x40:
    case APU_GAIN + 0x50:
    case APU_GAIN + 0x60:
    case APU_GAIN + 0x70:
		if (byte != APU.DSP [reg])
		{
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("[%d] %d gain: %02x\n",
				ICPU.Scanline, reg>>4, byte);
#endif
			{
				S9xFixEnvelope (reg >> 4, byte, APU.DSP [reg - 2],
					APU.DSP [reg - 1]);
			}
		}
		break;

    case APU_ENVX + 0x00:
    case APU_ENVX + 0x10:
    case APU_ENVX + 0x20:
    case APU_ENVX + 0x30:
    case APU_ENVX + 0x40:
    case APU_ENVX + 0x50:
    case APU_ENVX + 0x60:
    case APU_ENVX + 0x70:
		break;

    case APU_OUTX + 0x00:
    case APU_OUTX + 0x10:
    case APU_OUTX + 0x20:
    case APU_OUTX + 0x30:
    case APU_OUTX + 0x40:
    case APU_OUTX + 0x50:
    case APU_OUTX + 0x60:
    case APU_OUTX + 0x70:
		break;

    case APU_DIR:
#ifdef DEBUGGER
		if (Settings.TraceSoundDSP)
			S9xTraceSoundDSP ("[%d] Sample directory to: %02x\n",
			ICPU.Scanline, byte);
#endif
		break;

    case APU_PMON:
		if (byte != APU.DSP [APU_PMON])
		{
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
			{
				S9xTraceSoundDSP ("[%d] FreqMod:", ICPU.Scanline);
				uint8 mask = 1;
				for (int c = 0; c < 8; c++, mask <<= 1)
				{
					if (byte & mask)
					{
						if (APU.DSP [reg] & mask)
							S9xTraceSoundDSP ("%d", c);
						else
							S9xTraceSoundDSP ("%d(on),", c);
					}
					else
					{
						if (APU.DSP [reg] & mask)
							S9xTraceSoundDSP ("%d(off),", c);
					}
				}
				S9xTraceSoundDSP ("\n");
			}
#endif
			S9xSetFrequencyModulationEnable (byte);
		}
		break;

    case APU_EON:
		if (byte != APU.DSP [APU_EON])
		{
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
			{
				S9xTraceSoundDSP ("[%d] Echo:", ICPU.Scanline);
				uint8 mask = 1;
				for (int c = 0; c < 8; c++, mask <<= 1)
				{
					if (byte & mask)
					{
						if (APU.DSP [reg] & mask)
							S9xTraceSoundDSP ("%d", c);
						else
							S9xTraceSoundDSP ("%d(on),", c);
					}
					else
					{
						if (APU.DSP [reg] & mask)
							S9xTraceSoundDSP ("%d(off),", c);
					}
				}
				S9xTraceSoundDSP ("\n");
			}
#endif
			S9xSetEchoEnable (byte);
		}
		break;

    case APU_EFB:
		S9xSetEchoFeedback ((signed char) byte);
		break;

    case APU_ESA:
		break;

    case APU_EDL:
		S9xSetEchoDelay (byte & 0xf);
		break;

    case APU_C0:
    case APU_C1:
    case APU_C2:
    case APU_C3:
    case APU_C4:
    case APU_C5:
    case APU_C6:
    case APU_C7:
		S9xSetFilterCoefficient (reg >> 4, (signed char) byte);
		break;
    default:
		// XXX
		//printf ("Write %02x to unknown APU register %02x\n", byte, reg);
		break;
    }

	KeyOnPrev|=KeyOn;
	KeyOn=0;

    if (reg < 0x80)
		APU.DSP [reg] = byte;
}

void S9xFixEnvelope (int channel, uint8 gain, uint8 adsr1, uint8 adsr2)
{
	if (adsr1 & 0x80)
	{
		if (S9xSetSoundMode (channel, MODE_ADSR))
			S9xSetSoundADSR (channel, adsr1 & 0xf, (adsr1 >> 4) & 7,
				adsr2 & 0x1f, (adsr2 >> 5) & 7);
	}
	else
	{
		if ((gain & 0x80) == 0)
		{
			if (S9xSetSoundMode (channel, MODE_GAIN))
				S9xSetEnvelopeHeight (channel, (gain & 0x7f) << ENV_SHIFT);
		}
		else
		{
			if (gain & 0x40)
			{
				if (S9xSetSoundMode (channel, (gain & 0x20) ?
					MODE_INCREASE_BENT_LINE : MODE_INCREASE_LINEAR))
					S9xSetEnvelopeRate (channel, env_counter_table[gain & 0x1f], ENV_MAX);
			}
			else
			{
				if (S9xSetSoundMode (channel, (gain & 0x20) ?
					MODE_DECREASE_EXPONENTIAL : MODE_DECREASE_LINEAR))
					S9xSetEnvelopeRate (channel, env_counter_table[gain & 0x1f], 0);
			}
		}
	}
}

void S9xSetAPUControl (uint8 byte)
{
	//if (byte & 0x40)
	//printf ("*** Special SPC700 timing enabled\n");
    if ((byte & 1) != 0 && !APU.TimerEnabled [0])
    {
		APU.Timer [0] = 0;
		IAPU.RAM [0xfd] = 0;
		if ((APU.TimerTarget [0] = IAPU.RAM [0xfa]) == 0)
			APU.TimerTarget [0] = 0x100;
    }
    if ((byte & 2) != 0 && !APU.TimerEnabled [1])
    {
		APU.Timer [1] = 0;
		IAPU.RAM [0xfe] = 0;
		if ((APU.TimerTarget [1] = IAPU.RAM [0xfb]) == 0)
			APU.TimerTarget [1] = 0x100;
    }
    if ((byte & 4) != 0 && !APU.TimerEnabled [2])
    {
		APU.Timer [2] = 0;
		IAPU.RAM [0xff] = 0;
		if ((APU.TimerTarget [2] = IAPU.RAM [0xfc]) == 0)
			APU.TimerTarget [2] = 0x100;
    }
    APU.TimerEnabled [0] = byte & 1;
    APU.TimerEnabled [1] = (byte & 2) >> 1;
    APU.TimerEnabled [2] = (byte & 4) >> 2;

    if (byte & 0x10)
		IAPU.RAM [0xF4] = IAPU.RAM [0xF5] = 0;

    if (byte & 0x20)
		IAPU.RAM [0xF6] = IAPU.RAM [0xF7] = 0;

    if (byte & 0x80)
    {
		if (!APU.ShowROM)
		{
			memmove (&IAPU.RAM [0xffc0], APUROM, sizeof (APUROM));
			APU.ShowROM = TRUE;
		}
    }
    else
    {
		if (APU.ShowROM)
		{
			APU.ShowROM = FALSE;
			memmove (&IAPU.RAM [0xffc0], APU.ExtraRAM, sizeof (APUROM));
		}
    }
    IAPU.RAM [0xf1] = byte;
}

void S9xSetAPUTimer (uint16 Address, uint8 byte)
{
    IAPU.RAM [Address] = byte;

    switch (Address)
    {
    case 0xfa:
		if ((APU.TimerTarget [0] = IAPU.RAM [0xfa]) == 0)
			APU.TimerTarget [0] = 0x100;
		APU.TimerValueWritten [0] = TRUE;
		break;
    case 0xfb:
		if ((APU.TimerTarget [1] = IAPU.RAM [0xfb]) == 0)
			APU.TimerTarget [1] = 0x100;
		APU.TimerValueWritten [1] = TRUE;
		break;
    case 0xfc:
		if ((APU.TimerTarget [2] = IAPU.RAM [0xfc]) == 0)
			APU.TimerTarget [2] = 0x100;
		APU.TimerValueWritten [2] = TRUE;
		break;
    }
}


uint8 S9xGetAPUDSP ()
{
    uint8 reg = IAPU.RAM [0xf2] & 0x7f;

	// Bug fix: Once the value is written to the DSP by the
	// SPC code, we must allow it to read it back immediately
	// despite delaying the write to our 3DS DSP emulated core.
	//
    uint8 byte = IAPU.DSPCopy [reg];
	
    switch (reg)
    {
		case APU_KON:
			break;
		case APU_KOFF:
			break;

		case APU_OUTX + 0x00:
		case APU_OUTX + 0x10:
		case APU_OUTX + 0x20:
		case APU_OUTX + 0x30:
		case APU_OUTX + 0x40:
		case APU_OUTX + 0x50:
		case APU_OUTX + 0x60:
		case APU_OUTX + 0x70:
		if(Settings.FakeMuteFix)
		{
			// hack that is off by default: fixes Terranigma desync
			return (0);
		}
		else
		{
			if (SoundData.channels [reg >> 4].state == SOUND_SILENT)
				return (0);
			return (int8) (SoundData.channels [reg >> 4].out_sample >> 8);
		}

		case APU_ENVX + 0x00:
		case APU_ENVX + 0x10:
		case APU_ENVX + 0x20:
		case APU_ENVX + 0x30:
		case APU_ENVX + 0x40:
		case APU_ENVX + 0x50:
		case APU_ENVX + 0x60:
		case APU_ENVX + 0x70:
			return (S9xGetEnvelopeHeight (reg >> 4));

		case APU_ENDX:
			// To fix speech in Magical Drop 2 6/11/00
			//	APU.DSP [APU_ENDX] = 0;
			
			// Bug fix: The ENDX is written to the APU.DSP
			// so we cannot return IAPU.DSPCopy. This fixes
			// Clock Tower.
			//
			byte = APU.DSP[APU_ENDX];
			break;

		default:
			break;
	}

	return (byte);
}


inline void S9xIncrementAPUTimers()
{
	// SNES_APUTIMER2_CYCLEx10000 = 3355836
	//
	// Bug Fix: Is this an inherent SNES9X bug? The NextAPUTimerPos (int32) has a high 
	// chance of overflowing when the DMA transfer length is large!
	//
	// Unfortunately in devKitPro for 3DS, software 64-bit math is not supported, so we will
	// have to resort to dividing the NextAPUTimerPos by 10 to minimize the possibility
	// of overflow.
	//
	// Hopefully, this change in the SPC timing doesn't cause games to break!
	//
	IAPU.NextAPUTimerPos = IAPU.NextAPUTimerPos + (SNES_APUTIMER2_CYCLEx10000 / 10);	
	IAPU.NextAPUTimerPosDiv10000 = IAPU.NextAPUTimerPos / 1000;
	
	if (APU.TimerEnabled [2])
	{
		APU.Timer [2] ++;
		if (APU.Timer [2] >= APU.TimerTarget [2])
		{
			IAPU.RAM [0xff] = (IAPU.RAM [0xff] + 1) & 0xf;
			APU.Timer [2] = 0;
		#ifdef SPC700_SHUTDOWN		
			IAPU.WaitCounter++;
			IAPU.APUExecuting = TRUE;
		#endif		
		}
	}

	if (++IAPU.APUTimerCounter == 8)
	{
		IAPU.APUTimerCounter = 0;
		
		if (APU.TimerEnabled [0])
		{
			APU.Timer [0]++;
			if (APU.Timer [0] >= APU.TimerTarget [0])
			{
				IAPU.RAM [0xfd] = (IAPU.RAM [0xfd] + 1) & 0xf;
				APU.Timer [0] = 0;
			#ifdef SPC700_SHUTDOWN		
				IAPU.WaitCounter++;
				IAPU.APUExecuting = TRUE;
			#endif		    
			}
		}

		if (APU.TimerEnabled [1])
		{
			APU.Timer [1]++;
			if (APU.Timer [1] >= APU.TimerTarget [1])
			{
				IAPU.RAM [0xfe] = (IAPU.RAM [0xfe] + 1) & 0xf;
				APU.Timer [1] = 0;
			#ifdef SPC700_SHUTDOWN		
				IAPU.WaitCounter++;
				IAPU.APUExecuting = TRUE;
			#endif		    
			}
		}
	}	
}


void S9xUpdateAPUTimer (void)
{
	//int opcodesExecuted = 0;
	//t3dsStartTiming(51, "APU");
	int32 nextEventCycles = IAPU.NextAPUTimerPosDiv10000;
	if (nextEventCycles >= CPU.Cycles)
		nextEventCycles = CPU.Cycles;
	//printf ("S9xAPUTimer:\n");
	while (true)
	{
#ifndef DEBUG_APU
		#define DEBUG_OUTPUT
#else
		static char debugOutputLine[255];
		#define DEBUG_OUTPUT \
			if (IAPU.PC - IAPU.RAM == 0x10000 && APURegisters.YA.B.A == 0x6c) GPU3DS.enableDebug = true; \
			if (GPU3DS.enableDebug) \
			{ \
				S9xAPUOPrint(debugOutputLine, (IAPU.PC - IAPU.RAM)); \
				printf ("%s", debugOutputLine); \
				DEBUG_WAIT_L_KEY \
			}

#endif

		#define EXECUTE_ONE_APU_OPCODE \
			if (APU.Cycles > nextEventCycles) break; \
			APU.Cycles += APU.S9xAPUCycles [*IAPU.PC]; \
			(*S9xApuOpcodes[*IAPU.PC]) (); \
			DEBUG_OUTPUT
			
		// Execute on the APU.
		//
		//opcodesExecuted = 0;
		while (true) 
		{ 
			EXECUTE_ONE_APU_OPCODE
			EXECUTE_ONE_APU_OPCODE
			EXECUTE_ONE_APU_OPCODE
			EXECUTE_ONE_APU_OPCODE
			EXECUTE_ONE_APU_OPCODE
			EXECUTE_ONE_APU_OPCODE
			EXECUTE_ONE_APU_OPCODE
			EXECUTE_ONE_APU_OPCODE
			EXECUTE_ONE_APU_OPCODE
			EXECUTE_ONE_APU_OPCODE
		} 
		//printf ("  opcodes executed %d:\n", opcodesExecuted);

		if (nextEventCycles >= CPU.Cycles)
			break;
		else if (nextEventCycles >= IAPU.NextAPUTimerPosDiv10000) 
		{ 
			//printf ("  inc timers:\n");
			
			S9xIncrementAPUTimers(); 
			nextEventCycles = IAPU.NextAPUTimerPosDiv10000;
			if (nextEventCycles >= CPU.Cycles)
				nextEventCycles = CPU.Cycles;
		}
		else 
		{
			printf ("Strange: APU cycles:%d nec:%d ntimer:%d\n", APU.Cycles, nextEventCycles, IAPU.NextAPUTimerPosDiv10000);
			break;
		}
	}

	//CPU.PrevCycles = CPU.Cycles;
	//t3dsEndTiming(51);
}


