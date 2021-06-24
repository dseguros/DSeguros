#include "Common.h"
#include "MemoryDB.h"
using namespace std;
using namespace dev;

namespace dev
{

const char* DBChannel::name() { return "TDB"; }
const char* DBWarn::name() { return "TDB"; }


std::unordered_map<h256, std::string> MemoryDB::get() const
{
#if DEV_GUARDED_DB
	ReadGuard l(x_this);
#endif
	std::unordered_map<h256, std::string> ret;
	for (auto const& i: m_main)
		if (!m_enforceRefs || i.second.second > 0)
			ret.insert(make_pair(i.first, i.second.first));
	return ret;
}

MemoryDB& MemoryDB::operator=(MemoryDB const& _c)
{
	if (this == &_c)
		return *this;
#if DEV_GUARDED_DB
	ReadGuard l(_c.x_this);
	WriteGuard l2(x_this);
#endif
	m_main = _c.m_main;
	m_aux = _c.m_aux;
	return *this;
}


std::string MemoryDB::lookup(h256 const& _h) const
{
#if DEV_GUARDED_DB
	ReadGuard l(x_this);
#endif
	auto it = m_main.find(_h);
	if (it != m_main.end())
	{
		if (!m_enforceRefs || it->second.second > 0)
			return it->second.first;
		else
			cwarn << "Lookup required for value with refcount == 0. This is probably a critical trie issue" << _h;
	}
	return std::string();
}

}
