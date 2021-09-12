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
