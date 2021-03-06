#include "copyright.h"


#ifndef _CPUADDR_H_
#define _CPUADDR_H_


typedef enum {
    NONE = 0,
    READ = 1,
    WRITE = 2,
    MODIFY = 3,
    JUMP = 4
} AccessMode;

STATIC inline void __attribute__((always_inline)) Immediate8 (AccessMode a)
{
    OpAddress = ICPU.ShiftedPB + CPU.PC - CPU.PCBase;
    CPU.PC++;
}

STATIC inline long __attribute__((always_inline)) Immediate8Fast (AccessMode a)
{
    long addr = ICPU.ShiftedPB + CPU.PC - CPU.PCBase;
    CPU.PC++;
    return addr;
}

STATIC inline void __attribute__((always_inline)) Immediate16 (AccessMode a)
{
    OpAddress = ICPU.ShiftedPB + CPU.PC - CPU.PCBase;
    CPU.PC += 2;
}

STATIC inline long __attribute__((always_inline)) Immediate16Fast (AccessMode a)
{
    long addr = ICPU.ShiftedPB + CPU.PC - CPU.PCBase;
    CPU.PC += 2;
    return addr;
}

STATIC inline void __attribute__((always_inline)) Relative (AccessMode a)
{
    Int8 = *CPU.PC++;
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeed;
#endif    
    OpAddress = ((int) (CPU.PC - CPU.PCBase) + Int8) & 0xffff;
}

STATIC inline long __attribute__((always_inline)) RelativeFast (AccessMode a)
{
    signed char i8 = *CPU.PC++;
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeed;
#endif    
    return ((int) (CPU.PC - CPU.PCBase) + i8) & 0xffff;
}

STATIC inline void __attribute__((always_inline)) RelativeLong (AccessMode a)
{
#ifdef FAST_LSB_WORD_ACCESS
    OpAddress = *(uint16 *) CPU.PC;
#else
    OpAddress = *CPU.PC + (*(CPU.PC + 1) << 8);
#endif
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeedx2 + ONE_CYCLE;
#endif
    CPU.PC += 2;
    OpAddress += (CPU.PC - CPU.PCBase);
    OpAddress &= 0xffff;
}

STATIC inline void __attribute__((always_inline)) AbsoluteIndexedIndirect (AccessMode a)
{
#ifdef FAST_LSB_WORD_ACCESS
    OpAddress = (Registers.X.W + *(uint16 *) CPU.PC) & 0xffff;
#else
    OpAddress = (Registers.X.W + *CPU.PC + (*(CPU.PC + 1) << 8)) & 0xffff;
#endif
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeedx2;
#endif
    OpenBus = *(CPU.PC + 1);
    CPU.PC += 2;
    OpAddress = S9xGetWord (ICPU.ShiftedPB + OpAddress);
    if(a&READ) OpenBus = (uint8)(OpAddress>>8);
}

STATIC inline void __attribute__((always_inline)) AbsoluteIndirectLong (AccessMode a)
{
#ifdef FAST_LSB_WORD_ACCESS
    OpAddress = *(uint16 *) CPU.PC;
#else
    OpAddress = *CPU.PC + (*(CPU.PC + 1) << 8);
#endif

#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeedx2;
#endif
    OpenBus = *(CPU.PC + 1);
    CPU.PC += 2;
    if(a&READ) {
	OpAddress = S9xGetWord (OpAddress) | ((OpenBus=S9xGetByte (OpAddress + 2)) << 16);
    } else {
    OpAddress = S9xGetWord (OpAddress) | (S9xGetByte (OpAddress + 2) << 16);
    }
}

STATIC inline void __attribute__((always_inline)) AbsoluteIndirect (AccessMode a)
{
#ifdef FAST_LSB_WORD_ACCESS
    OpAddress = *(uint16 *) CPU.PC;
#else
    OpAddress = *CPU.PC + (*(CPU.PC + 1) << 8);
#endif

#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeedx2;
#endif
    OpenBus = *(CPU.PC + 1);
    CPU.PC += 2;
    OpAddress = S9xGetWord (OpAddress);
    if(a&READ) OpenBus = (uint8)(OpAddress>>8);
    OpAddress += ICPU.ShiftedPB;
}

STATIC inline void __attribute__((always_inline)) Absolute (AccessMode a)
{
#ifdef FAST_LSB_WORD_ACCESS
    OpAddress = *(uint16 *) CPU.PC + ICPU.ShiftedDB;
#else
    OpAddress = *CPU.PC + (*(CPU.PC + 1) << 8) + ICPU.ShiftedDB;
#endif
    if(a&READ) OpenBus = *(CPU.PC+1);
    CPU.PC += 2;
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeedx2;
#endif
}

STATIC inline long __attribute__((always_inline)) AbsoluteFast (AccessMode a)
{
#ifdef FAST_LSB_WORD_ACCESS
    long addr = *(uint16 *) CPU.PC + ICPU.ShiftedDB;
#else
    long addr = *CPU.PC + (*(CPU.PC + 1) << 8) + ICPU.ShiftedDB;
#endif
    if(a&READ) OpenBus = *(CPU.PC+1);
    CPU.PC += 2;
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeedx2;
#endif
    return addr;
}

STATIC inline long __attribute__((always_inline)) AbsoluteFastRead ()
{
#ifdef FAST_LSB_WORD_ACCESS
    long addr = *(uint16 *) CPU.PC + ICPU.ShiftedDB;
#else 
    long addr = *CPU.PC + (*(CPU.PC + 1) << 8) + ICPU.ShiftedDB;
#endif
    OpenBus = *(CPU.PC+1);
    CPU.PC += 2;
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeedx2;
#endif
    return addr;
}

STATIC inline long __attribute__((always_inline)) AbsoluteFastWrite ()
{
#ifdef FAST_LSB_WORD_ACCESS
    long addr = *(uint16 *) CPU.PC + ICPU.ShiftedDB;
#else
    long addr = *CPU.PC + (*(CPU.PC + 1) << 8) + ICPU.ShiftedDB;
#endif
    //if(a&READ) OpenBus = *(CPU.PC+1);
    CPU.PC += 2;
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeedx2;
#endif
    return addr;
}

STATIC inline void __attribute__((always_inline)) AbsoluteLong (AccessMode a)
{
#ifdef FAST_LSB_WORD_ACCESS
    OpAddress = (*(uint32 *) CPU.PC) & 0xffffff;
#else
    OpAddress = *CPU.PC + (*(CPU.PC + 1) << 8) + (*(CPU.PC + 2) << 16);
#endif
    if(a&READ) OpenBus = *(CPU.PC+2);
    CPU.PC += 3;
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeedx2 + CPU.MemSpeed;
#endif
}

STATIC inline void __attribute__((always_inline)) Direct(AccessMode a)
{
    if(a&READ) OpenBus = *CPU.PC;
    OpAddress = (*CPU.PC++ + Registers.D.W) & 0xffff;
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeed;
#endif
//    if (Registers.DL != 0) CPU.Cycles += ONE_CYCLE;
}

STATIC inline long __attribute__((always_inline)) DirectFast (AccessMode a)
{
    if(a&READ) OpenBus = *CPU.PC;
    long addr = (*CPU.PC++ + Registers.D.W) & 0xffff;
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeed;
#endif
    return addr;
//    if (Registers.DL != 0) CPU.Cycles += ONE_CYCLE;
}

STATIC inline long __attribute__((always_inline)) DirectFastRead()
{
    OpenBus = *CPU.PC;
    long addr = (*CPU.PC++ + Registers.D.W) & 0xffff;
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeed;
#endif
    return addr;
//    if (Registers.DL != 0) CPU.Cycles += ONE_CYCLE;
}

STATIC inline long __attribute__((always_inline)) DirectFastWrite()
{
    long addr = (*CPU.PC++ + Registers.D.W) & 0xffff;
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeed;
#endif
    return addr;
//    if (Registers.DL != 0) CPU.Cycles += ONE_CYCLE;
}

STATIC inline void __attribute__((always_inline)) DirectIndirectIndexed (AccessMode a)
{
    OpenBus = *CPU.PC;
    OpAddress = (*CPU.PC++ + Registers.D.W) & 0xffff;
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeed;
#endif

    OpAddress = S9xGetWord (OpAddress);
    if(a&READ) OpenBus = (uint8)(OpAddress>>8);
    OpAddress += ICPU.ShiftedDB + Registers.Y.W;

//    if (Registers.DL != 0) CPU.Cycles += ONE_CYCLE;
    // XXX: always add one if STA
    // XXX: else Add one cycle if crosses page boundary
}

STATIC inline long __attribute__((always_inline)) DirectIndirectIndexedFast (AccessMode a)
{
    OpenBus = *CPU.PC;
    long addr = (*CPU.PC++ + Registers.D.W) & 0xffff;
#ifndef SA1_OPCODES
    CPU_Cycles += CPU.MemSpeed;
#endif

    addr = S9xGetWord (addr);
    if(a&READ) OpenBus = (uint8)(addr>>8);
    addr += ICPU.ShiftedDB + Registers.Y.W;

//    if (Registers.DL != 0) CPU_Cycles += ONE_CYCLE;
    // XXX: always add one if STA
    // XXX: else Add one cycle if crosses page boundary
    return addr;
}

STATIC inline void __attribute__((always_inline)) DirectIndirectIndexedLong (AccessMode a)
{
    OpenBus = *CPU.PC;
    OpAddress = (*CPU.PC++ + Registers.D.W) & 0xffff;
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeed;
#endif

    if(a&READ){
	OpAddress = S9xGetWord (OpAddress) + ((OpenBus = S9xGetByte (OpAddress + 2)) << 16) + Registers.Y.W;
    } else {
	OpAddress = S9xGetWord (OpAddress) + (S9xGetByte (OpAddress + 2) << 16) + Registers.Y.W;
    }
//    if (Registers.DL != 0) CPU.Cycles += ONE_CYCLE;
}

STATIC inline void __attribute__((always_inline)) DirectIndexedIndirect(AccessMode a)
{
    OpenBus = *CPU.PC;
    OpAddress = (*CPU.PC++ + Registers.D.W + Registers.X.W) & 0xffff;
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeed;
#endif

    OpAddress = S9xGetWord (OpAddress);
    if(a&READ) OpenBus = (uint8)(OpAddress>>8);
    OpAddress += ICPU.ShiftedDB;

#ifndef SA1_OPCODES
//    if (Registers.DL != 0)
//	CPU.Cycles += TWO_CYCLES;
//    else
	CPU.Cycles += ONE_CYCLE;
#endif
}

STATIC inline long __attribute__((always_inline)) DirectIndexedIndirectFast (AccessMode a)
{
    OpenBus = *CPU.PC;
    long addr = (*CPU.PC++ + Registers.D.W + Registers.X.W) & 0xffff;
#ifndef SA1_OPCODES
    CPU_Cycles += CPU.MemSpeed;
#endif

    addr = S9xGetWord (addr);
    if(a&READ) OpenBus = (uint8)(addr>>8);
    addr += ICPU.ShiftedDB;

#ifndef SA1_OPCODES
//    if (Registers.DL != 0)
//	CPU_Cycles += TWO_CYCLES;
//    else
	CPU_Cycles += ONE_CYCLE;
#endif
    return addr;
}

STATIC inline void __attribute__((always_inline)) DirectIndexedX (AccessMode a)
{
	if(a&READ) OpenBus = *CPU.PC;
    OpAddress = (*CPU.PC++ + Registers.D.W + Registers.X.W);
    OpAddress &= CheckEmulation() ? 0xff : 0xffff;

#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeed;
#endif

#ifndef SA1_OPCODES
//    if (Registers.DL != 0)
//	CPU.Cycles += TWO_CYCLES;
//    else
	CPU.Cycles += ONE_CYCLE;
#endif
}

STATIC inline long __attribute__((always_inline)) DirectIndexedXFast (AccessMode a)
{
	if(a&READ) OpenBus = *CPU.PC;
    long addr = (*CPU.PC++ + Registers.D.W + Registers.X.W);
    addr &= CheckEmulation() ? 0xff : 0xffff;

#ifndef SA1_OPCODES
    CPU_Cycles += CPU.MemSpeed;
#endif

#ifndef SA1_OPCODES
//    if (Registers.DL != 0)
//	CPU_Cycles += TWO_CYCLES;
//    else
	CPU_Cycles += ONE_CYCLE;
#endif
    return addr;
}

STATIC inline void __attribute__((always_inline)) DirectIndexedY (AccessMode a)
{
	if(a&READ) OpenBus = *CPU.PC;
    OpAddress = (*CPU.PC++ + Registers.D.W + Registers.Y.W);
    OpAddress &= CheckEmulation() ? 0xff : 0xffff;
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeed;
#endif

#ifndef SA1_OPCODES
//    if (Registers.DL != 0)
//	CPU.Cycles += TWO_CYCLES;
//    else
	CPU.Cycles += ONE_CYCLE;
#endif
}

STATIC inline long __attribute__((always_inline)) DirectIndexedYFast (AccessMode a)
{
	if(a&READ) OpenBus = *CPU.PC;
    long addr = (*CPU.PC++ + Registers.D.W + Registers.Y.W);
    addr &= CheckEmulation() ? 0xff : 0xffff;
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeed;
#endif

#ifndef SA1_OPCODES
//    if (Registers.DL != 0)
//	CPU.Cycles += TWO_CYCLES;
//    else
	CPU.Cycles += ONE_CYCLE;
#endif
    return addr;
}

STATIC inline void __attribute__((always_inline)) AbsoluteIndexedX (AccessMode a)
{
#ifdef FAST_LSB_WORD_ACCESS
    OpAddress = ICPU.ShiftedDB + *(uint16 *) CPU.PC + Registers.X.W;
#else
    OpAddress = ICPU.ShiftedDB + *CPU.PC + (*(CPU.PC + 1) << 8) +
		Registers.X.W;
#endif
    if(a&READ) OpenBus = *(CPU.PC+1);
    CPU.PC += 2;
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeedx2;
#endif
    // XXX: always add one cycle for ROL, LSR, etc
    // XXX: else is cross page boundary add one cycle
}

STATIC inline long __attribute__((always_inline)) AbsoluteIndexedXFast (AccessMode a)
{
#ifdef FAST_LSB_WORD_ACCESS
    long addr = ICPU.ShiftedDB + *(uint16 *) CPU.PC + Registers.X.W;
#else
    long addr = ICPU.ShiftedDB + *CPU.PC + (*(CPU.PC + 1) << 8) +
		Registers.X.W;
#endif
    if(a&READ) OpenBus = *(CPU.PC+1);
    CPU.PC += 2;
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeedx2;
#endif
    // XXX: always add one cycle for ROL, LSR, etc
    // XXX: else is cross page boundary add one cycle
    return addr;
}

STATIC inline void __attribute__((always_inline)) AbsoluteIndexedY (AccessMode a)
{
#ifdef FAST_LSB_WORD_ACCESS
    OpAddress = ICPU.ShiftedDB + *(uint16 *) CPU.PC + Registers.Y.W;
#else
    OpAddress = ICPU.ShiftedDB + *CPU.PC + (*(CPU.PC + 1) << 8) +
		Registers.Y.W;
#endif    
    if(a&READ) OpenBus = *(CPU.PC+1);
    CPU.PC += 2;
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeedx2;
#endif
    // XXX: always add cycle for STA
    // XXX: else is cross page boundary add one cycle
}

STATIC inline long __attribute__((always_inline)) AbsoluteIndexedYFast (AccessMode a)
{
#ifdef FAST_LSB_WORD_ACCESS
    long addr = ICPU.ShiftedDB + *(uint16 *) CPU.PC + Registers.Y.W;
#else
    long addr = ICPU.ShiftedDB + *CPU.PC + (*(CPU.PC + 1) << 8) +
		Registers.Y.W;
#endif    
    if(a&READ) OpenBus = *(CPU.PC+1);
    CPU.PC += 2;
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeedx2;
#endif
    // XXX: always add cycle for STA
    // XXX: else is cross page boundary add one cycle
    return addr;
}

STATIC inline void __attribute__((always_inline)) AbsoluteLongIndexedX (AccessMode a)
{
#ifdef FAST_LSB_WORD_ACCESS
    OpAddress = (*(uint32 *) CPU.PC + Registers.X.W) & 0xffffff;
#else
    OpAddress = (*CPU.PC + (*(CPU.PC + 1) << 8) + (*(CPU.PC + 2) << 16) + Registers.X.W) & 0xffffff;
#endif
    if(a&READ) OpenBus = *(CPU.PC+2);
    CPU.PC += 3;
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeedx2 + CPU.MemSpeed;
#endif
}

STATIC inline void __attribute__((always_inline)) DirectIndirect (AccessMode a)
{
    OpenBus = *CPU.PC;
    OpAddress = (*CPU.PC++ + Registers.D.W) & 0xffff;
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeed;
#endif
    OpAddress = S9xGetWord (OpAddress);
    if(a&READ) OpenBus = (uint8)(OpAddress>>8);
    OpAddress += ICPU.ShiftedDB;

//    if (Registers.DL != 0) CPU.Cycles += ONE_CYCLE;
}

STATIC inline long __attribute__((always_inline)) DirectIndirectFast (AccessMode a)
{
    OpenBus = *CPU.PC;
    long addr = (*CPU.PC++ + Registers.D.W) & 0xffff;
#ifndef SA1_OPCODES
    CPU_Cycles += CPU.MemSpeed;
#endif
    addr = S9xGetWord (addr);
    if(a&READ) OpenBus = (uint8)(addr>>8);
    addr += ICPU.ShiftedDB;

//    if (Registers.DL != 0) CPU_Cycles += ONE_CYCLE;
    return addr;
}

STATIC inline void __attribute__((always_inline)) DirectIndirectLong (AccessMode a)
{
    OpenBus = *CPU.PC;
    OpAddress = (*CPU.PC++ + Registers.D.W) & 0xffff;
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeed;
#endif
    if(a&READ){
	OpAddress = S9xGetWord (OpAddress) + ((OpenBus=S9xGetByte (OpAddress + 2)) << 16);
    } else {
	OpAddress = S9xGetWord (OpAddress) + (S9xGetByte (OpAddress + 2) << 16);
    }
//    if (Registers.DL != 0) CPU.Cycles += ONE_CYCLE;
}

STATIC inline void __attribute__((always_inline)) StackRelative (AccessMode a)
{
    if(a&READ) OpenBus = *CPU.PC;
    OpAddress = (*CPU.PC++ + Registers.S.W) & 0xffff;
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeed;
    CPU.Cycles += ONE_CYCLE;
#endif
}

STATIC inline void __attribute__((always_inline)) StackRelativeIndirectIndexed (AccessMode a)
{
    OpenBus = *CPU.PC;
    OpAddress = (*CPU.PC++ + Registers.S.W) & 0xffff;
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeed;
    CPU.Cycles += TWO_CYCLES;
#endif
    OpAddress = S9xGetWord (OpAddress);
    if(a&READ) OpenBus = (uint8)(OpAddress>>8);
    OpAddress = (OpAddress + ICPU.ShiftedDB +
		 Registers.Y.W) & 0xffffff;
}
#endif

