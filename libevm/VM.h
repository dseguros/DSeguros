#pragma once

#include <unordered_map>
#include <libdevcore/Exceptions.h>
#include <libethcore/Common.h>
#include <libevmcore/Instruction.h>
#include <libdevcore/SHA3.h>
#include <libethcore/BlockHeader.h>
#include "VMFace.h"

namespace dev
{

namespace eth
{

// Convert from a 256-bit integer stack/memory entry into a 160-bit Address hash.
// Currently we just pull out the right (low-order in BE) 160-bits.
inline Address asAddress(u256 _item)
{
	return right160(h256(_item));
}

inline u256 fromAddress(Address _a)
{
	return (u160)_a;
}

struct InstructionMetric
{
	Tier gasPriceTier;
	int args;
	int ret;
};

class VM: public VMFace
{
public:
	virtual owning_bytes_ref exec(u256& _io_gas, ExtVMFace& _ext, OnOpFunc const& _onOp) override final;

#if EVM_JUMPS_AND_SUBS
	// invalid code will throw an exeption
	void validate(ExtVMFace& _ext);
	void validateSubroutine(uint64_t _PC, uint64_t* _rp, u256* _sp);
#endif

	bytes const& memory() const { return m_mem; }
	u256s stack() const {
		u256s stack(m_SP, m_stackEnd);
		reverse(stack.begin(), stack.end());
		return stack;
	};

};

}
}
