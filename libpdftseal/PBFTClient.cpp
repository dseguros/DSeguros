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
