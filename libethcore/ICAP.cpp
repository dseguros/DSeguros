#include "ICAP.h"
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string.hpp>
#include <libdevcore/Base64.h>
#include <libdevcore/SHA3.h>
#include "Exceptions.h"
#include "ABI.h"
using namespace std;
using namespace dev;
using namespace dev::eth;

namespace dev
{
namespace eth
{

string ICAP::iban(std::string _c, std::string _d)
{
	boost::to_upper(_c);
	boost::to_upper(_d);
	auto totStr = _d + _c + "00";
	bigint tot = 0;
	for (char x: totStr)
		if (x >= 'A')
			tot = tot * 100 + x - 'A' + 10;
		else
			tot = tot * 10 + x - '0';
	unsigned check = (unsigned)(u256)(98 - tot % 97);
	ostringstream out;
	out << _c << setfill('0') << setw(2) << check << _d;
	return out.str();
}

std::pair<string, string> ICAP::fromIBAN(std::string _iban)
{
	if (_iban.size() < 4)
		return std::make_pair(string(), string());
	boost::to_upper(_iban);
	std::string c = _iban.substr(0, 2);
	std::string d = _iban.substr(4);
	if (iban(c, d) != _iban)
		return std::make_pair(string(), string());
	return make_pair(c, d);
}

}
}
