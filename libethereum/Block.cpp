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

Block& Block::operator=(Block const& _s)
{
	if (&_s == this)
		return *this;

	m_state = _s.m_state;
	m_transactions = _s.m_transactions;
	m_receipts = _s.m_receipts;
	m_transactionSet = _s.m_transactionSet;
	m_previousBlock = _s.m_previousBlock;
	m_currentBlock = _s.m_currentBlock;
	m_currentBytes = _s.m_currentBytes;
	m_author = _s.m_author;
	m_sealEngine = _s.m_sealEngine;

	m_precommit = m_state;
	m_committedToSeal = false;
	return *this;
}

void Block::resetCurrent(u256 const& _timestamp)
{
	m_transactions.clear();
	m_receipts.clear();
	m_transactionSet.clear();
	m_currentBlock = BlockHeader();
	m_currentBlock.setAuthor(m_author);
	m_currentBlock.setTimestamp(max(m_previousBlock.timestamp() + 1, _timestamp));
	m_currentBytes.clear();
	sealEngine()->populateFromParent(m_currentBlock, m_previousBlock);

	// TODO: check.

	m_state.setRoot(m_previousBlock.stateRoot());
	m_precommit = m_state;
	m_committedToSeal = false;

	performIrregularModifications();
}

void Block::resetCurrentTime(u256 const& _timestamp) {
    m_currentBlock.setTimestamp(max(m_previousBlock.timestamp() + 1, _timestamp));
}
void Block::setIndex(u256 _idx) {
    m_currentBlock.setIndex(_idx);
}
void Block::setNodeList(h512s const& _nodes) {
    m_currentBlock.setNodeList(_nodes);
}

SealEngineFace* Block::sealEngine() const
{
	if (!m_sealEngine)
		BOOST_THROW_EXCEPTION(ChainOperationWithUnknownBlockChain());
	return m_sealEngine;
}

void Block::noteChain(BlockChain const& _bc)
{
	if (!m_sealEngine)
	{
		m_state.noteAccountStartNonce(_bc.chainParams().accountStartNonce);
		m_precommit.noteAccountStartNonce(_bc.chainParams().accountStartNonce);
		m_sealEngine = _bc.sealEngine();
	}
}

PopulationStatistics Block::populateFromChain(BlockChain const& _bc, h256 const& _h, ImportRequirements::value _ir)
{
	noteChain(_bc);

	PopulationStatistics ret { 0.0, 0.0 };

	if (!_bc.isKnown(_h))
	{
		// Might be worth throwing here.
		cwarn << "Invalid block given for state population: " << _h;
		BOOST_THROW_EXCEPTION(BlockNotFound() << errinfo_target(_h));
	}

	auto b = _bc.block(_h);
	BlockHeader bi(b);		// No need to check - it's already in the DB.
	if (bi.number())
	{
		// Non-genesis:

		// 1. Start at parent's end state (state root).
		BlockHeader bip(_bc.block(bi.parentHash()));
		sync(_bc, bi.parentHash(), bip);

		// 2. Enact the block's transactions onto this state.
		m_author = bi.author();
		Timer t;
		auto vb = _bc.verifyBlock(&b, function<void(Exception&)>(), _ir | ImportRequirements::TransactionBasic);
		ret.verify = t.elapsed();
		t.restart();
		enact(vb, _bc);
		ret.enact = t.elapsed();
	}
	else
	{
		// Genesis required:
		// We know there are no transactions, so just populate directly.
		m_state = State(m_state.accountStartNonce(), m_state.db(), BaseState::Empty);	// TODO: try with PreExisting.
		sync(_bc, _h, bi);
	}

	return ret;
}

bool Block::sync(BlockChain const& _bc)
{
	return sync(_bc, _bc.currentHash());
}

bool Block::sync(BlockChain const& _bc, h256 const& _block, BlockHeader const& _bi)
{
	noteChain(_bc);

	bool ret = false;
	// BLOCK
	BlockHeader bi = _bi ? _bi : _bc.info(_block);
#if ETH_PARANOIA
	if (!bi)
		while (1)
		{
			try
			{
				auto b = _bc.block(_block);
				bi.populate(b);
				break;
			}
			catch (Exception const& _e)
			{
				// TODO: Slightly nicer handling? :-)
				cerr << "ERROR: Corrupt block-chain! Delete your block-chain DB and restart." << endl;
				cerr << diagnostic_information(_e) << endl;
			}
			catch (std::exception const& _e)
			{
				// TODO: Slightly nicer handling? :-)
				cerr << "ERROR: Corrupt block-chain! Delete your block-chain DB and restart." << endl;
				cerr << _e.what() << endl;
			}
		}
#endif
	if (bi == m_currentBlock)
	{
		// We mined the last block.
		// Our state is good - we just need to move on to next.
		m_previousBlock = m_currentBlock;
		resetCurrent();
		ret = true;
	}
	else if (bi == m_previousBlock)
	{
		// No change since last sync.
		// Carry on as we were.
	}
	else
	{
		// New blocks available, or we've switched to a different branch. All change.
		// Find most recent state dump and replay what's left.
		// (Most recent state dump might end up being genesis.)

		if (m_state.db().lookup(bi.stateRoot()).empty())	// TODO: API in State for this?
		{
			cwarn << "Unable to sync to" << bi.hash() << "; state root" << bi.stateRoot() << "not found in database.";
			cwarn << "Database corrupt: contains block without stateRoot:" << bi;
			cwarn << "Try rescuing the database by running: eth --rescue";
			BOOST_THROW_EXCEPTION(InvalidStateRoot() << errinfo_target(bi.stateRoot()));
		}
		m_previousBlock = bi;
		resetCurrent();
		ret = true;
	}
#if ALLOW_REBUILD
	else
	{
		// New blocks available, or we've switched to a different branch. All change.
		// Find most recent state dump and replay what's left.
		// (Most recent state dump might end up being genesis.)

		std::vector<h256> chain;
		while (bi.number() != 0 && m_db.lookup(bi.stateRoot()).empty())	// while we don't have the state root of the latest block...
		{
			chain.push_back(bi.hash());				// push back for later replay.
			bi.populate(_bc.block(bi.parentHash()));	// move to parent.
		}

		m_previousBlock = bi;
		resetCurrent();

		// Iterate through in reverse, playing back each of the blocks.
		try
		{
			for (auto it = chain.rbegin(); it != chain.rend(); ++it)
			{
				auto b = _bc.block(*it);
				enact(&b, _bc, _ir);
				cleanup(true);
			}
		}
		catch (...)
		{
			// TODO: Slightly nicer handling? :-)
			cerr << "ERROR: Corrupt block-chain! Delete your block-chain DB and restart." << endl;
			cerr << boost::current_exception_diagnostic_information() << endl;
			exit(1);
		}

		resetCurrent();
		ret = true;
	}
#endif
	return ret;
}

pair<TransactionReceipts, bool> Block::sync(BlockChain const& _bc, TransactionQueue& _tq, GasPricer const& _gp, unsigned msTimeout)
{
	if (isSealed())
		BOOST_THROW_EXCEPTION(InvalidOperationOnSealedBlock());

	noteChain(_bc);

	// TRANSACTIONS
	pair<TransactionReceipts, bool> ret;

	auto ts = _tq.topTransactions(c_maxSyncTransactions, m_transactionSet);
	ret.second = (ts.size() == c_maxSyncTransactions);	// say there's more to the caller if we hit the limit

	LastHashes lh;

	auto deadline =  chrono::steady_clock::now() + chrono::milliseconds(msTimeout);

	for (int goodTxs = max(0, (int)ts.size() - 1); goodTxs < (int)ts.size(); )
	{
		goodTxs = 0;
		for (auto const& t: ts)
			if (!m_transactionSet.count(t.sha3()))
			{
				try
				{
					if (t.gasPrice() >= _gp.ask(*this))
					{
//						Timer t;
						if (lh.empty())
							lh = _bc.lastHashes();
						execute(lh, t);
						ret.first.push_back(m_receipts.back());
						++goodTxs;
//						cnote << "TX took:" << t.elapsed() * 1000;
					}
					else if (t.gasPrice() < _gp.ask(*this) * 9 / 10)
					{
						clog(StateTrace) << t.sha3() << "Dropping El Cheapo transaction (<90% of ask price)";
						_tq.drop(t.sha3());
					}
				}
				catch (InvalidNonce const& in)
				{
					bigint const& req = *boost::get_error_info<errinfo_required>(in);
					bigint const& got = *boost::get_error_info<errinfo_got>(in);

					if (req > got)
					{
						// too old
						clog(StateTrace) << t.sha3() << "Dropping old transaction (nonce too low)";
						_tq.drop(t.sha3());
					}
					else if (got > req + _tq.waiting(t.sender()))
					{
						// too new
						clog(StateTrace) << t.sha3() << "Dropping new transaction (too many nonces ahead)";
						_tq.drop(t.sha3());
					}
					else
						_tq.setFuture(t.sha3());
				}
				catch (BlockGasLimitReached const& e)
				{
					bigint const& got = *boost::get_error_info<errinfo_got>(e);
					if (got > m_currentBlock.gasLimit())
					{
						clog(StateTrace) << t.sha3() << "Dropping over-gassy transaction (gas > block's gas limit)";
						_tq.drop(t.sha3());
					}
					else
					{
						clog(StateTrace) << t.sha3() << "Temporarily no gas left in current block (txs gas > block's gas limit)";
						//_tq.drop(t.sha3());
						// Temporarily no gas left in current block.
						// OPTIMISE: could note this and then we don't evaluate until a block that does have the gas left.
						// for now, just leave alone.
					}
				}
				catch (Exception const& _e)
				{
					// Something else went wrong - drop it.
					clog(StateTrace) << t.sha3() << "Dropping invalid transaction:" << diagnostic_information(_e);
					_tq.drop(t.sha3());
				}
				catch (std::exception const&)
				{
					// Something else went wrong - drop it.
					_tq.drop(t.sha3());
					cwarn << t.sha3() << "Transaction caused low-level exception :(";
				}
			}
		if (chrono::steady_clock::now() > deadline)
		{
			ret.second = true;	// say there's more to the caller if we ended up crossing the deadline.
			break;
		}
	}
	return ret;
}


/*
pair<TransactionReceipts, bool> Block::sync(BlockChain const& _bc, TransactionQueue& _tq, GasPricer const& _gp, bool _exec, u256 const& _max_block_txs)
{
    cdebug << "Block::sync ";

    if (isSealed())
        BOOST_THROW_EXCEPTION(InvalidOperationOnSealedBlock());

    noteChain(_bc);

    // TRANSACTIONS
    pair<TransactionReceipts, bool> ret;

    unsigned max_sync_txs = 0;
    if (_max_block_txs == Invalid256) {
        max_sync_txs = c_maxSyncTransactions;
    } else {
        max_sync_txs = static_cast<unsigned>(_max_block_txs > m_transactions.size() ? _max_block_txs - m_transactions.size() : 0);
    }
  
    auto ts = _tq.allTransactions();

    LastHashes lh;
    unsigned goodTxs = 0;
   
    {
        //goodTxs = 0;
        for (auto const& t : ts)
            if (!m_transactionSet.count(t.sha3()))
            {
                try
                {
                    cdebug << "PACK-TX: Hash=" << (t.sha3()) << ",time=" << utcTime();

                    //u256 check = _bc.filterCheck(t, FilterCheckScene::PackTranscation);
                    //if ( (u256)SystemContractCode::Ok != check  )
                    //{
                    //    cwarn << "Block::sync " << t.sha3() << " transition filterCheck PackTranscation Fail" << check;
                    //    BOOST_THROW_EXCEPTION(FilterCheckFail());
                    //}

                    if ( ! _bc.isBlockLimitOk(t)  ) 
                    {
                        cwarn << "Block::sync " << t.sha3() << " transition blockLimit=" << t.blockLimit() << " chain number=" << _bc.number();
                        BOOST_THROW_EXCEPTION(BlockLimitCheckFail());
                    }

                    if ( !_bc.isNonceOk(t) ) 
                    {
                        cwarn << "Block::sync " << t.sha3() << " " << t.randomid();
                        BOOST_THROW_EXCEPTION(NonceCheckFail());
                    }
                    for ( size_t pIndex = 0; pIndex < m_transactions.size(); pIndex++) 
                    {
                        if ( (m_transactions[pIndex].from() == t.from() ) && (m_transactions[pIndex].randomid() == t.randomid()) )
                            BOOST_THROW_EXCEPTION(NonceCheckFail());
                    }//for

                    if (_exec) {
                        u256 _t = _gp.ask(*this);

                        if ( _t )
                            _t = 0;
                        //Timer t;
                        if (lh.empty())
                            lh = _bc.lastHashes();
                        execute(lh, t, Permanence::Committed, OnOpFunc(), &_bc);
                        ret.first.push_back(m_receipts.back());
                    } else {
                        cdebug << "Block::sync no need exec: t=" << toString(t.sha3());
                        m_transactions.push_back(t);
                        m_transactionSet.insert(t.sha3());
                    }
                    ++goodTxs;
                }
                catch ( FilterCheckFail const& in)
                {
                    cwarn << t.sha3() << "Block::sync Dropping  transaction (filter check fail!)";
                    _tq.drop(t.sha3());
                }
                catch ( NoDeployPermission const &in)
                {
                    cwarn << t.sha3() << "Block::sync Dropping  transaction (NoDeployPermission  fail!)";
                    _tq.drop(t.sha3());
                }
                catch (BlockLimitCheckFail const& in)
                {
                    cwarn << t.sha3() << "Block::sync Dropping  transaction (blocklimit  check fail!)";
                    _tq.drop(t.sha3());
                }
                catch (NonceCheckFail const& in)
                {
                    cwarn << t.sha3() << "Block::sync Dropping  transaction (nonce check fail!)";
                    _tq.drop(t.sha3());
                }
                catch (InvalidNonce const& in)
                {
                    bigint const& req = *boost::get_error_info<errinfo_required>(in);
                    bigint const& got = *boost::get_error_info<errinfo_got>(in);

                    if (req > got)
                    {
                        // too old
                        cdebug << t.sha3() << "Dropping old transaction (nonce too low)";
                        _tq.drop(t.sha3());
                    }
                    else if (got > req + _tq.waiting(t.sender()))
                    {
                        // too new
                        cdebug << t.sha3() << "Dropping new transaction (too many nonces ahead)";
                        _tq.drop(t.sha3());
                    }
                    else
                        _tq.setFuture(t.sha3());
                }
                catch (BlockGasLimitReached const& e)
                {
                    bigint const& got = *boost::get_error_info<errinfo_got>(e);
                    if (got > m_currentBlock.gasLimit())
                    {
                        cdebug << t.sha3() << "Dropping over-gassy transaction (gas > block's gas limit)";
                        _tq.drop(t.sha3());
                    }
                    else
                    {
                        cdebug << t.sha3() << "Temporarily no gas left in current block (txs gas > block's gas limit)";
                    }
                }
                catch (Exception const& _e)
                {
                    // Something else went wrong - drop it.
                    cdebug << t.sha3() << "Dropping invalid transaction:" << diagnostic_information(_e);
                    _tq.drop(t.sha3());
                }
                catch (std::exception const&)
                {
                    // Something else went wrong - drop it.
                    _tq.drop(t.sha3());
                    cwarn << t.sha3() << "Transaction caused low-level exception :(";
                }

                if (goodTxs >= max_sync_txs) {
                    break;
                }
            }
        ret.second = (goodTxs >= max_sync_txs);
    }
    return ret;
}
*/

TransactionReceipts Block::exec(BlockChain const& _bc, TransactionQueue& _tq)
{
    cdebug << "Block::exec ";

    if (isSealed())
        BOOST_THROW_EXCEPTION(InvalidOperationOnSealedBlock());

    noteChain(_bc);

    // TRANSACTIONS
    TransactionReceipts ret;

    LastHashes lh;
    DEV_TIMED_ABOVE("lastHashes", 500)
    lh = _bc.lastHashes();

    unsigned i = 0;
    DEV_TIMED_ABOVE("txExec,blk=" + toString(info().number()) + ",txs=" + toString(m_transactions.size()), 500)
    for (Transaction const& tr : m_transactions)
    {
        try
        {
            cdebug << "Block::exec transaction: " << tr.from() /*<< state().transactionsFrom(tr.from()) */ << tr.value() << toString(tr.sha3());
            execute(lh, tr, Permanence::Committed, OnOpFunc(), &_bc);
        }
        catch (Exception& ex)
        {
            ex << errinfo_transactionIndex(i);
            _tq.drop(tr.sha3());  
            throw;
        }
        catch (std::exception& ex)
        {
	    cwarn<<"execute t="<<toString(tr.sha3())<<" failed, error message:"<<ex.what();
            _tq.drop(tr.sha3());  
            throw;
        }
        cdebug << "Block::exec: t=" << toString(tr.sha3());
        cdebug << "Block::exec: stateRoot=" << toString(m_receipts.back().stateRoot()) << ",gasUsed=" << toString(m_receipts.back().gasUsed()) << ",sha3=" << toString(sha3(m_receipts.back().rlp()));

        RLPStream receiptRLP;
        m_receipts.back().streamRLP(receiptRLP);
        ret.push_back(m_receipts.back());
        ++i;
    }

    return ret;
}

