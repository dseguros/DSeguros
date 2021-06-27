#include "Block.h"

#include <ctime>
#include <boost/filesystem.hpp>
#include <boost/timer.hpp>
#include <libdevcore/CommonIO.h>
#include <libdevcore/Assertions.h>
#include <libdevcore/TrieHash.h>
#include <libevmcore/Instruction.h>
#include <libethcore/Exceptions.h>
#include <libethcore/SealEngine.h>
#include <libevm/VMFactory.h>
#include "BlockChain.h"
#include "Defaults.h"
#include "ExtVM.h"
#include "Executive.h"
#include "BlockChain.h"
#include "TransactionQueue.h"
#include "GenesisInfo.h"
using namespace std;
using namespace dev;
using namespace dev::eth;
namespace fs = boost::filesystem;

#define ETH_TIMED_ENACTMENTS 0

static const unsigned c_maxSyncTransactions = 1024;

const char* BlockSafeExceptions::name() { return EthViolet "⚙" EthBlue " ℹ"; }
const char* BlockDetail::name() { return EthViolet "⚙" EthWhite " ◌"; }
const char* BlockTrace::name() { return EthViolet "⚙" EthGray " ◎"; }
const char* BlockChat::name() { return EthViolet "⚙" EthWhite " ◌"; }

Block::Block(BlockChain const& _bc, OverlayDB const& _db, BaseState _bs, Address const& _author):
	m_state(Invalid256, _db, _bs),
	m_precommit(Invalid256),
	m_author(_author)
{
	noteChain(_bc);
	m_previousBlock.clear();
	m_currentBlock.clear();
//	assert(m_state.root() == m_previousBlock.stateRoot());
}

Block::Block(BlockChain const& _bc, OverlayDB const& _db, h256 const& _root, Address const& _author):
	m_state(Invalid256, _db, BaseState::PreExisting),
	m_precommit(Invalid256),
	m_author(_author)
{
	noteChain(_bc);
	m_state.setRoot(_root);
	m_previousBlock.clear();
	m_currentBlock.clear();
//	assert(m_state.root() == m_previousBlock.stateRoot());
}

Block::Block(Block const& _s):
	m_state(_s.m_state),
	m_transactions(_s.m_transactions),
	m_receipts(_s.m_receipts),
	m_transactionSet(_s.m_transactionSet),
	m_precommit(_s.m_state),
	m_previousBlock(_s.m_previousBlock),
	m_currentBlock(_s.m_currentBlock),
	m_currentBytes(_s.m_currentBytes),
	m_author(_s.m_author),
	m_sealEngine(_s.m_sealEngine)
{
	m_committedToSeal = false;
}


