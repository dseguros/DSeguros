
#include <boost/filesystem.hpp>
#include <libethcore/ChainOperationParams.h>
#include <libethcore/CommonJS.h>
#include <libethereum/Interface.h>
#include <libethereum/BlockChain.h>
#include <libethereum/Block.h>
#include <libethereum/EthereumHost.h>
//#include <libethereum/NodeConnParamsManagerApi.h>
#include <libdevcrypto/Common.h>
#include "PBFT.h"
//#include <libdevcore/easylog.h>
//#include <libdevcore/LogGuard.h>
//#include <libethereum/StatLog.h>
//#include <libethereum/ConsensusControl.h>
using namespace std;
using namespace dev;
using namespace eth;

void PBFT::init()
{
	ETH_REGISTER_SEAL_ENGINE(PBFT);
}

PBFT::PBFT()
{
}

PBFT::~PBFT() {
	if (m_backup_db) {
		delete m_backup_db;
	}
	stopWorking();
}

void PBFT::initEnv(std::weak_ptr<PBFTHost> _host, BlockChain* _bc, OverlayDB* _db, BlockQueue *bq, KeyPair const& _key_pair, unsigned _view_timeout)
{
	Guard l(m_mutex);

	m_host = _host;
	m_bc.reset(_bc);
	m_stateDB.reset(_db);
	m_bq.reset(bq);

	m_bc->setSignChecker([this](BlockHeader const & _header, std::vector<std::pair<u256, Signature>> _sign_list) {
		return checkBlockSign(_header, _sign_list);
	});

	m_key_pair = _key_pair;

	resetConfig();

	m_view_timeout = _view_timeout;
	m_consensus_block_number = 0;
	m_last_consensus_time =  utcTime();
	m_change_cycle = 0;
	m_to_view = 0;
	m_leader_failed = false;

	m_last_sign_time = 0;

	m_last_collect_time = std::chrono::system_clock::now();

	m_future_prepare_cache = std::make_pair(Invalid256, PrepareReq());

	m_last_exec_finish_time = utcTime();

	initBackupDB();

	cdebug << "PBFT initEnv success";
}


void PBFT::initBackupDB() {
	ldb::Options o;
	o.max_open_files = 256;
	o.create_if_missing = true;
	std::string path = m_bc->chainParams().dataDir + "/pbftMsgBackup";
	ldb::Status status = ldb::DB::Open(o, path, &m_backup_db);
	if (!status.ok() || !m_backup_db)
	{
		if (boost::filesystem::space(path).available < 1024)
		{
			cwarn << "Not enough available space found on hard drive. Please free some up and then re-run. Bailing.";
			cwarn << "Not enough available space found on hard drive. Please free some up and then re-run. Bailing.";
			BOOST_THROW_EXCEPTION(NotEnoughAvailableSpace());
		}
		else
		{
			cwarn << status.ToString();
			cwarn << "Database " << path << "already open. You appear to have another instance of ethereum running. Bailing.";
			cwarn << status.ToString();
			cwarn << "Database " << path << "already open. You appear to have another instance of ethereum running. Bailing.";
			BOOST_THROW_EXCEPTION(DatabaseAlreadyOpen());
		}
	}

	// reload msg from db
	reloadMsg(backup_key_committed, &m_committed_prepare_cache);
}
