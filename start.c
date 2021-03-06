#include "start.h"
#include "PM_DEFS.H"
#include "/home/yogi/NetBeansProjects/BMVisor/include/cross.h"

ASM_START
	jmp _main
USE16 386
ASM_END

//Variable declarations
    static char dig[37] = "0123456789abcdefghijklmnopqrstuvwxyz";
DWORD ticks = 0x0;
DWORD gTempDWord = 0;
BYTE gTempByte = 0;
WORD gTempWord = 0, gTempDS = 0;
char *lTempChar;
WORD GDT_LOC = 0x1000;
DESCR_SEG *GDT=0;
WORD *HostPDir=0;
WORD *HostPTable=0;
WORD *GuestPDir=0;
WORD *GuestPTable=0;
WORD *GuestGDT=0;
WORD *VMCB=0;
int	 DataSegment = 0x0;
GDTR gdtr = 0;				/* GDTR */
IDTR idtr = 0;
DESCR_INT* IDT=0x1c00;
DESCR_INT* hostIDT = 0x5000;
DWORD Pre_GDTrLo = 0, Pre_GDTrHi = 0, Pre_IDTrLo, Pre_IDTrHi;
char StartMsg[] = "Starting ...\n\r\0";
char PModeMsg[] = "About to make the leap to protected mode\n\r\0";
char DiskInfo[] = "                                                       \n\r\0";
char Space[] = " - \0";
char *pDiskInfo = &DiskInfo;
char *pSpace = &Space; 
//method forward declarations
void RelocateDosMBR();
void SetupGDTs();
void SetupPaging();
void SetupVMCB();
void EnableProtectedMode();
void setupGuestIDT();
void Initialize();
char* Itoa(value, str, radix);
WORD Sects = 0, Heads = 0;
DWORD Cyls = 0;
WORD *pSects = (WORD *)C_SPT_ADDR;
WORD *pHeads = (WORD *)C_HEADS_ADDR;
DWORD *pCyls = (WORD *)C_CYLS_ADDR;
void main ()
{
	ASM_START
		MOV AX, #MODULE_START_BEGIN_ADDRESS
		PUSH AX
		POP DS
	ASM_END

	ASM_START
		CLI
		MOV AH,#0x08
		MOV DL, #0x80
		INT #0x13
		mov _Heads,DH
		mov _Sects, CL
		and _Sects, #0x3F
		mov _Cyls, CH
		shr CL,#0x6
		mov _Cyls+1,CL
	ASM_END
	*pSects = Sects;
	*pHeads = Heads;
	*pCyls = Cyls;
	pDiskInfo = Itoa((int)Heads, pDiskInfo, 16);
	ASM_START 
		mov si,#_DiskInfo
		CALL print_string
		mov si,#_Space
		CALL print_string
	ASM_END
	pDiskInfo = Itoa((int)Sects, pDiskInfo, 16);
	ASM_START 
		mov si,#_DiskInfo
		CALL print_string
		mov si,#_Space
		CALL print_string
	ASM_END
	pDiskInfo = Itoa((int)Cyls, pDiskInfo, 16);
	ASM_START 
		mov si,#_DiskInfo
		CALL print_string
		mov si,#_Space
		CALL print_string
		mov si,#_StartMsg
		CALL print_string
		STI
	ASM_END
//Break1:
//	goto Break1;
	ASM_START
	CALL getMemMap
	ASM_END
	Initialize();
	RelocateDosMBR();
	ASM_START
		PUSH word	#0x1000
		PUSH word   #0x1000
		POP		DS
		POP		ES
	ASM_END
  /* setup GDT */
	SetupGDTs();
	//setupGuestIDT();
	//SetupPaging();
	ASM_START
		PUSH #0x2000
		POP DS
		PUSH #0X2000
		POP ES
	ASM_END
	SetupVMCB();
	ASM_START
	MOV AX, #MODULE_START_BEGIN_ADDRESS
	PUSH AX
	POP DS
	PUSH AX
	POP ES
	ASM_END
	EnableProtectedMode();
	

ASM_START
.ENDLESS_LOOP:
	jmp .ENDLESS_LOOP
ASM_END
}

void RelocateDosMBR()
{
		ASM_START
		push	ds
		mov		si, #0x0
		mov		di, #0x7c00
		mov		cx, #0x200
		cld
		push word	#0x0000
		push word	#0x6000
		pop		ds
		pop		es
		REP 
		MOVSB
		mov		si, #0x0
		pop		ds
	ASM_END
}


void outportb (port, value) WORD port; BYTE value;
{
	gTempWord = port;
	gTempByte = value;
ASM_START
		mov al,_gTempByte
		mov dx, _gTempWord
		out dx,al
ASM_END	
}

BYTE inportb (port) WORD port;
{
	gTempWord = port;

ASM_START
	mov dx,_gTempWord
	in al,dx
	mov _gTempByte, al
ASM_END
	return gTempByte;
}

void setup_PIC (master_vector,slave_vector) BYTE master_vector; BYTE slave_vector;
{
  outportb (PORT_8259M, 0x11);                  /* start 8259 initialization */
  outportb (PORT_8259S, 0x11);
  outportb (PORT_8259M+1, master_vector);       /* master base interrupt vector */
  outportb (PORT_8259S+1, slave_vector);        /* slave base interrupt vector */
  outportb (PORT_8259M+1, 1<<2);                /* bitmask for cascade on IRQ2 */
  outportb (PORT_8259S+1, 2);                   /* cascade on IRQ2 */
  outportb (PORT_8259M+1, 1);                   /* finish 8259 initialization */
  outportb (PORT_8259S+1, 1);
}

void set_efer_svme_bit()
{
	ASM_START
		mov ecx,#0xC0000080
		RDMSR
		OR EAX,#0x1000 //(I want to write 0x1000 but it won't let me)
		WRMSR
		RDMSR
	ASM_END
}

DWORD read_cr3()
{
	ASM_START
		mov     eax, cr3        ; read CR0 to eax
		mov		_gTempDWord, eax
	ASM_END
}

DWORD read_cr0()
{
	ASM_START
		mov     eax, cr0        ; read CR0 to eax
		mov		_gTempDWord, eax
	ASM_END
	return gTempDWord;
}

void write_cr0 (value) DWORD value;
{
	gTempWord = value;
	
	ASM_START
		mov     eax, _gTempDWord;
		mov     cr0, eax
	ASM_END
}

void write_cr3 (value) DWORD value;
{
	gTempDWord = value;
	
	ASM_START
		mov     eax, _gTempDWord;
		mov     cr3, eax
	ASM_END
}

void update_cs(value) WORD value;
{
	gTempWord = value;
	ASM_START
        push	ax
        mov     ax, _gTempWord ; ax = new cs
        push    ax              ; push segment
        push    word #.ABC         ; push offset
        retf                    ; we have a new cs now
.ABC:
		pop ax
	ASM_END
}

void set_ds ( value ) WORD value;
{
	gTempWord = value;
	ASM_START
		mov ax,_gTempWord
		push ax
		pop ds
	ASM_END
}
void set_es ( value ) WORD value;
{
	gTempWord = value;
	ASM_START
		mov ax,_gTempWord
		push ax
		pop es
	ASM_END
}
void set_ss ( value ) WORD value;
{
	gTempWord = value;
	ASM_START
		mov ax,_gTempWord
		push ax
		pop ss
	ASM_END
}
void Initialize()
{
/*	GDT = 			0x0000;
	HostPDir = 		0x1000;
	HostPTable = 	0x2000;
	NestedPDir = 	0xA000;
	NestedTable = 	0xB000;
	GuestGDT = 		0xF000;
	VMCB = 			0x2000;
	DataSegment = 	0x1000;
*/	ASM_START
		out	#0x21, al
		cli
	ASM_END
	//Initialize the GDT for our protected mode switch
	disable();
	/* disable NMIs as well */
  outportb (0x70, inportb(0x70) | 0x80);
}

void setup_GDT32_entry (item, base, limit, access, attribs) DESCR_SEG* item; DWORD base; DWORD limit; BYTE access; BYTE attribs;
{
  item->base_l = (WORD)(base & 0xFFFF);
  item->base_m = (BYTE)((base >> 16) & 0xFF);
  item->base_h = (BYTE)(base >> 24);
  item->limit = (WORD)limit & 0xFFFF;
  item->attribs = attribs | (BYTE)((limit >> 16) & 0x0F);
  item->access = access;
}

void SetupGDTs()
{
	//TODO: Make 0x6000 below HOST_GDT
	ASM_START
		mov ax, #0x6000
		push ax
		pop ds
	ASM_END

	//Initialize Host GDT
	DESCR_SEG *item = HOST_GDT;
	DESCR_SEG_16 *item16 = 0xF000;

	/* 0x00 -- null descriptor */
	setup_GDT32_entry (item, 0, 0, 0, 0);
	/* 0x08 -- VISOR code segment descriptor */
	setup_GDT32_entry (item+1, 0, 0, 0, 0);
	/* 0x10 -- Wide open data segment descriptor */
	setup_GDT32_entry(item+2, (DWORD)0x0, 0xFFFFFFFF, ACS_DATA, ATTR_GRANULARITY | ATTR_DEFAULT);
	/* 0x18 -- stack segment descriptor */
	setup_GDT32_entry(item+3, (DWORD)0x10000, 0xFFFFFFFF, ACS_STACK, ATTR_GRANULARITY | ATTR_DEFAULT);
	/* 0x20 -- Code segment after pmode switch */
	setup_GDT32_entry(item+4, (DWORD)MODULE_START_BEGIN_ADDRESS * 0x10, 0xFFFFFFFF, ACS_CODE, ATTR_GRANULARITY);
	/* 0x28 -- Guest code segment */
	setup_GDT32_entry (item+5, 0, 0, 0, 0);
	/* 0x30 -- Guest stack segment */
	setup_GDT32_entry (item+6, 0, 0, 0, 0);
	/* 0x38 -- VISOR data segment */
	setup_GDT32_entry(item+7, (DWORD)0x800000, 0xFFFFFFFF, ACS_DATA, ATTR_GRANULARITY | ATTR_DEFAULT);
	/* 0x40 -- Data segment after pmode switch*/
	setup_GDT32_entry(item+8, (DWORD)MODULE_START_BEGIN_ADDRESS * 0x10, 0xFFFFFFFF, ACS_DATA, ATTR_GRANULARITY | ATTR_DEFAULT);
	/* 0X48 -- Video Segment */
	setup_GDT32_entry(item+9, (DWORD)0xB8000, 0xFFFFFFFF, 0x93, 0xC0);
	/* 0x50 -- Temp VISOR code segment for debugging*/
	setup_GDT32_entry(item+10, (DWORD)VISOR_FINAL_ADDRESS, 0xFFFFFFFF, ACS_CODE, ATTR_GRANULARITY | ATTR_DEFAULT);
	/* 0x58 -- TSS - defined in visor.c*/
	//N/A
	ASM_START
		mov ax,#MODULE_START_BEGIN_ADDRESS
		push ax
		pop ds
	ASM_END
}

void setup_IDT_entry (item, selector, offset, access, param_cnt) DESCR_INT *item; WORD selector; DWORD offset; BYTE access; BYTE param_cnt; 
{
	DWORD Bottom = 0, Top = 0;
	WORD *ptr = (WORD *)item;

  ptr[0] = (WORD)offset;
  ptr[1] = selector;
  ptr[2] = access << 8 + 0;
  ptr[3] = (WORD)(offset >> 16);

}
void SetupPaging()
{
	DWORD *ptr = 0;
	DWORD *ptrT = 0;
	DWORD cnt, cnt2;
	
	ASM_START
		mov ax,#0x7000
		push ax
		pop ds
	ASM_END

	ptr = (HOST_CR3_VALUE & 0x0000FFFF);
	ptrT = ((HOST_CR3_VALUE & 0x0000FFFF) + 0x1000);
	//Initialize Host Page Directory @ 0x11000
	for (cnt=0;cnt <= (HOST_MEMORY_SIZE / 0x400000); cnt++)	
	{
		ptr[cnt] = (HOST_CR3_VALUE + 4096 + (cnt*4096)) | 7;
		//Create a guest page table for each Page Directory
		for (cnt2=0;cnt2<1024;cnt2++)
			ptrT[cnt2] = ((0x400000 * cnt) + ((cnt2) << 12)) | 7;
		ptrT += 0x400;
	}

	ASM_START
		mov ax,#0x1000
		push ax
		pop ds
	ASM_END

	ptr = NESTED_PAGING_DIRECTORY_ADDRESS; 
	ptrT = NESTED_PAGING_TABLE_ADDRESS;
	//Initialize Nested Paging table
	for (cnt=0;cnt<=(GUEST_MEMORY_SIZE / 0x400000); cnt++) //TODO: Should use GUEST_MEMORY_SIZE
	{
		ptr[cnt] = (NESTED_PAGING_DIRECTORY_ADDRESS + 4096 + (cnt*4096)) | 7;
		//Create a guest page table for each Page Directory
		for (cnt2 = 0;cnt2 < 1024;cnt2++) 
		{
			{
					ptrT[cnt2] = ((0x400000 * cnt) + OFFSET_TO_GUEST_PHYSICAL_PAGES + (cnt2 << 12)) | 7;
			}
		}	
		ptrT += 0x400;
	}
	ASM_START
		mov ax,#MODULE_START_BEGIN_ADDRESS
		push ax
		pop ds
	ASM_END
}

void SetupVMCB()
{
	BYTE *VMCB = 0x0000;
	DWORD lGDTLo = 0, lGDTHi = 0, lIDTLo = 0, lIDTHi = 0;
	DWORD lGUEST_CR3_VALUE = 0;

	ASM_START
		mov	ax, ds
		push ax
		mov ax, #MODULE_START_BEGIN_ADDRESS
		push ax
		pop ds
		sgdt _Pre_GDTrLo
		sidt _Pre_IDTrLo
	ASM_END
	lGDTLo = Pre_GDTrLo;
	lGDTHi = Pre_GDTrHi;
	lIDTLo = Pre_IDTrLo;
	lIDTHi = Pre_IDTrHi;
	ASM_START
		pop ds
	ASM_END

		//Initialize the VMCB
	VMCB[VMCB_CONTROL_INTERCEPT_CR_WRITE] = 0x0; //1=CR0 					//Intercept writes to CRs
	VMCB[VMCB_CONTROL_INTERCEPT_CR_READ] = 0x0; //0x9; 					//Intercept reads to CRs
	VMCB[VMCB_CONTROL_INTERCEPT_BYTE_08] = 0x00; //0xFF;					//Intercept exceptions 0-7
	VMCB[VMCB_CONTROL_INTERCEPT_BYTE_08+1] = 0x20; //0xFF; 					//Intercept exceptions 8-15 - DO NOT SET BIT 6 (#PF) HERE, it will be conditionally set below
	VMCB[VMCB_CONTROL_INTERCEPT_BYTE_08+2] = 0x00; //0xFF; 					//Intercept exceptions 16-23
	VMCB[VMCB_CONTROL_INTERCEPT_BYTE_08+3] = 0x00; //0xFF; 					//Intercept exceptions 24-31
	VMCB[VMCB_CONTROL_INTERCEPT_BYTE_0C]= 0x0; 						//0x01 = Intercept INTR interrupt (not INTn), 0x02 = Intercept NMI
	VMCB[VMCB_CONTROL_INTERCEPT_BYTE_0C + 2] |= 0x20;	  				//Intercept INTn instructions = 0x20
	VMCB[VMCB_CONTROL_INTERCEPT_BYTE_0C + 2] |= 0x00;					//Intercept IRET instructions = 0x10
	VMCB[VMCB_CONTROL_INTERCEPT_BYTE_0C + 3] |= 0x01;					//Intercept HLT instruction
	VMCB[VMCB_CONTROL_INTERCEPT_BYTE_0C + 3] |= 0x0; //0x08; 				//was 8 which is for IO intercept
	VMCB[VMCB_CONTROL_INTERCEPT_BYTE_0C + 3] = 0x81;					//Intercept IOIO = 0x8 ... HLT = 1 ... shutdown = 0x80
	VMCB[VMCB_CONTROL_INTERCEPT_BYTE_10] = 0xff; //0x1;  					//has to be at least 1 (intercept VMRUN)
	VMCB[VMCB_CONTROL_TLB_CONTROL] = 1;							//Guest ASID
	VMCB[VMCB_CONTROL_TLB_CONTROL+4] = 1;


#ifdef USE_NESTED_PAGING
	VMCB[VMCB_CONTROL_INTERCEPT_BYTE_08+1] |= 0x20; //&= 0xBF; 					//don't want paging intercept set for nested mode
	VMCB[VMCB_CONTROL_NESTED_PAGING] = 1;							//enable nested paging (NOTE: Implies no paging intercept)
	VMCB[VMCB_CONTROL_NESTED_CR3] = (BYTE)(NESTED_PAGING_DIR_FINAL_ADDRESS & 0xFF);		//address of nested paging table (CR3 value)
	VMCB[VMCB_CONTROL_NESTED_CR3+1] = (BYTE)((NESTED_PAGING_DIR_FINAL_ADDRESS >> 8) & 0xFF);
	VMCB[VMCB_CONTROL_NESTED_CR3+2] = (BYTE)((NESTED_PAGING_DIR_FINAL_ADDRESS >> 16) & 0xFF);
	VMCB[VMCB_CONTROL_NESTED_CR3+3] = (BYTE)((NESTED_PAGING_DIR_FINAL_ADDRESS >> 24) & 0xFF);
#else
	VMCB[VMCB_CONTROL_NESTED_PAGING] = 0;							//disable nested paging (NOTE: Implies paging intercept)
	VMCB[VMCB_CONTROL_INTERCEPT_BYTE_08+1] |= 0x40; 					//set paging intercept
#endif
	
	VMCB[VMCB_SAVE_STATE_CS] = 0x00;
	VMCB[VMCB_SAVE_STATE_CS+1] = 0x00;
	VMCB[VMCB_SAVE_STATE_CS + 2] = 0X93;
	VMCB[VMCB_SAVE_STATE_CS + 4] = 0xFF;
	VMCB[VMCB_SAVE_STATE_CS + 5] = 0xFF;
	VMCB[VMCB_SAVE_STATE_CS + 6] = 0xFF;
	VMCB[VMCB_SAVE_STATE_CS + 7] = 0xFF; //0f00FFFF
	VMCB[VMCB_SAVE_STATE_CS + 8] = 0x00;
	VMCB[VMCB_SAVE_STATE_CS + 9] = 0x00;
	VMCB[VMCB_SAVE_STATE_CS + 10] = 0x00;
	VMCB[VMCB_SAVE_STATE_CS + 11] = 0x00;
	
	VMCB[VMCB_SAVE_STATE_ES] = 0x00;
	VMCB[VMCB_SAVE_STATE_ES+1] = 0x00;
	VMCB[VMCB_SAVE_STATE_ES + 2] = 0X93;
	VMCB[VMCB_SAVE_STATE_ES + 4] = 0xFF;
	VMCB[VMCB_SAVE_STATE_ES + 5] = 0xFF;
	VMCB[VMCB_SAVE_STATE_ES + 6] = 0xFF;
	VMCB[VMCB_SAVE_STATE_ES + 7] = 0xFF;
	VMCB[VMCB_SAVE_STATE_ES + 8] = 0x00;
	VMCB[VMCB_SAVE_STATE_ES + 9] = 0x00;
	VMCB[VMCB_SAVE_STATE_ES + 10] = 0x00;
	VMCB[VMCB_SAVE_STATE_ES + 11] = 0x00;

	VMCB[VMCB_SAVE_STATE_DS] = 0x00;
	VMCB[VMCB_SAVE_STATE_DS+1] = 0x00;
	VMCB[VMCB_SAVE_STATE_DS + 2] = 0X93;
	VMCB[VMCB_SAVE_STATE_DS + 4] = 0xFF;
	VMCB[VMCB_SAVE_STATE_DS + 5] = 0xFF;
	VMCB[VMCB_SAVE_STATE_DS + 6] = 0xFF;
	VMCB[VMCB_SAVE_STATE_DS + 7] = 0xFF;
	VMCB[VMCB_SAVE_STATE_DS + 8] = 0x00;
	VMCB[VMCB_SAVE_STATE_DS + 9] = 0x00;
	VMCB[VMCB_SAVE_STATE_DS + 10] = 0x00;
	VMCB[VMCB_SAVE_STATE_DS + 11] = 0x00;

	VMCB[VMCB_SAVE_STATE_SS] = 0x00;
	VMCB[VMCB_SAVE_STATE_SS+1] = 0x00;
	VMCB[VMCB_SAVE_STATE_SS + 2] = 0X93;
	VMCB[VMCB_SAVE_STATE_SS + 4] = 0xFF;
	VMCB[VMCB_SAVE_STATE_SS + 5] = 0xFF;
	VMCB[VMCB_SAVE_STATE_SS + 6] = 0xFf;
	VMCB[VMCB_SAVE_STATE_SS + 7] = 0xFF;
	VMCB[VMCB_SAVE_STATE_SS + 8] = 0x00;
	VMCB[VMCB_SAVE_STATE_SS + 9] = 0x00;
	VMCB[VMCB_SAVE_STATE_SS + 10] = 0x00;
	VMCB[VMCB_SAVE_STATE_SS + 11] = 0x00;


	VMCB[VMCB_SAVE_STATE_FS] = 0x00;
	VMCB[VMCB_SAVE_STATE_FS+1] = 0x00;
	VMCB[VMCB_SAVE_STATE_FS + 2] = 0X93;
	VMCB[VMCB_SAVE_STATE_FS + 4] = 0xFF;
	VMCB[VMCB_SAVE_STATE_FS + 5] = 0xFF;
	VMCB[VMCB_SAVE_STATE_FS + 6] = 0xFF;
	VMCB[VMCB_SAVE_STATE_FS + 7] = 0xFF;
	VMCB[VMCB_SAVE_STATE_FS + 8] = 0x00;
	VMCB[VMCB_SAVE_STATE_FS + 9] = 0x00;
	VMCB[VMCB_SAVE_STATE_FS + 10] = 0x00;
	VMCB[VMCB_SAVE_STATE_FS + 11] = 0x00;

	VMCB[VMCB_SAVE_STATE_GS] = 0x00;
	VMCB[VMCB_SAVE_STATE_GS+1] = 0x00;
	VMCB[VMCB_SAVE_STATE_GS + 2] = 0X93;
	VMCB[VMCB_SAVE_STATE_GS + 4] = 0xFF;
	VMCB[VMCB_SAVE_STATE_GS + 5] = 0xFF;
	VMCB[VMCB_SAVE_STATE_GS + 6] = 0xFF;
	VMCB[VMCB_SAVE_STATE_GS + 7] = 0xFF;
	VMCB[VMCB_SAVE_STATE_GS + 8] = 0x00;
	VMCB[VMCB_SAVE_STATE_GS + 9] = 0x00;
	VMCB[VMCB_SAVE_STATE_GS + 10] = 0x00;
	VMCB[VMCB_SAVE_STATE_GS + 11] = 0x00;
	
	VMCB[VMCB_SAVE_STATE_GDTR] = 0x00;								//First 4 bytes of GDTR are reserved
	VMCB[VMCB_SAVE_STATE_GDTR+1] = 0x00;
	VMCB[VMCB_SAVE_STATE_GDTR + 2] = 0x00;
	VMCB[VMCB_SAVE_STATE_GDTR + 3] = 0x00;
/*l*/	VMCB[VMCB_SAVE_STATE_GDTR + 4] = (BYTE)(lGDTLo);			//LIMIT
/*l*/	VMCB[VMCB_SAVE_STATE_GDTR + 5] = (BYTE)((lGDTLo >> 8) & 0xFF);
/*l*/	VMCB[VMCB_SAVE_STATE_GDTR + 6] = 0x00;						//Upper 2 bytes of limit are reserved
/*l*/	VMCB[VMCB_SAVE_STATE_GDTR + 7] = 0x00;
/*b*/	VMCB[VMCB_SAVE_STATE_GDTR + 8] = (BYTE)((lGDTLo >> 16) & 0xFF); //BASE
/*b*/	VMCB[VMCB_SAVE_STATE_GDTR + 9] = (BYTE)((lGDTLo >> 24) & 0xFF);
/*b*/	VMCB[VMCB_SAVE_STATE_GDTR + 10] = (BYTE)(lGDTHi & 0xFF);
/*b*/	VMCB[VMCB_SAVE_STATE_GDTR + 11] = (BYTE)((lGDTHi >> 8) & 0xFF);

	VMCB[VMCB_SAVE_STATE_IDTR] = 0x00;
	VMCB[VMCB_SAVE_STATE_IDTR+1] = 0x00;
	VMCB[VMCB_SAVE_STATE_IDTR + 2] = 0x00;
	VMCB[VMCB_SAVE_STATE_IDTR + 4] = 0xFF;
	VMCB[VMCB_SAVE_STATE_IDTR + 5] = 0x03;
	VMCB[VMCB_SAVE_STATE_IDTR + 6] = 0x00;
	VMCB[VMCB_SAVE_STATE_IDTR + 7] = 0x00;
	VMCB[VMCB_SAVE_STATE_IDTR + 8] = 0x00;
	VMCB[VMCB_SAVE_STATE_IDTR + 9] = 0x00;
	VMCB[VMCB_SAVE_STATE_IDTR + 10] = 0x00;
	VMCB[VMCB_SAVE_STATE_IDTR + 11] = 0x00;

	VMCB[VMCB_SAVE_STATE_CPL] = 	0x00;	//real mode forced to 0, virtual mode forced to 3
	VMCB[VMCB_SAVE_STATE_EFER] = 	0x00;
	VMCB[VMCB_SAVE_STATE_EFER+1] = 0x10; //this was 0x10 but guest SVME should not be on, HOST should be
	VMCB[VMCB_SAVE_STATE_CR0] = 	0x10;
	VMCB[VMCB_SAVE_STATE_CR0 + 1] = 0x00;
	VMCB[VMCB_SAVE_STATE_CR0 + 2] = 0x00;
	VMCB[VMCB_SAVE_STATE_CR0 + 3] = 0x00;
	VMCB[VMCB_SAVE_STATE_CR2] = 	0x00;
	VMCB[VMCB_SAVE_STATE_CR2 + 1] = 0x00;
	VMCB[VMCB_SAVE_STATE_CR2 + 2] = 0x00;
	VMCB[VMCB_SAVE_STATE_CR2 + 3] = 0x00;
	lGUEST_CR3_VALUE = GUEST_CR3_VALUE + OFFSET_TO_GUEST_PHYSICAL_PAGES;
	VMCB[VMCB_SAVE_STATE_CR3] = (BYTE)(lGUEST_CR3_VALUE & 0xFF);
	VMCB[VMCB_SAVE_STATE_CR3+1] = (BYTE)((lGUEST_CR3_VALUE >> 8) & 0xFF);	
	VMCB[VMCB_SAVE_STATE_CR3+2] = (BYTE)((lGUEST_CR3_VALUE >> 16) & 0xFF);
	VMCB[VMCB_SAVE_STATE_CR3+3] = (BYTE)((lGUEST_CR3_VALUE >> 24) & 0xFF);
	VMCB[VMCB_SAVE_STATE_CR4] = 	0x00;
	VMCB[VMCB_SAVE_STATE_CR4 + 1] = 0x00;
	VMCB[VMCB_SAVE_STATE_CR4 + 2] = 0x00;
	VMCB[VMCB_SAVE_STATE_CR4 + 3] = 0x00;
	VMCB[VMCB_SAVE_STATE_RFLAGS] = 0x82;
	VMCB[VMCB_SAVE_STATE_RFLAGS+1] = 0x00;
	VMCB[VMCB_SAVE_STATE_RFLAGS+3] = 0x00;
	VMCB[VMCB_SAVE_STATE_RFLAGS+4] = 0x00;
	VMCB[VMCB_SAVE_STATE_RSP] = 0xd6;
	VMCB[VMCB_SAVE_STATE_RSP + 1] = 0xFF;
	VMCB[VMCB_SAVE_STATE_RSP + 2] = 0x00;
	VMCB[VMCB_SAVE_STATE_RSP + 3] = 0x00;
	VMCB[VMCB_SAVE_STATE_RIP] = 0x00;
	VMCB[VMCB_SAVE_STATE_RIP + 1] = 0x7C;
	VMCB[VMCB_SAVE_STATE_RIP + 2] = 0x00;
	VMCB[VMCB_SAVE_STATE_RIP + 3] = 0x00;
	VMCB[VMCB_SAVE_STATE_RAX] = 0x55;
	VMCB[VMCB_SAVE_STATE_RAX+1] = 0xaa;
}

void MoveVisor()
{
	//TODO: Make this use VISOR_FINAL_ADDRESS
		ASM_START
		push	ds
		push    es
		mov		esi, #0x50000
		mov		edi, #VISOR_FINAL_ADDRESS
		mov		ecx, #0x50000
		cld
		push word	#0x10
		push word	#0x10
		pop		ds
		pop		es
		db 0x66
		db 0x67
		REP 
		MOVSB
		mov		si, #0x0
		pop		es
		pop		ds
		ASM_END
}

void EnableProtectedMode()
{
   /* setting up the IDTR register */
  idtr.base = ((DWORD)HOST_IDT);
  idtr.base += 0;
  idtr.limit = (0x21*8)-1;

 /* setting up the GDTR register */
  gdtr.base = ((DWORD)HOST_GDT);
  gdtr.base += 0;
  gdtr.limit = (21*8)-1;
  gTempWord = &gdtr;
ASM_START
		mov ebx, #0
		mov	bx, _gTempWord
		lgdt [ebx]
ASM_END

ASM_START
	mov si, #_PModeMsg
	call print_string
ASM_END

  /* setup PIC */
  setup_PIC (0x08, 0x70);

  /* set new IRQ masks */
  outportb (PORT_8259M+1, 0xB8);       /* enable timer ans keyboard (master) */
  outportb (PORT_8259S+1, 0x8F);       /* disable all (slave) */
  /* disable NMIs as well */
 //outportb (0x70, inportb(0x70) | 0x80);

	//outportb (0x21,inportb(0x21) & 0xFD);

  /* WOW!!! This switches us to PMode just setting up CR0.PM bit to 1 */
  gTempDWord = read_cr0() | 1;
  write_cr0 (gTempDWord);
  /* loading segment registers with PMode selectors */
	update_cs (0x20);
	set_ds ( 0x40 );
	set_es ( 0x10 );
	set_ss ( 0x18 );
	set_efer_svme_bit();

	//gTempDWord = HOST_CR3_VALUE;
	//write_cr3(gTempDWord);
	//gTempDWord = read_cr0() | 0x80000000;
	//write_cr0(gTempDWord);

	MoveVisor();

  outportb (PORT_8259M+1, 0x0);       /* enable everything - was 00*/ 
  outportb (PORT_8259S+1, 0x0);       /* enable everything (slave) - was 00*/
ASM_START
	mov ax,#0x10
	push ds
	push ax
	pop ds
	mov eax, 0x50000
	cmp eax, #0x4c8d00eb		/*clr 09/24/2017 - changed from #0x000000E9*/
	je  OverError
	mov cx,#0x100
	mov eax,#0x07630764
	mov edi, #0xb8000
	db 0x66
	db 0x67
	rep 
	stosw 
CLI
HLT
OverError:
	pop ds
	JMP #0x50:0x0
ASM_END
}
    /* The Itoa code is in the public domain */
char* Itoa(value, str, radix) int value; char* str; int radix;
{
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
  // Bit32u (unsigned long) and long helper functions
  ASM_START

print_string:
   lodsb        ; grab a byte from SI
 
   or al, al  ; logical or AL by itself
   jz .done   ; if the result is zero, get out
 
   mov ah, #0x0E
   mov bh, #0
   mov bl,#0x07
   int #0x10      ; otherwise, print out the character!
 
   jmp print_string
 
 .done:
   ret
   
  ;; and function
  landl:
  landul:
    SEG SS
      and ax,[di]
    SEG SS
      and bx,2[di]
    ret

  ;; add function
  laddl:
  laddul:
    SEG SS
      add ax,[di]
    SEG SS
      adc bx,2[di]
    ret

  ;; cmp function
  lcmpl:
  lcmpul:
    and eax, #0x0000FFFF
    shl ebx, #16
    or  eax, ebx
    shr ebx, #16
    SEG SS
      cmp eax, dword ptr [di]
    ret

  ;; sub function
  lsubl:
  lsubul:
    SEG SS
    sub ax,[di]
    SEG SS
    sbb bx,2[di]
    ret

  ;; mul function
  lmull:
  lmulul:
    and eax, #0x0000FFFF
    shl ebx, #16
    or  eax, ebx
    SEG SS
    mul eax, dword ptr [di]
    mov ebx, eax
    shr ebx, #16
    ret

  ;; dec function
  ldecl:
  ldecul:
    SEG SS
    dec dword ptr [bx]
    ret

  ;; or function
  lorl:
  lorul:
    SEG SS
    or  ax,[di]
    SEG SS
    or  bx,2[di]
    ret

  ;; inc function
  lincl:
  lincul:
    SEG SS
    inc dword ptr [bx]
    ret

  ;; tst function
  ltstl:
  ltstul:
    and eax, #0x0000FFFF
    shl ebx, #16
    or  eax, ebx
    shr ebx, #16
    test eax, eax
    ret

  ;; sr function
  lsrul:
    mov  cx,di
    jcxz lsr_exit
    and  eax, #0x0000FFFF
    shl  ebx, #16
    or   eax, ebx
  lsr_loop:
    shr  eax, #1
    loop lsr_loop
    mov  ebx, eax
    shr  ebx, #16
  lsr_exit:
    ret

  ;; sl function
  lsll:
  lslul:
    mov  cx,di
    jcxz lsl_exit
    and  eax, #0x0000FFFF
    shl  ebx, #16
    or   eax, ebx
  lsl_loop:
    shl  eax, #1
    loop lsl_loop
    mov  ebx, eax
    shr  ebx, #16
  lsl_exit:
    ret

  idiv_:
    cwd
    idiv bx
    ret

  idiv_u:
    xor dx,dx
    div bx
    ret

  ldivul:
    and  eax, #0x0000FFFF
    shl  ebx, #16
    or   eax, ebx
    xor  edx, edx
    SEG SS
    mov  bx,  2[di]
    shl  ebx, #16
    SEG SS
    mov  bx,  [di]
    div  ebx
    mov  ebx, eax
    shr  ebx, #16
    ret

    ;; imodu doesnt preserve dx (returns quotient in it)
imodu:
	xor	dx,dx
	div	bx
	mov	ax,dx		
	ret

_lidt:
        push    bp
        mov     bp, sp
        push    bx
		SEG SS
        mov     bx, [bp+4]   ; ds:bx = pointer to IDTR structure
        SEG DS
        lidt    [bx]         ; load IDTR
        pop     bx
        pop     bp
        ret

_lldt:
        push    bp
        mov     bp, sp
        SEG SS
        mov     ax, [bp+4]
        lldt    ax              ; load LDTR
        pop     bp
        ret

_disable:
		cli
		ret;
_enable:
		sti
		ret;

getMemMap:
	mov EBX, #0x0
	mov DI, #0x0
	mov CX, #0x0
getLoop:
	mov eax, #0x0000e820
	add DI,cx		//Most E820 (ACPI < 3.0) is 20 bytes, but ACPI 3.0 is 24 bytes, so leave room for those extra bytes in each record
	mov ECX, #24
	mov EDX, #0x534d4150
	mov SI, #INT15_MEMORY_MAP
	push SI
	pop ES
	int #0x15
	mov cx,#24
	cmp ebx, #0x0
	jne getLoop
	ret
  ASM_END

