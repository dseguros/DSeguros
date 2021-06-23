
#pragma once

#include <array>
#include <unordered_map>
#include <libdevcore/Common.h>
#include <libdevcore/RLP.h>
#include <libdevcore/TrieDB.h>
#include <libdevcore/OverlayDB.h>
#include <libethcore/Exceptions.h>
#include <libethcore/BlockHeader.h>
#include <libethcore/ChainOperationParams.h>
#include <libevm/ExtVMFace.h>
#include "Account.h"
#include "Transaction.h"
#include "TransactionReceipt.h"
#include "GasPricer.h"
#include "State.h"

namespace dev
{

namespace test { class ImportTest; class StateLoader; }

namespace eth
{

class SealEngineFace;
class BlockChain;
class State;
class TransactionQueue;
struct VerifiedBlockRef;

struct BlockChat: public LogChannel { static const char* name(); static const int verbosity = 4; };
struct BlockTrace: public LogChannel { static const char* name(); static const int verbosity = 5; };
struct BlockDetail: public LogChannel { static const char* name(); static const int verbosity = 14; };
struct BlockSafeExceptions: public LogChannel { static const char* name(); static const int verbosity = 21; };

}

}
