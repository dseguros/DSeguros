#include "CommonData.h"
#include <random>

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4724) // potential mod by 0, line 78 of boost/random/uniform_int_distribution.hpp (boost 1.55)
#endif
#include <boost/random/uniform_int_distribution.hpp>
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
#include "Exceptions.h"
#include "Log.h"
using namespace std;
using namespace dev;


namespace
{
int fromHexChar(char _i) noexcept
{
	if (_i >= '0' && _i <= '9')
		return _i - '0';
	if (_i >= 'a' && _i <= 'f')
		return _i - 'a' + 10;
	if (_i >= 'A' && _i <= 'F')
		return _i - 'A' + 10;
	return -1;
}
}

bool dev::isHex(string const& _s) noexcept
{
	auto it = _s.begin();
	if (_s.compare(0, 2, "0x") == 0)
		it += 2;
	return std::all_of(it, _s.end(), [](char c){ return fromHexChar(c) != -1; });
}

std::string dev::escaped(std::string const& _s, bool _all)
{
	static const map<char, char> prettyEscapes{{'\r', 'r'}, {'\n', 'n'}, {'\t', 't'}, {'\v', 'v'}};
	std::string ret;
	ret.reserve(_s.size() + 2);
	ret.push_back('"');
	for (auto i: _s)
		if (i == '"' && !_all)
			ret += "\\\"";
		else if (i == '\\' && !_all)
			ret += "\\\\";
		else if (prettyEscapes.count(i) && !_all)
		{
			ret += '\\';
			ret += prettyEscapes.find(i)->second;
		}
		else if (i < ' ' || _all)
		{
			ret += "\\x";
			ret.push_back("0123456789abcdef"[(uint8_t)i / 16]);
			ret.push_back("0123456789abcdef"[(uint8_t)i % 16]);
		}
		else
			ret.push_back(i);
	ret.push_back('"');
	return ret;
}

std::string dev::randomWord()
{
	static std::mt19937_64 s_eng(0);
	std::string ret(boost::random::uniform_int_distribution<int>(1, 5)(s_eng), ' ');
	char const n[] = "qwertyuiop";//asdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM1234567890";
	boost::random::uniform_int_distribution<int> d(0, sizeof(n) - 2);
	for (char& c: ret)
		c = n[d(s_eng)];
	return ret;
}
