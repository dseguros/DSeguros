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

#if defined(__i386__) || defined(__x86_64__)
enum { HAVE_TRAP_INSTRUCTION = 1, };
__attribute__((gnu_inline, always_inline))
static void __inline__ trap_instruction(void)
{
	__asm__ volatile("int $0x03");
}
#elif defined(__thumb__)
enum { HAVE_TRAP_INSTRUCTION = 1, };
/* FIXME: handle __THUMB_INTERWORK__ */
__attribute__((gnu_inline, always_inline))
static void __inline__ trap_instruction(void)
{
	/* See 'arm-linux-tdep.c' in GDB source.
	 * Both instruction sequences below works. */
#if 1
	/* 'eabi_linux_thumb_le_breakpoint' */
	__asm__ volatile(".inst 0xde01");
#else
	/* 'eabi_linux_thumb2_le_breakpoint' */
	__asm__ volatile(".inst.w 0xf7f0a000");
#endif

	/* Known problem:
	 * After a breakpoint hit, can't stepi, step, or continue in GDB.
	 * 'step' stuck on the same instruction.
	 *
	 * Workaround: a new GDB command,
	 * 'debugbreak-step' is defined in debugbreak-gdb.py
	 * that does:
	 * (gdb) set $instruction_len = 2
	 * (gdb) tbreak *($pc + $instruction_len)
	 * (gdb) jump   *($pc + $instruction_len)
	 */
}
#elif defined(__arm__) && !defined(__thumb__)
enum { HAVE_TRAP_INSTRUCTION = 1, };
__attribute__((gnu_inline, always_inline))
static void __inline__ trap_instruction(void)
{
	/* See 'arm-linux-tdep.c' in GDB source,
	 * 'eabi_linux_arm_le_breakpoint' */
	__asm__ volatile(".inst 0xe7f001f0");
	/* Has same known problem and workaround
	 * as Thumb mode */
}
#else
enum { HAVE_TRAP_INSTRUCTION = 0, };
#endif

__attribute__((gnu_inline, always_inline))
static void __inline__ debug_break(void)
{
	if (HAVE_TRAP_INSTRUCTION) {
#if defined(ETH_EMSCRIPTEN)
		asm("debugger");
#else
		trap_instruction();
#endif
	} else if (DEBUG_BREAK_PREFER_BUILTIN_TRAP_TO_SIGTRAP) {
		 /* raises SIGILL on Linux x86{,-64}, to continue in gdb:
		  * (gdb) handle SIGILL stop nopass
		  * */
		__builtin_trap();
	} else {
		raise(SIGTRAP);
	}
}

#ifdef __cplusplus
}
#endif

#endif

#endif
