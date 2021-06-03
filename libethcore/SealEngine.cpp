#include "SealEngine.h"
#include "Transaction.h"
#include <libevm/ExtVMFace.h>

using namespace std;
using namespace dev;
using namespace eth;

SealEngineRegistrar* SealEngineRegistrar::s_this = nullptr;

void NoProof::init()
{
	ETH_REGISTER_SEAL_ENGINE(NoProof);
}

