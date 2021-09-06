#pragma once

#include <set>
#include <libdevcore/concurrent_queue.h>
#include <libdevcore/db.h>
#include <libdevcore/Worker.h>
#include <libdevcrypto/Common.h>
#include <libethcore/BlockHeader.h>
#include <libethcore/SealEngine.h>
#include <libethereum/CommonNet.h>
#include "Common.h"
#include "PBFTHost.h"

namespace dev
{

namespace eth
{

DEV_SIMPLE_EXCEPTION(PbftInitFailed);
DEV_SIMPLE_EXCEPTION(UnexpectError);

class PBFT: public SealEngineFace, Worker
{
public:
	PBFT();
	virtual ~PBFT();

	static void init();

	std::string name() const override { return "PBFT"; }
	StringHashMap jsInfo(BlockHeader const& _bi) const override;

	strings sealers() const override { return {m_sealer}; }
	std::string sealer() const override { return m_sealer; }
	void setSealer(std::string const& _sealer) override { m_sealer = _sealer; }
	u256 nodeIdx() const { /*Guard l(m_mutex);*/ return m_node_idx; }
	uint64_t lastConsensusTime() const { /*Guard l(m_mutex);*/ return m_last_consensus_time;}
	unsigned accountType() const { return m_account_type; }
	u256 view() const { return m_view; }
	u256 to_view() const {return m_to_view; }
	u256 quorum() const { return m_node_num - m_f; }
	const BlockHeader& getHighestBlock() const { return m_highest_block; } 
	bool isLeader() {
		auto ret = getLeader();
		bool isLeader = false;
		if (ret.first) // if errer, isLeader is False isLeaderfalse
			isLeader = ret.second == m_node_idx;
		return isLeader;
	}


	void startGeneration() { setName("PBFT"); m_last_consensus_time = utcTime(); resetConfig(); startWorking(); }
	void cancelGeneration() override { stopWorking(); }

	void generateSeal(BlockHeader const& , bytes const& ) {}
	bool generateSeal(BlockHeader const& _bi, bytes const& _block_data, u256 &_view);
	bool generateCommit(BlockHeader const& _bi, bytes const& _block_data, u256 const& _view);
	void onSealGenerated(std::function<void(bytes const&)> const&) override {}
	void onSealGenerated(std::function<void(bytes const&, bool)> const& _f)  { m_onSealGenerated = _f;}
	void onViewChange(std::function<void()> const& _f) { m_onViewChange = _f; }
	bool shouldSeal(Interface* _i) override;

	// should be called before start
	void initEnv(std::weak_ptr<PBFTHost> _host, BlockChain* _bc, OverlayDB* _db, BlockQueue *bq, KeyPair const& _key_pair, unsigned _view_timeout);
	void setOmitEmptyBlock(bool _flag) {m_omit_empty_block = _flag;}

	// report newest block 
	void reportBlock(BlockHeader const& _b, u256 const& td);

	void onPBFTMsg(unsigned _id, std::shared_ptr<p2p::Capability> _peer, RLP const& _r);

	h512s getMinerNodeList() const {  /*Guard l(m_mutex);*/ return m_miner_list; }

	void changeViewForEmptyBlockWithoutLock(u256 const& _from);
	void changeViewForEmptyBlockWithLock();

	uint64_t lastExecFinishTime() const { return m_last_exec_finish_time; }

	private:
	void initBackupDB();
	void resetConfig();
	// ：，viewchange
	// thread: handle msg response, broadcast viewchange when timeout
	void workLoop() override;

	//，
	// check timeout, if timeout, change view
	void checkTimeout();

	void collectGarbage();


	bool getMinerList(int _blk_no, h512s & _miner_list) const;

	std::pair<bool, u256> getLeader() const;

	Signature signHash(h256 const& _hash) const;
	bool checkSign(u256 const& _idx, h256 const& _hash, Signature const& _sign) const;
	bool checkSign(PBFTMsg const& _req) const;

	// 
	// broadcast msg
	bool broadcastPrepareReq(BlockHeader const& _bi, bytes const& _block_data);
	bool broadcastSignReq(PrepareReq const& _req);
	bool broadcastCommitReq(PrepareReq const & _req);
	bool broadcastViewChangeReq();
	bool broadcastMsg(std::string const& _key, unsigned _id, bytes const& _data, std::unordered_set<h512> const& _filter = std::unordered_set<h512>());
	bool broadcastFilter(std::string const& _key, unsigned _id, std::shared_ptr<PBFTPeer> _p);
	void broadcastMark(std::string const& _key, unsigned _id, std::shared_ptr<PBFTPeer> _p);
	void clearMask();

	// 
	// handle msg
	void handleMsg(unsigned _id, u256 const& _from, h512 const& _node, RLP const& _r);
	void handlePrepareMsg(u256 const& _from, PrepareReq const& _req, bool _self = false);
	void handleSignMsg(u256 const& _from, SignReq const& _req);
	void handleCommitMsg(u256 const& _from, CommitReq const& _req);
	void handleViewChangeMsg(u256 const& _from, ViewChangeReq const& _req);

	void reHandlePrepareReq(PrepareReq const& _req);

	// cache（，）
	// access cache (no thread safe )
	bool addRawPrepare(PrepareReq const& _req);
	bool addPrepareReq(PrepareReq const& _req);
	void addSignReq(SignReq const& _req);
	void addCommitReq(CommitReq const& _req);
	void delCache(h256 const& _hash);
	void delViewChange();

	bool isExistPrepare(PrepareReq const& _req);
	bool isExistSign(SignReq const& _req);
	bool isExistCommit(CommitReq const& _req);
	bool isExistViewChange(ViewChangeReq const& _req);

	void checkAndChangeView();
	void checkAndCommit();
	void checkAndSave();
};

}
}
