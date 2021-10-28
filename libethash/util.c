#include <stdarg.h>
#include <stdio.h>
#include "util.h"

#ifdef _MSC_VER

// foward declare without all of Windows.h
__declspec(dllimport) void __stdcall OutputDebugStringA(const char* lpOutputString);



#endif
