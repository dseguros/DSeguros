#pragma once

#include <chrono>
#include "Interface.h"
#include "LogFilter.h"
#include "TransactionQueue.h"
#include "Block.h"
#include "CommonNet.h"

namespace dev
{

namespace eth
{

struct InstalledFilter
{
	InstalledFilter(LogFilter const& _f): filter(_f) {}

	LogFilter filter;
	unsigned refCount = 1;
	LocalisedLogEntries changes;
};

static const h256 PendingChangedFilter = u256(0);
static const h256 ChainChangedFilter = u256(1);

static const LogEntry SpecialLogEntry = LogEntry(Address(), h256s(), bytes());
static const LocalisedLogEntry InitialChange(SpecialLogEntry);

struct ClientWatch
{
	ClientWatch(): lastPoll(std::chrono::system_clock::now()) {}
	explicit ClientWatch(h256 _id, Reaping _r): id(_id), lastPoll(_r == Reaping::Automatic ? std::chrono::system_clock::now() : std::chrono::system_clock::time_point::max()) {}

	h256 id;
#if INITIAL_STATE_AS_CHANGES
	LocalisedLogEntries changes = LocalisedLogEntries{ InitialChange };
#else
	LocalisedLogEntries changes;
#endif
	mutable std::chrono::system_clock::time_point lastPoll = std::chrono::system_clock::now();
};

struct WatchChannel: public LogChannel { static const char* name(); static const int verbosity = 7; };
#define cwatch LogOutputStream<WatchChannel, true>()
struct WorkInChannel: public LogChannel { static const char* name(); static const int verbosity = 16; };
struct WorkOutChannel: public LogChannel { static const char* name(); static const int verbosity = 16; };
struct WorkChannel: public LogChannel { static const char* name(); static const int verbosity = 21; };
#define cwork LogOutputStream<WorkChannel, true>()
#define cworkin LogOutputStream<WorkInChannel, true>()
#define cworkout LogOutputStream<WorkOutChannel, true>()

class ClientBase: public Interface
{
public:
	ClientBase(TransactionQueue::Limits const& _l = TransactionQueue::Limits{1024, 1024}): m_tq(_l) {}
	virtual ~ClientBase() {}

	/// Submits the given transaction.
	/// @returns the new transaction's hash.
	virtual std::pair<h256, Address> submitTransaction(TransactionSkeleton const& _t, Secret const& _secret) override;
	using Interface::submitTransaction;

	/// Makes the given call. Nothing is recorded into the state.
	virtual ExecutionResult call(Address const& _secret, u256 _value, Address _dest, bytes const& _data, u256 _gas, u256 _gasPrice, BlockNumber _blockNumber, FudgeFactor _ff = FudgeFactor::Strict) override;
	using Interface::call;

	/// Makes the given create. Nothing is recorded into the state.
	virtual ExecutionResult create(Address const& _secret, u256 _value, bytes const& _data, u256 _gas, u256 _gasPrice, BlockNumber _blockNumber, FudgeFactor _ff = FudgeFactor::Strict) override;

	/// Estimate gas usage for call/create.
	/// @param _maxGas An upper bound value for estimation, if not provided default value of c_maxGasEstimate will be used.
	/// @param _callback Optional callback function for progress reporting
	virtual std::pair<u256, ExecutionResult> estimateGas(Address const& _from, u256 _value, Address _dest, bytes const& _data, int64_t _maxGas, u256 _gasPrice, BlockNumber _blockNumber, GasEstimationCallback const& _callback) override;

	using Interface::create;

	using Interface::balanceAt;
	using Interface::countAt;
	using Interface::stateAt;
	using Interface::codeAt;
	using Interface::codeHashAt;
	using Interface::storageAt;

}

}}
