#pragma once

#include <functional>
#include <condition_variable>
#include <thread>
#include <deque>
#include <libdevcore/Common.h>
#include <libdevcore/Guards.h>
#include <libdevcore/Log.h>
#include <libethcore/Common.h>
#include "Transaction.h"

namespace dev
{
namespace eth
{

struct TransactionQueueChannel: public LogChannel { static const char* name(); static const int verbosity = 4; };
struct TransactionQueueTraceChannel: public LogChannel { static const char* name(); static const int verbosity = 7; };
#define ctxq dev::LogOutputStream<dev::eth::TransactionQueueTraceChannel, true>()

class TransactionQueue
{
public:
	struct Limits { size_t current; size_t future; };

	/// @brief TransactionQueue
	/// @param _limit Maximum number of pending transactions in the queue.
	/// @param _futureLimit Maximum number of future nonce transactions.
	TransactionQueue(unsigned _limit = 1024, unsigned _futureLimit = 1024);
	TransactionQueue(Limits const& _l): TransactionQueue(_l.current, _l.future) {}
	~TransactionQueue();
	/// Add transaction to the queue to be verified and imported.
	/// @param _data RLP encoded transaction data.
	/// @param _nodeId Optional network identified of a node transaction comes from.
	void enqueue(RLP const& _data, h512 const& _nodeId);

	/// Verify and add transaction to the queue synchronously.
	/// @param _tx RLP encoded transaction data.
	/// @param _ik Set to Retry to force re-addinga transaction that was previously dropped.
	/// @returns Import result code.
	ImportResult import(bytes const& _tx, IfDropped _ik = IfDropped::Ignore) { return import(&_tx, _ik); }

	/// Verify and add transaction to the queue synchronously.
	/// @param _tx Trasnaction data.
	/// @param _ik Set to Retry to force re-addinga transaction that was previously dropped.
	/// @returns Import result code.
	ImportResult import(Transaction const& _tx, IfDropped _ik = IfDropped::Ignore);

	/// Remove transaction from the queue
	/// @param _txHash Trasnaction hash
	void drop(h256 const& _txHash);

	/// Get number of pending transactions for account.
	/// @returns Pending transaction count.
	unsigned waiting(Address const& _a) const;

	/// Get top transactions from the queue. Returned transactions are not removed from the queue automatically.
	/// @param _limit Max number of transactions to return.
	/// @param _avoid Transactions to avoid returning.
	/// @returns up to _limit transactions ordered by nonce and gas price.
	Transactions topTransactions(unsigned _limit, h256Hash const& _avoid = h256Hash()) const;

	Transactions allTransactions() const;

	/// Get a hash set of transactions in the queue
	/// @returns A hash set of all transactions in the queue
	h256Hash knownTransactions() const;

	/// Get max nonce for an account
	/// @returns Max transaction nonce for account in the queue
	u256 maxNonce(Address const& _a) const;

	/// Mark transaction as future. It wont be retured in topTransactions list until a transaction with a preceeding nonce is imported or marked with dropGood
	/// @param _t Transaction hash
	void setFuture(h256 const& _t);

	/// Drop a trasnaction from the list if exists and move following future trasnactions to current (if any)
	/// @param _t Transaction hash
	void dropGood(Transaction const& _t);

	struct Status
	{
		size_t current;
		size_t future;
		size_t unverified;
		size_t dropped;
	};
	/// @returns the status of the transaction queue.
	Status status() const { Status ret; DEV_GUARDED(x_queue) { ret.unverified = m_unverified.size(); } ReadGuard l(m_lock); ret.dropped = m_dropped.size(); ret.current = m_currentByHash.size(); ret.future = m_future.size(); return ret; }

	/// @returns the transacrtion limits on current/future.
	Limits limits() const { return Limits{m_limit, m_futureLimit}; }

	/// Clear the queue
	void clear();

	/// Register a handler that will be called once there is a new transaction imported
	template <class T> Handler<> onReady(T const& _t) { return m_onReady.add(_t); }

	/// Register a handler that will be called once asynchronous verification is comeplte an transaction has been imported
	template <class T> Handler<ImportResult, h256 const&, h512 const&> onImport(T const& _t) { return m_onImport.add(_t); }

	/// Register a handler that will be called once asynchronous verification is comeplte an transaction has been imported
	template <class T> Handler<h256 const&> onReplaced(T const& _t) { return m_onReplaced.add(_t); }
};
}
}

