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

}
}
