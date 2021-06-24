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

}
