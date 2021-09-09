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


	/// hash of earliest block which should be filtered
	h256 earliest() const { return m_earliest; }

	/// hash of latest block which should be filtered
	h256 latest() const { return m_latest; }

	/// Range filter is a filter which doesn't care about addresses or topics
	/// Matches are all entries from earliest to latest
	/// @returns true if addresses and topics are unspecified
	bool isRangeFilter() const;

	/// @returns bloom possibilities for all addresses and topics
	std::vector<LogBloom> bloomPossibilities() const;
};

}

}
