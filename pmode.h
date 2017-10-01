#include "PM_DEFS_gcc.H"

void setup_GDT32_entry_gcc (DESCR_SEG* item, DWORD base, DWORD limit, BYTE access, BYTE attribs);
void SetupHostIDT();
