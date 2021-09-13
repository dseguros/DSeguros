#include "LogFilter.h"

#include <libdevcore/SHA3.h>
#include "Block.h"
using namespace std;
using namespace dev;
using namespace dev::eth;


std::ostream& dev::eth::operator<<(std::ostream& _out, LogFilter const& _s)
{
	// TODO
	_out << "(@" << _s.m_addresses << "#" << _s.m_topics << ">" << _s.m_earliest << "-" << _s.m_latest << "< )";
	return _out;
}

void LogFilter::streamRLP(RLPStream& _s) const
{
	_s.appendList(4) << m_addresses << m_topics << m_earliest << m_latest;
}

h256 LogFilter::sha3() const
{
	RLPStream s;
	streamRLP(s);
	return dev::sha3(s.out());
}

bool LogFilter::isRangeFilter() const
{
	if (m_addresses.size())
		return false;

	for (auto const& t: m_topics)
		if (t.size())
			return false;

	return true;
}

bool LogFilter::matches(LogBloom _bloom) const
{
	if (m_addresses.size())
	{
		for (auto const& i: m_addresses)
			if (_bloom.containsBloom<3>(dev::sha3(i)))
				goto OK1;
		return false;
	}
	OK1:
	for (auto const& t: m_topics)
		if (t.size())
		{
			for (auto const& i: t)
				if (_bloom.containsBloom<3>(dev::sha3(i)))
					goto OK2;
			return false;
			OK2:;
		}
	return true;
}
