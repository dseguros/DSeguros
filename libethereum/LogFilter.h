#pragma once

#include <libdevcore/Common.h>
#include <libdevcore/RLP.h>
#include <libethcore/Common.h>
#include "TransactionReceipt.h"

#ifdef __INTEL_COMPILER
#pragma warning(disable:1098) //the qualifier on this friend declaration is ignored
#endif

namespace dev
{

namespace eth
{
class LogFilter;
}


namespace eth
{

/// Simple stream output for the StateDiff.
std::ostream& operator<<(std::ostream& _out, dev::eth::LogFilter const& _s);

class State;
class Block;

class LogFilter
{
public:
	LogFilter(h256 _earliest = EarliestBlockHash, h256 _latest = PendingBlockHash): m_earliest(_earliest), m_latest(_latest) {}

	void streamRLP(RLPStream& _s) const;
	h256 sha3() const;

};

}

}
