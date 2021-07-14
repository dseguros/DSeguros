#pragma once

#include <thread>
#include <condition_variable>
#include <mutex>
#include <list>
#include <queue>
#include <atomic>
#include <string>
#include <array>
#include <libdevcore/Common.h>
#include <libdevcore/CommonIO.h>
#include <libdevcore/Guards.h>
#include <libdevcore/Worker.h>
#include <libethcore/SealEngine.h>
#include <libethcore/ABI.h>
#include <libp2p/Common.h>
#include "BlockChain.h"
#include "Block.h"
#include "CommonNet.h"
#include "ClientBase.h"

namespace dev
{
namespace eth
{

class Client;
class DownloadMan;

enum ClientWorkState
{
	Active = 0,
	Deleting,
	Deleted
};

struct ClientNote: public LogChannel { static const char* name(); static const int verbosity = 2; };
struct ClientChat: public LogChannel { static const char* name(); static const int verbosity = 4; };
struct ClientTrace: public LogChannel { static const char* name(); static const int verbosity = 7; };
struct ClientDetail: public LogChannel { static const char* name(); static const int verbosity = 14; };

struct ActivityReport
{
	unsigned ticks = 0;
	std::chrono::system_clock::time_point since = std::chrono::system_clock::now();
};

std::ostream& operator<<(std::ostream& _out, ActivityReport const& _r);

/**
 * @brief Main API hub for interfacing with Ethereum.
 */
class Client: public ClientBase, protected Worker
{
public:
	Client(
		ChainParams const& _params,
		int _networkID,
		p2p::Host* _host,
		std::shared_ptr<GasPricer> _gpForAdoption,
		std::string const& _dbPath = std::string(),
		WithExisting _forceAction = WithExisting::Trust,
		TransactionQueue::Limits const& _l = TransactionQueue::Limits{1024, 1024}
	);
	/// Destructor.
	virtual ~Client();

	/// Get information on this chain.
	ChainParams const& chainParams() const { return bc().chainParams(); }

	/// Resets the gas pricer to some other object.
	void setGasPricer(std::shared_ptr<GasPricer> _gp) { m_gp = _gp; }
	std::shared_ptr<GasPricer> gasPricer() const { return m_gp; }

        /// Blocks until all pending transactions have been processed.
	virtual void flushTransactions() override;

	/// Queues a block for import.
	ImportResult queueBlock(bytes const& _block, bool _isSafe = false);

	using Interface::call; // to remove warning about hiding virtual function
	/// Makes the given call. Nothing is recorded into the state. This cheats by creating a null address and endowing it with a lot of ETH.
	ExecutionResult call(Address _dest, bytes const& _data = bytes(), u256 _gas = 125000, u256 _value = 0, u256 _gasPrice = 1 * ether, Address const& _from = Address());

	/// Get the remaining gas limit in this block.
	virtual u256 gasLimitRemaining() const override { return m_postSeal.gasLimitRemaining(); }
	/// Get the gas bid price
	virtual u256 gasBidPrice() const override { return m_gp->bid(); }

	// [PRIVATE API - only relevant for base clients, not available in general]
	/// Get the block.
	dev::eth::Block block(h256 const& _blockHash, PopulationStatistics* o_stats) const;
	/// Get the state of the given block part way through execution, immediately before transaction
	/// index @a _txi.
	dev::eth::State state(unsigned _txi, h256 const& _block) const;
	/// Get the state of the currently pending block part way through execution, immediately before
	/// transaction index @a _txi.
	dev::eth::State state(unsigned _txi) const;
};

}
}
