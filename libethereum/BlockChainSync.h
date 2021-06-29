
#pragma once

#include <mutex>
#include <unordered_map>

#include <libdevcore/Guards.h>
#include <libethcore/Common.h>
#include <libethcore/BlockHeader.h>
#include <libp2p/Common.h>
#include "CommonNet.h"

namespace dev
{

class RLPStream;

namespace eth
{

class EthereumHost;
class BlockQueue;
class EthereumPeer;

/**
 * @brief Base BlockChain synchronization strategy class.
 * Syncs to peers and keeps up to date. Base class handles blocks downloading but does not contain any details on state transfer logic.
 */
class BlockChainSync: public HasInvariants
{
public:
	BlockChainSync(EthereumHost& _host);
	~BlockChainSync();
	void abortSync(); ///< Abort all sync activity

	/// @returns true is Sync is in progress
	bool isSyncing() const;

	/// Restart sync
	void restartSync();

	/// Called after all blocks have been downloaded
	/// Public only for test mode
	void completeSync();

        /// Called by peer to report status
	void onPeerStatus(std::shared_ptr<EthereumPeer> _peer);

	/// Called by peer once it has new block headers during sync
	void onPeerBlockHeaders(std::shared_ptr<EthereumPeer> _peer, RLP const& _r);

	/// Called by peer once it has new block bodies
	void onPeerBlockBodies(std::shared_ptr<EthereumPeer> _peer, RLP const& _r);

	/// Called by peer once it has new block bodies
	void onPeerNewBlock(std::shared_ptr<EthereumPeer> _peer, RLP const& _r);


        void onPeerNewHashes(std::shared_ptr<EthereumPeer> _peer, std::vector<std::pair<h256, u256>> const& _hashes);

	/// Called by peer when it is disconnecting
	void onPeerAborting();

	/// Called when a blockchain has imported a new block onto the DB
	void onBlockImported(BlockHeader const& _info);

	/// @returns Synchonization status
	SyncStatus status() const;

	static char const* stateName(SyncState _s) { return s_stateNames[static_cast<int>(_s)]; }
};

}

}
