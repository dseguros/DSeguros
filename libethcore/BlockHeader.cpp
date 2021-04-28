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

RLP BlockHeader::extractHeader(bytesConstRef _block)
{
	RLP root(_block);
	if (!root.isList())
		BOOST_THROW_EXCEPTION(InvalidBlockFormat() << errinfo_comment("Block must be a list") << BadFieldError(0, _block.toString()));
	RLP header = root[0];
	if (!header.isList())
		BOOST_THROW_EXCEPTION(InvalidBlockFormat() << errinfo_comment("Block header must be a list") << BadFieldError(0, header.data().toString()));
	if (!root[1].isList())
		BOOST_THROW_EXCEPTION(InvalidBlockFormat() << errinfo_comment("Block transactions must be a list") << BadFieldError(1, root[1].data().toString()));
	if (!root[2].isList())
		BOOST_THROW_EXCEPTION(InvalidBlockFormat() << errinfo_comment("Block uncles must be a list") << BadFieldError(2, root[2].data().toString()));
	return header;
}

void BlockHeader::populate(RLP const& _header)
{
	int field = 0;
	try
	{
		m_parentHash = _header[field = 0].toHash<h256>(RLP::VeryStrict);
		m_sha3Uncles = _header[field = 1].toHash<h256>(RLP::VeryStrict);
		m_author = _header[field = 2].toHash<Address>(RLP::VeryStrict);
		m_stateRoot = _header[field = 3].toHash<h256>(RLP::VeryStrict);
		m_transactionsRoot = _header[field = 4].toHash<h256>(RLP::VeryStrict);
		m_receiptsRoot = _header[field = 5].toHash<h256>(RLP::VeryStrict);
		m_logBloom = _header[field = 6].toHash<LogBloom>(RLP::VeryStrict);
		m_difficulty = _header[field = 7].toInt<u256>();
		m_number = _header[field = 8].toInt<u256>();
		m_gasLimit = _header[field = 9].toInt<u256>();
		m_gasUsed = _header[field = 10].toInt<u256>();
		m_timestamp = _header[field = 11].toInt<u256>();
		m_extraData = _header[field = 12].toBytes();
		m_seal.clear();
		for (unsigned i = 13; i < _header.itemCount(); ++i)
			m_seal.push_back(_header[i].data().toBytes());
	}
	catch (Exception const& _e)
	{
		_e << errinfo_name("invalid block header format") << BadFieldError(field, toHex(_header[field].data().toBytes()));
		throw;
	}
}

struct BlockInfoDiagnosticsChannel: public LogChannel { static const char* name() { return EthBlue "▧" EthWhite " ◌"; } static const int verbosity = 9; };

