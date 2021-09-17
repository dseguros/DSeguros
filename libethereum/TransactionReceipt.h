#pragma once

#include <array>
#include <libdevcore/Common.h>
#include <libdevcore/RLP.h>
#include <libevm/ExtVMFace.h>

namespace dev
{

namespace eth
{

class TransactionReceipt
{
public:
	TransactionReceipt(bytesConstRef _rlp);
	TransactionReceipt(h256 _root, u256 _gasUsed, LogEntries const& _log);

	h256 const& stateRoot() const { return m_stateRoot; }
	u256 const& gasUsed() const { return m_gasUsed; }
	LogBloom const& bloom() const { return m_bloom; }
	LogEntries const& log() const { return m_log; }

};

}
}
