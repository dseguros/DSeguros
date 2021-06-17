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

    /// Construct an alive Account, with given endowment, for either a normal (non-contract) account or for a
	/// contract account in the
	/// conception phase, where the code is not yet known.
	Account(u256 _nonce, u256 _balance, Changedness _c = Changed): m_isAlive(true), m_isUnchanged(_c == Unchanged), m_nonce(_nonce), m_balance(_balance) {}

	/// Explicit constructor for wierd cases of construction or a contract account.
	Account(u256 _nonce, u256 _balance, h256 _contractRoot, h256 _codeHash, Changedness _c): m_isAlive(true), m_isUnchanged(_c == Unchanged), m_nonce(_nonce), m_balance(_balance), m_storageRoot(_contractRoot), m_codeHash(_codeHash) { assert(_contractRoot); }


	/// Kill this account. Useful for the suicide opcode. Following this call, isAlive() returns false.
	void kill() { m_isAlive = false; m_storageOverlay.clear(); m_codeHash = EmptySHA3; m_storageRoot = EmptyTrie; m_balance = 0; m_nonce = 0; changed(); }

	/// @returns true iff this object represents an account in the state. Returns false if this object
	/// represents an account that should no longer exist in the trie (an account that never existed or was
	/// suicided).
	bool isAlive() const { return m_isAlive; }

	/// @returns true if the account is unchanged from creation.
	bool isDirty() const { return !m_isUnchanged; }

	void untouch() { m_isUnchanged = true; }
};

}
}
