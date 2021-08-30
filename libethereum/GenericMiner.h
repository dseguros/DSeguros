#pragma once

#include <libdevcore/Common.h>
#include <libdevcore/Log.h>
#include <libdevcore/Worker.h>
#include <libethcore/Common.h>

namespace dev
{

namespace eth
{

struct MineInfo: public WorkingProgress {};

inline std::ostream& operator<<(std::ostream& _out, WorkingProgress _p)
{
	_out << _p.rate() << " H/s = " <<  _p.hashes << " hashes / " << (double(_p.ms) / 1000) << " s";
	return _out;
}

template <class PoW> class GenericMiner;

/**
 * @brief Class for hosting one or more Miners.
 * @warning Must be implemented in a threadsafe manner since it will be called from multiple
 * miner threads.
 */
template <class PoW> class GenericFarmFace
{
public:
	using WorkPackage = typename PoW::WorkPackage;
	using Solution = typename PoW::Solution;
	using Miner = GenericMiner<PoW>;

	virtual ~GenericFarmFace() {}

	/**
	 * @brief Called from a Miner to note a WorkPackage has a solution.
	 * @param _p The solution.
	 * @param _wp The WorkPackage that the Solution is for; this will be reset if the work is accepted.
	 * @param _finder The miner that found it.
	 * @return true iff the solution was good (implying that mining should be .
	 */
	virtual bool submitProof(Solution const& _p, Miner* _finder) = 0;
};

template <class PoW> class GenericMiner
{
public:
	using WorkPackage = typename PoW::WorkPackage;
	using Solution = typename PoW::Solution;
	using FarmFace = GenericFarmFace<PoW>;
	using ConstructionInfo = std::pair<FarmFace*, unsigned>;

	GenericMiner(ConstructionInfo const& _ci):
		m_farm(_ci.first),
		m_index(_ci.second)
	{}
	virtual ~GenericMiner() {}

	// API FOR THE FARM TO CALL IN WITH

	void setWork(WorkPackage const& _work = WorkPackage())
	{
		auto old = m_work;
		{
			Guard l(x_work);
			m_work = _work;
		}
		if (!!_work)
		{
			DEV_TIMED_ABOVE("pause", 250)
				pause();
			DEV_TIMED_ABOVE("kickOff", 250)
				kickOff();
		}
		else if (!_work && !!old)
			pause();
		m_hashCount = 0;
	}

	uint64_t hashCount() const { return m_hashCount; }

	void resetHashCount() { m_hashCount = 0; }

	unsigned index() const { return m_index; }

}

}
}
