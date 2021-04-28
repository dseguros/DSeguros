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
