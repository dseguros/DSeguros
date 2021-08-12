#include "EthereumPeer.h"

#include <chrono>
#include <libdevcore/Common.h>
#include <libethcore/Exceptions.h>
#include <libp2p/Session.h>
#include <libp2p/Host.h>
#include "EthereumHost.h"

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace p2p;

static const unsigned c_maxIncomingNewHashes = 1024;
static const unsigned c_maxHeadersToSend = 1024;


static string toString(Asking _a)
{
	switch (_a)
	{
	case Asking::BlockHeaders: return "BlockHeaders";
	case Asking::BlockBodies: return "BlockBodies";
	case Asking::NodeData: return "NodeData";
	case Asking::Receipts: return "Receipts";
	case Asking::Nothing: return "Nothing";
	case Asking::State: return "State";
	}
	return "?";
}

EthereumPeer::EthereumPeer(std::shared_ptr<SessionFace> _s, HostCapabilityFace* _h, unsigned _i, CapDesc const& _cap, uint16_t _capID):
	Capability(_s, _h, _i, _capID),
	m_peerCapabilityVersion(_cap.second)
{
	session()->addNote("manners", isRude() ? "RUDE" : "nice");
}

EthereumPeer::~EthereumPeer()
{
	if (m_asking != Asking::Nothing)
	{
		clog(NetAllDetail) << "Peer aborting while being asked for " << ::toString(m_asking);
		setRude();
	}
	abortSync();
}

void EthereumPeer::init(unsigned _hostProtocolVersion, u256 _hostNetworkId, u256 _chainTotalDifficulty, h256 _chainCurrentHash, h256 _chainGenesisHash, shared_ptr<EthereumHostDataFace> _hostData, shared_ptr<EthereumPeerObserverFace> _observer)
{
	m_hostData = _hostData;
	m_observer = _observer;
	m_hostProtocolVersion = _hostProtocolVersion;
	requestStatus(_hostNetworkId, _chainTotalDifficulty, _chainCurrentHash, _chainGenesisHash);
}

bool EthereumPeer::isRude() const
{
	auto s = session();
	if (s)
		return s->repMan().isRude(*s, name());
	return false;
}

unsigned EthereumPeer::askOverride() const
{
	std::string static const badGeth = "Geth/v0.9.27";
	auto s = session();
	if (!s)
		return c_maxBlocksAsk;
	if (s->info().clientVersion.substr(0, badGeth.size()) == badGeth)
		return 1;
	bytes const& d = s->repMan().data(*s, name());
	return d.empty() ? c_maxBlocksAsk : RLP(d).toInt<unsigned>(RLP::LaissezFaire);
}

void EthereumPeer::setRude()
{
	auto s = session();
	if (!s)
		return;
	auto old = askOverride();
	s->repMan().setData(*s, name(), rlp(askOverride() / 2 + 1));
	cnote << "Rude behaviour; askOverride now" << askOverride() << ", was" << old;
	s->repMan().noteRude(*s, name());
	session()->addNote("manners", "RUDE");
}

void EthereumPeer::requestBlockHeaders(h256 const& _startHash, unsigned _count, unsigned _skip, bool _reverse)
{
	if (m_asking != Asking::Nothing)
	{
		clog(NetWarn) << "Asking headers while requesting " << ::toString(m_asking);
	}
	setAsking(Asking::BlockHeaders);
	RLPStream s;
	prep(s, GetBlockHeadersPacket, 4) << _startHash << _count << _skip << (_reverse ? 1 : 0);
	clog(NetMessageDetail) << "Requesting " << _count << " block headers starting from " << _startHash << (_reverse ? " in reverse" : "");
	m_lastAskedHeaders = _count;
	sealAndSend(s);
}


void EthereumPeer::requestBlockBodies(h256s const& _blocks)
{
	requestByHashes(_blocks, Asking::BlockBodies, GetBlockBodiesPacket);
}

void EthereumPeer::requestNodeData(h256s const& _hashes)
{
	requestByHashes(_hashes, Asking::NodeData, GetNodeDataPacket);
}

void EthereumPeer::requestReceipts(h256s const& _blocks)
{
	requestByHashes(_blocks, Asking::Receipts, GetReceiptsPacket);
}

void EthereumPeer::requestByHashes(h256s const& _hashes, Asking _asking, SubprotocolPacketType _packetType)
{
	if (m_asking != Asking::Nothing)
	{
		clog(NetWarn) << "Asking "<< ::toString(_asking) << " while requesting " << ::toString(m_asking);
	}
	setAsking(_asking);
	if (_hashes.size())
	{
		RLPStream s;
		prep(s, _packetType, _hashes.size());
		for (auto const& i: _hashes)
			s << i;
		sealAndSend(s);
	}
	else
		setIdle();
}
