#ifndef __start_h__
	#include "start.h"
#endif

void UpdatePDEntry(DWORD *PDAddress, WORD Entry, DWORD Page );

void UpdatePDEntry(DWORD *PDAddress, WORD Entry, DWORD Page )
{
		PDAddress[Entry] = (Page & 0xFFFFF000) | 7;
}
