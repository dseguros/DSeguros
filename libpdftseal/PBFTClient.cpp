//#include <libdevcore/easylog.h>
#include <libethereum/EthereumHost.h>
//#include <libethereum/NodeConnParamsManagerApi.h>
#include <libp2p/Host.h>
#include "PBFTClient.h"
#include "PBFT.h"
#include "PBFTHost.h"
//#include <libdevcore/easylog.h>
//#include <libethereum/StatLog.h>
using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace p2p;

PBFTClient& dev::eth::asPBFTClient(Interface& _c)
{
	if (dynamic_cast<PBFT*>(_c.sealEngine()))
		return dynamic_cast<PBFTClient&>(_c);
	throw NotPBFTSealEngine();
}

PBFTClient* dev::eth::asPBFTClient(Interface* _c)
{
	if (dynamic_cast<PBFT*>(_c->sealEngine()))
		return &dynamic_cast<PBFTClient&>(*_c);
	throw NotPBFTSealEngine();
lient::PBFTClient(
    ChainParams const& _params,
    int _networkID,
    p2p::Host* _host,
    std::shared_ptr<GasPricer> _gpForAdoption,
    std::string const& _dbPath,
    WithExisting _forceAction,
    TransactionQueue::Limits const& _limits
):
	Client(_params, _networkID, _host, _gpForAdoption, _dbPath, _forceAction, _limits)
{
	// will throw if we're not an PBFT seal engine.
	asPBFTClient(*this);

	init(_params, _host);

	m_empty_block_flag = false;
	m_exec_time_per_tx = 0;
	m_last_exec_finish_time = utcTime();
}

PBFTClient::~PBFTClient() {
	pbft()->cancelGeneration();
	stopWorking();
}

void PBFTClient::init(ChainParams const& _params, p2p::Host *_host) {
	m_params = _params;
	//m_working.setEvmCoverLog(m_params.evmCoverLog);
	//m_working.setEvmEventLog(m_params.evmEventLog);


	// register PBFTHost
	auto pbft_host = _host->registerCapability(make_shared<PBFTHost>([this](unsigned _id, std::shared_ptr<Capability> _peer, RLP const & _r) {
		pbft()->onPBFTMsg(_id, _peer, _r);
	}));

	pbft()->initEnv(pbft_host, &m_bc, &m_stateDB, &m_bq, _host->keyPair(), static_cast<unsigned>(sealEngine()->getIntervalBlockTime()) * 3);
	pbft()->setOmitEmptyBlock(m_omit_empty_block);

	pbft()->reportBlock(bc().info(), bc().details().totalDifficulty);

	pbft()->onSealGenerated([ this ](bytes const & _block, bool _isOurs) {
		if (!submitSealed(_block, _isOurs))
			cwarn << "Submitting block failed...";
	});

	pbft()->onViewChange([this]() {
		DEV_WRITE_GUARDED(x_working)
		{
			if (m_working.isSealed()) {
				m_working.resetCurrent();
			}
		}
	});

	cdebug << "Init PBFTClient success";
}

PBFT* PBFTClient::pbft() const
{
	return dynamic_cast<PBFT*>(Client::sealEngine());
}

void PBFTClient::startSealing() {
	setName("Client");
	pbft()->reportBlock(bc().info(), bc().details().totalDifficulty);
	pbft()->startGeneration();
	//Client::startSealing();
	if (m_wouldSeal == true)
		return;
	cdebug << "Mining Beneficiary: " << author();
	if (author())
	{
		m_wouldSeal = true;
		m_signalled.notify_all();
	}
	else
		cdebug << "You need to set an author in order to seal!";
}

void PBFTClient::stopSealing() {
	Client::stopSealing();
	pbft()->cancelGeneration();
}

void PBFTClient::syncBlockQueue() {
	Client::syncBlockQueue();

	pbft()->reportBlock(bc().info(), bc().details().totalDifficulty);

	m_empty_block_flag = false;
	pbft()->setOmitEmptyBlock(m_omit_empty_block);

	DEV_WRITE_GUARDED(x_working)
	{
		if (m_working.isSealed() && m_working.info().number() <= bc().info().number()) {
			m_working.resetCurrent();
		}
	}
	// start new block log
	//PBFTFlowLog(pbft()->getHighestBlock().number() + pbft()->view(), 
	//	"new block", (int)pbft()->isLeader(), true);
}

void PBFTClient::onTransactionQueueReady() {
	m_syncTransactionQueue = true;
	m_signalled.notify_all();
	// info EhtereumHost to broadcast txs EthereumHost
	if (auto h = m_host.lock()) {
		h->noteNewTransactions();
	}
}

void PBFTClient::syncTransactionQueue(u256 const& _max_block_txs)
{
	(void)_max_block_txs;
	TransactionReceipts newPendingReceipts;
	//DEV_WRITE_GUARDED(x_working)
	{
		if (m_working.isSealed())
		{
			cdebug << "Skipping txq sync for a sealed block.";
			return;
		}
		if ( m_working.pending().size() >= m_maxBlockTranscations )
		{
			cdebug << "Skipping txq sync for Full block .";
			return;
		}

		tie(newPendingReceipts, m_syncTransactionQueue) = m_working.sync(bc(), m_tq, *m_gp);
	}
	
	if (!newPendingReceipts.empty())
	{
		DEV_WRITE_GUARDED(x_postSeal)
		m_postSeal = m_working; //add this to let RPC interface "eth_pendingTransactions" to get value RPCeth_pendingTransactions	
	}
}

void PBFTClient::executeTransaction()
{
	Timer timer;

	h256Hash changeds;
	TransactionReceipts newPendingReceipts;

	//DEV_WRITE_GUARDED(x_working)
	{
		newPendingReceipts = m_working.exec(bc(), m_tq);
	}

	if (newPendingReceipts.empty())
	{
		auto s = m_tq.status();
		cdebug << "No transactions to process. " << m_working.pending().size() << " pending, " << s.current << " queued, " << s.future << " future, " << s.unverified << " unverified";
		return;
	}

	//DEV_READ_GUARDED(x_working)
	DEV_WRITE_GUARDED(x_postSeal)
	m_postSeal = m_working;

	DEV_READ_GUARDED(x_postSeal)
	for (size_t i = 0; i < newPendingReceipts.size(); i++)
		appendFromNewPending(newPendingReceipts[i], changeds, m_postSeal.pending()[i].sha3());

	// Tell farm about new transaction (i.e. restart mining).
	onPostStateChanged();

	// Tell watches about the new transactions.
	noteChanged(changeds);

	// Tell network about the new transactions.
	//if (auto h = m_host.lock())
	//	h->noteNewTransactions();

	cdebug << "Processed " << newPendingReceipts.size() << " transactions in" << (timer.elapsed() * 1000) << "(" << (bool)m_syncTransactionQueue << ")";
}

void PBFTClient::doWork(bool _doWait)
{
	bool t = true;
	if (m_syncBlockQueue.compare_exchange_strong(t, false))
		syncBlockQueue();

	if (m_needStateReset)
	{
		resetState();
		m_needStateReset = false;
	}

	tick();

	rejigSealing();

	callQueuedFunctions();

	if (!m_syncBlockQueue && _doWait)
	{
		std::unique_lock<std::mutex> l(x_signalled);
		m_signalled.wait_for(l, chrono::milliseconds(10));
	}
}

void PBFTClient::rejigSealing() {
	bool would_seal = m_wouldSeal && (pbft()->accountType() == EN_ACCOUNT_TYPE_MINER);
	bool is_major_syncing = isMajorSyncing();
	if (would_seal && !is_major_syncing)
	{
		if (pbft()->shouldSeal(this)) // am i leader? leader？
		{
			uint64_t tx_num = 0;
			bytes block_data;
			u256 max_block_txs = m_maxBlockTranscations;
			DEV_WRITE_GUARDED(x_working)
			{
				if (m_working.isSealed()) {
					cdebug << "Tried to seal sealed block...";
					return;
				}

				tx_num = m_working.pending().size();
				//}

				// 
				/*auto last_exec_finish_time = m_last_exec_finish_time;
				if (last_exec_finish_time < pbft()->lastExecFinishTime()) {
					last_exec_finish_time = pbft()->lastExecFinishTime();
				}
				auto passed_time = utcTime() - last_exec_finish_time;
				auto left_time = m_empty_block_flag ? static_cast<uint64_t>(sealEngine()->getIntervalBlockTime()) : 0;
				if (passed_time < sealEngine()->getIntervalBlockTime()) {
					left_time = static_cast<uint64_t>(sealEngine()->getIntervalBlockTime()) - passed_time;
				}*/
				auto last_exec_finish_time = m_last_exec_finish_time < pbft()->lastExecFinishTime() ? pbft()->lastExecFinishTime() : m_last_exec_finish_time;
				//auto passed_time = pbft()->view() == 0 ? (utcTime() - last_exec_finish_time) : (utcTime() - pbft()->lastConsensusTime());
				auto passed_time = 0;
				if (pbft()->view() != 0) {
					// empty block or other error reason to let viewchange, reset new time ，。 
					passed_time = utcTime() - pbft()->lastConsensusTime();
				} else {
					if (pbft()->lastConsensusTime() - last_exec_finish_time >= sealEngine()->getIntervalBlockTime()) {
						// timeout in consensus phases, reset new time ，
						passed_time = utcTime() - pbft()->lastConsensusTime();
					} else {
						passed_time = utcTime() - last_exec_finish_time;
					}
				}
				auto left_time = 0;
				if (passed_time < sealEngine()->getIntervalBlockTime()) {
					left_time = static_cast<uint64_t>(sealEngine()->getIntervalBlockTime()) - passed_time;
				}

				if (m_exec_time_per_tx != 0) {
					max_block_txs = static_cast<uint64_t>(left_time / m_exec_time_per_tx);
					if (left_time > 0 && left_time < m_exec_time_per_tx) {
						max_block_txs = 1; // try pack one tx at least ，
					}
					if (max_block_txs > m_maxBlockTranscations) {
						max_block_txs = m_maxBlockTranscations;
					}
				}

				cdebug << "last_exec=" << last_exec_finish_time << ",passed_time=" << passed_time << ",left=" << left_time << ",max_block_txs=" << max_block_txs << ",tx_num=" << tx_num;

				bool t = true;
				if (tx_num < max_block_txs && !isSyncing() && !m_remoteWorking && m_syncTransactionQueue.compare_exchange_strong(t, false)) {
					syncTransactionQueue(max_block_txs);
				}

				//DEV_WRITE_GUARDED(x_working)
				{
					tx_num = m_working.pending().size();
					if (tx_num < max_block_txs && utcTime() - pbft()->lastConsensusTime() < sealEngine()->getIntervalBlockTime()) {
						cdebug << "Wait for next interval, tx:" << tx_num;
						return;
					}
					// issue block 
					m_working.resetCurrentTime();
					m_working.setIndex(pbft()->nodeIdx());
					m_working.setNodeList(pbft()->getMinerNodeList());
					m_working.commitToSeal(bc(), m_extraData);

					m_sealingInfo = m_working.info();
					RLPStream ts;
					m_sealingInfo.streamRLP(ts, WithoutSeal);

					if (!m_working.sealBlock(&ts.out(), block_data)) {
						cwarn << "Error: sealBlock failed 1";
						m_working.resetCurrent();
						return;
					}
				}
				// sealed log
				stringstream ss;
				ss << "hash:" << m_sealingInfo.hash(WithoutSeal).abridged() 
					<< " height:" << m_sealingInfo.number() << " txnum:" << tx_num;
				//PBFTFlowLog(pbft()->getHighestBlock().number() + pbft()->view(), ss.str(), tx_num == 0);
			}
			// broadcast which not contains execution result （）
			cdebug << "+++++++++++++++++++++++++++ Generating seal on" << m_sealingInfo.hash(WithoutSeal) << "#" << m_sealingInfo.number() << "tx:" << tx_num << ",maxtx:" << max_block_txs << "time:" << utcTime();

			u256 view = 0;
			bool generate_ret = pbft()->generateSeal(m_sealingInfo, block_data, view);

			// empty block 
			if (generate_ret && tx_num == 0 && m_omit_empty_block) {
				m_empty_block_flag = true;
				pbft()->changeViewForEmptyBlockWithLock();
			}

			DEV_WRITE_GUARDED(x_working)
			{
				if (!generate_ret) {
					m_working.resetCurrent();
					return;
				}

				if (tx_num == 0 && m_omit_empty_block) {
					m_working.resetCurrent();
					return;
				}

				if (tx_num != m_working.pending().size()) {// if txs number is not equal, it may be reset reset
					m_working.resetCurrent();
					return;
				}

				// run txs 
				auto start_exec_time = utcTime();

				cdebug << "start exec tx, blk=" << m_sealingInfo.number() << ", time=" << start_exec_time;

				try {
					executeTransaction();
				} catch (Exception &e) {
					cwarn << "executeTransaction exception " << e.what();
					m_working.resetCurrent();
					return;
				}

				//DEV_WRITE_GUARDED(x_working)
				{
					m_working.commitToSealAfterExecTx(bc());

					bc().addBlockCache(m_working, m_working.info().difficulty());

					m_sealingInfo = m_working.info();
					RLPStream ts2;
					m_sealingInfo.streamRLP(ts2, WithoutSeal);

					if (!m_working.sealBlock(ts2.out())) {
						cwarn << "Error: sealBlock failed 2";
						m_working.resetCurrent();
						return;
					}
				}

				m_last_exec_finish_time = utcTime();
				if (tx_num != 0) {
					auto exec_time_per_tx = (float)(m_last_exec_finish_time - start_exec_time) / tx_num;
					m_exec_time_per_tx = (m_exec_time_per_tx + exec_time_per_tx) / 2;
					if (m_exec_time_per_tx >= (float)sealEngine()->getIntervalBlockTime()) {
						m_exec_time_per_tx = (float)sealEngine()->getIntervalBlockTime();
					}
				}
				cdebug << "finish exec blk=" << m_sealingInfo.number() << ",finish_exec=" << m_last_exec_finish_time << ",per=" << m_exec_time_per_tx << ",tx_num=" << tx_num;
			}

			DEV_READ_GUARDED(x_working)
			{
				if (!m_working.isSealed()) {
					cwarn << "m_working is not sealed, may be reset by view change";
					return;
				}

				DEV_WRITE_GUARDED(x_postSeal)
				m_postSeal = m_working;
				// execed log
				// TODO FLAG3 m_sealingInfo.hash(WithoutSeal).abridged() => m_sealingInfo.hash(WithoutSeal)
				// PBFTFlowLog(pbft()->getHighestBlock().number() + pbft()->view(), "hash:" + m_sealingInfo.hash(WithoutSeal).abridged());
				stringstream ss;
				ss << "hash:" << m_sealingInfo.hash(WithoutSeal) << " height:" << m_sealingInfo.number();
				//PBFTFlowLog(pbft()->getHighestBlock().number() + pbft()->view(), ss.str());

				cdebug << "************************** Generating sign on" << m_sealingInfo.hash(WithoutSeal) << "#" << m_sealingInfo.number() << "tx:" << tx_num << "time:" << utcTime();

				if (!pbft()->generateCommit(m_sealingInfo, m_working.blockData(), view)) {
					//do nothing, wait timeout & viewchange
				}
			}

		}
	}
}
