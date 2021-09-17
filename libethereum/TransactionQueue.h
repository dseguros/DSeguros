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

}
}

