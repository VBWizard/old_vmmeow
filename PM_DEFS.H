/*
  PMode tutorials in C and Asm
  Copyright (C) 2000 Alexei A. Frounze
  The programs and sources come under the GPL 
  (GNU General Public License), for more information
  read the file gnu-gpl.txt (originally named COPYING).
*/

#ifndef _pm_defs_
#define _pm_defs_

#define BYTE unsigned char
#define WORD unsigned int
#define DWORD unsigned long

/* Ports */
#define PORT_8259M      0x20
#define PORT_8259S      0xA0
#define PORT_KBD_A      0x60

#define EOI             0x20

/* Access BYTE's flags */
#define ACS_PRESENT     0x80            /* present segment */
#define ACS_CSEG        0x18            /* code segment */
#define ACS_DSEG        0x10            /* data segment */
#define ACS_CONFORM     0x04            /* conforming segment */
#define ACS_READ        0x02            /* readable segment */
#define ACS_WRITE       0x02            /* writable segment */
#define ACS_IDT         ACS_DSEG        /* segment type is the same type */
#define ACS_INT_GATE    0x0E            /* int gate for 386 */
#define ACS_INT_GATE16  0x06
#define ACS_INT16		(ACS_PRESENT | ACS_INT_GATE16) /*present int gate 16-bit*/
#define ACS_INT         (ACS_PRESENT | ACS_INT_GATE) /* present int gate */
#define ACS_TSS_GATE    0x09
#define ACS_TSS         (ACS_PRESENT | ACS_TSS_GATE) /* present tss gate */
#define ACS_DPL_0       0x00            /* descriptor privilege level #0 */
#define ACS_DPL_1       0x20            /* descriptor privilege level #1 */
#define ACS_DPL_2       0x40            /* descriptor privilege level #2 */
#define ACS_DPL_3       0x60            /* descriptor privilege level #3 */
#define ACS_LDT         0x02            /* ldt descriptor */
#define ACS_TASK_GATE   0x05
#define ACS_TASK        (ACS_PRESENT | ACS_TASK_GATE) /* present task gate */
#define ACS_CALL_GATE16 0x04
#define ACS_CALL16      (ACS_PRESENT | ACS_CALL_GATE16) /* present 16-bit call gate */
#define ACS_CALL_GATE32 0x0C
#define ACS_CALL32      (ACS_PRESENT | ACS_CALL_GATE32) /* present 32-bit call gate */

/* Ready-made values */
#define ACS_CODE        (ACS_PRESENT | ACS_CSEG | ACS_READ)
#define ACS_DATA        (ACS_PRESENT | ACS_DSEG | ACS_WRITE)
#define ACS_STACK       (ACS_PRESENT | ACS_DSEG | ACS_WRITE)

/* Attributes (in turms of size) */
#define ATTR_GRANULARITY 0x80           /* segment limit is given in 4KB pages rather than in BYTEs */
#define ATTR_BIG         0x40           /* ESP is used rather than SP */
#define ATTR_DEFAULT     0x40           /* 32-bit code segment rather than 16-bit */

#pragma pack (push, 1)  /* align structures to a BYTE boundary */

/* Segment desciptor definition */
typedef struct {
  WORD limit,
       base_l;
  BYTE base_m,
       access,
       attribs,
       base_h;
} DESCR_SEG;

typedef struct {
	WORD limit,
		 base_low;
	BYTE base_mid,
		 flags;
} DESCR_SEG_16;


/* GDTR register definition */
typedef struct {
  WORD limit;
  DWORD base;
} GDTR;

/* Interrupt desciptor definition */
typedef struct {
  WORD offset_l,
       selector;
  BYTE param_cnt,
       access;
  WORD offset_h;
} DESCR_INT;

/* IDTR register definition */
typedef struct {
  WORD limit;
  DWORD base;
} IDTR;

/* TSS definition */
typedef struct {        /* TSS for 386+ */
  DWORD link,
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
  WORD  trace,
        io_map_addr;
  BYTE  io_map[8192];
} TSS;

typedef struct {        /* TSS for 386+ (no I/O bit map) */
  DWORD link,
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
  WORD  trace,
        io_map_addr;
} _TSS;

#pragma pack (pop)      /* align structures to default boundary */

#endif
