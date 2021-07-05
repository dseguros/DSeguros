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

