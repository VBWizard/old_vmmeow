#include "visor.h"
#include "start.h"
#include "utility.h"
#include "PM_DEFS_gcc.H"
#include "pmode.h"
#include "paging.h"
#include "guest.h"
#include "isr_wrapper.h"
#include "screen.h"

__asm__("jmp main");

DWORD *VMCB = 0x0;
BYTE *VMCBB = 0x0;
DWORD GuestIOPermissionMap = 0x0; //0x3000 = 3 4k pages of IO bitmap
DWORD lTempVal = 0;
DWORD lTempAddr = 0;
BYTE lTempByte;
DWORD lTempDWord, lTempDWord2;
DWORD gDebugString = 0, gDebugString2 = 0, gDebugString3 = 0, gDebugString4 = 0;
DWORD VMCBFinalAddress = VMCB_FINAL_ADDRESS, cnt = 0, Value = 0, GuestSavedCR0 = 0;
IDTR *idtPtr = 0;
extern volatile struct sIntDetails* IntDetails;
TSS *tss;
BYTE *task_stack = (BYTE *)(TASK_STACK_FINAL);
BYTE *pl0_stack = (BYTE *)(TASK_STACK_FINAL + 0x5000);
WORD _exc_CS = 0, _exc_IP = 0;
extern DWORD GuestRequestedCR3, GuestRequestedCR0;
DWORD gInterceptCount = 0;
BYTE IntNo = 0;
volatile long long ticks = 0;
volatile long long jifs = 0;
BYTE scancode;
int gInterceptCode = 0;

//void exc_handler (WORD exc_no, WORD cs, DWORD ip, WORD error);
char HostExceptMsg[] = "In host exception handler for err:                \0";  //blank is at 35
void timer_handler();
void kbd_handler();
void call_gate_proc();
void VisorLoop();
void CopySegment0();
void MoveStuff();
volatile int Wait(int interval);
void InfiniteLoop();

void main()
{
	WORD svmResult;

    setHostSegReg(DS, SELECTOR_VISOR_DATA);
__asm__(    "	mov eax,0\n"
	    "   mov cr4,eax\n"
		"	mov ax,0x0038\n"
		"	push ax\n"
		"	pop ds\n"
		"	mov ax,0x10\n"
		"	push ax\n"
		"	pop fs\n"
		"	mov ax,0x38\n"
		"	push ax\n"
		"	pop gs\n"
		);
	lTempVal = (MODULE_START_BEGIN_ADDRESS << 4) | C_SPT_ADDR;
	//Retrieve the CHS values looked up in real mode by the Start program
	asm("push es\n"
	    "mov bx,0x10\n"
	    "mov es, bx\n"
	    "push %%eax\n"
	    "mov eax, es:[eax]\n"
	    "movw ds:Boot_SPT, ax\n"
	    "pop eax\npush eax\n"
	    "add eax,4\n"
	    "mov eax, es:[eax]\n"
	    "movw ds:Boot_Heads, ax\n"
	    "pop eax\n"
	    "add eax,8\n"
	    "mov eax, es:[eax]\n"
	    "movw ds:Boot_Cylinders, ax\n"
	    "pop es\n"
		:
		: "a"(lTempVal));

	//Move the stack (GDT entry 0x18)
	DESCR_SEG *item = (DESCR_SEG*)HOST_GDT;
	setup_GDT32_entry_gcc(item+3, (DWORD)HOST_STACK_SEG_BASE, 0xFFFFFF, ACS_STACK, 0X92);
	__asm__("mov eax,0x18\n push eax\n pop ss\n mov esp,0xFF00");
	CopyData(HOST_GDT, HOST_GDT_FINAL, 0x400);

	Print("Visor loaded!!!");

	svmResult = VerifySVM();

	switch (svmResult)
	{
		case SVM_ALLOWED:
		Print("SVM available\0");
		case SVM_NOT_AVAIL:
		Print("SVM is not available on this machine.\0");
		break;
		case SVM_DISABLED_AT_BIOS_NOT_UNLOCKABLE:
		Print("SVM is disabled in the BIOS and is not unlockable\0");
		break;
		case SVM_DISABLED_WITH_KEY:
		Print("SVM is disabled with key\0");
		break;
		default:
		Print("Unknown error detecting SVM availability\0");
		break;
	}
	
	if (svmResult > 0)
	__asm__("svnLoop1:\n jmp svnLoop1\n");
	
	GuestSavedCR0 = GetVMCBD(VMCB_SAVE_STATE_CR0);

	CopySegment0();
	SetupGuestPages();
	GuestIOPermissionMap = VMCB_START_ADDRESS + VMCB_IOIO_DEF;
	__asm__("push fs\n mov eax,0x10\n push eax\n pop fs");
	for (cnt=0;cnt<0x12*1024;cnt++)
	{
		Value = 0x00;
		__asm__("mov EAX,GuestIOPermissionMap\n"
				"add eax,cnt\n"
				"mov bl,Value\n"
				"movb FS:[eax],bl\n");
		
	}
	VMCB = (DWORD *)VMCB_START_ADDRESS;
	VMCBB = (BYTE *)VMCB_START_ADDRESS;
	VMCB[VMCB_CONTROL_IOPM_BASE_PA]=GuestIOPermissionMap & 0xFF;
	VMCB[VMCB_CONTROL_IOPM_BASE_PA+1]=(GuestIOPermissionMap >> 8) & 0xFF;
	VMCB[VMCB_CONTROL_IOPM_BASE_PA+2]=(GuestIOPermissionMap >> 16) & 0xFF;
	VMCB[VMCB_CONTROL_IOPM_BASE_PA+3]=(GuestIOPermissionMap >> 24) & 0xFF;
	__asm__("pop fs");

//	bitarray_UpdateBit( VMCBB + VMCB_IOIO_DEF, 0x70, 1, 0x10);
//	bitarray_UpdateBit( VMCBB + VMCB_IOIO_DEF, 0x71, 1, 0x10);

	MoveStuff();
	VisorLoop();
}


void MoveStuff()
{
int cnt=0;
	__asm__("mov eax,0x10\n push eax\n pop es\n");
	
OPEN_UP_DS
	//CreateAndLoadTSS();
	GDTR *gdtP = (GDTR*)HOST_GDT_R_LOAD;
	gdtP->base = HOST_GDT_FINAL;
	gdtP->limit = (11*8)-1;
	__asm__(	"mov ebx,%[gdtr]\n lgdt [ebx]\n"
	:
	: [gdtr] "r" (gdtP));
	
	SetupHostIDT();
	//Move IDT
	//CopyData (HOST_IDT, HOST_IDT_FINAL, 0x850);
	idtPtr = (IDTR *)(HOST_IDT_FINAL + 0x900);  //IDT entries take up 0x800
	idtPtr->base = HOST_IDT_FINAL;
	idtPtr->limit = (128*8)-1;
	asm(	"mov ebx,%[idtr]\n lidt [ebx]\n"
	:
	: [idtr] "r" (idtPtr));
	setup_pit_channel_0( 56 );
	asm("STI\n");

RESTORE_DS
	CopyData(VMCB_START_ADDRESS, VMCB_FINAL_ADDRESS, 0x20000);
	CopyData(NESTED_PAGING_DIRECTORY_ADDRESS, NESTED_PAGING_DIR_FINAL_ADDRESS, 0x1000);
	CopyData(NESTED_PAGING_TABLE_ADDRESS, NESTED_PAGING_TAB_FINAL_ADDRESS, (GUEST_MEMORY_SIZE / 0x1000)*4);
	CopyData (HOST_CR3_VALUE, HOST_CR3_FINAL_VALUE, 0x1000);
	CopyData (HOST_CR3_VALUE + 0x1000, HOST_CR3_FINAL_VALUE + 0x1000, (HOST_MEMORY_SIZE / 0x1000)*4);
OPEN_UP_DS
	for (cnt=0;cnt<=(GUEST_MEMORY_SIZE / 0x400000); cnt++) 
		UpdatePDEntry((DWORD*)(NESTED_PAGING_DIR_FINAL_ADDRESS), cnt, NESTED_PAGING_DIR_FINAL_ADDRESS + 4096 + (cnt*4096)) ;
	//Update host dir entries based on the move
	for (cnt=0;cnt<(HOST_MEMORY_SIZE / 0x400000); cnt++) 
		UpdatePDEntry((DWORD*)(HOST_CR3_FINAL_VALUE), cnt, HOST_CR3_FINAL_VALUE + 4096 + (cnt*4096)) ;
	__asm("mov eax,%[PD]\n mov cr3, eax\n"
	:
	:[PD] "r" (HOST_CR3_FINAL_VALUE));
	
	//Update the IOPM base in the VMCB (io intercept map base)
	SetVMCBD(VMCB_CONTROL_IOPM_BASE_PA,VMCB_FINAL_ADDRESS+0x1000);


RESTORE_DS
	
}

void VisorLoop()
{

		WORD ErrorCode = 0;
		DWORD ExitInfo1, ExitInfo2;
		DWORD loopCount = 0;
		char *pErr = "                    \0";
		DWORD EAX, EDX;

//cpu_write_msr(MSR_K8_VM_HSAVE_PA,HOST_HSAVE_AREA);

		GuestRegisters[4] = 0xFFac;
		GuestRegisters[3] = 0xe0000;
		monitor_clear();
#ifdef __BOOT_FROM_HD__
		GuestRegisters[2] = 0x80; //DX = 80 for hard drive;
#else
		GuestRegisters[2] = 0x00; //DX = 0 for floppy drive;
#endif
	asm("mov eax,0x10\n push ax\n pop FS\n");
	//asm("mov eax,0x10\n push ax\n pop GS\n"); - set to 38 earlier!!!
	asm(	"push FS\n"
			"pop eax\n"
			"mov HostRegisters+24, ax\n"
			"push GS\n"
			"pop eax\n"
			"mov HostRegisters+28, ax\n");

	monitor_write_at("Entering main visor loop             ",0,1);
		asm("mov eax,VMCBFinalAddress");
		asm("vmload");


	while (1)
	{
		//Save the host registers
		asm("mov HostRegisters+0, EBX\n"
				"mov HostRegisters+4, ECX\n"
				"mov HostRegisters+8, EDX\n"
				"mov HostRegisters+12, ESI\n"
				"mov HostRegisters+16, EDI\n"
				"mov HostRegisters+20, EBP\n");
#ifdef DEBUG
		monitor_write_at("                         ",0,0);
#endif
		if (loopCount++ == 0)
loopCount = loopCount + 1 - 1 + 2 - 2 + 3 - 3;
		else
		{
			//Only store host FS/GS after first run as the VMLOAD the first run changes them
		        asm("mov eax, fs\n mov HostRegisters+24,eax\n"
		                "mov eax, gs\n mov HostRegisters+28,eax\n");
//Wait(100);
		}
		asm("mov eax,VMCBFinalAddress");
		//Load the guest registers
		asm("mov EBX, GuestRegisters+0\n"
				"mov ECX, GuestRegisters+4\n"
				"mov EDX, GuestRegisters+8\n"
				"mov ESI, GuestRegisters+12\n"
				"mov EDI, GuestRegisters+16\n"
				"mov EBP, GuestRegisters+20\n"
//				"mov AX, GuestRegisters+24\n push eax\n pop fs\n"
//				"mov AX, GuestRegisters+28\n push eax\n pop gs\n"
);
//		asm("CLGI\n");
//		asm("sti\n");
		asm("vmload\n");
		asm("vmrun");
		asm("vmsave\n");
//		asm("cli\n");
//		asm("STGI\n"); 
		//Save the guest registers
		asm("mov GuestRegisters+0, EBX\n"
			"mov GuestRegisters+4, ECX\n"
			"mov GuestRegisters+8, EDX\n"
			"mov GuestRegisters+12, ESI\n"
			"mov GuestRegisters+16, EDI\n"
			"mov GuestRegisters+20, EBP\n"
//			"mov GuestRegisters+24, fs\n"
//			"mov GuestRegisters+28, gs\n"
);
		//Load the host registers
		asm("mov EBX, HostRegisters+0\n"
			"mov ECX, HostRegisters+4\n"
			"mov EDX, HostRegisters+8\n"
			"mov ESI, HostRegisters+12\n"
			"mov EDI, HostRegisters+16\n"
			"mov EBP, HostRegisters+20\n"
			"mov eax, HostRegisters+24\n push 0x10\n pop fs\n"
			"mov eax, HostRegisters+28\n push 0x38\n pop gs\n");

//asm("STI\n");
//asm("xchg ah,al");
//asm("xchg al,ah");
//asm("CLI\n");

		if ( (GetVMCBD(VMCB_SAVE_STATE_CR0) & 0x1) == 0x1)
			monitor_write_at("P",1,0);
		else
			monitor_write_at("R",20,0);
		//_ltr(HOST_TSS_SEGMENT);
		if ( (GetVMCBD(VMCB_SAVE_STATE_CR0) & 0x80000000) == 0x80000000)
			monitor_write("G - 0x");
		else
 			monitor_write("n - 0x");
		pErr = Itoa((int)++gInterceptCount, pErr, 16);
		monitor_write(pErr);
		ErrorCode = GetVMCBW(VMCB_CONTROL_EXITCODE);
		 pErr = Itoa((int)ErrorCode, pErr, 16);
		monitor_write(", Exit: ");
		monitor_write(pErr);
		ExitInfo1 = GetVMCBD(VMCB_CONTROL_EXITINFO1);
		ExitInfo2 = GetVMCBD(VMCB_CONTROL_EXITINFO2);
		pErr = Itoa((int)ExitInfo1, pErr, 16);
		monitor_write(", Info1: ");
		monitor_write(pErr);
		pErr = Itoa((int)ExitInfo2, pErr, 16);
		monitor_write(", Info2: ");
		monitor_write(pErr);
		DWORD GuestCS = GetVMCBD(VMCB_SAVE_STATE_CS + 8);
		DWORD GuestEIP = GetVMCBD(VMCB_SAVE_STATE_RIP);
#ifdef DEBUG
		pErr = Itoa((int)GuestCS, pErr, 16);
		monitor_write("                                           ");
		monitor_write("gCS=0x");
		monitor_write(pErr);
		pErr = Itoa((int)GuestEIP, pErr, 16);
		monitor_write(", gEIP=0x");
		monitor_write(pErr);
#endif
		SetVMCBD(VMCB_CONTROL_EVENT_VALID, 0); //Make sure event injection is turned off
		SetVMCBW(VMCB_CONTROL_CLEAN_BITS, 0xFFFF);  //Everything is clean for now
		switch (ErrorCode)
		{
			case VMEXIT_NPF :	//Paging exception
			case VMEXIT_PF:
				HandleGuestPagingException(GetVMCBD(VMCB_CONTROL_EXITINFO2));
				break;
			case VMEXIT_SWINT: 
			case VMEXIT_INTR:
				IntNo = ExitInfo1;
				EAX = GetVMCBW(VMCB_SAVE_STATE_RAX);
				EDX = GuestRegisters[RAN_EDX];
#ifdef DEBUG
				pErr = Itoa((int)IntNo, pErr, 16);
				monitor_write(", IntNo: ");
				monitor_write(pErr);
				monitor_write(" ");
				pErr = Itoa(EAX, pErr, 16);
				monitor_write(pErr);
				monitor_write("   ");
#endif
				//Interrupts that we want to emulate
				     //Interrupt 15 AH=88: SYSTEM - GET EXTENDED MEMORY SIZE
				if ( ((IntNo == 0x15 && (EAX & 0xFF00) == 0x8800)  )/* | IntNo == 0x1C)*/
				     //Interrupt 13 AH=08: DISK - GET DRIVE PARAMETERS 
				     | (IntNo == 0x13 && (EAX & 0x0F00) == 0x0800 && (EDX & 0xFF) == 0x80) 
				     //Interrupt 13 Read/Write/Verify/anything that uses CHS values.
				     //| (IntNo == 0x13 && ( (EAX & 0x0002) == 0x0002 || (EAX & 0x0003) == 0x0003 || (EAX & 0x0004) == 0x0004) && (EDX & 0xFF) == 0x80) 
				     //| (IntNo == 0x12)
				   )
				{
					HandleRealModeSWInterrupt(IntNo); 
					SetVMCBW(VMCB_CONTROL_CLEAN_BITS, GetVMCBW(VMCB_CONTROL_CLEAN_BITS) | 0xFEFF); //CS changed
					SetVMCBW(VMCB_SAVE_STATE_RIP, GetVMCBW(VMCB_CONTROL_NRIP));
				}
				else
				{
					//Inject an event to make the INT happen
					SetVMCBW(VMCB_SAVE_STATE_RIP, GetVMCBW(VMCB_CONTROL_NRIP));
					SetVMCBD(VMCB_CONTROL_EVENTINJ, (DWORD)(1 << 31) | (DWORD)(0 << 11) | (DWORD)(4 << 8) | (BYTE)IntNo);
				}
				break;
			case VMEXIT_IRET:
				HandleInterrutpReturn();
				SetVMCBW(VMCB_CONTROL_CLEAN_BITS, GetVMCBW(VMCB_CONTROL_CLEAN_BITS) | 0x100); //CS changed
				break;
			case VMEXIT_CR0_READ:
				HandleCR0Read();
				break;
			case VMEXIT_CR3_READ:
				HandleCR3Read();
				break;
			case VMEXIT_CR0_WRITE:
			GuestCS = GetVMCBD(VMCB_SAVE_STATE_CS + 8);
			GuestEIP = GetVMCBD(VMCB_SAVE_STATE_RIP);
			EAX = GetVMCBW(VMCB_SAVE_STATE_RAX);
#ifdef DEBUG
			pErr = Itoa((int)GuestCS, pErr, 16);
			monitor_write_at("CR0 write attempt, guest CS:EIP: ",0,6);
			monitor_write_at("gCS=0x",0,3);
			monitor_write(pErr);
			pErr = Itoa((int)GuestEIP, pErr, 16);
			monitor_write(", gEIP=0x");
			monitor_write(pErr);
			monitor_write("  -  EAX = 0x");
			pErr = Itoa(EAX, pErr, 16);
#endif
//cmonitor_write(pErr);
//cInfiniteLoop(ErrorCode);
	//SetVMCBD(VMCB_SAVE_STATE_CR0,GetVMCBD(VMCB_SAVE_STATE_CR0) | 0x80000000);
	//if ( (	(GetVMCBD(VMCB_CONTROL_EXITINFO1_HIDWORD) & 0x80000000) == 0x80000000 ) )
//ExitInfo1 bit 63 will be set if this is a MOV/CRX instruction
		//IncrementGuestIP(2);
	//else
//If it is not set then this is a LMSW instruction
//TODO: This can also mean its a CLTS instruction which is 2 bytes, so handle this!
		//IncrementGuestIP(3);
SetVMCBW(VMCB_CONTROL_CLEAN_BITS, 0xFFDF);  //CRX unclean
SetVMCBD(VMCB_SAVE_STATE_RIP, GetVMCBD(VMCB_CONTROL_NRIP));
				HandleCR0Write();
				break;
			case VMEXIT_CR3_WRITE:
				HandleCR3Write();
				break;
			case VMEXIT_IOIO:
				//monitor_write_at("VISOR: In IOIO Handler",0,16);
				//return;
				//IOIOLoop1:
				//goto IOIOLoop1;
				HandleGuestIO();
				break;
			case VMEXIT_SHUTDOWN:
				monitor_write("VM Shutdown State Entered ... locking up now!!!");
				InfiniteLoop(ErrorCode);
				break;
			case VMEXIT_EXCEPTION_00: case VMEXIT_EXCEPTION_01: case VMEXIT_EXCEPTION_02: case VMEXIT_EXCEPTION_03: case VMEXIT_EXCEPTION_04: case VMEXIT_EXCEPTION_05: case VMEXIT_EXCEPTION_06: case VMEXIT_EXCEPTION_07: case VMEXIT_EXCEPTION_08:
			case VMEXIT_EXCEPTION_09: case VMEXIT_EXCEPTION_10: case VMEXIT_EXCEPTION_11: case VMEXIT_EXCEPTION_12: case VMEXIT_EXCEPTION_13: case VMEXIT_EXCEPTION_15: case VMEXIT_EXCEPTION_16: case VMEXIT_EXCEPTION_17: case VMEXIT_EXCEPTION_18:
			case VMEXIT_EXCEPTION_19: case VMEXIT_EXCEPTION_20: case VMEXIT_EXCEPTION_21: case VMEXIT_EXCEPTION_22: case VMEXIT_EXCEPTION_23: case VMEXIT_EXCEPTION_24: case VMEXIT_EXCEPTION_25: case VMEXIT_EXCEPTION_26: case VMEXIT_EXCEPTION_27:
			case VMEXIT_EXCEPTION_28: case VMEXIT_EXCEPTION_29: case VMEXIT_EXCEPTION_30: case VMEXIT_EXCEPTION_31:
				GuestCS = GetVMCBD(VMCB_SAVE_STATE_CS + 8);
				GuestEIP = GetVMCBD(VMCB_SAVE_STATE_RIP);
				EAX = GetVMCBD(VMCB_SAVE_STATE_CR0);
#ifdef DEBUG
				monitor_write_at("Exception Number: 0x",0,10);
				pErr = Itoa(ErrorCode - 0x40, pErr, 16);
				monitor_write(pErr);
				pErr = Itoa((int)GuestCS, pErr, 16);
				monitor_write_at("Guest CS:EIP: ",0,11);
				monitor_write("gCS=0x");
				monitor_write(pErr);
				pErr = Itoa((int)GuestEIP, pErr, 16);
				monitor_write(", gEIP=0x");
				monitor_write(pErr);
				monitor_write("  ");
				monitor_write_at("CR0 value: ",0,18);
				pErr = Itoa(EAX, pErr, 16);
				monitor_write(pErr);
				monitor_write(", CR3 value: ");
				pErr = Itoa(GetVMCBD(VMCB_SAVE_STATE_CR3), pErr, 16);
				monitor_write(pErr);
#endif
//Set the event for the exception
if (ErrorCode != VMEXIT_EXCEPTION_13)
	asm("xchg bx,bx");
if (ErrorCode == VMEXIT_EXCEPTION_01)
{
	//#DB trap gets injected as an INTR with no error code
	SetVMCBD(VMCB_CONTROL_EVENTINJ, (DWORD)(1 << 31) | (DWORD)(0 << 11)   | (DWORD)(0 << 8) | ((BYTE)ErrorCode - 0x40) );
}
else if (ErrorCode == VMEXIT_EXCEPTION_03)
{
	SetVMCBD(VMCB_CONTROL_EVENTINJ, (DWORD)(1 << 31) | (DWORD)(0 << 11)   | (DWORD)(3 << 8) | ((BYTE)3) );
}
else if (ErrorCode == VMEXIT_EXCEPTION_06)
{
	SetVMCBD(VMCB_CONTROL_EVENTINJ, (DWORD)(1 << 31) | (DWORD)(0 << 11)  | (DWORD)(3 << 8) | ((BYTE)ErrorCode - 0x40) );
}
else if (ErrorCode == VMEXIT_EXCEPTION_13)
{
	DWORD IntInfo = GetVMCBD(VMCB_CONTROL_EXITINTINFO);
	DWORD ExceptInfo = GetVMCBD(VMCB_CONTROL_EXITINFO1);
	SetVMCBD(VMCB_CONTROL_EVENTINJ, (DWORD)(1 << 31) | (DWORD)(1 << 11)  | (DWORD)(3 << 8) | ((BYTE)ErrorCode - 0x40) );
	//If the exception was of an interrupt, add the interrupt info to the exception's error message
	if ((IntInfo & 0x80000000) == 0x80000000)
	{
		IntInfo &= 0xFF;
		SetVMCBD(VMCB_CONTROL_EVENTINJ+4, (IntInfo << 3) + 2);
		SetVMCBW(VMCB_SAVE_STATE_RIP, GetVMCBW(VMCB_SAVE_STATE_RIP) -2);
	}
	else
		SetVMCBD(VMCB_CONTROL_EVENTINJ+4, ExceptInfo);
}
break;
				InfiniteLoop(ErrorCode);
				if ( GUEST_IN_PROTECTED_MODE )
					GuestExceptionHandler(ErrorCode);
				else
					HandleRealModeSWInterrupt(ErrorCode);  //TEMP: 1239 vmrun 045f
				break;
			case VMEXIT_HLT:
				if ( (GetVMCBW(VMCB_SAVE_STATE_RFLAGS) & 0x200) != 0x200)
				{
					monitor_write_at("VISOR: HLT called with IF=0",0,1);
//IncrementGuestIP(1);
//break;
				InfiniteLoop(ErrorCode);
				}
				break;
			case VMEXIT_NMI:
				monitor_write_at("     VISOR: NMI intercepted, something broke!!!",0,1);
				InfiniteLoop(ErrorCode);
//			case VMEXIT_INTR:
//					monitor_write_at("     VISOR: INTR intercept, INFO1: ",0,1);
//					pErr = Itoa(ExitInfo1, pErr, 16);
//					monitor_write(pErr);
//					monitor_write(", INFO2: ");
//					pErr = Itoa(ExitInfo2, pErr, 16);
//					monitor_write(pErr);
//					INTRLoop:
//					goto INTRLoop;
//					
//				break;
			default:
					monitor_write_at("Unknown Intercept: ",0,2);
					pErr = Itoa(ErrorCode, pErr, 16);
					monitor_write(pErr);
				InfiniteLoop(ErrorCode);
				break;
		}
	}
}
//Put the DOS MBR at 0000:7C00 in preparation for boot later on (incase we aren't using paging)
//The boot sector is already at 6000:7c00 incase we are using paging
void CopySegment0()
{
	
	lTempDWord = 0x7c00 + OFFSET_TO_GUEST_PHYSICAL_PAGES;
	lTempDWord2 = 0xA0000 + OFFSET_TO_GUEST_PHYSICAL_PAGES;
__asm__("	push	ds\n"
		"	push	es\n"

		"	mov		esi, 0x67c00\n"
		"	mov		edi, %[lTemp]\n"
		"	mov		ecx, 0x200\n"

		"	mov 	ebx, 0x10\n"
		"	push 	ebx\n"
		"	push	ebx\n"
		"	pop		es\n"
		"	pop		ds\n"
		"	cld\n"
		"	REP \n"
		"	MOVSB\n"
		
		"	mov	esi,0x000A0000\n"	//Copy entire upper memory --- A0000-FFFFF
		"	mov edi,%[lTemp2]\n"
		"	mov	ecx, 0x18000\n" //Was 0x4000 for just segment F0000
		"	REP\n"
		"	MOVSD\n"

		"	pop		es\n"
		"	pop		ds\n" 
		:
		: [lTemp] "r" (lTempDWord), [lTemp2] "r" (lTempDWord2));
		
		if (OFFSET_TO_GUEST_PHYSICAL_PAGES > 0)
		{
			lTempDWord = OFFSET_TO_GUEST_PHYSICAL_PAGES;
	__asm__("	push	ds\n"
			"	push	es\n"

			"	mov		esi, 0x0\n"
			"	mov		edi, %[lTemp]\n"
			"	mov		ecx, 0x1000\n"

			"	mov 	ebx, 0x10\n"
			"	push 	ebx\n"
			"	push	ebx\n"
			"	pop		es\n"
			"	pop		ds\n"
			"	cld\n"
			"	REP \n"
			"	MOVSB\n"
			"	pop		es\n"
			"	pop		ds\n" 
			:
			: [lTemp] "r" (lTempDWord));
		}
}
void ChangeMappingTableEntries(TableAddress, StartEntry, EntryCount, NewStart) DWORD TableAddress; DWORD StartEntry; DWORD EntryCount; DWORD NewStart;
{
	DWORD cnt = 0;

	for (cnt=0;cnt<EntryCount;cnt++)
	{
		lTempAddr = TableAddress+StartEntry+(cnt*4);
		lTempVal = cnt * 0x1000;
		lTempVal += NewStart;
		lTempVal |= 7;
		__asm__("	push ecx\n"
				"	push edx\n"
				"mov ecx, lTempVal\n"
				"mov edx, lTempAddr\n"
				"mov ax,0x10\n"
				"push ds\n"
				"push eax\n"
				"pop ds\n"
				"mov [edx], ecx\n"
				"pop ds\n"
				"pop edx\n"
				"pop ecx");
	}
	
}
void HostExceptionHandler()
{
//	char msg[] = "In exception handler for exception # ";
	
	
		
}

void host_isr_handler (WORD exc_no, WORD cs, DWORD ip, WORD error) 
{
	char* pErr = "                         ";
	if (_exception_number == 0x8 || _exception_number == 0x20)
		return;
	asm("cli\n");

	switch (_exception_number)
	{
		case 0x0:
			HostExceptMsg[35] = '0';
			break;
		case 0x1:
			HostExceptMsg[35] = '1';
			break;
		case 0x2:
			HostExceptMsg[35] = '2';
			break;
		case 0x3:
			HostExceptMsg[35] = '3';
			break;
		case 0x4:
			HostExceptMsg[35] = '4';
			break;
		case 0x5:
			HostExceptMsg[35] = '5';
			break;
		case 0x6:
			HostExceptMsg[35] = '6';
			break;
		case 0x7:
			HostExceptMsg[35] = '7';
			break;
		case 0x8:
			HostExceptMsg[35] = '8';
			break;
		case 0x9:
			HostExceptMsg[35] = '9';
			break;
		case 0xa:
			HostExceptMsg[35] = 'A';
			break;
		case 0xb:
			HostExceptMsg[35] = 'B';
			break;
		case 0xc:
			HostExceptMsg[35] = 'C';
			break;
		case 0xd:
			HostExceptMsg[35] = 'D';
			break;
		case 0xe:
			HostExceptMsg[35] = 'E';
			break;
		case 0xf:
			HostExceptMsg[35] = 'F';
			break;
		case 0x10:
			HostExceptMsg[35] = '1';
			HostExceptMsg[36] = '0';
			break;
		case 0x11:
			HostExceptMsg[35] = '1';
			HostExceptMsg[36] = '1';
			break;
		case 0x12:
			HostExceptMsg[35] = '1';
			HostExceptMsg[36] = '2';
			break;
		case 0x13:
			HostExceptMsg[35] = '1';
			HostExceptMsg[36] = '3';
			break;
		case 0x14:
			HostExceptMsg[35] = '1';
			HostExceptMsg[36] = '4';
			break;
		case 0x15:
			HostExceptMsg[35] = '1';
			HostExceptMsg[36] = '5';
			break;
		case 0x16:
			HostExceptMsg[35] = '1';
			HostExceptMsg[36] = '6';
			break;
		case 0x17:
			HostExceptMsg[35] = '1';
			HostExceptMsg[36] = '7';
			break;
		case 0x18:
			HostExceptMsg[35] = '1';
			HostExceptMsg[36] = '8';
			break;
		case 0x19:
			HostExceptMsg[35] = '1';
			HostExceptMsg[36] = '9';
			break;
		case 0x1a:
			HostExceptMsg[35] = '1';
			HostExceptMsg[36] = 'A';
			break;
		case 0x1b:
			HostExceptMsg[35] = '1';
			HostExceptMsg[36] = 'B';
			break;
		case 0x1c:
			HostExceptMsg[35] = '1';
			HostExceptMsg[36] = 'C';
			break;
		case 0x1d:
			HostExceptMsg[35] = '1';
			HostExceptMsg[36] = 'D';
			break;
		case 0x1e:
			HostExceptMsg[35] = '1';
			HostExceptMsg[36] = 'E';
			break;
		case 0x1f:
			HostExceptMsg[35] = '1';
			HostExceptMsg[36] = 'F';
			break;
		case 0x21:
			HostExceptMsg[35] = '2';
			HostExceptMsg[36] = '1';
			break;
		default:
			HostExceptMsg[35] = 'E';
			HostExceptMsg[36] = 'R';
			HostExceptMsg[37] = 'R';
	}
	monitor_write_at(HostExceptMsg,0,16);
	pErr = Itoa(_exc_CS, pErr, 16);
	monitor_write(pErr);
	monitor_write(":");
	pErr = Itoa(_exc_IP, pErr, 16);
	monitor_write(pErr);
HostExceptionLoop:	
	goto HostExceptionLoop;
}

void timer_handler() {
	asm("mov eax, GS:ticks\n");
	asm("inc eax\n");
	asm("mov GS:ticks, eax\n");
	asm("mov al, 0x20\n");
	asm("out 0x20, al\n");
//	asm("mov ax,gs\n");
//	asm("cmp ax,0\n");
//	asm("jnz TimerExit1\n");
//	asm("xchg bx,bx\n");
//	asm("TimerExit1:\n");
}

void kbd_handler() {
BYTE scancode;

	asm("mov dx, 0x60\n");
	asm("inb al, dx\n");
	asm("mov GS:scancode, al\n");
	asm("mov al, 0x20\n");
	asm("out 0x20, al\n");
}

void call_gate_proc() {
//Code below calls code that simply increments ticks
//  char far *scr = MK_FP (0x20, 80*10+10*2);
//  scr[0]++;
	__asm__("nop\n");
}
void task()
{
	return;
}
//void CreateAndLoadTSS()
//{
//	return;
//	tss = (TSS *)HOST_TSS_FINAL;
//	tss[1].trace = 0;
//	tss[1].io_map_addr = sizeof(TSS);           /* I/O map just after the TSS */
//	tss[1].ldtr = 0;                              /* ldtr=0 */
//
//	tss[1].fs = tss[1].gs = 0;                    /* fs=gs=0 */
//
//	tss[1].ds = tss[1].es = tss[1].ss = 0x40 | 3; /* ds=es=ss = data segment */
//
//	tss[1].cs = 0x50 | 3;
//
//	tss[1].eflags = 0x3202L;                      /* IOPL=3, interrupts are enabled */
//	tss[1].esp = (WORD)&task_stack[1];            /* sp points to task stack top */
//
//	tss[1].eip = (WORD)&task;                     /* cs:eip point to task() */
//
//	tss[1].ss0 = 0x10;
//	tss[1].esp0 = (WORD)&pl0_stack[1];
//	
//	DESCR_SEG *item = (DESCR_SEG*)HOST_GDT;
//	setup_GDT32_entry_gcc(item+11, (DWORD)&_call_gate_wrapper, sizeof(tss), ACS_TSS, 0);
//	_ltr(HOST_TSS_SEGMENT);
//}

volatile int Wait(int interval)
{
	asm("STI\n");
	DWORD y = ticks + interval;
	while (ticks<y)
	{asm("xchg eax,eax");}
	asm("CLI\n");
	return 0;
}

void InfiniteLoop(int InterceptCode)
{
char *pErr = "                    \0";

	gInterceptCode = InterceptCode;
	asm("cli\n");
	monitor_write_at("In the toilet (aka infinite loop) from intercept code: 0x",0,15);
	pErr = Itoa(InterceptCode, pErr, 16);
	monitor_write(pErr);
	monitor_write_at("CR0 value: ",0,17);
	pErr = Itoa(GetVMCBD(VMCB_SAVE_STATE_CR0), pErr, 16);
	monitor_write(pErr);
	monitor_write(", CR3 value: ");
	pErr = Itoa(GetVMCBD(VMCB_SAVE_STATE_CR3), pErr, 16);
	monitor_write(pErr);
InfiniteLoop1:
	goto InfiniteLoop1;
}

