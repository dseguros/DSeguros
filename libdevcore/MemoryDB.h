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

        std::string lookup(h256 const& _h) const;
	bool exists(h256 const& _h) const;
	void insert(h256 const& _h, bytesConstRef _v);
	bool kill(h256 const& _h);
	void purge();

	bytes lookupAux(h256 const& _h) const;
	void removeAux(h256 const& _h);
	void insertAux(h256 const& _h, bytesConstRef _v);

	h256Hash keys() const;

protected:
#if DEV_GUARDED_DB
	mutable SharedMutex x_this;
#endif
	std::unordered_map<h256, std::pair<std::string, unsigned>> m_main;
	std::unordered_map<h256, std::pair<bytes, bool>> m_aux;

	mutable bool m_enforceRefs = false;
};

}
