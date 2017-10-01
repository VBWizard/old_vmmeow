#include "isr_wrapper.h"
#include "utility.h"
#include "pmode.h"
#include "PM_DEFS_gcc.H"
#ifndef __start_h__
	#include "start.h"
#endif

DWORD GDT_Save=0;
DWORD idt=0;
void setup_GDT32_entry_gcc (DESCR_SEG* item, DWORD base, DWORD limit, BYTE access, BYTE attribs);
void SetupHostIDT();

void setup_GDT32_entry_gcc (DESCR_SEG* item, DWORD base, DWORD limit, BYTE access, BYTE attribs)
{
OPEN_UP_DS
  item->base_l = (WORD)(base & 0xFFFF);
  item->base_m = (BYTE)((base >> 16) & 0xFF);
  item->base_h = (BYTE)(base >> 24);
  item->limit = (WORD)limit & 0xFFFF;
  item->attribs = attribs | (BYTE)((limit >> 16) & 0x0F);
  item->access = access;
RESTORE_DS
}

void ReloadGDT()
{
		__asm__("sgdt GDT_Save");
		__asm__("lgdt GDT_Save");
}

void setup_IDT_entry (DESCR_INT *item, WORD selector, DWORD offset, BYTE access, BYTE param_cnt) 
{
	WORD *ptr = (WORD *)item;

  ptr[0] = (WORD)offset;
  ptr[1] = selector;
  ptr[2] = (access << 8) + 0;
  ptr[3] = (WORD)(offset >> 16);
}

void SetupHostIDT()
{
	DESCR_INT *item = (DESCR_INT*)HOST_IDT_FINAL;

  /* setting up the exception handlers and timer, keyboard ISRs */
  setup_IDT_entry (item+0x00, 0x50, (DWORD)_isr_00_wrapper, ACS_INT, 0);
  setup_IDT_entry (item+0x01, 0x50, (DWORD)_isr_01_wrapper, ACS_INT, 0);
  setup_IDT_entry (item+0x02, 0x50, (DWORD)_isr_02_wrapper, ACS_INT, 0);
  setup_IDT_entry (item+0x03, 0x50, (DWORD)_isr_03_wrapper, ACS_INT, 0);
  setup_IDT_entry (item+0x04, 0x50, (DWORD)_isr_04_wrapper, ACS_INT, 0);
  setup_IDT_entry (item+0x05, 0x50, (DWORD)_isr_05_wrapper, ACS_INT, 0);
  setup_IDT_entry (item+0x06, 0x50, (DWORD)_isr_06_wrapper, ACS_INT, 0);
  setup_IDT_entry (item+0x07, 0x50, (DWORD)_isr_07_wrapper, ACS_INT, 0);
  //CLR 09/26/2014 - Changed to 0x8 ... was 0x50
  setup_IDT_entry (item+0x08, 0x50, (DWORD)_isr_08_wrapper, ACS_INT, 0);
  setup_IDT_entry (item+0x09, 0x50, (DWORD)_isr_09_wrapper, ACS_INT, 0);
  setup_IDT_entry (item+0x0a, 0x50, (DWORD)_isr_0A_wrapper, ACS_INT, 0);
  setup_IDT_entry (item+0x0b, 0x50, (DWORD)_isr_0B_wrapper, ACS_INT, 0);
  setup_IDT_entry (item+0x0c, 0x50, (DWORD)_isr_0C_wrapper, ACS_INT, 0);
  setup_IDT_entry (item+0x0d, 0x50, (DWORD)_isr_0D_wrapper, ACS_INT, 0);
  setup_IDT_entry (item+0x0e, 0x50, (DWORD)_isr_0E_wrapper, ACS_INT, 0);
  setup_IDT_entry (item+0x0f, 0x50, (DWORD)_isr_0F_wrapper, ACS_INT, 0);
  setup_IDT_entry (item+0x10, 0x50, (DWORD)_isr_10_wrapper, ACS_INT, 0);
  setup_IDT_entry (item+0x11, 0x50, (DWORD)_isr_11_wrapper, ACS_INT, 0);
  setup_IDT_entry (item+0x12, 0x50, (DWORD)_isr_12_wrapper, ACS_INT, 0);
  setup_IDT_entry (item+0x13, 0x50, (DWORD)_isr_13_wrapper, ACS_INT, 0);
  setup_IDT_entry (item+0x14, 0x50, (DWORD)_isr_14_wrapper, ACS_INT, 0);
  setup_IDT_entry (item+0x15, 0x50, (DWORD)_isr_15_wrapper, ACS_INT, 0);
  setup_IDT_entry (item+0x16, 0x50, (DWORD)_isr_16_wrapper, ACS_INT, 0);
  setup_IDT_entry (item+0x17, 0x50, (DWORD)_isr_17_wrapper, ACS_INT, 0);
  setup_IDT_entry (item+0x18, 0x50, (DWORD)_isr_18_wrapper, ACS_INT, 0);
  setup_IDT_entry (item+0x19, 0x50, (DWORD)_isr_19_wrapper, ACS_INT, 0);
  setup_IDT_entry (item+0x1a, 0x50, (DWORD)_isr_1A_wrapper, ACS_INT, 0);
  setup_IDT_entry (item+0x1b, 0x50, (DWORD)_isr_1B_wrapper, ACS_INT, 0);
  setup_IDT_entry (item+0x1c, 0x50, (DWORD)_isr_1C_wrapper, ACS_INT, 0);
  setup_IDT_entry (item+0x1d, 0x50, (DWORD)_isr_1D_wrapper, ACS_INT, 0);
  setup_IDT_entry (item+0x1e, 0x50, (DWORD)_isr_1E_wrapper, ACS_INT, 0);
  setup_IDT_entry (item+0x1f, 0x50, (DWORD)_isr_1F_wrapper, ACS_INT, 0);
  setup_IDT_entry (item+0x20, 0x50, (DWORD)_isr_20_wrapper, ACS_INT, 0);
  setup_IDT_entry (item+0x21, 0x50, (DWORD)_isr_21_wrapper, ACS_INT, 0);
}
