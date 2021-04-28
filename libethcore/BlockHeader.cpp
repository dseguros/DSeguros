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
