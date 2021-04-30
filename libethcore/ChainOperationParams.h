#pragma once

#include <libdevcore/Common.h>
#include "Common.h"
#include <libethcore/Precompiled.h>

#include <libevmcore/EVMSchedule.h>

namespace dev
{
namespace eth
{

class PrecompiledContract
{
public:
	PrecompiledContract() = default;
	PrecompiledContract(
		PrecompiledPricer const& _cost,
		PrecompiledExecutor const& _exec,
		u256 const& _startingBlock = 0
	):
		m_cost(_cost),
		m_execute(_exec),
		m_startingBlock(_startingBlock)
	{}

	PrecompiledContract(
		unsigned _base,
		unsigned _word,
		PrecompiledExecutor const& _exec,
		u256 const& _startingBlock = 0
	);

	bigint cost(bytesConstRef _in) const { return m_cost(_in); }
	std::pair<bool, bytes> execute(bytesConstRef _in) const { return m_execute(_in); }

	u256 const& startingBlock() const { return m_startingBlock; }
}


}
}
