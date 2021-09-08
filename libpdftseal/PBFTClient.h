#pragma once

#include <libethereum/Client.h>

namespace dev
{
namespace eth
{

class PBFT;

DEV_SIMPLE_EXCEPTION(NotPBFTSealEngine);
DEV_SIMPLE_EXCEPTION(ChainParamsNotPBFT);
DEV_SIMPLE_EXCEPTION(InitFailed);

enum AccountType {
	EN_ACCOUNT_TYPE_NORMAL = 0,
	EN_ACCOUNT_TYPE_MINER = 1
};

class PBFTClient: public Client
{
public:
	/// Trivial forwarding constructor.
	PBFTClient(
	    ChainParams const& _params,
	    int _networkID,
	    p2p::Host* _host,
	    std::shared_ptr<GasPricer> _gpForAdoption,
	    std::string const& _dbPath = std::string(),
	    WithExisting _forceAction = WithExisting::Trust,
	    TransactionQueue::Limits const& _l = TransactionQueue::Limits {102400, 102400}
	);

};

}
}
