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

}
}
