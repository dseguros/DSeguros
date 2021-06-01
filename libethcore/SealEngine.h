#pragma once

#include <functional>
#include <unordered_map>
#include <libdevcore/Guards.h>
#include <libdevcore/RLP.h>
#include "BlockHeader.h"
#include "Common.h"

namespace dev
{
namespace eth
{

class BlockHeader;
struct ChainOperationParams;
class Interface;
class PrecompiledFace;
class TransactionBase;
class EnvInfo;

class SealEngineFace
{
public:
	virtual ~SealEngineFace() {}

	virtual std::string name() const = 0;
	virtual unsigned revision() const { return 0; }
	virtual unsigned sealFields() const { return 0; }
	virtual bytes sealRLP() const { return bytes(); }
	virtual StringHashMap jsInfo(BlockHeader const&) const { return StringHashMap(); }

};

}
}


