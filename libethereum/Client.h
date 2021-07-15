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

	/// Get the object representing the current state of Ethereum.
	dev::eth::Block postState() const { ReadGuard l(x_postSeal); return m_postSeal; }
	/// Get the object representing the current canonical blockchain.
	BlockChain const& blockChain() const { return bc(); }
	/// Get some information on the block queue.
	BlockQueueStatus blockQueueStatus() const { return m_bq.status(); }
	/// Get some information on the block syncing.
	SyncStatus syncStatus() const override;
	/// Get the block queue.
	BlockQueue const& blockQueue() const { return m_bq; }
	/// Get the block queue.
	OverlayDB const& stateDB() const { return m_stateDB; }
	/// Get some information on the transaction queue.
	TransactionQueue::Status transactionQueueStatus() const { return m_tq.status(); }
	TransactionQueue::Limits transactionQueueLimits() const { return m_tq.limits(); }

	/// Freeze worker thread and sync some of the block queue.
	std::tuple<ImportRoute, bool, unsigned> syncQueue(unsigned _max = 1);

	// Sealing stuff:
	// Note: "mining"/"miner" is deprecated. Use "sealing"/"sealer".

	virtual Address author() const override { ReadGuard l(x_preSeal); return m_preSeal.author(); }
	virtual void setAuthor(Address const& _us) override { WriteGuard l(x_preSeal); m_preSeal.setAuthor(_us); }

	/// Type of sealers available for this seal engine.
	strings sealers() const { return sealEngine()->sealers(); }
	/// Current sealer in use.
	std::string sealer() const { return sealEngine()->sealer(); }
	/// Change sealer.
	void setSealer(std::string const& _id) { sealEngine()->setSealer(_id); if (wouldSeal()) startSealing(); }
	/// Review option for the sealer.
	bytes sealOption(std::string const& _name) const { return sealEngine()->option(_name); }
	/// Set option for the sealer.
	bool setSealOption(std::string const& _name, bytes const& _value) { auto ret = sealEngine()->setOption(_name, _value); if (wouldSeal()) startSealing(); return ret; }

	/// Start sealing.
	void startSealing() override;
	/// Stop sealing.
	void stopSealing() override { m_wouldSeal = false; }
	/// Are we sealing now?
	bool wouldSeal() const override { return m_wouldSeal; }

	/// Are we updating the chain (syncing or importing a new block)?
	bool isSyncing() const override;
	/// Are we syncing the chain?
	bool isMajorSyncing() const override;

	/// Gets the network id.
	u256 networkId() const override;
	/// Sets the network id.
	void setNetworkId(u256 const& _n) override;

	/// Get the seal engine.
	SealEngineFace* sealEngine() const override { return bc().sealEngine(); }

	// Debug stuff:

	DownloadMan const* downloadMan() const;
	/// Clears pending transactions. Just for debug use.
	void clearPending();
	/// Kills the blockchain. Just for debug use.
	void killChain() { reopenChain(WithExisting::Kill); }
	/// Reloads the blockchain. Just for debug use.
	void reopenChain(ChainParams const& _p, WithExisting _we = WithExisting::Trust);
	void reopenChain(WithExisting _we);
	/// Retries all blocks with unknown parents.
	void retryUnknown() { m_bq.retryAllUnknown(); }
	/// Get a report of activity.
	ActivityReport activityReport() { ActivityReport ret; std::swap(m_report, ret); return ret; }
	/// Set the extra data that goes into sealed blocks.
	void setExtraData(bytes const& _extraData) { m_extraData = _extraData; }
	/// Rewind to a prior head.
	void rewind(unsigned _n);
	/// Rescue the chain.
	void rescue() { bc().rescue(m_stateDB); }

	/// Queues a function to be executed in the main thread (that owns the blockchain, etc).
	void executeInMainThread(std::function<void()> const& _function);

	virtual Block block(h256 const& _block) const override;
	using ClientBase::block;

protected:
	/// Perform critical setup functions.
	/// Must be called in the constructor of the finally derived class.
	void init(p2p::Host* _extNet, std::string const& _dbPath, WithExisting _forceAction, u256 _networkId);

	/// InterfaceStub methods
	BlockChain& bc() override { return m_bc; }
	BlockChain const& bc() const override { return m_bc; }

	/// Returns the state object for the full block (i.e. the terminal state) for index _h.
	/// Works properly with LatestBlock and PendingBlock.
	virtual Block preSeal() const override { ReadGuard l(x_preSeal); return m_preSeal; }
	virtual Block postSeal() const override { ReadGuard l(x_postSeal); return m_postSeal; }
	virtual void prepareForTransaction() override;

	/// Collate the changed filters for the bloom filter of the given pending transaction.
	/// Insert any filters that are activated into @a o_changed.
	void appendFromNewPending(TransactionReceipt const& _receipt, h256Hash& io_changed, h256 _sha3);

	/// Collate the changed filters for the hash of the given block.
	/// Insert any filters that are activated into @a o_changed.
	void appendFromBlock(h256 const& _blockHash, BlockPolarity _polarity, h256Hash& io_changed);

	/// Record that the set of filters @a _filters have changed.
	/// This doesn't actually make any callbacks, but incrememnts some counters in m_watches.
	void noteChanged(h256Hash const& _filters);

	/// Submit
	virtual bool submitSealed(bytes const& _s);
};

}
}
