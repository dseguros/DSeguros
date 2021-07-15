#pragma once

#include <memory>
#include "db.h"
#include "Common.h"
#include "Log.h"
#include "Exceptions.h"
#include "SHA3.h"
#include "MemoryDB.h"
#include "TrieCommon.h"

namespace dev
{

struct TrieDBChannel: public LogChannel  { static const char* name(); static const int verbosity = 17; };
#define tdebug clog(TrieDBChannel)

struct InvalidTrie: virtual dev::Exception {};
extern const h256 c_shaNull;
extern const h256 EmptyTrie;

enum class Verification {
	Skip,
	Normal
};
}
