#include "copyright.h"


#ifndef _DSP1_H_
#define _DSP1_H_

extern void  (*SetDSP)(uint8, uint16);
extern uint8 (*GetDSP)(uint16);

void  DSP1SetByte(uint8 byte, uint16 address);
uint8 DSP1GetByte(uint16 address);

void  DSP2SetByte(uint8 byte, uint16 address);
uint8 DSP2GetByte(uint16 address);

void  DSP3SetByte(uint8 byte, uint16 address);
uint8 DSP3GetByte(uint16 address);
void  DSP3_Reset();

void  DSP4SetByte(uint8 byte, uint16 address);
uint8 DSP4GetByte(uint16 address);

struct SDSP1 {
	uint8 version;
    bool8 waiting4command;
    bool8 first_parameter;
    uint8 command;
    uint32 in_count;
    uint32 in_index;
    uint32 out_count;
    uint32 out_index;
    uint8 parameters [512];
    uint8 output [512];
};

START_EXTERN_C
void S9xResetDSP1 ();
uint8 S9xGetDSP (uint16 Address);
void S9xSetDSP (uint8 Byte, uint16 Address);
extern struct SDSP1 DSP1;
END_EXTERN_C



#endif
