#pragma once

#include <libdevcore/Common.h>
#include <libdevcore/RLP.h>
#include <libdevcore/TrieDB.h>
#include <libdevcore/SHA3.h>
#include <libethcore/Common.h>

namespace dev
{
namespace eth
{

class Account
{
public:
	/// Changedness of account to create.
	enum Changedness
	{
		/// Account starts as though it has been changed.
		Changed,
		/// Account starts as though it has not been changed.
		Unchanged
	};

	/// Construct a dead Account.
	Account() {}
};

}
}
