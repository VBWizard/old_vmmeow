#ifndef __visor_h__
#define __visor_h__
#include "start.h"
DWORD Our_Cylinders = 0x103;  //259 (260 zero based)
DWORD Our_SPT = 0x3F; //63
DWORD Our_Heads = 0xF; //15 (16 zero based)
DWORD HostRegisters[14], GuestRegisters[14];
//Host Register Array Indexes - Used in Aseembly Code
DWORD* HostRegisterEBXi, *HostRegisterECXi, *HostRegisterEDXi, *HostRegisterESIi, *HostRegisterEDIi, *HostRegisterEBPi, *HostRegisterFSi, *HostRegisterGSi;
//Guest Register Array Indexes - Used in Aseembly Code
DWORD* GuestRegisterEBXi, *GuestRegisterECXi, *GuestRegisterEDXi, *GuestRegisterESIi, *GuestRegisterEDIi, *GuestRegisterEBPi, *GuestRegisterFSi, *GuestRegisterGSi;
DWORD Boot_Cylinders, Boot_SPT, Boot_Heads;

#endif //#ifndef __visor_h__

