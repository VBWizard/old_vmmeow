#include "logging.h"
#include "start.h"
#include "utility.h"

DWORD theLog = LOG_MEMORY_ADDRESS_START;
DWORD LogCurrentPtr = 0;
DWORD Combined;
char lTheChar;

void log_base_write(char *Message, char crlf)
{
int i = 0;
	if (theLog + LogCurrentPtr >= VISOR_FINAL_ADDRESS)
		return;
	while (Message[i])	
	{
		//asm("xchg bx,bx\n");
		Combined = (theLog + LogCurrentPtr++);
		lTheChar = Message[i++];
		SetMemB(Combined, lTheChar, false);
	}
	if (crlf)
	{
		Combined = (theLog + LogCurrentPtr++);
		SetMemB(Combined, '\10', false);
	}

	LogCurrentPtr++;
//	LogCurrentPtr++ = '\0';
}
void log_writeln(char *Message)
{
	log_base_write(Message, true);
}
void log_write(char *Message)
{
	log_base_write(Message, false);
}
