/*
  PMode tutorials in C and Asm
  Copyright (C) 2000 Alexei A. Frounze
  The programs and sources come under the GPL 
  (GNU General Public License), for more information
  read the file gnu-gpl.txt (originally named COPYING).
*/

#ifndef _isr_wrap_h_
#define _isr_wrap_h_


extern  unsigned char exc_has_error[0x20];
void isr_00_wrapper();					//used to be void prefix keyword
void isr_01_wrapper();
void isr_02_wrapper();
void isr_03_wrapper();
void isr_04_wrapper();
void isr_05_wrapper();
void isr_06_wrapper();
void isr_07_wrapper();
void isr_08_wrapper();
void isr_09_wrapper();
void isr_0A_wrapper();
void isr_0B_wrapper();
void isr_0C_wrapper();
void isr_0D_wrapper();
void isr_0E_wrapper();
void isr_0F_wrapper();
void isr_10_wrapper();
void isr_11_wrapper();
void isr_12_wrapper();
void isr_13_wrapper();
void isr_14_wrapper();
void isr_15_wrapper();
void isr_16_wrapper();
void isr_17_wrapper();
void isr_18_wrapper();
void isr_19_wrapper();
void isr_1A_wrapper();
void isr_1B_wrapper();
void isr_1C_wrapper();
void isr_1D_wrapper();
void isr_1E_wrapper();
void isr_1F_wrapper();
void isr_20_wrapper();
void isr_21_wrapper();
void call_gate_wrapper();

#endif
