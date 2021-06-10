#include "FixedHash.h"
#include <ctime>
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace dev;

boost::random_device dev::s_fixedHashEngine;

h128 dev::fromUUID(std::string const& _uuid)
{
	try
	{
		return h128(boost::replace_all_copy(_uuid, "-", ""));
	}
	catch (...)
	{
		return h128();
	}
}

