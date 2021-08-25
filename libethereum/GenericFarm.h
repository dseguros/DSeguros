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

	/**
	 * @brief Sets the current mining mission.
	 * @param _wp The work package we wish to be mining.
	 */
	void setWork(WorkPackage const& _wp)
	{
		WriteGuard l(x_minerWork);
		if (_wp.headerHash == m_work.headerHash)
			return;
		m_work = _wp;
		for (auto const& m: m_miners)
			m->setWork(m_work);
		resetTimer();
	}

	void setSealers(std::map<std::string, SealerDescriptor> const& _sealers) { m_sealers = _sealers; }
};

}
}
