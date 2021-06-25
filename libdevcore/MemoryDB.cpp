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

bool MemoryDB::exists(h256 const& _h) const
{
#if DEV_GUARDED_DB
	ReadGuard l(x_this);
#endif
	auto it = m_main.find(_h);
	if (it != m_main.end() && (!m_enforceRefs || it->second.second > 0))
		return true;
	return false;
}

void MemoryDB::insert(h256 const& _h, bytesConstRef _v)
{
#if DEV_GUARDED_DB
	WriteGuard l(x_this);
#endif
	auto it = m_main.find(_h);
	if (it != m_main.end())
	{
		it->second.first = _v.toString();
		it->second.second++;
	}
	else
		m_main[_h] = make_pair(_v.toString(), 1);
#if ETH_PARANOIA
	dbdebug << "INST" << _h << "=>" << m_main[_h].second;
#endif
}

bool MemoryDB::kill(h256 const& _h)
{
#if DEV_GUARDED_DB
	ReadGuard l(x_this);
#endif
	if (m_main.count(_h))
	{
		if (m_main[_h].second > 0)
		{
			m_main[_h].second--;
			return true;
		}
#if ETH_PARANOIA
		else
		{
			// If we get to this point, then there was probably a node in the level DB which we need to remove and which we have previously
			// used as part of the memory-based MemoryDB. Nothing to be worried about *as long as the node exists in the DB*.
			dbdebug << "NOKILL-WAS" << _h;
		}
		dbdebug << "KILL" << _h << "=>" << m_main[_h].second;
	}
	else
	{
		dbdebug << "NOKILL" << _h;
#endif
	}
	return false;
}
}
