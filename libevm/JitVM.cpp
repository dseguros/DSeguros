#include "JitVM.h"

#include <libdevcore/Log.h>
#include <libevm/VM.h>
#include <libevm/VMFactory.h>

namespace dev
{
namespace eth
{
namespace
{

static_assert(sizeof(Address) == sizeof(evm_uint160be),
              "Address types size mismatch");
static_assert(alignof(Address) == alignof(evm_uint160be),
              "Address types alignment mismatch");

inline evm_uint160be toEvmC(Address _addr)
{
	return *reinterpret_cast<evm_uint160be*>(&_addr);
}


}

}
}
