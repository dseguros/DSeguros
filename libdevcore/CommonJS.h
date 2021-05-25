#pragma once

#include <string>
#include "FixedHash.h"
#include "CommonData.h"
#include "CommonIO.h"

namespace dev
{

template <unsigned S> std::string toJS(FixedHash<S> const& _h)
{
	return "0x" + toHex(_h.ref());
}

}

