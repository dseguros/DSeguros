#pragma once

#include <unordered_map>
#include "Common.h"
#include "Guards.h"
#include "FixedHash.h"
#include "Log.h"
#include "RLP.h"
#include "SHA3.h"

namespace dev
{

struct DBChannel: public LogChannel  { static const char* name(); static const int verbosity = 18; };
struct DBWarn: public LogChannel  { static const char* name(); static const int verbosity = 1; };

}
