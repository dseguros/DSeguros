#pragma once

#include <thread>
#include <condition_variable>
#include <mutex>
#include <list>
#include <queue>
#include <atomic>
#include <string>
#include <array>
#include <libdevcore/Common.h>
#include <libdevcore/CommonIO.h>
#include <libdevcore/Guards.h>
#include <libdevcore/Worker.h>
#include <libethcore/SealEngine.h>
#include <libethcore/ABI.h>
#include <libp2p/Common.h>
#include "BlockChain.h"
#include "Block.h"
#include "CommonNet.h"
#include "ClientBase.h"

namespace dev
{
namespace eth
{

class Client;
class DownloadMan;

enum ClientWorkState
{
	Active = 0,
	Deleting,
	Deleted
};

struct ClientNote: public LogChannel { static const char* name(); static const int verbosity = 2; };
struct ClientChat: public LogChannel { static const char* name(); static const int verbosity = 4; };
struct ClientTrace: public LogChannel { static const char* name(); static const int verbosity = 7; };
struct ClientDetail: public LogChannel { static const char* name(); static const int verbosity = 14; };

struct ActivityReport
{
	unsigned ticks = 0;
	std::chrono::system_clock::time_point since = std::chrono::system_clock::now();
};

std::ostream& operator<<(std::ostream& _out, ActivityReport const& _r);

/**
 * @brief Main API hub for interfacing with Ethereum.
 */
class Client: public ClientBase, protected Worker
{
public:
	Client(
		ChainParams const& _params,
		int _networkID,
		p2p::Host* _host,
		std::shared_ptr<GasPricer> _gpForAdoption,
		std::string const& _dbPath = std::string(),
		WithExisting _forceAction = WithExisting::Trust,
		TransactionQueue::Limits const& _l = TransactionQueue::Limits{1024, 1024}
	);
	/// Destructor.
	virtual ~Client();

	/// Get information on this chain.
	ChainParams const& chainParams() const { return bc().chainParams(); }

	/// Resets the gas pricer to some other object.
	void setGasPricer(std::shared_ptr<GasPricer> _gp) { m_gp = _gp; }
	std::shared_ptr<GasPricer> gasPricer() const { return m_gp; }


};

}
}
