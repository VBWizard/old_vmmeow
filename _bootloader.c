typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
 #asm
JMP _main
USE16 386
 #endasm
unsigned short SectorsPerTrack = 0;
unsigned short NumHeads = 0;
int StartMainAdd = 0;
char WelcomeMsg[] = "Welcome to VBWizard's Hypervisor!\n\r\0";
char EndMsg[] = "Initialization complete, booting ...\n\r\0";
void main ()
{
 #asm
  PUSH #0X1000
  POP SS
  mov sp, #0xFFB0
  mov ax,#0x1000
  push ax
  pop ds
  MOV SI, #_WelcomeMsg
  ADD SI, #0x7c00
  CALL print_string
 MOV AH,#0x08
  MOV DL,#0x80
  int 0x13
  MOV _NumHeads, dh
  MOV _SectorsPerTrack, cl
 #endasm
 SectorsPerTrack = SectorsPerTrack & 0x3f;
 NumHeads = NumHeads + 1;
 #asm
  MOV AX,#00
  MOV DL,#0x80
  INT #0x13
  PUSH #0x2000
  POP DS
  mov ax,#0x0010
  mov 0x0,ax
  mov ax,#0x000c
  mov 0x2,ax
  mov ax,#0x0
  mov 0x4,ax
  mov ax,#0x9000
  mov 0x6,ax
  mov eax,#0x3
  mov 0x8,eax
  mov eax,#0x0
  mov 0xc,eax
  MOV DL,#0x80
  mov SI,#0x0
  MOV AX,#0x4200
  INT #0x13
  mov ax,#0x0018
  mov 0x0,ax
  mov ax,#0x0070
  mov 0x2,ax
  mov ax,#0x0
  mov 0x4,ax
  mov ax,#0x5000
  mov 0x6,ax
  mov eax,#0xf
  mov 0x8,eax
  mov eax,#0x0
  mov 0xc,eax
  MOV DL,#0x80
  mov SI,#0x0
  MOV AX,#0x4200
  INT #0x13
  mov ax,#0x0010
  mov 0x0,ax
  mov ax,#0x0001
  mov 0x2,ax
  mov ax,#0x7c00
  mov 0x4,ax
  mov ax,#0x6000
  mov 0x6,ax
  mov eax,#0x1
  mov 0x8,eax
  mov eax,#0x0
  mov 0xc,eax
  MOV DL,#0x80
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
   lodsb ; grab a byte from SI
   or al, al ; logical or AL by itself
   jz .done ; if the result is zero, get out
   mov ah, #0x0E
   mov bh, #0
   mov bl,#0x07
   int #0x10 ; otherwise, print out the character!
   jmp print_string
 .done:
   ret
 #endasm
}
