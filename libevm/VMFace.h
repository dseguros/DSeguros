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

struct RevertInstruction: VMException
{
	explicit RevertInstruction(owning_bytes_ref&& _output) : m_output(std::move(_output)) {}
	RevertInstruction(RevertInstruction const&) = delete;
	RevertInstruction(RevertInstruction&&) = default;
	RevertInstruction& operator=(RevertInstruction const&) = delete;
	RevertInstruction& operator=(RevertInstruction&&) = default;

	char const* what() const noexcept override { return "Revert instruction"; }

	owning_bytes_ref&& output() { return std::move(m_output); }

private:
	owning_bytes_ref m_output;
};

}
}
