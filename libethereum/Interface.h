#pragma once

#include <libdevcore/Common.h>
#include <libdevcore/CommonIO.h>
#include <libdevcore/Guards.h>
#include <libdevcrypto/Common.h>
#include <libethcore/SealEngine.h>
#include "GasPricer.h"
#include "LogFilter.h"
#include "Transaction.h"
#include "TransactionQueue.h"
#include "BlockDetails.h"

namespace dev
{
namespace eth
{

struct SyncStatus;

using TransactionHashes = h256s;
using UncleHashes = h256s;

enum class Reaping
{
	Automatic,
	Manual
};

enum class FudgeFactor
{
	Strict,
	Lenient
};

struct GasEstimationProgress
{
	u256 lowerBound;
	u256 upperBound;
};

using GasEstimationCallback = std::function<void(GasEstimationProgress const&)>;


}
}

namespace std
{

inline void swap(dev::eth::Watch& _a, dev::eth::Watch& _b)
{
	swap(_a.m_c, _b.m_c);
	swap(_a.m_id, _b.m_id);
}

}
