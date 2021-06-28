#pragma once

#include <memory>
#include <libdevcore/db.h>
#include <libdevcore/Common.h>
#include <libdevcore/Log.h>
#include <libdevcore/MemoryDB.h>

namespace dev
{

class OverlayDB: public MemoryDB
{
public:
	OverlayDB(ldb::DB* _db = nullptr): m_db(_db) {}
	~OverlayDB();

	ldb::DB* db() const { return m_db.get(); }

	void commit();
	void rollback();

	std::string lookup(h256 const& _h) const;
	bool exists(h256 const& _h) const;
	void kill(h256 const& _h);
	bool deepkill(h256 const& _h);

	bytes lookupAux(h256 const& _h) const;

private:
	using MemoryDB::clear;

	std::shared_ptr<ldb::DB> m_db;

	ldb::ReadOptions m_readOptions;
	ldb::WriteOptions m_writeOptions;
};

}
