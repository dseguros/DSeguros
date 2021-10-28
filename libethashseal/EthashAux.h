#pragma once

#include <condition_variable>
#include <libethash/ethash.h>
#include <libdevcore/Log.h>
#include <libdevcore/Worker.h>
#include "EthashProofOfWork.h"
#include "Ethash.h"

namespace dev
{
namespace eth
{

struct DAGChannel: public LogChannel { static const char* name(); static const int verbosity = 1; };

class BlockHeader;

class EthashAux
{
public:
	~EthashAux();

	static EthashAux* get();

	struct LightAllocation
	{
		LightAllocation(h256 const& _seedHash);
		~LightAllocation();
		bytesConstRef data() const;
		EthashProofOfWork::Result compute(h256 const& _headerHash, Nonce const& _nonce) const;
		ethash_light_t light;
		uint64_t size;
	};
};


}
}
