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

	/**
	 * @brief Start a number of miners.
	 */
	bool start(std::string const& _sealer)
	{
		WriteGuard l(x_minerWork);
		if (!m_miners.empty() && m_lastSealer == _sealer)
			return true;
		if (!m_sealers.count(_sealer))
			return false;

		m_miners.clear();
		auto ins = m_sealers[_sealer].instances();
		m_miners.reserve(ins);
		for (unsigned i = 0; i < ins; ++i)
		{
			m_miners.push_back(std::shared_ptr<Miner>(m_sealers[_sealer].create(std::make_pair(this, i))));
			m_miners.back()->setWork(m_work);
		}
		m_isMining = true;
		m_lastSealer = _sealer;
		resetTimer();
		return true;
	}
	
	/**
	 * @brief Stop all mining activities.
	 */
	void stop()
	{
		WriteGuard l(x_minerWork);
		m_miners.clear();
		m_work.reset();
		m_isMining = false;
	}

	bool isMining() const
	{
		return m_isMining;
	}

	/**
	 * @brief Get information on the progress of mining this work package.
	 * @return The progress with mining so far.
	 */
	WorkingProgress const& miningProgress() const
	{
		WorkingProgress p;
		p.ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_lastStart).count();
		{
			ReadGuard l2(x_minerWork);
			for (auto const& i: m_miners)
				p.hashes += i->hashCount();
		}
		WriteGuard l(x_progress);
		m_progress = p;
		return m_progress;
	}
};

}
}
