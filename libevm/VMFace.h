#pragma once

#include <memory>
#include <libdevcore/Exceptions.h>
#include "ExtVMFace.h"

namespace dev
{
namespace eth
{

struct VMException: Exception {};
#define ETH_SIMPLE_EXCEPTION_VM(X) struct X: VMException { const char* what() const noexcept override { return #X; } }
ETH_SIMPLE_EXCEPTION_VM(BadInstruction);
ETH_SIMPLE_EXCEPTION_VM(BadJumpDestination);
ETH_SIMPLE_EXCEPTION_VM(OutOfGas);
ETH_SIMPLE_EXCEPTION_VM(OutOfStack);
ETH_SIMPLE_EXCEPTION_VM(StackUnderflow);

}
}
