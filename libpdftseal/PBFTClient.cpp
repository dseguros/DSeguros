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


