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

#if defined(_WIN32)
const char* ClientNote::name() { return EthTeal "^" EthBlue " i"; }
const char* ClientChat::name() { return EthTeal "^" EthWhite " o"; }
const char* ClientTrace::name() { return EthTeal "^" EthGray " O"; }
const char* ClientDetail::name() { return EthTeal "^" EthCoal " 0"; }
#else
const char* ClientNote::name() { return EthTeal "⧫" EthBlue " ℹ"; }
const char* ClientChat::name() { return EthTeal "⧫" EthWhite " ◌"; }
const char* ClientTrace::name() { return EthTeal "⧫" EthGray " ◎"; }
const char* ClientDetail::name() { return EthTeal "⧫" EthCoal " ●"; }
#endif

Client::Client(
	ChainParams const& _params,
	int _networkID,
	p2p::Host* _host,
	std::shared_ptr<GasPricer> _gpForAdoption,
	std::string const& _dbPath,
	WithExisting _forceAction,
	TransactionQueue::Limits const& _l
):
	ClientBase(_l),
	Worker("eth", 0),
	m_bc(_params, _dbPath, _forceAction, [](unsigned d, unsigned t){ std::cerr << "REVISING BLOCKCHAIN: Processed " << d << " of " << t << "...\r"; }),
	m_gp(_gpForAdoption ? _gpForAdoption : make_shared<TrivialGasPricer>()),
	m_preSeal(chainParams().accountStartNonce),
	m_postSeal(chainParams().accountStartNonce),
	m_working(chainParams().accountStartNonce)
{
	init(_host, _dbPath, _forceAction, _networkID);
}

