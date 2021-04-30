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

private:
	PrecompiledPricer m_cost;
	PrecompiledExecutor m_execute;
	u256 m_startingBlock = 0;
};

struct ChainOperationParams
{
	ChainOperationParams();

	explicit operator bool() const { return accountStartNonce != Invalid256; }

	/// The chain sealer name: e.g. Ethash, NoProof, BasicAuthority
	std::string sealEngineName = "NoProof";

	/// General chain params.
	u256 blockReward = 0;
	u256 maximumExtraDataSize = 1024;
	u256 accountStartNonce = 0;
	bool tieBreakingGas = true;
	std::string dataDir;
}

}
}
