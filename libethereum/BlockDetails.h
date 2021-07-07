#pragma once

#include <unordered_map>
#include <libdevcore/Log.h>
#include <libdevcore/RLP.h>
#include "TransactionReceipt.h"

namespace dev
{
namespace eth
{

// TODO: OPTIMISE: constructors take bytes, RLP used only in necessary classes.

static const unsigned c_bloomIndexSize = 16;
static const unsigned c_bloomIndexLevels = 2;

static const unsigned InvalidNumber = (unsigned)-1;

}
}

