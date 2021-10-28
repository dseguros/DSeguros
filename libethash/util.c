#include <stdarg.h>
#include <stdio.h>
#include "util.h"

#ifdef _MSC_VER

// foward declare without all of Windows.h
__declspec(dllimport) void __stdcall OutputDebugStringA(const char* lpOutputString);

void debugf(const char *str, ...)
{
	va_list args;
    va_start(args, str);

	char buf[1<<16];
	_vsnprintf_s(buf, sizeof(buf), sizeof(buf), str, args);
	buf[sizeof(buf)-1] = '\0';
	OutputDebugStringA(buf);
}

#endif
