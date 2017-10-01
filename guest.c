#include "start.h"
#include "utility.h"

void SetupGuestPages();
void HandleRealModeSWInterrupt(WORD vector);
void HandleInterrutpReturn();
void HandleCR0Write();
void GuestInt13Handler();
void UpdateGuestForSoftwareInterrupt(BYTE Vector);
void HandleGuestIO();
void GuestExceptionHandler(WORD ErrorCode);
void HandleGuestPagingException(DWORD Addr);
void SetGuestMODRegGenRegValue(BYTE RegOffsetFromCurrIns, DWORD Value, bool DW, bool W);

volatile struct sIntDetails* IntDetails = (volatile struct sIntDetails*)SW_INT_DETAILS_ADDRESS;;
DWORD GuestInt13ReturnAddress = 0, GuestInt13IntDetailsPtr = 0, GuestInt13BufferAddress=0, GuestInt13BufferSize=0, IntDetailsPtr = 0;
bool GuestInInt13 = false;
extern DWORD HostRegisters[14], GuestRegisters[14];
char GuestExceptMsg[] = "In ***GUEST*** exception handler for error                                  \0";  //blank is at 43
bool GuestPModeFlag = false;
bool GuestPagingFlag = false;
DWORD GuestRequestedCR3 = GUEST_CR3_VALUE, GuestRequestedCR0 = GUEST_INITIAL_CR0;
static DWORD IOPortInValue = 0;
static DWORD IOPortInValueSet = false;
void HandleCR3Read();
void HandleCR0Write();
void HandleCR3Write();
void HandleCR0Read();

void SetupGuestPages()
{
	DWORD hdPtr = 0;
	DWORD htPtr = 0;
	DWORD *ptr = (DWORD *)hdPtr;
	DWORD *ptrT = (DWORD *)htPtr;
	DWORD cnt=0;
#ifdef USE_NESTED_PAGING
	DWORD cnt2=0;
#endif
	//BYTE props = 7;
	
__asm__("	push	ds\n"
		"	push	es\n"
		"	mov 	ebx, 0x10\n"
		"	push 	ebx\n"
		"	push 	ebx\n"
		"	pop		ds\n"
		"	pop		es\n");
	
	//TRUE GUEST TABLES!!!
	
	ptr = (DWORD*)(OFFSET_TO_GUEST_PHYSICAL_PAGES + GUEST_CR3_VALUE);
	ptrT =(DWORD*)(OFFSET_TO_GUEST_PHYSICAL_PAGES + GUEST_CR3_VALUE + 0x1000);
	for (cnt=0;cnt <= (GUEST_MEMORY_SIZE / 0x400000); cnt++) //Guest memory size / 4 MB
	{
		ptr[cnt] = (OFFSET_TO_GUEST_PHYSICAL_PAGES + GUEST_CR3_VALUE + 4096 + (cnt*4096)) | 7;
#ifdef USE_NESTED_PAGING
		for (cnt2=0;cnt2<1024;cnt2++)
			ptrT[cnt2] = ((0x400000 * cnt) + ((cnt2) << 12)) | 7;
#endif
		ptrT += 0x400;
	}
__asm__("	pop		es\n"
		"	pop		ds\n");
		
	//ChangeMappingTableEntries(0x102000, 0, 1, 0);
	//0xF0000 segment only
	//ChangeMappingTableEntries(0x102000, (DWORD)0x3C0, (DWORD)0x10, 0x000F0000); 
	//***THIS SHOULD BE RUN ON A REAL PC!!!***
	//ChangeMappingTableEntries(0x102000, (DWORD)0x280, (DWORD)0x60, 0x000A0000); 
	//Skipping A0000-BFFFF as it causes errors in BOCHS.
	//ChangeMappingTableEntries(0x102000, (DWORD)0x300, (DWORD)0x40, 0x000C0000); 
}
void HandleRealModeSWInterrupt(WORD vector)
{
	WORD EAX = GetVMCBW(VMCB_SAVE_STATE_RAX);
//	if ((vector == 0x8) || (vector == 0x0) || (vector == 0x1c) || (vector==0x10) || (vector == 0x16))
//	{
//		UpdateGuestForSoftwareInterrupt(vector);
//		return;
//	}
/*
	IntDetails[IntDetailsPtr].IntNum = vector;
	IntDetails[IntDetailsPtr].AXVal = GetVMCBW(VMCB_SAVE_STATE_RAX);
	IntDetails[IntDetailsPtr].BXVal = GuestRegisters[0];
	IntDetails[IntDetailsPtr].ESVal = GetVMCBW(VMCB_SAVE_STATE_ES);	
	IntDetails[IntDetailsPtr].CSVal = GetVMCBW(VMCB_SAVE_STATE_CS);
	IntDetails[IntDetailsPtr].IPVal = GetVMCBW(VMCB_SAVE_STATE_RIP);
*/

	//Do something based on the interrupt #
	switch (vector)
	{
		case 0x11:	//GET EQUIPMENT LIST
			SetVMCBW(VMCB_SAVE_STATE_RAX,0x0122);  //only video mode & 80x87 & DMA set  100100010
			break;
		case 0x12:
			SetVMCBW(VMCB_SAVE_STATE_RAX,0x27f);	//4 MB of ram installed - mem size / 1k
			break;
		case 0x13:
			//GuestInt13Handler(); //CLR 09/21/2014 - Finding out if this is broken
			if ( (EAX & 0x0800) == 0x0800)
			{
				SetVMCBW(VMCB_SAVE_STATE_RAX, 0x0000);
				//asm("xchg bx, bx\n");
				GuestRegisters[RAN_EBX] = 0x0000;
				GuestRegisters[RAN_ECX] = 0x037F;	//CH=low eight bits of maximum cylinder number, CL=high two bits of maximum cylinder number + 
									//						   maximum sector number (bits 5-0) (0x80, 0x3f = BF)
									//
				GuestRegisters[RAN_EDX] = 0x0f01;	//DH = maximum head number, DL = number of drives
				CLEAR_GUEST_CARRY_FLAG
			}
			else
			{
				SetVMCBW(VMCB_SAVE_STATE_RIP, GetVMCBW(VMCB_CONTROL_NRIP));
				SetVMCBD(VMCB_CONTROL_EVENTINJ, (DWORD)(1 << 31) | (DWORD)(0 << 11) | (DWORD)(4 << 8) | (BYTE)vector);
			}
			break;
		case 0x15:
			if ( (EAX & 0xFF00) == 0x8800)
			{
				SetVMCBW(VMCB_SAVE_STATE_RAX, (GUEST_MEMORY_SIZE-0x100000) / 0x400); //0xC00
				CLEAR_GUEST_CARRY_FLAG
			}
			else if ( EAX == 0xE820 || EAX == 0xE802 || EAX == 0xE801)
			{
				SetVMCBW(VMCB_SAVE_STATE_RAX,0x8600 | (EAX & 0x00FF) );
				SET_GUEST_CARRY_FLAG
			}
			else
			{
				SetVMCBW(VMCB_SAVE_STATE_RIP, GetVMCBW(VMCB_CONTROL_NRIP));
				SetVMCBD(VMCB_CONTROL_EVENTINJ, (DWORD)(1 << 31) | (DWORD)(0 << 11) | (DWORD)(4 << 8) | (BYTE)vector);
			}
			break;
		default:
			{
				SetVMCBW(VMCB_SAVE_STATE_RIP, GetVMCBW(VMCB_CONTROL_NRIP));
				SetVMCBD(VMCB_CONTROL_EVENTINJ, (DWORD)(1 << 31) | (DWORD)(0 << 11) | (DWORD)(4 << 8) | (BYTE)vector);
			}
			break;
	}
/*	if ( (vector != 0x13) && (vector != 0x21) )
		IntDetailsPtr++;
*/
}
//This is only called on INTERRUPT which implies only REAL or V8086 mode -- probably change my mind about this later
void PushCSIPFlagsOntoGuestStackReal16(DWORD cs, WORD ip, WORD flags)
{
	DWORD GuestStackAddressAfterPush = GetVMCBW(VMCB_SAVE_STATE_SS + 8) ;
	SetVMCBW(VMCB_SAVE_STATE_RSP, GetVMCBD(VMCB_SAVE_STATE_RSP)-6);
	GuestStackAddressAfterPush += GetVMCBD(VMCB_SAVE_STATE_RSP);
	//Make sure the guest stack doesn't page fault
	HandleGuestPagingException(GuestStackAddressAfterPush);
	SetMemW(GuestStackAddressAfterPush+4, flags, true);
	SetMemW(GuestStackAddressAfterPush+2,cs >> 4, true);
	SetMemW(GuestStackAddressAfterPush,ip, true);	//Don't add 2 to the IP to account for the executed INT instruction (CDXX) as it was done elsewhere
	flags &= 0xFDFF; //Clear IF
	flags &= 0xFEFF; //Clear TF
	SetVMCBW(VMCB_SAVE_STATE_RFLAGS, flags);
}

//This can be called because of a Protected mode IRET too, so we have to do it by the book.
void PopCSIPFlagsOffGuestStackReal16()
{
	DWORD GuestStackAddressBeforePop = GetVMCBW(VMCB_SAVE_STATE_SS + 8) ;
	 GuestStackAddressBeforePop += GetVMCBW(VMCB_SAVE_STATE_RSP);
	DWORD lNewCS = 0, lNewDS, lNewSS, lNewES, lNewGS, lNewFS;
	DWORD lNewIP = 0, lNewSP;
	DWORD lNewFlags = 0;
	//Make sure the guest stack doesn't page fault
	HandleGuestPagingException( GuestStackAddressBeforePop);
//CLR 09/14/2014 - Copied block from below to see if it helps my IRET
		lNewFlags = GetMemW( GuestStackAddressBeforePop+4, true);
		lNewFlags |= 0x200;		//Turn on interrupts
		lNewFlags &= 0xFEFF;	//Turn off debug (single step)
		SetVMCBW(VMCB_SAVE_STATE_RFLAGS,lNewFlags);
		//CS:IP handling
		lNewCS = GetMemW( GuestStackAddressBeforePop+2, true);
		//SetVMCBW(VMCB_SAVE_STATE_CS, lNewCS);		//CLR 09/22/2014 - This was wrong!
		SetVMCBD(VMCB_SAVE_STATE_CS + 8, lNewCS << 4);
		lNewIP = GetMemW( GuestStackAddressBeforePop, true);
		SetVMCBW(VMCB_SAVE_STATE_RIP, lNewIP);
		//Make sure the guest CS:IP doesnt' page fault
		HandleGuestPagingException( ( (lNewCS) + lNewIP));
		//Increase the stack pointer to release the "popped" values above
		SetVMCBW(VMCB_SAVE_STATE_RSP, GetVMCBW(VMCB_SAVE_STATE_RSP)+6);
return;






	if ( GUEST_IN_PROTECTED_MODE ) //NOTE: GUEST_IN_PROTECTED_MODE verifies that VM Flag is not set
	{
		if ((GetMemB(GuestStackAddressBeforePop+9,true) & 0x2) == 0x2)  //V8086 mode FROM PMode
		{
			lNewIP = GetMemW(GuestStackAddressBeforePop, true);
			lNewCS = GetMemW(GuestStackAddressBeforePop+4, true);
			lNewFlags = ( (GetVMCBD(VMCB_SAVE_STATE_RFLAGS) & 0xFFFE0000) | (GetMemW(GuestStackAddressBeforePop+8, true)) );
			lNewSP = GetMemW(GuestStackAddressBeforePop+12, true);
			lNewSS = GetMemW(GuestStackAddressBeforePop+16, true);
			lNewES = GetMemW(GuestStackAddressBeforePop+20, true);
			lNewDS = GetMemW(GuestStackAddressBeforePop+24, true);
			lNewFS = GetMemW(GuestStackAddressBeforePop+28, true);
			lNewGS = GetMemW(GuestStackAddressBeforePop+32, true);
			SetVMCBW(VMCB_SAVE_STATE_RIP, lNewIP);
			//SetVMCBW(VMCB_SAVE_STATE_CS, (WORD)lNewCS);			//clr 09/21/2014 - This is wrong!  Even for Protected mode!
			SetVMCBD(VMCB_SAVE_STATE_CS + 8, (WORD)(lNewCS));
			SetVMCBW(VMCB_SAVE_STATE_RSP, lNewSP);
			SetVMCBW(VMCB_SAVE_STATE_SS, (WORD)lNewSS);
			SetVMCBD(VMCB_SAVE_STATE_SS + 8, (WORD)(lNewSS) << 4);
			SetVMCBW(VMCB_SAVE_STATE_ES, (WORD)lNewES);
			SetVMCBD(VMCB_SAVE_STATE_ES + 8, (WORD)(lNewES) << 4);
			SetVMCBW(VMCB_SAVE_STATE_DS, (WORD)lNewDS);
			SetVMCBD(VMCB_SAVE_STATE_DS + 8, (WORD)(lNewDS) << 4);
			SetVMCBW(VMCB_SAVE_STATE_FS, (WORD)lNewFS);
			SetVMCBD(VMCB_SAVE_STATE_FS + 8, (WORD)(lNewFS) << 4);
			SetVMCBW(VMCB_SAVE_STATE_GS, (WORD)lNewGS);
			SetVMCBD(VMCB_SAVE_STATE_GS + 8, (WORD)(lNewGS) << 4);
			SetVMCBD(VMCB_SAVE_STATE_CPL,3);
			//(*Resume execution in Virtual-8086 mode*)
		}
		else				//PMode from PMode
		{
			lNewFlags = ( (GetVMCBD(VMCB_SAVE_STATE_RFLAGS) & 0xFFFF0000) | (GetMemW(GuestStackAddressBeforePop+4, true)) );
			lNewFlags |= 0x200;		//Turn on interrupts
			lNewFlags &= 0xFEFF;	//Turn off debug (single step)
			SetVMCBW(VMCB_SAVE_STATE_RFLAGS,lNewFlags);
			//CS:IP handling
			lNewCS = GetMemW(GuestStackAddressBeforePop+2, true);
			//SetVMCBW(VMCB_SAVE_STATE_CS, lNewCS);				//CLR 09/21/2014 - this is also wrong
			SetVMCBD(VMCB_SAVE_STATE_CS + 8, lNewCS << 4);
			lNewIP = GetMemW(GuestStackAddressBeforePop, true);
			SetVMCBW(VMCB_SAVE_STATE_RIP, lNewIP);
			//Make sure the guest CS:IP doesnt' page fault
			HandleGuestPagingException( (lNewCS << 4) | lNewIP);

			//Increase the stack pointer to release the "popped" values above
				SetVMCBW(VMCB_SAVE_STATE_RSP, GetVMCBW(VMCB_SAVE_STATE_RSP)+6);
		}
	}
	else
	{
		lNewFlags = GetMemW(GuestStackAddressBeforePop+4, true);
		lNewFlags |= 0x200;		//Turn on interrupts
		lNewFlags &= 0xFEFF;	//Turn off debug (single step)
		if (GUEST_IN_V8086_MODE)
		{
			lNewFlags = (GetVMCBD(VMCB_SAVE_STATE_RFLAGS) & 0xFFFF3000) | (lNewFlags & 0xCFFF);
		}
		SetVMCBW(VMCB_SAVE_STATE_RFLAGS,lNewFlags);

		//CS:IP handling
		lNewCS = GetMemW(GuestStackAddressBeforePop+2, true);
		//SetVMCBW(VMCB_SAVE_STATE_CS, lNewCS);		//CLR 09/22/2014 - This was wrong!
		SetVMCBD(VMCB_SAVE_STATE_CS + 8, lNewCS << 4);
		lNewIP = GetMemW(GuestStackAddressBeforePop, true);
		SetVMCBW(VMCB_SAVE_STATE_RIP, lNewIP);
		//Make sure the guest CS:IP doesnt' page fault
		HandleGuestPagingException( (lNewCS << 4) | lNewIP);

		//Increase the stack pointer to release the "popped" values above
		SetVMCBW(VMCB_SAVE_STATE_RSP, GetVMCBW(VMCB_SAVE_STATE_RSP)+6);
	}
}
void UpdateGuestForSoftwareInterrupt(BYTE Vector)
{
	WORD lNewCS = 0;
	WORD lNewIP = 0;
	//Push CS:IP onto the guest's stack
	//NOTE: THE +2 on IP is because all SW ints are 2 bytes in length and the return address we want to push on the stack is the one after the interrupt
	//NOTE: CS is saved in the protected mode format even in real mode, so to push it on the stack we have to shift it back to where it would be in real mode
	PushCSIPFlagsOntoGuestStackReal16(GetVMCBW(VMCB_SAVE_STATE_CS+8), GetVMCBW(VMCB_SAVE_STATE_RIP) +2, GetVMCBW(VMCB_SAVE_STATE_RFLAGS));
	//Get Interrupt Vector
	lNewCS = GetMemW(Vector * 4 + 2, true);
	//SetVMCBW(VMCB_SAVE_STATE_CS, lNewCS);				
	SetVMCBD(VMCB_SAVE_STATE_CS + 8, lNewCS << 4);			
	lNewIP = GetMemW(Vector * 4, true);
	SetVMCBW(VMCB_SAVE_STATE_RIP, lNewIP );
	//Make sure the guest CS:IP doesnt' page fault
	HandleGuestPagingException( (lNewCS << 4) | lNewIP);
}
void GuestInt13Handler()
{
	bool SingleRead = false;
	
	DWORD lEAX = GetVMCBW(VMCB_SAVE_STATE_RAX);
	DWORD DiskTransferPacketAddress = 0;
	
			//If reading
			if ( (lEAX & 0xFF00) == 0x0200 || ( (lEAX & 0xFF00) == 0x4200))
			{
				if ( (lEAX & 0xFF00) == 0x0200)
					SingleRead = true;
				GuestInInt13 = true;
				GuestInt13IntDetailsPtr = IntDetailsPtr;
				GuestInt13ReturnAddress = ((GetVMCBW(VMCB_SAVE_STATE_CS + 8)) | GetVMCBW(VMCB_SAVE_STATE_RIP)) + 2;

				//Redirect boot sector accesses to where we moved it on the diskette
				//TODO: Handle multiple sector read starting at boot sector
				if (((WORD)GuestRegisters[1]/*CX*/ == 0x0001) && ((WORD)GuestRegisters[2]/*DX*/ == 0x0000))
				{
					GuestRegisters[1] = 0x4f12;
					GuestRegisters[2] = 0x0100;
				}
				else if (((WORD)GuestRegisters[1]/*CX*/ == 0x0001) && ((WORD)GuestRegisters[2]/*DX*/ == 0x0080))
				{
					GuestRegisters[1] = 0xF811;
					GuestRegisters[2] = 0x0080;
				}
				if (SingleRead)
				{
					GuestInt13BufferAddress	= (GetVMCBW(VMCB_SAVE_STATE_ES) ) | GuestRegisters[0]/*BX*/;
					GuestInt13BufferSize = (lEAX & 0xFF) * 0x200; //512 byte sectors
				}
				else
				{
					DiskTransferPacketAddress = (GetVMCBW(VMCB_SAVE_STATE_DS) ) +(WORD)GuestRegisters[3/*SI*/] ;
					//NOTE: Buffer address is in SEG:OFS (XXXX:YYYY) format
					GuestInt13BufferAddress	= ( (GetMemW(DiskTransferPacketAddress+6 ,true) ) + GetMemW(DiskTransferPacketAddress+4 ,true) ) ;
					GuestInt13BufferSize = GetMemW(DiskTransferPacketAddress+2,true) * 0x200 ; //512 byte sectors
				}
			}
			else
			{
				GuestInt13BufferAddress	= 0xFFFFFFFF;
				GuestInt13BufferSize = 0;
			}
		UpdateGuestForSoftwareInterrupt(0x13);
}
void HandleInterrutpReturn()
{
	PopCSIPFlagsOffGuestStackReal16();
/*	if ((GuestInInt13==true) && (GuestInt13BufferAddress != 0xFFFFFFFF))
		if (GuestInt13ReturnAddress == ((GetVMCBW(VMCB_SAVE_STATE_CS+ 8)) | GetVMCBW(VMCB_SAVE_STATE_RIP)))
		{
			//IntDetails[GuestInt13IntDetailsPtr].AXRetVal = GetVMCBW(VMCB_SAVE_STATE_RAX);
			//IntDetails[GuestInt13IntDetailsPtr].FLAGSVal = GetVMCBW(VMCB_SAVE_STATE_RFLAGS);
			GuestInInt13 = false;
			DWORD BufferHostPhysAddy = TranslateGuestPhysicalAddress(GuestInt13BufferAddress);
			if (GuestInt13BufferAddress != BufferHostPhysAddy)
				CopyData(GuestInt13BufferAddress, BufferHostPhysAddy, GuestInt13BufferSize);
		}
*/}

DWORD GetCSIp()
{
	//NOTE: Regardless of prot or real mode, in the VMCB CS is always saved in the prot mode format so no need to shift it to the left ... ever
	return GetVMCBD(VMCB_SAVE_STATE_CS + 8) + GetVMCBD(VMCB_SAVE_STATE_RIP);
}

DWORD GetGuestMODRegGenRegValue(BYTE RegOffsetFromCurrIns, bool DW, bool W)
{
	BYTE reg = 0;
	DWORD CSIp = GetCSIp();

	
	reg = GetMemB( CSIp + RegOffsetFromCurrIns,true);

	switch ( (reg & 0x7) ) //ModRM specifies general register
	{
		case MODRM_REG_EAX:
			if (DW)
				return GetVMCBD(VMCB_SAVE_STATE_RAX);
			else if (W)
				return GetVMCBW(VMCB_SAVE_STATE_RAX);
			else
				return GetVMCBB(VMCB_SAVE_STATE_RAX);
		case MODRM_REG_ECX:
			if (DW)
				return GuestRegisters[RAN_ECX]; 
			else if (W)
				return (WORD)GuestRegisters[RAN_ECX]; 
			else
				return (BYTE)GuestRegisters[RAN_ECX]; 
		case MODRM_REG_EDX:
			if (DW)
				return GuestRegisters[RAN_EDX]; 
			else if (W)
				return (WORD)GuestRegisters[RAN_EDX]; 
			else
				return (BYTE)GuestRegisters[RAN_EDX]; 
		case MODRM_REG_EBX:
			if (DW)
				return GuestRegisters[RAN_EBX]; 
			else if (W)
				return (WORD)GuestRegisters[RAN_EBX]; 
			else
				return (BYTE)GuestRegisters[RAN_EBX]; 
		case MODRM_REG_ESP:
			if (DW)
				return GetVMCBD(VMCB_SAVE_STATE_RSP);
			else if (W)
				return GetVMCBW(VMCB_SAVE_STATE_RSP);
			else
				return GetVMCBB(VMCB_SAVE_STATE_RSP);
		case MODRM_REG_EBP:
			if (DW)
				return GuestRegisters[RAN_EBP]; 
			else if (W)
				return (WORD)GuestRegisters[RAN_EBP]; 
			else
				return (BYTE)GuestRegisters[RAN_EBP]; 
		case MODRM_REG_ESI:
			if (DW)
				return GuestRegisters[RAN_ESI]; 
			else if (W)
				return (WORD)GuestRegisters[RAN_ESI]; 
			else
				return (BYTE)GuestRegisters[RAN_ESI]; 
		case MODRM_REG_EDI:
			if (DW)
				return GuestRegisters[RAN_EDI]; 
			else if (W)
				return (WORD)GuestRegisters[RAN_EDI]; 
			else
				return (BYTE)GuestRegisters[RAN_EDI]; 
	}
	return 0xED;
}
void SetGuestMODRegGenRegValue(BYTE RegOffsetFromCurrIns, DWORD Value, bool DW, bool W)
{
	BYTE reg = 0;
	if (GetVMCBB(VMCB_SAVE_STATE_CR0) & 0x01)
		//Get CS base + RIP + Offset
		reg = GetMemB(GetVMCBW(VMCB_SAVE_STATE_CS + 8) + GetVMCBD(VMCB_SAVE_STATE_RIP) + RegOffsetFromCurrIns,true);
	else
		reg = GetMemB( (GetVMCBW(VMCB_SAVE_STATE_CS + 8)) + GetVMCBD(VMCB_SAVE_STATE_RIP) + RegOffsetFromCurrIns,true );
	
	switch ( reg & 0x7 )
	{
		case MODRM_REG_EAX:
			if (DW)
				SetVMCBD(VMCB_SAVE_STATE_RAX,Value);
			else if (W)
				SetVMCBW(VMCB_SAVE_STATE_RAX,(WORD)Value);
			else
				SetVMCBB(VMCB_SAVE_STATE_RAX,(BYTE)Value);
			break;
		case MODRM_REG_EBX:
			if (DW)
				GuestRegisters[0] = Value; 
			else if (W)
				GuestRegisters[0] = (WORD)Value;
			else
				GuestRegisters[0] = (BYTE)Value;
			break;
		case MODRM_REG_ECX:
			if (DW)
				GuestRegisters[1] = Value; 
			else if (W)
				GuestRegisters[1] = (WORD)Value;
			else
				GuestRegisters[1] = (BYTE)Value;
			break;
		case MODRM_REG_EDX:
			if (DW)
				GuestRegisters[2] = Value; 
			else if (W)
				GuestRegisters[2] = (WORD)Value;
			else
				GuestRegisters[2] = (BYTE)Value;
			break;
		case MODRM_REG_ESI:
			if (DW)
				GuestRegisters[3] = Value; 
			else if (W)
				GuestRegisters[3] = (WORD)Value;
			else
				GuestRegisters[3] = (BYTE)Value;
			break;
		case MODRM_REG_EDI:
			if (DW)
				GuestRegisters[4] = Value; 
			else if (W)
				GuestRegisters[4] = (WORD)Value;
			else
				GuestRegisters[4] = (BYTE)Value;
			break;
		case MODRM_REG_ESP:
			if (DW)
				SetVMCBD(VMCB_SAVE_STATE_RIP,Value);
			else if (W)
				SetVMCBW(VMCB_SAVE_STATE_RIP,(WORD)Value);
			else
				SetVMCBB(VMCB_SAVE_STATE_RIP,(BYTE)Value);
			break;
		case MODRM_REG_EBP:
			if (DW)
				GuestRegisters[5] = Value; 
			else if (W)
				GuestRegisters[5] = (WORD)Value;
			else
				GuestRegisters[5] = (BYTE)Value;
			break;
	}
}

void HandleCR0Write()
{
DWORD EAX = 0;
DWORD CSIp = GetCSIp();
WORD Instruction = GetMemW(CSIp, true);
WORD IPIncrement = 3;

return;
	if (GetVMCBD(VMCB_CONTROL_EXITINFO1_HIDWORD & 0x80000000)==0x80000000)
		GuestRequestedCR0 = GetVMCBD(VMCB_SAVE_STATE_RAX)  | 0x80000000; //GetGuestMODRegGenRegValue(2,true, false);//for now
	else if (Instruction == OPCODE_CLTS)
	{
		GuestRequestedCR0 &= 0xFFFFFFF7; //Clear task switching flag
		SetVMCBD(VMCB_SAVE_STATE_RIP,GetVMCBD(VMCB_CONTROL_NRIP));
		return;
	}
	else //LMSW
		GuestRequestedCR0 = ((GuestRequestedCR0 & 0xFFFF0000) | GetVMCBD(VMCB_SAVE_STATE_RAX)) | 0x80000000; //GetGuestMODRegGenRegValue(2, false, true);
	EAX = GuestRequestedCR0;
	SetVMCBD(VMCB_SAVE_STATE_CR0,EAX);
	if ( ((EAX & 1) == 1) )
		GuestPModeFlag = true;
	else
		GuestPModeFlag = false;
return;
	//We're going to leave this alone for now and skip it because CR3 paging is more complicated than it is coded here
	if ( GuestPModeFlag && (((EAX & 0x80000000) & 0x80000000) == 0x80000000) )
	{
		SetVMCBD(VMCB_SAVE_STATE_CR3, GuestRequestedCR3);
		GuestPagingFlag = true;
	}
	else if ( GuestPModeFlag && (((EAX & 0x80000000) & 0x80000000) != 0x80000000))
	{
		SetVMCBD(VMCB_SAVE_STATE_CR3, GUEST_CR3_VALUE);
		GuestPagingFlag = false;
	}
}

void HandleCR3Write()
{
	GuestRequestedCR3 = GetGuestMODRegGenRegValue(2,true,false);
	if (GuestPModeFlag && GuestPagingFlag && GuestRequestedCR0 > 0)
	{
		SetVMCBD(VMCB_SAVE_STATE_CR3,GuestRequestedCR3);

	}
	IncrementGuestIP(3);
}

void HandleCR0Read()
{
DWORD CSIp = GetCSIp();
WORD Instruction = GetMemW(CSIp, true);

	//TODO: SMSW can write to memory which isn't covered here
	if (Instruction == OPCODE_SMSW)
		SetGuestMODRegGenRegValue(2, GuestRequestedCR0, false, true);
	else
		SetGuestMODRegGenRegValue(2, GuestRequestedCR0, true, false);
	IncrementGuestIP(3);
}

void HandleCR3Read()
{
	if (GuestPModeFlag && GuestPagingFlag)
		SetGuestMODRegGenRegValue(2,GuestRequestedCR3, true, false);
	else
		SetGuestMODRegGenRegValue(2,0, true, false);

	IncrementGuestIP(3);
}


void HandleGuestIO()	
{
	DWORD ExitInfo1  = GetVMCBD(VMCB_CONTROL_EXITINFO1);
	DWORD EAX = 0;
	WORD port = (WORD)(ExitInfo1 >> 16);
	WORD direction = (WORD)ExitInfo1 & 0x1;
	bool sz8 =  ((ExitInfo1 & 0x10) == 0x10);
	bool sz16 = ((ExitInfo1 & 0x20) == 0x20);
	//bool sz32 = ((ExitInfo1 & 0x40) == 0x40);

	switch (port)
	{
		case 0x70:  //0070	w	CMOS RAM index register port (ISA, EISA) - write only
			//17h 	23 	1 byte 	Extended Memory Low Order Byte - Least significant byte
			//18h 	24 	1 byte 	Extended Memory High Order Byte - Most significant byte
			if (direction != PORT_DIRECTION_IN)
			{
				EAX = GetVMCBD(VMCB_SAVE_STATE_RAX);
				if ( (EAX & 0xFF) == 0x17 )
				{
					IOPortInValue = (BYTE)(((GUEST_MEMORY_SIZE - 0x100000) / 0x400) & 0xFF);
					IOPortInValueSet = true;
					goto lblReturn;
				}
				else if ( (EAX & 0xFF) == 0x18 )
				{
					IOPortInValue = (BYTE)((((GUEST_MEMORY_SIZE - 0x100000) / 0x400) >> 8) & 0xFF);
					IOPortInValueSet = true;
					goto lblReturn;
				}
				//IO port was 70, direction out, but index wasn't 17 or 18 so do standard stuff
			}
			else
				//Invalid, port 70 is write (OUT) only so do nothing
				goto lblReturn;
			
			break;
		case 0x71: //CMOS data port
			if (direction == PORT_DIRECTION_IN)
			{
				if (IOPortInValueSet)
				{
					//NOTE: Port 71 only returns 1 byte
					SetVMCBB(VMCB_SAVE_STATE_RAX, (BYTE)IOPortInValue);
					IOPortInValue = 0;
					IOPortInValueSet = false;
					goto lblReturn;
				}
				//Port 71 but prior read from port 70 wasn't 17 or 18 so do standard stuff
			}
			//IO port was 71 but direction was out so do standard stuff
			break;
	}
	
	//Forward the In/Out to the real CMOS
	if (direction == PORT_DIRECTION_IN)
	{
		if (sz8)
		{
			EAX = inb(port);
			SetVMCBB(VMCB_SAVE_STATE_RAX,EAX);
		}
		else if (sz16)
		{
			EAX = (inb(port+1) << 8) | inb(port);
			SetVMCBW(VMCB_SAVE_STATE_RAX,EAX);
		}
		else
		{
			EAX = (inb(port+3) << 24) | (inb(port+2) << 16) | (inb(port+1) << 8) | inb(port);
			SetVMCBD(VMCB_SAVE_STATE_RAX,EAX);
		}
	}
	else //195b
	{
		EAX = GetVMCBD(VMCB_SAVE_STATE_RAX);
		if (sz8)
			outb(port, (BYTE)EAX & 0xFF); 
		else if (sz16)
		{
			outb(port, (BYTE)EAX & 0xFF); 
			outb(port+1, (BYTE)(EAX & 0xFF00) >> 8);
		}
		else
		{
			outb(port, (BYTE)EAX & 0xFF); 
			outb(port+1, (BYTE)(EAX & 0xFF00) >> 8);
			outb(port+2, (BYTE)(EAX & 0xFF0000) >> 16);
			outb(port+3, (BYTE)(EAX & 0xFF000000) >> 24);
		}
	}
lblReturn:
	SetVMCBW(VMCB_SAVE_STATE_RIP, GetVMCBW(VMCB_CONTROL_EXITINFO2));
}
void GuestExceptionHandler(WORD ErrorCode)
{
	
	switch (ErrorCode - 0x40) //Convert VMCB exception codes 40-5f to "normal" exception codes
	{
		case 0x0:
			GuestExceptMsg[43] = '0';
			break;
		case 0x1:
			GuestExceptMsg[43] = '1';
			break;
		case 0x2:
			GuestExceptMsg[43] = '2';
			break;
		case 0x3:
			GuestExceptMsg[43] = '3';
			break;
		case 0x4:
			GuestExceptMsg[43] = '4';
			break;
		case 0x5:
			GuestExceptMsg[43] = '5';
			break;
		case 0x6:
			GuestExceptMsg[43] = '6';
			break;
		case 0x7:
			GuestExceptMsg[43] = '7';
			break;
		case 0x8:
			GuestExceptMsg[43] = '8';
			break;
		case 0x9:
			GuestExceptMsg[43] = '9';
			break;
		case 0xa:
			GuestExceptMsg[43] = 'a';
			break;
		case 0xb:
			GuestExceptMsg[43] = 'b';
			break;
		case 0xc:
			GuestExceptMsg[43] = 'c';
			break;
		case 0xd:
			GuestExceptMsg[43] = 'd';
			break;
		case 0xe:
			GuestExceptMsg[43] = 'e';
			break;
		case 0xf:
			GuestExceptMsg[43] = 'f';
			break;
		case 0x10:
			GuestExceptMsg[43] = '1';
			GuestExceptMsg[44] = '0';
			break;
		case 0x11:
			GuestExceptMsg[43] = '1';
			GuestExceptMsg[44] = '1';
			break;
		case 0x12:
			GuestExceptMsg[43] = '1';
			GuestExceptMsg[44] = '2';
			break;
		case 0x13:
			GuestExceptMsg[43] = '1';
			GuestExceptMsg[44] = '3';
			break;
		case 0x14:
			GuestExceptMsg[43] = '1';
			GuestExceptMsg[44] = '4';
			break;
		case 0x15:
			GuestExceptMsg[43] = '1';
			GuestExceptMsg[44] = '5';
			break;
		case 0x16:
			GuestExceptMsg[43] = '1';
			GuestExceptMsg[44] = '6';
			break;
		case 0x17:
			GuestExceptMsg[43] = '1';
			GuestExceptMsg[44] = '7';
			break;
		case 0x18:
			GuestExceptMsg[43] = '1';
			GuestExceptMsg[44] = '8';
			break;
		case 0x19:
			GuestExceptMsg[43] = '1';
			GuestExceptMsg[44] = '9';
			break;
		case 0x1a:
			GuestExceptMsg[43] = '1';
			GuestExceptMsg[44] = 'a';
			break;
		case 0x1b:
			GuestExceptMsg[43] = '1';
			GuestExceptMsg[44] = 'b';
			break;
		case 0x1c:
			GuestExceptMsg[43] = '1';
			GuestExceptMsg[44] = 'c';
			break;
		case 0x1d:
			GuestExceptMsg[43] = '1';
			GuestExceptMsg[44] = 'd';
			break;
		case 0x1e:
			GuestExceptMsg[43] = '1';
			GuestExceptMsg[44] = 'e';
			break;
		case 0x1f:
			GuestExceptMsg[43] = '1';
			GuestExceptMsg[44] = 'f';
			break;
		case 0x20:
			GuestExceptMsg[43] = '1';
			GuestExceptMsg[44] = 'f';
			break;
		case 0x21:
			GuestExceptMsg[43] = '2';
			GuestExceptMsg[44] = '1';
			break;
		default:
			GuestExceptMsg[43] = 'X';
			GuestExceptMsg[44] = 'X';
			GuestExceptMsg[45] = 'X';
			GuestExceptMsg[46] = 'X';
			GuestExceptMsg[47] = 'X';
	}
	Print(GuestExceptMsg);
GuestExceptionLoop:	
	goto GuestExceptionLoop;	
}

void HandleGuestPagingException(DWORD Addr)
{
return;
	DWORD *GuestTable = (DWORD *)(OFFSET_TO_GUEST_PHYSICAL_PAGES + GetVMCBD(VMCB_SAVE_STATE_CR3) + 0x1000);  //this originally used GUEST_CR3_VALUE
	DWORD FaultAddr = Addr; //GetVMCBD(VMCB_CONTROL_EXITINFO2);
	DWORD *GuestDir = (DWORD *)(OFFSET_TO_GUEST_PHYSICAL_PAGES + GetVMCBD(VMCB_SAVE_STATE_CR3));
	DWORD TableEntryNum = (FaultAddr >> 0xC) & 0x3FF;
	DWORD DirNum = 0; //fake it out for now ... FaultAddr / 0x400000; HACK!
	
__asm__("	NOP\n"
		"	mov ax,0x0010\n"
		"	push ax\n"
		"	pop ds\n"
		"	push ax\n"
		"	pop es\n");

	GuestTable[TableEntryNum] = ((FaultAddr & 0xFFFFF000)) | 0x007;
	if (GuestDir[DirNum] == 0x0)
		GuestDir[0] = (OFFSET_TO_GUEST_PHYSICAL_PAGES + GetVMCBD(VMCB_SAVE_STATE_CR3) + (0x1000 * (DirNum+1))) | 7;
	goto HandlePagingException_EXIT;
HandlePagingException_EXIT:
	__asm__("	nop\n"
			"	mov eax,0x38\n"
			"	push eax\n"
			"	push eax\n"
			"	pop es\n"
			"	pop ds\n");
}


//This routine will only handle real mode interrupts and protected mode interrupts w/ a vector > 0x1F, since pmode < 0x1F is exceptions, not interrupts
void guest_real_mode_isr_handler()
{
	
}
