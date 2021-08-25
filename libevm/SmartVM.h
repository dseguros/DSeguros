#pragma once

#include "VMFace.h"

namespace dev
{
namespace eth
{

/// Smart VM proxy.
///
/// This class is a strategy pattern implementation for VM. For every EVM code
/// execution request it tries to select the best VM implementation (Interpreter or JIT)
/// by analyzing available information like: code size, hit count, JIT status, etc.
class SmartVM: public VMFace
{
public:
	owning_bytes_ref exec(u256& io_gas, ExtVMFace& _ext, OnOpFunc const& _onOp) override final;
};

}
}
