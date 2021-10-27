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
}
#endif
