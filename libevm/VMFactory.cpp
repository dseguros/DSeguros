#include "VMFactory.h"
#include <libdevcore/Assertions.h>
#include "VM.h"

#if ETH_EVMJIT
#include "JitVM.h"
#include "SmartVM.h"
#endif


namespace dev
{
namespace eth
{
namespace
{
	auto g_kind = VMKind::Interpreter;
}

}
}
