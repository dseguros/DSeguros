#pragma once

#include <thread>
#include <list>
#include <atomic>
#include <libdevcore/Common.h>
#include <libdevcore/Worker.h>
#include <libethcore/Common.h>
#include <libethereum/GenericMiner.h>
#include <libethcore/BlockHeader.h>

namespace dev
{

namespace eth
{

template <class PoW>
class GenericFarm: public GenericFarmFace<PoW>
{
public:
	using WorkPackage = typename PoW::WorkPackage;
	using Solution = typename PoW::Solution;
	using Miner = GenericMiner<PoW>;

	struct SealerDescriptor
	{
		std::function<unsigned()> instances;
		std::function<Miner*(typename Miner::ConstructionInfo ci)> create;
	};

	~GenericFarm()
	{
		stop();
	}
};

}
}
