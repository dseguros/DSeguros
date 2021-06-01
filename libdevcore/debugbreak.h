#ifndef DEBUG_BREAK_H
#define DEBUG_BREAK_H

#if defined(_MSC_VER) || defined(__MINGW32__)

#define debug_break __debugbreak

#else

#include <signal.h>
#include <unistd.h>
#include <sys/syscall.h>

