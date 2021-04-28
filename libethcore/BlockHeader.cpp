#include <libdevcore/Common.h>
#include <libdevcore/Log.h>
#include <libdevcore/RLP.h>
#include <libdevcore/TrieDB.h>
#include <libdevcore/TrieHash.h>
#include <libethcore/Common.h>
#include "Exceptions.h"
#include "BlockHeader.h"

BlockHeader::BlockHeader()
{
}


BlockHeader::BlockHeader(bytesConstRef _block, BlockDataType _bdt, h256 const& _hashWith)
{
	RLP header = _bdt == BlockData ? extractHeader(_block) : RLP(_block);
	m_hash = _hashWith ? _hashWith : sha3(header.data());
	populate(header);
}

void BlockHeader::clear()
{
	m_parentHash = h256();
	m_sha3Uncles = EmptyListSHA3;
	m_author = Address();
	m_stateRoot = EmptyTrie;
	m_transactionsRoot = EmptyTrie;
	m_receiptsRoot = EmptyTrie;
	m_logBloom = LogBloom();
	m_difficulty = 0;
	m_number = 0;
	m_gasLimit = 0;
	m_gasUsed = 0;
	m_timestamp = Invalid256;
	m_extraData.clear();
	m_seal.clear();
	noteDirty();
}

h256 BlockHeader::hash(IncludeSeal _i) const
{
	h256 dummy;
	h256& memo = _i == WithSeal ? m_hash : _i == WithoutSeal ? m_hashWithout : dummy;
	if (!memo)
	{
		RLPStream s;
		streamRLP(s, _i);
		memo = sha3(s.out());
	}
	return memo;
}

void BlockHeader::streamRLPFields(RLPStream& _s) const
{
	_s	<< m_parentHash << m_sha3Uncles << m_author << m_stateRoot << m_transactionsRoot << m_receiptsRoot << m_logBloom
		<< m_difficulty << m_number << m_gasLimit << m_gasUsed << m_timestamp << m_extraData;
}

void BlockHeader::streamRLP(RLPStream& _s, IncludeSeal _i) const
{
	if (_i != OnlySeal)
	{
		_s.appendList(BlockHeader::BasicFields + (_i == WithoutSeal ? 0 : m_seal.size()));
		BlockHeader::streamRLPFields(_s);
	}
	if (_i != WithoutSeal)
		for (unsigned i = 0; i < m_seal.size(); ++i)
			_s.appendRaw(m_seal[i]);
}

h256 BlockHeader::headerHashFromBlock(bytesConstRef _block)
{
	return sha3(RLP(_block)[0].data());
}
