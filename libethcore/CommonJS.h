#pragma once

#include <string>
#include <libdevcore/CommonJS.h>
#include <libdevcrypto/Common.h>
#include "Common.h"

namespace dev
{

/// Leniently convert string to Public (h512). Accepts integers, "0x" prefixing, non-exact length.
inline Public jsToPublic(std::string const& _s) { return jsToFixed<sizeof(dev::Public)>(_s); }

}

