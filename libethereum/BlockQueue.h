#pragma once

#include <condition_variable>
#include <thread>
#include <deque>
#include <boost/thread.hpp>
#include <libdevcore/Common.h>
#include <libdevcore/Log.h>
#include <libethcore/Common.h>
#include <libdevcore/Guards.h>
#include <libethcore/Common.h>
#include <libethcore/BlockHeader.h>
#include "VerifiedBlock.h"

namespace dev
{

namespace eth
{

class BlockChain;

struct BlockQueueChannel: public LogChannel { static const char* name(); static const int verbosity = 4; };
struct BlockQueueTraceChannel: public LogChannel { static const char* name(); static const int verbosity = 7; };
#define cblockq dev::LogOutputStream<dev::eth::BlockQueueChannel, true>()

struct BlockQueueStatus
{
	size_t importing;
	size_t verified;
	size_t verifying;
	size_t unverified;
	size_t future;
	size_t unknown;
	size_t bad;
};

}
}
