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

private:
	BlockChainSync& m_sync;
	RecursiveMutex& m_syncMutex;
	TransactionQueue& m_tq;
};

class EthereumHostData: public EthereumHostDataFace
{
public:
	EthereumHostData(BlockChain const& _chain, OverlayDB const& _db): m_chain(_chain), m_db(_db) {}

	pair<bytes, unsigned> blockHeaders(RLP const& _blockId, unsigned _maxHeaders, u256 _skip, bool _reverse) const override
	{
		auto numHeadersToSend = _maxHeaders;

		auto step = static_cast<unsigned>(_skip) + 1;
		assert(step > 0 && "step must not be 0");

		h256 blockHash;
		if (_blockId.size() == 32) // block id is a hash
		{
			blockHash = _blockId.toHash<h256>();
			clog(NetMessageSummary) << "GetBlockHeaders (block (hash): " << blockHash
				<< ", maxHeaders: " << _maxHeaders
				<< ", skip: " << _skip << ", reverse: " << _reverse << ")";

			if (!m_chain.isKnown(blockHash))
				blockHash = {};
			else if (!_reverse)
			{
				auto n = m_chain.number(blockHash);
				if (numHeadersToSend == 0)
					blockHash = {};
				else if (n != 0 || blockHash == m_chain.genesisHash())
				{
					auto top = n + uint64_t(step) * numHeadersToSend - 1;
					auto lastBlock = m_chain.number();
					if (top > lastBlock)
					{
						numHeadersToSend = (lastBlock - n) / step + 1;
						top = n + step * (numHeadersToSend - 1);
					}
					assert(top <= lastBlock && "invalid top block calculated");
					blockHash = m_chain.numberHash(static_cast<unsigned>(top)); // override start block hash with the hash of the top block we have
				}
				else
					blockHash = {};
			}
		}
		else // block id is a number
		{
			auto n = _blockId.toInt<bigint>();
			clog(NetMessageSummary) << "GetBlockHeaders (" << n
			<< "max: " << _maxHeaders
			<< "skip: " << _skip << (_reverse ? "reverse" : "") << ")";

			if (!_reverse)
			{
				auto lastBlock = m_chain.number();
				if (n > lastBlock || numHeadersToSend == 0)
					blockHash = {};
				else
				{
					bigint top = n + uint64_t(step) * (numHeadersToSend - 1);
					if (top > lastBlock)
					{
						numHeadersToSend = (lastBlock - static_cast<unsigned>(n)) / step + 1;
						top = n + step * (numHeadersToSend - 1);
					}
					assert(top <= lastBlock && "invalid top block calculated");
					blockHash = m_chain.numberHash(static_cast<unsigned>(top)); // override start block hash with the hash of the top block we have
				}
			}
			else if (n <= std::numeric_limits<unsigned>::max())
				blockHash = m_chain.numberHash(static_cast<unsigned>(n));
			else
				blockHash = {};
		}

pair<bytes, unsigned> blockBodies(RLP const& _blockHashes) const override
	{
		unsigned const count = static_cast<unsigned>(_blockHashes.itemCount());

		bytes rlp;
		unsigned n = 0;
		auto numBodiesToSend = std::min(count, c_maxBlocks);
		for (unsigned i = 0; i < numBodiesToSend && rlp.size() < c_maxPayload; ++i)
		{
			auto h = _blockHashes[i].toHash<h256>();
			if (m_chain.isKnown(h))
			{
				bytes blockBytes = m_chain.block(h);
				RLP block{blockBytes};
				RLPStream body;
				body.appendList(2);
				body.appendRaw(block[1].data()); // transactions
				body.appendRaw(block[2].data()); // uncles
				auto bodyBytes = body.out();
				rlp.insert(rlp.end(), bodyBytes.begin(), bodyBytes.end());
				++n;
			}
		}
		if (count > 20 && n == 0)
			clog(NetWarn) << "all" << count << "unknown blocks requested; peer on different chain?";
		else
			clog(NetMessageSummary) << n << "blocks known and returned;" << (numBodiesToSend - n) << "blocks unknown;" << (count > c_maxBlocks ? count - c_maxBlocks : 0) << "blocks ignored";

		return make_pair(rlp, n);
	}

	strings nodeData(RLP const& _dataHashes) const override
	{
		unsigned const count = static_cast<unsigned>(_dataHashes.itemCount());

		strings data;
		size_t payloadSize = 0;
		auto numItemsToSend = std::min(count, c_maxNodes);
		for (unsigned i = 0; i < numItemsToSend && payloadSize < c_maxPayload; ++i)
		{
			auto h = _dataHashes[i].toHash<h256>();
			auto node = m_db.lookup(h);
			if (!node.empty())
			{
				payloadSize += node.length();
				data.push_back(move(node));
			}
		}
		clog(NetMessageSummary) << data.size() << " nodes known and returned;" << (numItemsToSend - data.size()) << " unknown;" << (count > c_maxNodes ? count - c_maxNodes : 0) << " ignored";

		return data;
	}

	pair<bytes, unsigned> receipts(RLP const& _blockHashes) const override
	{
		unsigned const count = static_cast<unsigned>(_blockHashes.itemCount());

		bytes rlp;
		unsigned n = 0;
		auto numItemsToSend = std::min(count, c_maxReceipts);
		for (unsigned i = 0; i < numItemsToSend && rlp.size() < c_maxPayload; ++i)
		{
			auto h = _blockHashes[i].toHash<h256>();
			if (m_chain.isKnown(h))
			{
				auto const receipts = m_chain.receipts(h);
				auto receiptsRlpList = receipts.rlp();
				rlp.insert(rlp.end(), receiptsRlpList.begin(), receiptsRlpList.end());
				++n;
			}
		}
		clog(NetMessageSummary) << n << " receipt lists known and returned;" << (numItemsToSend - n) << " unknown;" << (count > c_maxReceipts ? count - c_maxReceipts : 0) << " ignored";

		return make_pair(rlp, n);
	}

private:
	BlockChain const& m_chain;
	OverlayDB const& m_db;
};

}
