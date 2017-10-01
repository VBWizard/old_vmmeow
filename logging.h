#ifndef __logging_h__
#define __logging_h__
#include "start.h"
DWORD theLog;
DWORD LogCurrentPtr;

void log_base_write(char *Message, char crlf);
void log_writeln(char *Message);
void log_write(char *Message);
#endif
