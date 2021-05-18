#pragma once

#include <string>
#include <functional>
#include <boost/algorithm/string/case_conv.hpp>
#include <libdevcore/Common.h>
#include <libdevcore/Exceptions.h>
#include <libdevcore/FixedHash.h>
#include "Common.h"

namespace dev
{
namespace eth
{

DEV_SIMPLE_EXCEPTION(InvalidICAP);

/**
 * @brief Encapsulation of an ICAP address.
 * Can be encoded, decoded, looked-up and inspected.
 */
class ICAP
{
public:
	/// Construct null ICAP object.
	ICAP() = default;
	/// Construct a direct ICAP object for given target address. Must have a zero first byte.
	ICAP(Address const& _target): m_type(Direct), m_direct(_target) {}
	/// Construct an indirect ICAP object for given client and institution names.
	ICAP(std::string const& _client, std::string const& _inst): m_type(Indirect), m_client(boost::algorithm::to_upper_copy(_client)), m_institution(boost::algorithm::to_upper_copy(_inst)), m_asset("XET") {}


}

}

}


