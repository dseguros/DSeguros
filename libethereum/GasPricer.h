#pragma once

#include <libethcore/Common.h>

namespace dev
{
namespace eth
{

class Block;
class BlockChain;

enum class TransactionPriority
{
	Lowest = 0,
	Low = 2,
	Medium = 4,
	High = 6,
	Highest = 8
};

static const u256 DefaultGasPrice = 20 * shannon;

class GasPricer
{
public:
	GasPricer() = default;
	virtual ~GasPricer() = default;

	virtual u256 ask(Block const&) const = 0;
	virtual u256 bid(TransactionPriority _p = TransactionPriority::Medium) const = 0;

	virtual void update(BlockChain const&) {}
};
}
}
