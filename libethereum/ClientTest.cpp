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
