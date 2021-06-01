#ifndef DEBUG_BREAK_H
#define DEBUG_BREAK_H

#if defined(_MSC_VER) || defined(__MINGW32__)

#define debug_break __debugbreak

#else

#include <signal.h>
#include <unistd.h>
#include <sys/syscall.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
	/* gcc optimizers consider code after __builtin_trap() dead.
	 * Making __builtin_trap() unsuitable for breaking into the debugger */
	DEBUG_BREAK_PREFER_BUILTIN_TRAP_TO_SIGTRAP = 0,
};
