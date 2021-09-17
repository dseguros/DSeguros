#pragma once

#include <mutex>
#include <array>
#include <set>
#include <memory>
#include <utility>

#include <libdevcore/RLP.h>
#include <libdevcore/Guards.h>
#include <libdevcore/SHA3.h>
#include "Common.h"
#include "Message.h"

namespace dev
{
namespace shh
{

class Watch;

struct InstalledFilter
{
	InstalledFilter(Topics const& _t): full(_t), filter(_t) {}

	Topics full;
	TopicFilter filter;
	unsigned refCount = 1;
};

}

}

