
#include <boost/filesystem.hpp>
#include <libethcore/ChainOperationParams.h>
#include <libethcore/CommonJS.h>
#include <libethereum/Interface.h>
#include <libethereum/BlockChain.h>
#include <libethereum/Block.h>
#include <libethereum/EthereumHost.h>
//#include <libethereum/NodeConnParamsManagerApi.h>
#include <libdevcrypto/Common.h>
#include "PBFT.h"
//#include <libdevcore/easylog.h>
//#include <libdevcore/LogGuard.h>
//#include <libethereum/StatLog.h>
//#include <libethereum/ConsensusControl.h>
using namespace std;
using namespace dev;
using namespace eth;

void PBFT::init()
{
	ETH_REGISTER_SEAL_ENGINE(PBFT);
}

PBFT::PBFT()
{
}

