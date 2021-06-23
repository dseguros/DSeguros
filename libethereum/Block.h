
#pragma once

#include <array>
#include <unordered_map>
#include <libdevcore/Common.h>
#include <libdevcore/RLP.h>
#include <libdevcore/TrieDB.h>
#include <libdevcore/OverlayDB.h>
#include <libethcore/Exceptions.h>
#include <libethcore/BlockHeader.h>
#include <libethcore/ChainOperationParams.h>
#include <libevm/ExtVMFace.h>
#include "Account.h"
#include "Transaction.h"
#include "TransactionReceipt.h"
#include "GasPricer.h"
#include "State.h"

namespace dev
{

namespace test { class ImportTest; class StateLoader; }

namespace eth
{

class SealEngineFace;
class BlockChain;
class State;
class TransactionQueue;
struct VerifiedBlockRef;

struct BlockChat: public LogChannel { static const char* name(); static const int verbosity = 4; };
struct BlockTrace: public LogChannel { static const char* name(); static const int verbosity = 5; };
struct BlockDetail: public LogChannel { static const char* name(); static const int verbosity = 14; };
struct BlockSafeExceptions: public LogChannel { static const char* name(); static const int verbosity = 21; };

struct PopulationStatistics
{
	double verify;
	double enact;
};

DEV_SIMPLE_EXCEPTION(ChainOperationWithUnknownBlockChain);
DEV_SIMPLE_EXCEPTION(InvalidOperationOnSealedBlock);


/**
 * @brief Active model of a block within the block chain.
 * Keeps track of all transactions, receipts and state for a particular block. Can apply all
 * needed transforms of the state for rewards and contains logic for sealing the block.
 */
class Block
{
	friend class ExtVM;
	friend class dev::test::ImportTest;
	friend class dev::test::StateLoader;
	friend class Executive;
	friend class BlockChain;

public:
	// TODO: pass in ChainOperationParams rather than u256

	/// Default constructor; creates with a blank database prepopulated with the genesis block.
	Block(u256 const& _accountStartNonce): m_state(_accountStartNonce, OverlayDB(), BaseState::Empty), m_precommit(_accountStartNonce) {}

	/// Basic state object from database.
	/// Use the default when you already have a database and you just want to make a Block object
	/// which uses it. If you have no preexisting database then set BaseState to something other
	/// than BaseState::PreExisting in order to prepopulate the Trie.
	/// You can also set the author address.
	Block(BlockChain const& _bc, OverlayDB const& _db, BaseState _bs = BaseState::PreExisting, Address const& _author = Address());

	/// Basic state object from database.
	/// Use the default when you already have a database and you just want to make a Block object
	/// which uses it.
	/// Will throw InvalidRoot if the root passed is not in the database.
	/// You can also set the author address.
	Block(BlockChain const& _bc, OverlayDB const& _db, h256 const& _root, Address const& _author = Address());


    enum NullType { Null };
	Block(NullType): m_state(0, OverlayDB(), BaseState::Empty), m_precommit(0) {}

	/// Construct from a given blockchain. Empty, but associated with @a _bc 's chain params.
	explicit Block(BlockChain const& _bc): Block(Null) { noteChain(_bc); }

	/// Copy state object.
	Block(Block const& _s);

	/// Copy state object.
	Block& operator=(Block const& _s);

	/// Get the author address for any transactions we do and rewards we get.
	Address author() const { return m_author; }

	/// Set the author address for any transactions we do and rewards we get.
	/// This causes a complete reset of current block.
	void setAuthor(Address const& _id) { m_author = _id; resetCurrent(); }

	/// Note the fact that this block is being used with a particular chain.
	/// Call this before using any non-const methods.
	void noteChain(BlockChain const& _bc);

	// Account-getters. All operate on the final state.

	/// Get an account's balance.
	/// @returns 0 if the address has never been used.
	u256 balance(Address const& _address) const { return m_state.balance(_address); }
}

}
