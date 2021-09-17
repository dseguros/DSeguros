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

};
}
}

