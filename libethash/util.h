#pragma once
#include <stdint.h>
#include "compiler.h"

#ifdef __cplusplus
extern "C" {

    #endif

#ifdef _MSC_VER
void debugf(char const* str, ...);
#else
#define debugf printf
#endif

static inline uint32_t min_u32(uint32_t a, uint32_t b)
{
	return a < b ? a : b;
}

}
#endif
