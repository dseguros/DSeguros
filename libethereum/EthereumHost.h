#pragma once

#include <mutex>
#include <unordered_map>
#include <vector>
#include <unordered_set>
#include <memory>
#include <utility>
#include <thread>

#include <libdevcore/Guards.h>
#include <libdevcore/Worker.h>
#include <libethcore/Common.h>
#include <libp2p/Common.h>
#include <libdevcore/OverlayDB.h>
#include <libethcore/BlockHeader.h>
#include <libethereum/BlockChainSync.h>
#include "CommonNet.h"
#include "EthereumPeer.h"

namespace dev
{

class RLPStream;

namespace eth
{

class TransactionQueue;
class BlockQueue;
class BlockChainSync;

struct EthereumHostTrace: public LogChannel { static const char* name(); static const int verbosity = 6; };

/**
 * @brief The EthereumHost class
 * @warning None of this is thread-safe. You have been warned.
 * @doWork Syncs to peers and sends new blocks and transactions.
 */
class EthereumHost: public p2p::HostCapability<EthereumPeer>, Worker
{
public:
	/// Start server, but don't listen.
	EthereumHost(BlockChain const& _ch, OverlayDB const& _db, TransactionQueue& _tq, BlockQueue& _bq, u256 _networkId);

	/// Will block on network process events.
	virtual ~EthereumHost();

	unsigned protocolVersion() const { return c_protocolVersion; }
	u256 networkId() const { return m_networkId; }
	void setNetworkId(u256 _n) { m_networkId = _n; }

	void reset();
	/// Don't sync further - used only in test mode
	void completeSync();

	bool isSyncing() const;
	bool isBanned(p2p::NodeID const& _id) const { return !!m_banned.count(_id); }

	void noteNewTransactions() { m_newTransactions = true; }
	void noteNewBlocks() { m_newBlocks = true; }
	void onBlockImported(BlockHeader const& _info) { m_sync->onBlockImported(_info); }

	BlockChain const& chain() const { return m_chain; }
	OverlayDB const& db() const { return m_db; }
	BlockQueue& bq() { return m_bq; }
	BlockQueue const& bq() const { return m_bq; }
	SyncStatus status() const;
	h256 latestBlockSent() { return m_latestBlockSent; }
	static char const* stateName(SyncState _s) { return s_stateNames[static_cast<int>(_s)]; }

	static unsigned const c_oldProtocolVersion;
	void foreachPeer(std::function<bool(std::shared_ptr<EthereumPeer>)> const& _f) const;

protected:
	std::shared_ptr<p2p::Capability> newPeerCapability(std::shared_ptr<p2p::SessionFace> const& _s, unsigned _idOffset, p2p::CapDesc const& _cap, uint16_t _capID) override;

};

}

}
