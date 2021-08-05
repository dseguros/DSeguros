#include "EthereumHost.h"

#include <chrono>
#include <thread>
#include <libdevcore/Common.h>
#include <libp2p/Host.h>
#include <libp2p/Session.h>
#include <libethcore/Exceptions.h>
#include "BlockChain.h"
#include "TransactionQueue.h"
#include "BlockQueue.h"
#include "EthereumPeer.h"
#include "BlockChainSync.h"

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace p2p;

unsigned const EthereumHost::c_oldProtocolVersion = 62; //TODO: remove this once v63+ is common
static unsigned const c_maxSendTransactions = 256;

char const* const EthereumHost::s_stateNames[static_cast<int>(SyncState::Size)] = {"NotSynced", "Idle", "Waiting", "Blocks", "State", "NewBlocks" };

#if defined(_WIN32)
const char* EthereumHostTrace::name() { return EthPurple "^" EthGray "  "; }
#else
const char* EthereumHostTrace::name() { return EthPurple "⧫" EthGray " "; }
#endif

namespace
{

class EthereumPeerObserver: public EthereumPeerObserverFace
{
public:
	EthereumPeerObserver(BlockChainSync& _sync, RecursiveMutex& _syncMutex, TransactionQueue& _tq): m_sync(_sync), m_syncMutex(_syncMutex), m_tq(_tq) {}

	void onPeerStatus(std::shared_ptr<EthereumPeer> _peer) override
	{
		RecursiveGuard l(m_syncMutex);
		try
		{
			m_sync.onPeerStatus(_peer);
		}
		catch (FailedInvariant const&)
		{
			// "fix" for https://github.com/ethereum/webthree-umbrella/issues/300
			clog(NetWarn) << "Failed invariant during sync, restarting sync";
			m_sync.restartSync();
		}
	}

	void onPeerTransactions(std::shared_ptr<EthereumPeer> _peer, RLP const& _r) override
	{
		unsigned itemCount = _r.itemCount();
		clog(EthereumHostTrace) << "Transactions (" << dec << itemCount << "entries)";
		m_tq.enqueue(_r, _peer->id());
	}

	void onPeerAborting() override
	{
		RecursiveGuard l(m_syncMutex);
		try
		{
			m_sync.onPeerAborting();
		}
		catch (Exception&)
		{
			cwarn << "Exception on peer destruciton: " << boost::current_exception_diagnostic_information();
		}
	}


	void onPeerBlockHeaders(std::shared_ptr<EthereumPeer> _peer, RLP const& _headers) override
	{
		RecursiveGuard l(m_syncMutex);
		try
		{
			m_sync.onPeerBlockHeaders(_peer, _headers);
		}
		catch (FailedInvariant const&)
		{
			// "fix" for https://github.com/ethereum/webthree-umbrella/issues/300
			clog(NetWarn) << "Failed invariant during sync, restarting sync";
			m_sync.restartSync();
		}
	}

	void onPeerBlockBodies(std::shared_ptr<EthereumPeer> _peer, RLP const& _r) override
	{
		RecursiveGuard l(m_syncMutex);
		try
		{
			m_sync.onPeerBlockBodies(_peer, _r);
		}
		catch (FailedInvariant const&)
		{
			// "fix" for https://github.com/ethereum/webthree-umbrella/issues/300
			clog(NetWarn) << "Failed invariant during sync, restarting sync";
			m_sync.restartSync();
		}
	}

	void onPeerNewHashes(std::shared_ptr<EthereumPeer> _peer, std::vector<std::pair<h256, u256>> const& _hashes) override
	{
		RecursiveGuard l(m_syncMutex);
		try
		{
			m_sync.onPeerNewHashes(_peer, _hashes);
		}
		catch (FailedInvariant const&)
		{
			// "fix" for https://github.com/ethereum/webthree-umbrella/issues/300
			clog(NetWarn) << "Failed invariant during sync, restarting sync";
			m_sync.restartSync();
		}
	}

	void onPeerNewBlock(std::shared_ptr<EthereumPeer> _peer, RLP const& _r) override
	{
		RecursiveGuard l(m_syncMutex);
		try
		{
			m_sync.onPeerNewBlock(_peer, _r);
		}
		catch (FailedInvariant const&)
		{
			// "fix" for https://github.com/ethereum/webthree-umbrella/issues/300
			clog(NetWarn) << "Failed invariant during sync, restarting sync";
			m_sync.restartSync();
		}
	}

	void onPeerNodeData(std::shared_ptr<EthereumPeer> /* _peer */, RLP const& _r) override
	{
		unsigned itemCount = _r.itemCount();
		clog(EthereumHostTrace) << "Node Data (" << dec << itemCount << "entries)";
	}

	void onPeerReceipts(std::shared_ptr<EthereumPeer> /* _peer */, RLP const& _r) override
	{
		unsigned itemCount = _r.itemCount();
		clog(EthereumHostTrace) << "Receipts (" << dec << itemCount << "entries)";
	}
};

}
