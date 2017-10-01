#ifndef __start_h__
	#include "start.h"
#endif
#define MSR_K8_VM_HSAVE_PA 0xc0010117
#define MSR_PAT 0x00000277
#define MSR_MISC_ENABLE 0x000001A0
void Print( const char *string );
BYTE GetVMCBB(DWORD Offset);
WORD GetVMCBW(DWORD Offset);
DWORD GetVMCBD(DWORD Offset);
BYTE GetMemB(DWORD Location, bool GuestMemory);
WORD GetMemW(DWORD Location, bool GuestMemory);
DWORD GetMemD(DWORD Location, bool GuestMemory);
void SetVMCBB(DWORD Offset, BYTE Value);
void SetVMCBW(DWORD Offset, WORD Value);
void SetVMCBD(DWORD Offset, DWORD Value);
void SetMemB(DWORD Location, BYTE Value, bool GuestMemory);
void SetMemW(DWORD Location, WORD Value, bool GuestMemory);
void SetMemD(DWORD Location, DWORD Value, bool GuestMemory);
void IncrementGuestIP(BYTE Count);
void IncrementGuestnRIP(BYTE Count);
DWORD TranslateGuestPhysicalAddress(DWORD GAddress);
WORD VerifySVM();
void CopyData(DWORD from, DWORD to, DWORD cnt);
void outb(u16int port, u8int value);
void outw(u16int port, u16int value);
void outd(u16int port, u32int value);
u8int inb(u16int port);
u16int inw(u16int port);
char* Itoa(int value, char* str, int radix);
u32int ind(u16int port);
BYTE bitarray_GetBit(BYTE *StartAddr, int BitNum, WORD DataSegment);
BYTE bitarray_UpdateBit(BYTE *StartAddr, int BitNum, BYTE Value, WORD DataSegment);
void Sleep(BYTE Seconds);
void setup_pit_channel_0( unsigned short frequency );
//From XVisor (0.2.4)/arch/cpu_features.h
inline void cpu_write_msr(DWORD msr, QWORD value);
//From XVisor (0.2.4)/arch/cpu_features.h
inline QWORD cpu_read_msr(DWORD msr);

__inline__ QWORD rdmsr(DWORD msr_id);

struct sIntDetails 
{
	WORD IntNum;
	WORD AXVal;
	WORD BXVal;
	WORD ESVal;
	WORD CSVal;
	WORD IPVal;
	WORD AXRetVal;
	WORD FLAGSVal;
}  __attribute__ ((packed));

#define cpuid(func,ax,bx,cx,dx)\
	__asm__ __volatile__ ("cpuid":\
	"=a" (ax), "=b" (bx), "=c" (cx), "=d" (dx) : "a" (func));

#define SVM_NOT_AVAIL 1
#define SVM_DISABLED_AT_BIOS_NOT_UNLOCKABLE 2
#define SVM_DISABLED_WITH_KEY 3
#define SVM_ALLOWED 0x0

void setHostSegReg( enum eRegisters sr, WORD Selector );

