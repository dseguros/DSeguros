#pragma once

#include <libdevcore/RLP.h>
#include <libdevcore/SHA3.h>
#include <libethcore/Common.h>

namespace dev
{
namespace eth
{

struct EVMSchedule;

/// Named-boolean type to encode whether a signature be included in the serialisation process.
enum IncludeSignature
{
	WithoutSignature = 0,	///< Do not include a signature.
	WithSignature = 1,		///< Do include a signature.
};

}
}

