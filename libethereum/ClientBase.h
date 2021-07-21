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

}}
