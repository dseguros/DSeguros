#include "Client.h"
#include <chrono>
#include <memory>
#include <thread>
#include <boost/filesystem.hpp>
#include <libdevcore/Log.h>
#include <libp2p/Host.h>
#include "Defaults.h"
#include "Executive.h"
#include "EthereumHost.h"
#include "Block.h"
#include "TransactionQueue.h"
using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace p2p;

static_assert(BOOST_VERSION == 106300, "Wrong boost headers version");

std::ostream& dev::eth::operator<<(std::ostream& _out, ActivityReport const& _r)
{
	_out << "Since " << toString(_r.since) << " (" << std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - _r.since).count();
	_out << "): " << _r.ticks << "ticks";
	return _out;
}
