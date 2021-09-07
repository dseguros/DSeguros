
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

void PBFT::resetConfig() {
	m_account_type = 1;
	m_node_num = 1;
	m_node_idx = 0;
	m_f = (m_node_num - 1 ) / 3;

	m_prepare_cache.clear();
	m_sign_cache.clear();
	m_recv_view_change_req.clear();
	m_commitMap.clear();
/*
	if (!NodeConnManagerSingleton::GetInstance().getAccountType(m_key_pair.pub(), m_account_type)) {
		cwarn << "resetConfig: can't find myself id, stop sealing";
		m_cfg_err = true;
		return;
	}

	auto node_num = NodeConnManagerSingleton::GetInstance().getMinerNum();
	if (node_num == 0) {
		cwarn << "resetConfig: miner_num = 0, stop sealing";
		m_cfg_err = true;
		return;
	}

	u256 node_idx;
	if (!NodeConnManagerSingleton::GetInstance().getIdx(m_key_pair.pub(), node_idx)) {
		//BOOST_THROW_EXCEPTION(PbftInitFailed() << errinfo_comment("NodeID not in cfg"));
		cwarn << "resetConfig: can't find myself id, stop sealing";
		m_cfg_err = true;
		return;
	}

	if (node_num != m_node_num || node_idx != m_node_idx) {
		m_node_num = node_num;
		m_node_idx = node_idx;
		m_f = (m_node_num - 1 ) / 3;

		m_prepare_cache.clear();
		m_sign_cache.clear();
		m_recv_view_change_req.clear();
		
		ConsensusControl::instance().clearAllCache();
		m_commitMap.clear();

		if (!getMinerList(-1, m_miner_list)) {
			cwarn << "resetConfig: getMinerList return false";
			m_cfg_err = true;
			return;
		}

		if (m_miner_list.size() != m_node_num) {
			cwarn << "resetConfig: m_miner_list.size=" << m_miner_list.size() << ",m_node_num=" << m_node_num;
			m_cfg_err = true;
			return;
		}
		cdebug << "resetConfig: m_node_idx=" << m_node_idx << ", m_node_num=" << m_node_num;
	}
	// consensuscontrol init cache
	ConsensusControl::instance().resetNodeCache();
	m_cfg_err = false;
*/
}

StringHashMap PBFT::jsInfo(BlockHeader const& _bi) const
{
	return { { "number", toJS(_bi.number()) }, { "timestamp", toJS(_bi.timestamp()) } };
}

bool PBFT::generateSeal(BlockHeader const& _bi, bytes const& _block_data, u256 &_view)
{
	Timer t;
	Guard l(m_mutex);
	_view = m_view;
	if (!broadcastPrepareReq(_bi, _block_data)) {
		cwarn << "broadcastPrepareReq failed, " << _bi.number() << _bi.hash(WithoutSeal);
		return false;
	}

	cdebug << "generateSeal, blk=" << _bi.number() << ", timecost=" << 1000 * t.elapsed();

	return true;
}

