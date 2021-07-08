#pragma once

#include <unordered_map>
#include <libdevcore/Log.h>
#include <libdevcore/RLP.h>
#include "TransactionReceipt.h"

namespace dev
{
namespace eth
{

// TODO: OPTIMISE: constructors take bytes, RLP used only in necessary classes.

static const unsigned c_bloomIndexSize = 16;
static const unsigned c_bloomIndexLevels = 2;

static const unsigned InvalidNumber = (unsigned)-1;

struct BlockDetails
{
	BlockDetails(): number(InvalidNumber), totalDifficulty(Invalid256) {}
	BlockDetails(unsigned _n, u256 _tD, h256 _p, h256s _c): number(_n), totalDifficulty(_tD), parent(_p), children(_c) {}
	BlockDetails(RLP const& _r);
	bytes rlp() const;

	bool isNull() const { return number == InvalidNumber; }
	explicit operator bool() const { return !isNull(); }

	unsigned number = InvalidNumber;
	u256 totalDifficulty = Invalid256;
	h256 parent;
	h256s children;

	mutable unsigned size;
};

struct BlockLogBlooms
{
	BlockLogBlooms() {}
	BlockLogBlooms(RLP const& _r) { blooms = _r.toVector<LogBloom>(); size = _r.data().size(); }
	bytes rlp() const { bytes r = dev::rlp(blooms); size = r.size(); return r; }

	LogBlooms blooms;
	mutable unsigned size;
};

struct BlocksBlooms
{
	BlocksBlooms() {}
	BlocksBlooms(RLP const& _r) { blooms = _r.toArray<LogBloom, c_bloomIndexSize>(); size = _r.data().size(); }
	bytes rlp() const { bytes r = dev::rlp(blooms); size = r.size(); return r; }

	std::array<LogBloom, c_bloomIndexSize> blooms;
	mutable unsigned size;
};

struct BlockReceipts
{
	BlockReceipts() {}
	BlockReceipts(RLP const& _r) { for (auto const& i: _r) receipts.emplace_back(i.data()); size = _r.data().size(); }
	bytes rlp() const { RLPStream s(receipts.size()); for (TransactionReceipt const& i: receipts) i.streamRLP(s); size = s.out().size(); return s.out(); }

	TransactionReceipts receipts;
	mutable unsigned size = 0;
};

struct BlockHash
{
	BlockHash() {}
	BlockHash(h256 const& _h): value(_h) {}
	BlockHash(RLP const& _r) { value = _r.toHash<h256>(); }
	bytes rlp() const { return dev::rlp(value); }

	h256 value;
	static const unsigned size = 65;
};
struct TransactionAddress
{
	TransactionAddress() {}
	TransactionAddress(RLP const& _rlp) { blockHash = _rlp[0].toHash<h256>(); index = _rlp[1].toInt<unsigned>(); }
	bytes rlp() const { RLPStream s(2); s << blockHash << index; return s.out(); }

	explicit operator bool() const { return !!blockHash; }

	h256 blockHash;
	unsigned index = 0;

	static const unsigned size = 67;
};

}
}

