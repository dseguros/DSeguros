#include "VMFactory.h"
#include <libdevcore/Assertions.h>
#include "VM.h"

#if ETH_EVMJIT
#include "JitVM.h"
#include "SmartVM.h"
#endif

