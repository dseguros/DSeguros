#pragma once

#include <mutex>
#include <array>
#include <memory>
#include <utility>

#include <libdevcore/RLP.h>
#include <libdevcore/Guards.h>
#include <libethcore/Common.h>
#include <libp2p/Capability.h>
#include "CommonNet.h"

namespace dev
{
namespace eth
{

class EthereumPeerObserverFace
{
public:
	virtual ~EthereumPeerObserverFace() {}

	virtual void onPeerStatus(std::shared_ptr<EthereumPeer> _peer) = 0;

	virtual void onPeerTransactions(std::shared_ptr<EthereumPeer> _peer, RLP const& _r) = 0;

	virtual void onPeerBlockHeaders(std::shared_ptr<EthereumPeer> _peer, RLP const& _headers) = 0;

	virtual void onPeerBlockBodies(std::shared_ptr<EthereumPeer> _peer, RLP const& _r) = 0;

	virtual void onPeerNewHashes(std::shared_ptr<EthereumPeer> _peer, std::vector<std::pair<h256, u256>> const& _hashes) = 0;

	virtual void onPeerNewBlock(std::shared_ptr<EthereumPeer> _peer, RLP const& _r) = 0;

	virtual void onPeerNodeData(std::shared_ptr<EthereumPeer> _peer, RLP const& _r) = 0;

	virtual void onPeerReceipts(std::shared_ptr<EthereumPeer> _peer, RLP const& _r) = 0;

	virtual void onPeerAborting() = 0;
};

class EthereumHostDataFace
{
public:
	virtual ~EthereumHostDataFace() {}

	virtual std::pair<bytes, unsigned> blockHeaders(RLP const& _blockId, unsigned _maxHeaders, u256 _skip, bool _reverse) const = 0;

	virtual std::pair<bytes, unsigned> blockBodies(RLP const& _blockHashes) const = 0;

	virtual strings nodeData(RLP const& _dataHashes) const = 0;

	virtual std::pair<bytes, unsigned> receipts(RLP const& _blockHashes) const = 0;
};

/**
 * @brief The EthereumPeer class
 * @todo Document fully.
 * @todo make state transitions thread-safe.
 */
class EthereumPeer: public p2p::Capability
{
	friend class EthereumHost; //TODO: remove this
	friend class BlockChainSync; //TODO: remove this

public:
	/// Basic constructor.
	EthereumPeer(std::shared_ptr<p2p::SessionFace> _s, p2p::HostCapabilityFace* _h, unsigned _i, p2p::CapDesc const& _cap, uint16_t _capID);

	/// Basic destructor.
	virtual ~EthereumPeer();

	/// What is our name?
	static std::string name() { return "eth"; }

	/// What is our version?
	static u256 version() { return c_protocolVersion; }

	/// How many message types do we have?
	static unsigned messageCount() { return PacketCount; }

	void init(unsigned _hostProtocolVersion, u256 _hostNetworkId, u256 _chainTotalDifficulty, h256 _chainCurrentHash, h256 _chainGenesisHash, std::shared_ptr<EthereumHostDataFace> _hostData, std::shared_ptr<EthereumPeerObserverFace> _observer);

	p2p::NodeID id() const { return session()->id(); }

	/// Abort sync and reset fetch
	void setIdle();

	/// Request hashes for given parent hash.
	void requestBlockHeaders(h256 const& _startHash, unsigned _count, unsigned _skip, bool _reverse);
	void requestBlockHeaders(unsigned _startNumber, unsigned _count, unsigned _skip, bool _reverse);

	/// Request specified blocks from peer.
	void requestBlockBodies(h256s const& _blocks);

	/// Request values for specified keys from peer.
	void requestNodeData(h256s const& _hashes);

	/// Request receipts for specified blocks from peer.
	void requestReceipts(h256s const& _blocks);

	/// Check if this node is rude.
	bool isRude() const;

	/// Set that it's a rude node.
	void setRude();

	/// Abort the sync operation.
	void abortSync();
};

}
}
