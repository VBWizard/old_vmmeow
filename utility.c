#include "utility.h"
#if ndef __start_h__
	#include "start.h"
#endif
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
BYTE bitarray_GetBit(BYTE *StartAddr, int BitNum, WORD DataSegment);
BYTE bitarray_UpdateBit(BYTE *StartAddr, int BitNum, BYTE Value, WORD DataSegment);
void outb(u16int port, u8int value);
u8int inb(u16int port);
u16int inw(u16int port);
u32int ind(u16int port);
void Sleep(BYTE Seconds);
void setup_pit_channel_0( unsigned short frequency );
void IncrementGuestnRIP(BYTE Count);

__inline__ QWORD rdmsr(DWORD msr_id);

//Variables
static DWORD GuestMemOffset = 0;
DWORD ScreenY = 0;


WORD VerifySVM()
{
	DWORD eax, ebx, ecx, edx;
	
	return 0;
	
	cpuid(0x80000001, eax, ebx, ecx, edx);
	
	if ( (ecx & 0x04) == 0)
		return SVM_NOT_AVAIL;
	else if ( (rdmsr(0xC0010114) & 0x16) == 0)
		return SVM_ALLOWED;

	cpuid(0x8000000A, eax, ebx, ecx, edx);
	
	if ((edx & 0x4 )==0)
	return SVM_DISABLED_AT_BIOS_NOT_UNLOCKABLE;
	// the user must change a BIOS setting to enable SVM
	else return SVM_DISABLED_WITH_KEY;
	// SVMLock may be unlockable; consult the BIOS or TPM to obtain the key.
}
__inline__ QWORD rdmsr(DWORD msr_id)
{
		QWORD msr_value;
		__asm__ __volatile__ ("   rdmsr"
								: "=A" (msr_value)
								: "c" (msr_id) );
		return msr_value;
}


void Print( const char *string )
{
	ScreenY = 1;
	__asm__("push eax\n mov eax, 0x38\n push eax\n pop ds\n pop eax\n");
	int colour = 0x7;

    volatile char *video = (volatile char*)(0xB8000 + ScreenY*160);
    while( *string != 0 )
    {
		const char *c = string++;
	__asm__("push eax\nmov eax, 0x10\n push eax\n pop ds\n pop eax\n");
        *video++ = (int)c;
        *video++ = colour;
	__asm__("push eax\n mov eax, 0x38\n push eax\n pop ds\n pop eax\n");
    }
    //ScreenY++;
}

BYTE GetVMCBB(DWORD Offset)
{
	return GetMemB(VMCB_FINAL_ADDRESS + Offset,false);
}

WORD GetVMCBW(DWORD Offset)
{
	return GetMemW(VMCB_FINAL_ADDRESS + Offset, false);
}

DWORD GetVMCBD(DWORD Offset)
{
	return GetMemD(VMCB_FINAL_ADDRESS + Offset, false);
}

BYTE GetMemB(DWORD Location, bool GuestMemory)
{
	BYTE lRetVal = 0;
	if (GuestMemory==true)
		GuestMemOffset = TranslateGuestPhysicalAddress(Location);
	else
		GuestMemOffset = Location; 

	__asm__(""	//Don't push EAX as our return value will be in it
			"	mov  eax,%[location]\n"		//Move Location to EAX
			"	mov  %[result], byte ptr FS:[eax]\n"	//Move byte Value to EAX
			: [result] "=r" (lRetVal)
			: [location] "r" (GuestMemOffset)
			:"eax"	);	
	return lRetVal;
}

WORD GetMemW(DWORD Location, bool GuestMemory) 
{
	WORD lRetVal = 0;
	if (GuestMemory==true)
		GuestMemOffset = TranslateGuestPhysicalAddress(Location);
	else
		GuestMemOffset = Location; 

	__asm__(""	//Don't push EAX as our return value will be in it
			"	mov  eax,%[location]\n"		//Move Location to EAX
			"	mov  %[result], word ptr FS:[eax]\n"	//Move byte Value to EAX
			: [result] "=r" (lRetVal)
			: [location] "r" (GuestMemOffset)	
			:"eax"	);	
	return lRetVal;
}

DWORD GetMemD(DWORD Location, bool GuestMemory)
{
	DWORD lRetVal = 0;
	if (GuestMemory==true)
		GuestMemOffset = TranslateGuestPhysicalAddress(Location);
	else
		GuestMemOffset = Location; 

	__asm__(""	//Don't push EAX as our return value will be in it
			"	mov  eax,%[location]\n"		//Move Location to EAX
			"	mov  %[result], dword ptr FS:[eax]\n"	//Move byte Value to EAX
			: [result] "=r" (lRetVal)
			: [location] "r" (GuestMemOffset)	
			:"eax"	);	
	return lRetVal;
}

void SetVMCBB(DWORD Offset, BYTE Value)
{
	SetMemB(VMCB_FINAL_ADDRESS + Offset, Value, false);
}

void SetVMCBW(DWORD Offset, WORD Value)
{
	SetMemW(VMCB_FINAL_ADDRESS + Offset, Value, false);
}

void SetVMCBD(DWORD Offset, DWORD Value)
{
	SetMemD(VMCB_FINAL_ADDRESS + Offset, Value, false);
}

void SetMemB(DWORD Location, BYTE Value, bool GuestMemory)
{
	if (GuestMemory==true)
		GuestMemOffset = TranslateGuestPhysicalAddress(Location);
	else
		GuestMemOffset = Location; 
	
	__asm__("	mov  eax,%[location]\n"		//Move Location to EAX
			"	mov  bl,%[value]\n"		//Move byte Value to EL
			"	mov	FS:[eax],bl\n"
			: : [location] "r" (GuestMemOffset), 
			    [value] "r" (Value) 
			:"eax"	);	
}

void SetMemW(DWORD Location, WORD Value, bool GuestMemory)
{
	if (GuestMemory==true)
		GuestMemOffset = TranslateGuestPhysicalAddress(Location);
	else
		GuestMemOffset = Location; 

	__asm__("	mov  eax,%[location]\n"		//Move Location to EAX
			"	mov  bx,%[value]\n"		//Move byte Value to EL
			"	mov	FS:[eax],bx\n"
			: : [location] "r" (GuestMemOffset), 
			    [value] "r" (Value) 
			:"eax"	);	
}

void SetMemD(DWORD Location, DWORD Value, bool GuestMemory)
{
	if (GuestMemory==true)
		GuestMemOffset = TranslateGuestPhysicalAddress(Location);
	else
		GuestMemOffset = Location; 

	__asm__("	mov  eax,%[location]\n"		//Move Location to EAX
			"	mov  ebx,%[value]\n"		//Move byte Value to EL
			"	mov	FS:[eax],ebx\n"
			: : [location] "r" (GuestMemOffset), 
			    [value] "r" (Value) 
			:"eax"	);	
}

void IncrementGuestIP(BYTE Count)
{
		SetVMCBW(VMCB_SAVE_STATE_RIP,GetVMCBW(VMCB_SAVE_STATE_RIP)+Count);
}
void IncrementGuestnRIP(BYTE Count)
{
		SetVMCBW(VMCB_CONTROL_NRIP,GetVMCBW(VMCB_CONTROL_NRIP)+Count);
}

DWORD GetGuestPhysicalAddress(DWORD GAddress)
{
	//Get the offset from the beginning of the page table where our entry resides.  i.e. 0x00007245 = 0x7000 / 0x400 = 0x1C, so the 0x1Cth byte
	DWORD PTEntryNum = ((GAddress & 0xFFFFF000) / 0x400);
	
	//Get the page table entry (i.e. 0x7245 = 0x00107007)
	DWORD PTEntry = GetMemD( (OFFSET_TO_GUEST_PHYSICAL_PAGES + GetVMCBD(VMCB_SAVE_STATE_CR3) + 0x1000 + PTEntryNum),false); //originally used GUEST_CR3_VALUE but the guest changes that when they enter pmode
	//Strip off the properties (i.e. 0x00107007 = 0x00107000)
	PTEntry &= 0xFFFFF000;
	//Add the pageoffset as we return the result i.e. 0x00107000 = 0x00107245
	return PTEntry | (GAddress & 0x00000FFF);
}
DWORD TranslateGuestPhysicalAddress(DWORD GAddress)
{
	DWORD GuestPhys = GetGuestPhysicalAddress(GAddress);

#ifdef USE_NESTED_PAGING	
	DWORD PTEntryNum = ((GuestPhys & 0xFFFFF000) / 0x400);
	//Get the page table entry (i.e. 0x7245 = 0x00107007)
	DWORD PTEntry = GetMemD( (NESTED_PAGING_DIR_FINAL_ADDRESS + 0x1000 + PTEntryNum),false);
	//Strip off the properties (i.e. 0x00107007 = 0x00107000)
	PTEntry &= 0xFFFFF000;
	//Add the pageoffset as we return the result i.e. 0x00107000 = 0x00107245
	return PTEntry | (GAddress & 0x00000FFF);
#else
		return GuestPhys;
#endif	
	//return ( (GetMemW( (OFFSET_TO_GUEST_PHYSICAL_PAGES + GUEST_CR3_VALUE + 0x1000) + ((GAddress & 0xFFFFF000) / 0x400),false) & 0xFFFFF000)) | (GAddress & 0x00000FFF)  ;
}

void CopyData(DWORD from, DWORD to, DWORD cnt)
{
	__asm__(
				"push eax\n mov eax, 0x10\n push eax\n pop ds\n pop eax\n"
				"mov esi, %[from]\n"
				"mov edi, %[to]\n"
				"mov ecx, %[cnt]\n"
				"cld\n"
				"rep movsb\n"
				"push eax\n mov eax, 0x38\n push eax\n pop ds\n pop eax\n"
				: //no return params
				: [from] "r" (from),
				  [to] "r" (to),
				  [cnt] "r" (cnt)
				);
}

// common.c -- Defines some global functions.
// From JamesM's kernel development tutorials.
// Write a byte out to the specified port.
void outb(u16int port, u8int value)
{
    asm volatile ("outb %0, %1" : : "dN" (port), "a" (value));
}

void outw(u16int port, u16int value)
{
    asm volatile ("outw %0, %1" : : "dN" (port), "a" (value));
}

void outd(u16int port, u32int value)
{
    asm volatile ("outd %0, %1" : : "dN" (port), "a" (value));
}

u8int inb(u16int port)
{
   u8int ret;
   asm volatile("inb %0, %1" : "=a" (ret) : "dN" (port));
   return ret;
}

u16int inw(u16int port)
{
   u16int ret;
   asm volatile ("inw %0, %1" : "=a" (ret) : "dN" (port));
   return ret;
} 

u32int ind(u16int port)
{
   u32int ret;
   asm volatile ("ind %0, %1" : "=a" (ret) : "dN" (port));
   return ret;
} 

    /* The Itoa code is in the public domain */
    char* Itoa(int value, char* str, int radix) {
    static char dig[] =
    "0123456789"
    "abcdefghijklmnopqrstuvwxyz";
    int n = 0, neg = 0;
    unsigned int v;
    char* p, *q;
    char c;
    if (radix == 10 && value < 0) {
    value = -value;
    neg = 1;
    }
    v = value;
    do {
    str[n++] = dig[v%radix];
    v /= radix;
    } while (v);
    if (neg)
    str[n++] = '-';
    str[n] = '\0';
    for (p = str, q = p + (n-1); p < q; ++p, --q)
    c = *p, *p = *q, *q = c;
    return str;
    }

BYTE bitarray_GetBit(BYTE *StartAddr, int BitNum, WORD DataSegment)
{
	//NOTE: BitNum is 1 based
	asm ( "push ds\n mov ds, %0   \n": : "a" (DataSegment) );
	BYTE lTheByte = StartAddr[BitNum/8];
	asm( "pop ds\n");
	return ( lTheByte >> (BitNum-1)) & 1;
}

BYTE bitarray_UpdateBit(BYTE *StartAddr, int BitNum, BYTE Value, WORD DataSegment)
{
	//NOTE: BitNum is 1 based
	asm ( "push ds\n mov ds, %0   \n": : "a" (DataSegment) );
	BYTE lTheByte = StartAddr[BitNum/8];
	lTheByte |= ((Value & 0x1) << (BitNum%8));
	StartAddr[BitNum/8] = lTheByte;
//	lTheByte = lTheByte >> (((BitNum%8)-1));
	asm( "pop ds\n");
	return lTheByte;
}

void lbaToCHS ( int LBA, int SPT, int HEADS, int *c, int *h, int *s)
{
	DWORD lTemp = LBA / SPT;
	*s = (LBA % SPT) +1;
	*h = lTemp % HEADS;
	*c = lTemp / HEADS;
}

DWORD chsToLBA(int cylinder, int head, int sector, int heads_per_cylinder, int SPT)
{
	return ( (cylinder * heads_per_cylinder + head ) * SPT ) + sector - 1;
}

void Sleep(BYTE Seconds)
{
BYTE Sec = 0;

//	"; input: cx is value to delay for.\n"
//	";        (approximate to seconds).\n"
asm(
	"xchg bx,bx\n"
	"timer:\n"
	"sub bx,bx\n"
	"mov al,0x00\n" //seconds address
	"out 0x70,al\n" // wake up port
	"jmp $+2\n"     //a short delay"
	"in al,0x71\n"
	"mov %[sec],al\n"
	"timer_loop:\n"
	"mov al,0x00\n"	//seconds address
	"out 0x70,al\n" //wake up port
	"jmp $+2    \n" //a short delay
	"in al,0x71\n"
	"cmp al,%[sec]\n"
	"je timer_loop\n"
	"mov %[sec],al\n"
	"inc bx\n"
	"cmp bl,%[Second]\n"
	"jne timer_loop\n"
	: : [sec] "r" (Sec), [Second] "r" (Seconds)
);
}

void setup_pit_channel_0( WORD frequency )
{
   // Variables
   unsigned short counter;

   // Calculate Counter
   counter = 0x1234DD / frequency;

   // Setup Mode
   outb( 0x43, 0x34 );

   // Send Low Count
   outb( 0x40, (counter % 256) );

   // Send High Count
   outb( 0x40, (counter / 256) );
}

inline void cpu_write_msr(DWORD msr, QWORD value)
{
        DWORD a, d;

        /* RDX contains the high order 32-bits */
        d = value >> 32;

        /* RAX contains low order */
        a = value & 0xFFFFFFFFUL;

        asm volatile ("wrmsr\n\t"
                      ::"a"(a),"d"(d),"c"(msr));
}

inline QWORD cpu_read_msr(DWORD msr)
{
        DWORD a, d;

        asm volatile ("rdmsr\n\t"
                      :"=a"(a),"=d"(d)
                      :"c"(msr)
                      :"rbx");

        return (QWORD)(((QWORD)d << 32)
                     | (a & 0xFFFFFFFFUL));
}

void setHostSegReg( enum eRegisters sr, WORD Selector )
{
    switch (sr)
    {
        case CS:
            break;
        case DS:
            asm("mov ax,%[sel]\n push ax\n pop DS" ::[sel] "r" (Selector) );
            break;
        case ES:
            asm("mov ax,%[sel]\n push ax\n pop ES" ::[sel] "r" (Selector) );
            break;
        case FS:
            asm("mov ax,%[sel]\n push ax\n pop FS" ::[sel] "r" (Selector) );
            break;
        case GS:
            asm("mov ax,%[sel]\n push ax\n pop GS" ::[sel] "r" (Selector) );
            break;
        case SS:
            asm("mov ax,%[sel]\n push ax\n pop SS" ::[sel] "r" (Selector) );
            break;
        default:
            break;
    }
}

