#if !defined(ETH_EMSCRIPTEN)

#include <thread>
#include <libdevcore/db.h>
#include <libdevcore/Common.h>
#include "OverlayDB.h"
using namespace std;
using namespace dev;

namespace dev
{

h256 const EmptyTrie = sha3(rlp(""));

OverlayDB::~OverlayDB()
{
	if (m_db.use_count() == 1 && m_db.get())
		ctrace << "Closing state DB";
}

}

#endif // ETH_EMSCRIPTEN
