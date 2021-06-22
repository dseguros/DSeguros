#pragma once

#include <unordered_map>
#include "Common.h"
#include "Guards.h"
#include "FixedHash.h"
#include "Log.h"
#include "RLP.h"
#include "SHA3.h"

namespace dev
{

struct DBChannel: public LogChannel  { static const char* name(); static const int verbosity = 18; };
struct DBWarn: public LogChannel  { static const char* name(); static const int verbosity = 1; };

#define dbdebug clog(DBChannel)
#define dbwarn clog(DBWarn)

class MemoryDB
{
	friend class EnforceRefs;

public:
	MemoryDB() {}
	MemoryDB(MemoryDB const& _c) { operator=(_c); }

	MemoryDB& operator=(MemoryDB const& _c);

	void clear() { m_main.clear(); m_aux.clear(); }	// WARNING !!!! didn't originally clear m_refCount!!!
	std::unordered_map<h256, std::string> get() const;

};

}
