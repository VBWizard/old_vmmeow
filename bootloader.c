//
#include "/home/yogi/NetBeansProjects/BMVisor/include/cross.h"
#define ASM_START #asm
#define ASM_END #endasm
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
ASM_START
JMP _main
USE16 386
ASM_END

unsigned short SectorsPerTrack = 0;
unsigned short NumHeads = 0;
int StartMainAdd = 0;
char WelcomeMsg[] = "Welcome to VBWizard's Hypervisor!\n\r\0";
char EndMsg[] = "Initialization complete, booting ...\n\r\0";

void main ()
{
	ASM_START
		PUSH #0X1000
		POP SS
		mov sp, #0xFFB0

		mov ax,#0x1000 //clr 09/30/2017 - changed from 0 to 0x1000
		push ax
		pop ds
		MOV SI, #_WelcomeMsg
		ADD SI, #0x7c00
		CALL print_string
	
	MOV AH,#0x08
#ifdef __BOOT_FROM_HD__
		MOV DL,#0x80
#else
		MOV DL,#0
#endif
		int 0x13
		MOV _NumHeads, dh
		MOV _SectorsPerTrack, cl	//Bits 0..5 only
	
	ASM_END
	SectorsPerTrack = SectorsPerTrack & 0x3f;
	NumHeads = NumHeads + 1;
	ASM_START
		MOV AX,#00
#ifdef __BOOT_FROM_HD__
		MOV DL,#0x80
#else
		MOV DL,#0
#endif
		INT #0x13
				//Use INT 13h, function 42 to get start.bin by LBA
		PUSH #0x2000	//Set DS so that we can put our dap packet in the right place
		POP DS
		mov ax,#0x0010	//packet size(B)
		mov 0x0,ax
		mov ax,#0x000c	//Sector count(W)
		mov 0x2,ax
		mov ax,#0x0	//transfer buffer offset(W)
		mov 0x4,ax
		mov ax,#0x9000	//transfer buffer segment(W)
		mov 0x6,ax
		mov eax,#START_MODULE_SECTOR	//starting lba(D)
		mov 0x8,eax
		mov eax,#0x0	//upper 16 bits of lba(D)
		mov 0xc,eax
#ifdef __BOOT_FROM_HD__
		MOV DL,#0x80
#else
		MOV DL,#0
#endif
		mov SI,#0x0
		MOV AX,#0x4200
		INT #0x13
				//Get visor
		mov ax,#0x0018	//packet size(B)
		mov 0x0,ax
		mov ax,#0x0070 //Sector count(W)
		mov 0x2,ax
		mov ax,#0x0	//transfer buffer offset(W)
		mov 0x4,ax
		mov ax,#0x5000	//transfer buffer segment(W)
		mov 0x6,ax
		mov eax,#VISOR_MODULE_START
		mov 0x8,eax
		mov eax,#0x0
		mov 0xc,eax
#ifdef __BOOT_FROM_HD__
		MOV DL,#0x80
#else
		MOV DL,#0
#endif
		mov SI,#0x0
		MOV AX,#0x4200
		INT #0x13		

		mov ax,#0x0010	//Size(B + zero(B) (note: we are little endian so we place everything backwards
		mov 0x0,ax
		mov ax,#0x0001	//Sector count(W)
		mov 0x2,ax
		mov ax,#0x7c00//transfer buffer offset(W)
		mov 0x4,ax
		mov ax,#0x6000	//transfer buffer segment(W)
		mov 0x6,ax
		mov eax,#0x1	//starting lba(D) (zero based)
		mov 0x8,eax
		mov eax,#0x0	//upper 16 bits of lba(D)
		mov 0xc,eax
#ifdef __BOOT_FROM_HD__
		MOV DL,#0x80
#else
		MOV DL,#0
#endif
		mov SI,#0x0
		MOV AX,#0x4200
		INT #0x13		
		PUSH WORD #0
		POP DS
		MOV SI, #_EndMsg
		ADD SI, #0x7c00
		CALL print_string

		PUSH WORD #0X9000
		PUSH WORD #0X9000
		POP DS
		POP ES


EXECUTE_START_PGM:
		JMP #0x9000:#0x0

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
    
/* ORG 0x1FE
 db 0x55
 db 0xAA
*/
ASM_END
}

