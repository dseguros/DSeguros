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

}}
