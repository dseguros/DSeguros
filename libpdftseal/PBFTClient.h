#pragma once

#include <libethereum/Client.h>

namespace dev
{
namespace eth
{

class PBFT;

DEV_SIMPLE_EXCEPTION(NotPBFTSealEngine);
DEV_SIMPLE_EXCEPTION(ChainParamsNotPBFT);
DEV_SIMPLE_EXCEPTION(InitFailed);

}
}
