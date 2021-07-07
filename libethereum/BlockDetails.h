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

}
}

