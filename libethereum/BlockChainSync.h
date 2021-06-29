
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

};

}

}
