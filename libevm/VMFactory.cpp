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

	void VMFactory::setKind(VMKind _kind)
{
	g_kind = _kind;
}

std::unique_ptr<VMFace> VMFactory::create()
{
	return create(g_kind);
}

std::unique_ptr<VMFace> VMFactory::create(VMKind _kind)
{
#if ETH_EVMJIT
	switch (_kind)
	{
	default:
	case VMKind::Interpreter:
		return std::unique_ptr<VMFace>(new VM);
	case VMKind::JIT:
		return std::unique_ptr<VMFace>(new JitVM);
	case VMKind::Smart:
		return std::unique_ptr<VMFace>(new SmartVM);
	}
#else
	asserts(_kind == VMKind::Interpreter && "JIT disabled in build configuration");
	return std::unique_ptr<VMFace>(new VM);
#endif
}
}

}
}
