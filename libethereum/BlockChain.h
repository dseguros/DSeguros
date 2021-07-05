#pragma once

#include <deque>
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <libdevcore/db.h>
#include <libdevcore/Log.h>
#include <libdevcore/Exceptions.h>
#include <libdevcore/Guards.h>
#include <libethcore/Common.h>
#include <libethcore/BlockHeader.h>
#include <libethcore/SealEngine.h>
#include <libevm/ExtVMFace.h>
#include "BlockDetails.h"
#include "Account.h"
#include "Transaction.h"
#include "BlockQueue.h"
#include "VerifiedBlock.h"
#include "ChainParams.h"
#include "State.h"

namespace std
{
template <> struct hash<pair<dev::h256, unsigned>>
{
	size_t operator()(pair<dev::h256, unsigned> const& _x) const { return hash<dev::h256>()(_x.first) ^ hash<unsigned>()(_x.second); }
};
}


namespace dev
{

class OverlayDB;

namespace eth
{

static const h256s NullH256s;

class State;
class Block;

DEV_SIMPLE_EXCEPTION(AlreadyHaveBlock);
DEV_SIMPLE_EXCEPTION(FutureTime);
DEV_SIMPLE_EXCEPTION(TransientError);

struct BlockChainChat: public LogChannel { static const char* name(); static const int verbosity = 5; };
struct BlockChainNote: public LogChannel { static const char* name(); static const int verbosity = 3; };
struct BlockChainWarn: public LogChannel { static const char* name(); static const int verbosity = 1; };
struct BlockChainDebug: public LogChannel { static const char* name(); static const int verbosity = 0; };

// TODO: Move all this Genesis stuff into Genesis.h/.cpp
std::unordered_map<Address, Account> const& genesisState();

ldb::Slice toSlice(h256 const& _h, unsigned _sub = 0);
ldb::Slice toSlice(uint64_t _n, unsigned _sub = 0);

using BlocksHash = std::unordered_map<h256, bytes>;
using TransactionHashes = h256s;
using UncleHashes = h256s;


enum {
	ExtraDetails = 0,
	ExtraBlockHash,
	ExtraTransactionAddress,
	ExtraLogBlooms,
	ExtraReceipts,
	ExtraBlocksBlooms
};

using ProgressCallback = std::function<void(unsigned, unsigned)>;

class VersionChecker
{
public:
	VersionChecker(std::string const& _dbPath, h256 const& _genesisHash);
};


/**
 * @brief Implements the blockchain database. All data this gives is disk-backed.
 * @threadsafe
 */
class BlockChain
{
public:
	/// Doesn't open the database - if you want it open it's up to you to subclass this and open it
	/// in the constructor there.
	BlockChain(ChainParams const& _p, std::string const& _path, WithExisting _we = WithExisting::Trust, ProgressCallback const& _pc = ProgressCallback());
	~BlockChain();

	/// Reopen everything.
	void reopen(WithExisting _we = WithExisting::Trust, ProgressCallback const& _pc = ProgressCallback()) { reopen(m_params, _we, _pc); }
	void reopen(ChainParams const& _p, WithExisting _we = WithExisting::Trust, ProgressCallback const& _pc = ProgressCallback());

	/// (Potentially) renders invalid existing bytesConstRef returned by lastBlock.
	/// To be called from main loop every 100ms or so.
	void process();

	/// Sync the chain with any incoming blocks. All blocks should, if processed in order.
	/// @returns fresh blocks, dead blocks and true iff there are additional blocks to be processed waiting.
	std::tuple<ImportRoute, bool, unsigned> sync(BlockQueue& _bq, OverlayDB const& _stateDB, unsigned _max);

	/// Attempt to import the given block directly into the BlockChain and sync with the state DB.
	/// @returns the block hashes of any blocks that came into/went out of the canonical block chain.
	std::pair<ImportResult, ImportRoute> attemptImport(bytes const& _block, OverlayDB const& _stateDB, bool _mutBeNew = true) noexcept;

	/// Import block into disk-backed DB.
	/// @returns the block hashes of any blocks that came into/went out of the canonical block chain.
	ImportRoute import(bytes const& _block, OverlayDB const& _stateDB, bool _mustBeNew = true);
	ImportRoute import(VerifiedBlockRef const& _block, OverlayDB const& _db, bool _mustBeNew = true);

	/// Import data into disk-backed DB.
	/// This will not execute the block and populate the state trie, but rather will simply add the
	/// block/header and receipts directly into the databases.
	void insert(bytes const& _block, bytesConstRef _receipts, bool _mustBeNew = true);
	void insert(VerifiedBlockRef _block, bytesConstRef _receipts, bool _mustBeNew = true);

	/// Returns true if the given block is known (though not necessarily a part of the canon chain).
	bool isKnown(h256 const& _hash, bool _isCurrent = true) const;

	/// Get the partial-header of a block (or the most recent mined if none given). Thread-safe.
	BlockHeader info(h256 const& _hash) const { return BlockHeader(headerData(_hash), HeaderData); }
	BlockHeader info() const { return info(currentHash()); }

	/// Get a block (RLP format) for the given hash (or the most recent mined if none given). Thread-safe.
	bytes block(h256 const& _hash) const;
	bytes block() const { return block(currentHash()); }

	/// Get a block (RLP format) for the given hash (or the most recent mined if none given). Thread-safe.
	bytes headerData(h256 const& _hash) const;
	bytes headerData() const { return headerData(currentHash()); }

	/// Get the familial details concerning a block (or the most recent mined if none given). Thread-safe.
	BlockDetails details(h256 const& _hash) const { return queryExtras<BlockDetails, ExtraDetails>(_hash, m_details, x_details, NullBlockDetails); }
	BlockDetails details() const { return details(currentHash()); }

	/// Get the transactions' log blooms of a block (or the most recent mined if none given). Thread-safe.
	BlockLogBlooms logBlooms(h256 const& _hash) const { return queryExtras<BlockLogBlooms, ExtraLogBlooms>(_hash, m_logBlooms, x_logBlooms, NullBlockLogBlooms); }
	BlockLogBlooms logBlooms() const { return logBlooms(currentHash()); }

	/// Get the transactions' receipts of a block (or the most recent mined if none given). Thread-safe.
	/// receipts are given in the same order are in the same order as the transactions
	BlockReceipts receipts(h256 const& _hash) const { return queryExtras<BlockReceipts, ExtraReceipts>(_hash, m_receipts, x_receipts, NullBlockReceipts); }
	BlockReceipts receipts() const { return receipts(currentHash()); }

	/// Get the transaction by block hash and index;
	TransactionReceipt transactionReceipt(h256 const& _blockHash, unsigned _i) const { return receipts(_blockHash).receipts[_i]; }

	/// Get the transaction receipt by transaction hash. Thread-safe.
	TransactionReceipt transactionReceipt(h256 const& _transactionHash) const { TransactionAddress ta = queryExtras<TransactionAddress, ExtraTransactionAddress>(_transactionHash, m_transactionAddresses, x_transactionAddresses, NullTransactionAddress); if (!ta) return bytesConstRef(); return transactionReceipt(ta.blockHash, ta.index); }

	/// Get a list of transaction hashes for a given block. Thread-safe.
	TransactionHashes transactionHashes(h256 const& _hash) const { auto b = block(_hash); RLP rlp(b); h256s ret; for (auto t: rlp[1]) ret.push_back(sha3(t.data())); return ret; }
	TransactionHashes transactionHashes() const { return transactionHashes(currentHash()); }

	/// Get a list of uncle hashes for a given block. Thread-safe.
	UncleHashes uncleHashes(h256 const& _hash) const { auto b = block(_hash); RLP rlp(b); h256s ret; for (auto t: rlp[2]) ret.push_back(sha3(t.data())); return ret; }
	UncleHashes uncleHashes() const { return uncleHashes(currentHash()); }
	
	/// Get the hash for a given block's number.
	h256 numberHash(unsigned _i) const { if (!_i) return genesisHash(); return queryExtras<BlockHash, uint64_t, ExtraBlockHash>(_i, m_blockHashes, x_blockHashes, NullBlockHash).value; }

	/// Get the last N hashes for a given block. (N is determined by the LastHashes type.)
	LastHashes lastHashes() const { return lastHashes(m_lastBlockHash); }
	LastHashes lastHashes(h256 const& _mostRecentHash) const;

	/** Get the block blooms for a number of blocks. Thread-safe.
	 * @returns the object pertaining to the blocks:
	 * level 0:
	 * 0x, 0x + 1, .. (1x - 1)
	 * 1x, 1x + 1, .. (2x - 1)
	 * ...
	 * (255x .. (256x - 1))
	 * level 1:
	 * 0x .. (1x - 1), 1x .. (2x - 1), ..., (255x .. (256x - 1))
	 * 256x .. (257x - 1), 257x .. (258x - 1), ..., (511x .. (512x - 1))
	 * ...
	 * level n, index i, offset o:
	 * i * (x ^ n) + o * x ^ (n - 1)
	 */
	BlocksBlooms blocksBlooms(unsigned _level, unsigned _index) const { return blocksBlooms(chunkId(_level, _index)); }
	BlocksBlooms blocksBlooms(h256 const& _chunkId) const { return queryExtras<BlocksBlooms, ExtraBlocksBlooms>(_chunkId, m_blocksBlooms, x_blocksBlooms, NullBlocksBlooms); }
	LogBloom blockBloom(unsigned _number) const { return blocksBlooms(chunkId(0, _number / c_bloomIndexSize)).blooms[_number % c_bloomIndexSize]; }
	std::vector<unsigned> withBlockBloom(LogBloom const& _b, unsigned _earliest, unsigned _latest) const;
	std::vector<unsigned> withBlockBloom(LogBloom const& _b, unsigned _earliest, unsigned _latest, unsigned _topLevel, unsigned _index) const;

	/// Returns true if transaction is known. Thread-safe
	bool isKnownTransaction(h256 const& _transactionHash) const { TransactionAddress ta = queryExtras<TransactionAddress, ExtraTransactionAddress>(_transactionHash, m_transactionAddresses, x_transactionAddresses, NullTransactionAddress); return !!ta; }

	/// Get a transaction from its hash. Thread-safe.
	bytes transaction(h256 const& _transactionHash) const { TransactionAddress ta = queryExtras<TransactionAddress, ExtraTransactionAddress>(_transactionHash, m_transactionAddresses, x_transactionAddresses, NullTransactionAddress); if (!ta) return bytes(); return transaction(ta.blockHash, ta.index); }
	std::pair<h256, unsigned> transactionLocation(h256 const& _transactionHash) const { TransactionAddress ta = queryExtras<TransactionAddress, ExtraTransactionAddress>(_transactionHash, m_transactionAddresses, x_transactionAddresses, NullTransactionAddress); if (!ta) return std::pair<h256, unsigned>(h256(), 0); return std::make_pair(ta.blockHash, ta.index); }

	/// Get a block's transaction (RLP format) for the given block hash (or the most recent mined if none given) & index. Thread-safe.
	bytes transaction(h256 const& _blockHash, unsigned _i) const { bytes b = block(_blockHash); return RLP(b)[1][_i].data().toBytes(); }
	bytes transaction(unsigned _i) const { return transaction(currentHash(), _i); }

	/// Get all transactions from a block.
	std::vector<bytes> transactions(h256 const& _blockHash) const { bytes b = block(_blockHash); std::vector<bytes> ret; for (auto const& i: RLP(b)[1]) ret.push_back(i.data().toBytes()); return ret; }
	std::vector<bytes> transactions() const { return transactions(currentHash()); }

};

}
}
