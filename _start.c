typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
enum eRegisters
{
    EAX,
            EBX,
            ECX,
            EDX,
            ESI,
            EDI,
            EBP,
            ESP,
            CS,
            DS,
            ES,
            FS,
            GS,
            SS,
            CR0,
            CR4,
            CR8
};
#pragma pack (push, 1)
typedef struct {
  unsigned int limit,
       base_l;
  unsigned char base_m,
       access,
       attribs,
       base_h;
} DESCR_SEG;
typedef struct {
 unsigned int limit,
   base_low;
 unsigned char base_mid,
   flags;
} DESCR_SEG_16;
typedef struct {
  unsigned int limit;
  unsigned long base;
} GDTR;
typedef struct {
  unsigned int offset_l,
       selector;
  unsigned char param_cnt,
       access;
  unsigned int offset_h;
} DESCR_INT;
typedef struct {
  unsigned int limit;
  unsigned long base;
} IDTR;
typedef struct {
  unsigned long link,
        esp0,
        ss0,
        esp1,
        ss1,
        esp2,
        ss2,
        cr3,
        eip,
        eflags,
        eax,
        ecx,
        edx,
        ebx,
        esp,
        ebp,
        esi,
        edi,
        es,
        cs,
        ss,
        ds,
        fs,
        gs,
        ldtr;
  unsigned int trace,
        io_map_addr;
  unsigned char io_map[8192];
} TSS;
typedef struct {
  unsigned long link,
        esp0,
        ss0,
        esp1,
        ss1,
        esp2,
        ss2,
        cr3,
        eip,
        eflags,
        eax,
        ecx,
        edx,
        ebx,
        esp,
        ebp,
        esi,
        edi,
        es,
        cs,
        ss,
        ds,
        fs,
        gs,
        ldtr;
  unsigned int trace,
        io_map_addr;
} _TSS;
#pragma pack (pop)
 #asm
 jmp _main
USE16 386
 #endasm
    static char dig[37] = "0123456789abcdefghijklmnopqrstuvwxyz";
unsigned long ticks = 0x0;
unsigned long gTempDWord = 0;
unsigned char gTempByte = 0;
unsigned int gTempWord = 0, gTempDS = 0;
char *lTempChar;
unsigned int GDT_LOC = 0x1000;
DESCR_SEG *GDT=0;
unsigned int *HostPDir=0;
unsigned int *HostPTable=0;
unsigned int *GuestPDir=0;
unsigned int *GuestPTable=0;
unsigned int *GuestGDT=0;
unsigned int *VMCB=0;
int DataSegment = 0x0;
GDTR gdtr = 0;
IDTR idtr = 0;
DESCR_INT* IDT=0x1c00;
DESCR_INT* hostIDT = 0x5000;
unsigned long Pre_GDTrLo = 0, Pre_GDTrHi = 0, Pre_IDTrLo, Pre_IDTrHi;
char StartMsg[] = "Starting ...\n\r\0";
char PModeMsg[] = "About to make the leap to protected mode\n\r\0";
char DiskInfo[] = "                                                       \n\r\0";
char Space[] = " - \0";
char *pDiskInfo = &DiskInfo;
char *pSpace = &Space;
void RelocateDosMBR();
void SetupGDTs();
void SetupPaging();
void SetupVMCB();
void EnableProtectedMode();
void setupGuestIDT();
void Initialize();
char* Itoa(value, str, radix);
unsigned int Sects = 0, Heads = 0;
unsigned long Cyls = 0;
unsigned int *pSects = (unsigned int *)0xfff0;
unsigned int *pHeads = (unsigned int *)0xfff4;
unsigned long *pCyls = (unsigned int *)0xfff8;
void main ()
{
 #asm
  MOV AX, #0x9000
  PUSH AX
  POP DS
 #endasm
 #asm
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
 #endasm
 *pSects = Sects;
 *pHeads = Heads;
 *pCyls = Cyls;
 pDiskInfo = Itoa((int)Heads, pDiskInfo, 16);
 #asm
  mov si,#_DiskInfo
  CALL print_string
  mov si,#_Space
  CALL print_string
 #endasm
 pDiskInfo = Itoa((int)Sects, pDiskInfo, 16);
 #asm
  mov si,#_DiskInfo
  CALL print_string
  mov si,#_Space
  CALL print_string
 #endasm
 pDiskInfo = Itoa((int)Cyls, pDiskInfo, 16);
 #asm
  mov si,#_DiskInfo
  CALL print_string
  mov si,#_Space
  CALL print_string
  mov si,#_StartMsg
  CALL print_string
  STI
 #endasm
 #asm
 CALL getMemMap
 #endasm
 Initialize();
 RelocateDosMBR();
 #asm
  PUSH word #0x1000
  PUSH word #0x1000
  POP DS
  POP ES
 #endasm
 SetupGDTs();
 #asm
  PUSH #0x2000
  POP DS
  PUSH #0X2000
  POP ES
 #endasm
 SetupVMCB();
 #asm
 MOV AX, #0x9000
 PUSH AX
 POP DS
 PUSH AX
 POP ES
 #endasm
 EnableProtectedMode();
 #asm
.ENDLESS_LOOP:
 jmp .ENDLESS_LOOP
 #endasm
}
void RelocateDosMBR()
{
  #asm
  push ds
  mov si, #0x0
  mov di, #0x7c00
  mov cx, #0x200
  cld
  push word #0x0000
  push word #0x6000
  pop ds
  pop es
  REP
  MOVSB
  mov si, #0x0
  pop ds
 #endasm
}
void outportb (port, value) unsigned int port; unsigned char value;
{
 gTempWord = port;
 gTempByte = value;
 #asm
  mov al,_gTempByte
  mov dx, _gTempWord
  out dx,al
 #endasm
}
unsigned char inportb (port) unsigned int port;
{
 gTempWord = port;
 #asm
 mov dx,_gTempWord
 in al,dx
 mov _gTempByte, al
 #endasm
 return gTempByte;
}
void setup_PIC (master_vector,slave_vector) unsigned char master_vector; unsigned char slave_vector;
{
  outportb (0x20, 0x11);
  outportb (0xA0, 0x11);
  outportb (0x20 +1, master_vector);
  outportb (0xA0 +1, slave_vector);
  outportb (0x20 +1, 1<<2);
  outportb (0xA0 +1, 2);
  outportb (0x20 +1, 1);
  outportb (0xA0 +1, 1);
}
void set_efer_svme_bit()
{
 #asm
  mov ecx,#0xC0000080
  RDMSR
  OR EAX,#0x1000
  WRMSR
  RDMSR
 #endasm
}
unsigned long read_cr3()
{
 #asm
  mov eax, cr3 ; read CR0 to eax
  mov _gTempDWord, eax
 #endasm
}
unsigned long read_cr0()
{
 #asm
  mov eax, cr0 ; read CR0 to eax
  mov _gTempDWord, eax
 #endasm
 return gTempDWord;
}
void write_cr0 (value) unsigned long value;
{
 gTempWord = value;
 #asm
  mov eax, _gTempDWord;
  mov cr0, eax
 #endasm
}
void write_cr3 (value) unsigned long value;
{
 gTempDWord = value;
 #asm
  mov eax, _gTempDWord;
  mov cr3, eax
 #endasm
}
void update_cs(value) unsigned int value;
{
 gTempWord = value;
 #asm
        push ax
        mov ax, _gTempWord ; ax = new cs
        push ax ; push segment
        push word #.ABC ; push offset
        retf ; we have a new cs now
.ABC:
  pop ax
 #endasm
}
void set_ds ( value ) unsigned int value;
{
 gTempWord = value;
 #asm
  mov ax,_gTempWord
  push ax
  pop ds
 #endasm
}
void set_es ( value ) unsigned int value;
{
 gTempWord = value;
 #asm
  mov ax,_gTempWord
  push ax
  pop es
 #endasm
}
void set_ss ( value ) unsigned int value;
{
 gTempWord = value;
 #asm
  mov ax,_gTempWord
  push ax
  pop ss
 #endasm
}
void Initialize()
{
   #asm
  out #0x21, al
  cli
 #endasm
 disable();
  outportb (0x70, inportb(0x70) | 0x80);
}
void setup_GDT32_entry (item, base, limit, access, attribs) DESCR_SEG* item; unsigned long base; unsigned long limit; unsigned char access; unsigned char attribs;
{
  item->base_l = (unsigned int)(base & 0xFFFF);
  item->base_m = (unsigned char)((base >> 16) & 0xFF);
  item->base_h = (unsigned char)(base >> 24);
  item->limit = (unsigned int)limit & 0xFFFF;
  item->attribs = attribs | (unsigned char)((limit >> 16) & 0x0F);
  item->access = access;
}
void SetupGDTs()
{
 #asm
  mov ax, #0x6000
  push ax
  pop ds
 #endasm
 DESCR_SEG *item = 0x60000;
 DESCR_SEG_16 *item16 = 0xF000;
 setup_GDT32_entry (item, 0, 0, 0, 0);
 setup_GDT32_entry (item+1, 0, 0, 0, 0);
 setup_GDT32_entry(item+2, (unsigned long)0x0, 0xFFFFFFFF, (0x80 | 0x10 | 0x02), 0x80 | 0x40);
 setup_GDT32_entry(item+3, (unsigned long)0x10000, 0xFFFFFFFF, (0x80 | 0x10 | 0x02), 0x80 | 0x40);
 setup_GDT32_entry(item+4, (unsigned long)0x9000 * 0x10, 0xFFFFFFFF, (0x80 | 0x18 | 0x02), 0x80);
 setup_GDT32_entry (item+5, 0, 0, 0, 0);
 setup_GDT32_entry (item+6, 0, 0, 0, 0);
 setup_GDT32_entry(item+7, (unsigned long)0x800000, 0xFFFFFFFF, (0x80 | 0x10 | 0x02), 0x80 | 0x40);
 setup_GDT32_entry(item+8, (unsigned long)0x9000 * 0x10, 0xFFFFFFFF, (0x80 | 0x10 | 0x02), 0x80 | 0x40);
 setup_GDT32_entry(item+9, (unsigned long)0xB8000, 0xFFFFFFFF, 0x93, 0xC0);
 setup_GDT32_entry(item+10, (unsigned long)0x800000, 0xFFFFFFFF, (0x80 | 0x18 | 0x02), 0x80 | 0x40);
 #asm
  mov ax,#0x9000
  push ax
  pop ds
 #endasm
}
void setup_IDT_entry (item, selector, offset, access, param_cnt) DESCR_INT *item; unsigned int selector; unsigned long offset; unsigned char access; unsigned char param_cnt;
{
 unsigned long Bottom = 0, Top = 0;
 unsigned int *ptr = (unsigned int *)item;
  ptr[0] = (unsigned int)offset;
  ptr[1] = selector;
  ptr[2] = access << 8 + 0;
  ptr[3] = (unsigned int)(offset >> 16);
}
void SetupPaging()
{
 unsigned long *ptr = 0;
 unsigned long *ptrT = 0;
 unsigned long cnt, cnt2;
 #asm
  mov ax,#0x7000
  push ax
  pop ds
 #endasm
 ptr = (0x70000 & 0x0000FFFF);
 ptrT = ((0x70000 & 0x0000FFFF) + 0x1000);
 for (cnt=0;cnt <= (0x1000000 / 0x400000); cnt++)
 {
  ptr[cnt] = (0x70000 + 4096 + (cnt*4096)) | 7;
  for (cnt2=0;cnt2<1024;cnt2++)
   ptrT[cnt2] = ((0x400000 * cnt) + ((cnt2) << 12)) | 7;
  ptrT += 0x400;
 }
 #asm
  mov ax,#0x1000
  push ax
  pop ds
 #endasm
 ptr = 0x1A000;
 ptrT = 0x1B000;
 for (cnt=0;cnt<=(0x400000 / 0x400000); cnt++)
 {
  ptr[cnt] = (0x1A000 + 4096 + (cnt*4096)) | 7;
  for (cnt2 = 0;cnt2 < 1024;cnt2++)
  {
   {
     ptrT[cnt2] = ((0x400000 * cnt) + 0x0 + (cnt2 << 12)) | 7;
   }
  }
  ptrT += 0x400;
 }
 #asm
  mov ax,#0x9000
  push ax
  pop ds
 #endasm
}
void SetupVMCB()
{
 unsigned char *VMCB = 0x0000;
 unsigned long lGDTLo = 0, lGDTHi = 0, lIDTLo = 0, lIDTHi = 0;
 unsigned long lGUEST_CR3_VALUE = 0;
 #asm
  mov ax, ds
  push ax
  mov ax, #0x9000
  push ax
  pop ds
  sgdt _Pre_GDTrLo
  sidt _Pre_IDTrLo
 #endasm
 lGDTLo = Pre_GDTrLo;
 lGDTHi = Pre_GDTrHi;
 lIDTLo = Pre_IDTrLo;
 lIDTHi = Pre_IDTrHi;
 #asm
  pop ds
 #endasm
 VMCB[0x2] = 0x0;
 VMCB[0x0] = 0x0;
 VMCB[0x08] = 0x00;
 VMCB[0x08 +1] = 0x20;
 VMCB[0x08 +2] = 0x00;
 VMCB[0x08 +3] = 0x00;
 VMCB[0x0C]= 0x0;
 VMCB[0x0C + 2] |= 0x20;
 VMCB[0x0C + 2] |= 0x00;
 VMCB[0x0C + 3] |= 0x01;
 VMCB[0x0C + 3] |= 0x0;
 VMCB[0x0C + 3] = 0x81;
 VMCB[0x10] = 0xff;
 VMCB[0x58] = 1;
 VMCB[0x58 +4] = 1;
 VMCB[0x08 +1] |= 0x20;
 VMCB[0x90] = 1;
 VMCB[0xb0] = (unsigned char)(0x621000 & 0xFF);
 VMCB[0xb0 +1] = (unsigned char)((0x621000 >> 8) & 0xFF);
 VMCB[0xb0 +2] = (unsigned char)((0x621000 >> 16) & 0xFF);
 VMCB[0xb0 +3] = (unsigned char)((0x621000 >> 24) & 0xFF);
 VMCB[0x410] = 0x00;
 VMCB[0x410 +1] = 0x00;
 VMCB[0x410 + 2] = 0X93;
 VMCB[0x410 + 4] = 0xFF;
 VMCB[0x410 + 5] = 0xFF;
 VMCB[0x410 + 6] = 0xFF;
 VMCB[0x410 + 7] = 0xFF;
 VMCB[0x410 + 8] = 0x00;
 VMCB[0x410 + 9] = 0x00;
 VMCB[0x410 + 10] = 0x00;
 VMCB[0x410 + 11] = 0x00;
 VMCB[0x400] = 0x00;
 VMCB[0x400 +1] = 0x00;
 VMCB[0x400 + 2] = 0X93;
 VMCB[0x400 + 4] = 0xFF;
 VMCB[0x400 + 5] = 0xFF;
 VMCB[0x400 + 6] = 0xFF;
 VMCB[0x400 + 7] = 0xFF;
 VMCB[0x400 + 8] = 0x00;
 VMCB[0x400 + 9] = 0x00;
 VMCB[0x400 + 10] = 0x00;
 VMCB[0x400 + 11] = 0x00;
 VMCB[0x430] = 0x00;
 VMCB[0x430 +1] = 0x00;
 VMCB[0x430 + 2] = 0X93;
 VMCB[0x430 + 4] = 0xFF;
 VMCB[0x430 + 5] = 0xFF;
 VMCB[0x430 + 6] = 0xFF;
 VMCB[0x430 + 7] = 0xFF;
 VMCB[0x430 + 8] = 0x00;
 VMCB[0x430 + 9] = 0x00;
 VMCB[0x430 + 10] = 0x00;
 VMCB[0x430 + 11] = 0x00;
 VMCB[0x420] = 0x00;
 VMCB[0x420 +1] = 0x00;
 VMCB[0x420 + 2] = 0X93;
 VMCB[0x420 + 4] = 0xFF;
 VMCB[0x420 + 5] = 0xFF;
 VMCB[0x420 + 6] = 0xFf;
 VMCB[0x420 + 7] = 0xFF;
 VMCB[0x420 + 8] = 0x00;
 VMCB[0x420 + 9] = 0x00;
 VMCB[0x420 + 10] = 0x00;
 VMCB[0x420 + 11] = 0x00;
 VMCB[0x440] = 0x00;
 VMCB[0x440 +1] = 0x00;
 VMCB[0x440 + 2] = 0X93;
 VMCB[0x440 + 4] = 0xFF;
 VMCB[0x440 + 5] = 0xFF;
 VMCB[0x440 + 6] = 0xFF;
 VMCB[0x440 + 7] = 0xFF;
 VMCB[0x440 + 8] = 0x00;
 VMCB[0x440 + 9] = 0x00;
 VMCB[0x440 + 10] = 0x00;
 VMCB[0x440 + 11] = 0x00;
 VMCB[0x450] = 0x00;
 VMCB[0x450 +1] = 0x00;
 VMCB[0x450 + 2] = 0X93;
 VMCB[0x450 + 4] = 0xFF;
 VMCB[0x450 + 5] = 0xFF;
 VMCB[0x450 + 6] = 0xFF;
 VMCB[0x450 + 7] = 0xFF;
 VMCB[0x450 + 8] = 0x00;
 VMCB[0x450 + 9] = 0x00;
 VMCB[0x450 + 10] = 0x00;
 VMCB[0x450 + 11] = 0x00;
 VMCB[0x460] = 0x00;
 VMCB[0x460 +1] = 0x00;
 VMCB[0x460 + 2] = 0x00;
 VMCB[0x460 + 3] = 0x00;
      VMCB[0x460 + 4] = (unsigned char)(lGDTLo);
      VMCB[0x460 + 5] = (unsigned char)((lGDTLo >> 8) & 0xFF);
      VMCB[0x460 + 6] = 0x00;
      VMCB[0x460 + 7] = 0x00;
      VMCB[0x460 + 8] = (unsigned char)((lGDTLo >> 16) & 0xFF);
      VMCB[0x460 + 9] = (unsigned char)((lGDTLo >> 24) & 0xFF);
      VMCB[0x460 + 10] = (unsigned char)(lGDTHi & 0xFF);
      VMCB[0x460 + 11] = (unsigned char)((lGDTHi >> 8) & 0xFF);
 VMCB[0x480] = 0x00;
 VMCB[0x480 +1] = 0x00;
 VMCB[0x480 + 2] = 0x00;
 VMCB[0x480 + 4] = 0xFF;
 VMCB[0x480 + 5] = 0x03;
 VMCB[0x480 + 6] = 0x00;
 VMCB[0x480 + 7] = 0x00;
 VMCB[0x480 + 8] = 0x00;
 VMCB[0x480 + 9] = 0x00;
 VMCB[0x480 + 10] = 0x00;
 VMCB[0x480 + 11] = 0x00;
 VMCB[0x4cB] = 0x00;
 VMCB[0x4D0] = 0x00;
 VMCB[0x4D0 +1] = 0x10;
 VMCB[0x558] = 0x10;
 VMCB[0x558 + 1] = 0x00;
 VMCB[0x558 + 2] = 0x00;
 VMCB[0x558 + 3] = 0x00;
 VMCB[0x640] = 0x00;
 VMCB[0x640 + 1] = 0x00;
 VMCB[0x640 + 2] = 0x00;
 VMCB[0x640 + 3] = 0x00;
 lGUEST_CR3_VALUE = 0x366000 + 0x0;
 VMCB[0x550] = (unsigned char)(lGUEST_CR3_VALUE & 0xFF);
 VMCB[0x550 +1] = (unsigned char)((lGUEST_CR3_VALUE >> 8) & 0xFF);
 VMCB[0x550 +2] = (unsigned char)((lGUEST_CR3_VALUE >> 16) & 0xFF);
 VMCB[0x550 +3] = (unsigned char)((lGUEST_CR3_VALUE >> 24) & 0xFF);
 VMCB[0x548] = 0x00;
 VMCB[0x548 + 1] = 0x00;
 VMCB[0x548 + 2] = 0x00;
 VMCB[0x548 + 3] = 0x00;
 VMCB[0x570] = 0x82;
 VMCB[0x570 +1] = 0x00;
 VMCB[0x570 +3] = 0x00;
 VMCB[0x570 +4] = 0x00;
 VMCB[0x5D8] = 0xd6;
 VMCB[0x5D8 + 1] = 0xFF;
 VMCB[0x5D8 + 2] = 0x00;
 VMCB[0x5D8 + 3] = 0x00;
 VMCB[0x578] = 0x00;
 VMCB[0x578 + 1] = 0x7C;
 VMCB[0x578 + 2] = 0x00;
 VMCB[0x578 + 3] = 0x00;
 VMCB[0x5F8] = 0x55;
 VMCB[0x5F8 +1] = 0xaa;
}
void MoveVisor()
{
  #asm
  push ds
  push es
  mov esi, #0x50000
  mov edi, #0x800000
  mov ecx, #0x50000
  cld
  push word #0x10
  push word #0x10
  pop ds
  pop es
  db 0x66
  db 0x67
  REP
  MOVSB
  mov si, #0x0
  pop es
  pop ds
  #endasm
}
void EnableProtectedMode()
{
  idtr.base = ((unsigned long)0x65000);
  idtr.base += 0;
  idtr.limit = (0x21*8)-1;
  gdtr.base = ((unsigned long)0x60000);
  gdtr.base += 0;
  gdtr.limit = (21*8)-1;
  gTempWord = &gdtr;
 #asm
  mov ebx, #0
  mov bx, _gTempWord
  lgdt [ebx]
 #endasm
 #asm
 mov si, #_PModeMsg
 call print_string
 #endasm
  setup_PIC (0x08, 0x70);
  outportb (0x20 +1, 0xB8);
  outportb (0xA0 +1, 0x8F);
  gTempDWord = read_cr0() | 1;
  write_cr0 (gTempDWord);
 update_cs (0x20);
 set_ds ( 0x40 );
 set_es ( 0x10 );
 set_ss ( 0x18 );
 set_efer_svme_bit();
 MoveVisor();
  outportb (0x20 +1, 0x0);
  outportb (0xA0 +1, 0x0);
 #asm
 mov ax,#0x10
 push ds
 push ax
 pop ds
 mov eax, 0x50000
 cmp eax, #0x4c8d00eb
 je OverError
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
 #endasm
}
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
  #asm
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
    or eax, ebx
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
    or eax, ebx
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
    or ax,[di]
    SEG SS
    or bx,2[di]
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
    or eax, ebx
    shr ebx, #16
    test eax, eax
    ret
  ;; sr function
  lsrul:
    mov cx,di
    jcxz lsr_exit
    and eax, #0x0000FFFF
    shl ebx, #16
    or eax, ebx
  lsr_loop:
    shr eax, #1
    loop lsr_loop
    mov ebx, eax
    shr ebx, #16
  lsr_exit:
    ret
  ;; sl function
  lsll:
  lslul:
    mov cx,di
    jcxz lsl_exit
    and eax, #0x0000FFFF
    shl ebx, #16
    or eax, ebx
  lsl_loop:
    shl eax, #1
    loop lsl_loop
    mov ebx, eax
    shr ebx, #16
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
    and eax, #0x0000FFFF
    shl ebx, #16
    or eax, ebx
    xor edx, edx
    SEG SS
    mov bx, 2[di]
    shl ebx, #16
    SEG SS
    mov bx, [di]
    div ebx
    mov ebx, eax
    shr ebx, #16
    ret
    ;; imodu doesnt preserve dx (returns quotient in it)
imodu:
 xor dx,dx
 div bx
 mov ax,dx
 ret
_lidt:
        push bp
        mov bp, sp
        push bx
  SEG SS
        mov bx, [bp+4] ; ds:bx = pointer to IDTR structure
        SEG DS
        lidt [bx] ; load IDTR
        pop bx
        pop bp
        ret
_lldt:
        push bp
        mov bp, sp
        SEG SS
        mov ax, [bp+4]
        lldt ax ; load LDTR
        pop bp
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
 add DI,cx
 mov ECX, #24
 mov EDX, #0x534d4150
 mov SI, #0x3C00
 push SI
 pop ES
 int #0x15
 mov cx,#24
 cmp ebx, #0x0
 jne getLoop
 ret
  #endasm
