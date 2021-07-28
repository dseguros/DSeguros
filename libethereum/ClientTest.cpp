#include <libethereum/EthereumHost.h>
#include <libethereum/ClientTest.h>

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace p2p;

ClientTest& dev::eth::asClientTest(Interface& _c)
{
	return dynamic_cast<ClientTest&>(_c);
}

ClientTest* dev::eth::asClientTest(Interface* _c)
{
	return &dynamic_cast<ClientTest&>(*_c);
}

ClientTest::ClientTest(
	ChainParams const& _params,
	int _networkID,
	p2p::Host* _host,
	std::shared_ptr<GasPricer> _gpForAdoption,
	std::string const& _dbPath,
	WithExisting _forceAction,
	TransactionQueue::Limits const& _limits
):
	Client(_params, _networkID, _host, _gpForAdoption, _dbPath, _forceAction, _limits)
{}
