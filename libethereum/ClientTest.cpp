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

void ClientTest::setChainParams(string const& _genesis)
{
	ChainParams params;
	try
	{
		params = params.loadConfig(_genesis);
		if (params.sealEngineName != "NoProof")
			BOOST_THROW_EXCEPTION(ChainParamsNotNoProof() << errinfo_comment("Provided configuration is not well formatted."));

		reopenChain(params, WithExisting::Kill);
		setAuthor(params.author); //for some reason author is not being set
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(ChainParamsInvalid() << errinfo_comment("Provided configuration is not well formatted."));
	}
}

bool ClientTest::addBlock(string const& _rlp)
{
	if (auto h = m_host.lock())
		h->noteNewBlocks();

	bytes rlpBytes = fromHex(_rlp, WhenError::Throw);
	RLP blockRLP(rlpBytes);
	return (m_bq.import(blockRLP.data(), true) == ImportResult::Success);
}
