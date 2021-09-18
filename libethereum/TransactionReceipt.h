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


	void streamRLP(RLPStream& _s) const;

	bytes rlp() const { RLPStream s; streamRLP(s); return s.out(); }

private:
	h256 m_stateRoot;
	u256 m_gasUsed;
	LogBloom m_bloom;
	LogEntries m_log;
};

using TransactionReceipts = std::vector<TransactionReceipt>;

std::ostream& operator<<(std::ostream& _out, eth::TransactionReceipt const& _r);

class LocalisedTransactionReceipt: public TransactionReceipt
{
public:
	LocalisedTransactionReceipt(
		TransactionReceipt const& _t,
		h256 const& _hash,
		h256 const& _blockHash,
		BlockNumber _blockNumber,
		unsigned _transactionIndex,
		Address const& _contractAddress = Address()
	):
		TransactionReceipt(_t),
		m_hash(_hash),
		m_blockHash(_blockHash),
		m_blockNumber(_blockNumber),
		m_transactionIndex(_transactionIndex),
		m_contractAddress(_contractAddress)
	{
		LogEntries entries = log();
		for (unsigned i = 0; i < entries.size(); i++)
			m_localisedLogs.push_back(LocalisedLogEntry(
				entries[i],
				m_blockHash,
				m_blockNumber,
				m_hash,
				m_transactionIndex,
				i
			));
	}

};


}
}
