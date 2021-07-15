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

/**
 * @brief Merkle Patricia Tree "Trie": a modifed base-16 Radix tree.
 * This version uses a database backend.
 * Usage:
 * @code
 * GenericTrieDB<MyDB> t(&myDB);
 * assert(t.isNull());
 * t.init();
 * assert(t.isEmpty());
 * t.insert(x, y);
 * assert(t.at(x) == y.toString());
 * t.remove(x);
 * assert(t.isEmpty());
 * @endcode
 */
template <class _DB>
class GenericTrieDB
{
public:
	using DB = _DB;

	explicit GenericTrieDB(DB* _db = nullptr): m_db(_db) {}
	GenericTrieDB(DB* _db, h256 const& _root, Verification _v = Verification::Normal) { open(_db, _root, _v); }
	~GenericTrieDB() {}

	void open(DB* _db) { m_db = _db; }
	void open(DB* _db, h256 const& _root, Verification _v = Verification::Normal) { m_db = _db; setRoot(_root, _v); }

	void init() { setRoot(forceInsertNode(&RLPNull)); assert(node(m_root).size()); }

	void setRoot(h256 const& _root, Verification _v = Verification::Normal)
	{
		m_root = _root;
		if (_v == Verification::Normal)
		{
			if (m_root == c_shaNull && !m_db->exists(m_root))
				init();
		}
		/*std::cout << "Setting root to " << _root << " (patched to " << m_root << ")" << std::endl;*/
#if ETH_DEBUG
		if (_v == Verification::Normal)
#endif
			if (!node(m_root).size())
				BOOST_THROW_EXCEPTION(RootNotFound());
	}

	/// True if the trie is uninitialised (i.e. that the DB doesn't contain the root node).
	bool isNull() const { return !node(m_root).size(); }
	/// True if the trie is initialised but empty (i.e. that the DB contains the root node which is empty).
	bool isEmpty() const { return m_root == c_shaNull && node(m_root).size(); }

	h256 const& root() const { if (node(m_root).empty()) BOOST_THROW_EXCEPTION(BadRoot(m_root)); /*std::cout << "Returning root as " << ret << " (really " << m_root << ")" << std::endl;*/ return m_root; }	// patch the root in the case of the empty trie. TODO: handle this properly.

	std::string at(bytes const& _key) const { return at(&_key); }
	std::string at(bytesConstRef _key) const;
	void insert(bytes const& _key, bytes const& _value) { insert(&_key, &_value); }
	void insert(bytesConstRef _key, bytes const& _value) { insert(_key, &_value); }
	void insert(bytes const& _key, bytesConstRef _value) { insert(&_key, _value); }
	void insert(bytesConstRef _key, bytesConstRef _value);
	void remove(bytes const& _key) { remove(&_key); }
	void remove(bytesConstRef _key);
	bool contains(bytes const& _key) { return contains(&_key); }
	bool contains(bytesConstRef _key) { return !at(_key).empty(); }

	class iterator
	{
	public:
		using value_type = std::pair<bytesConstRef, bytesConstRef>;

		iterator() {}
		explicit iterator(GenericTrieDB const* _db);
		iterator(GenericTrieDB const* _db, bytesConstRef _key);

		iterator& operator++() { next(); return *this; }

		value_type operator*() const { return at(); }
		value_type operator->() const { return at(); }

		bool operator==(iterator const& _c) const { return _c.m_trail == m_trail; }
		bool operator!=(iterator const& _c) const { return _c.m_trail != m_trail; }

		value_type at() const;

	private:
		void next();
		void next(NibbleSlice _key);

		struct Node
		{
			std::string rlp;
			std::string key;		// as hexPrefixEncoding.
			byte child;				// 255 -> entering, 16 -> actually at the node, 17 -> exiting, 0-15 -> actual children.

			// 255 -> 16 -> 0 -> 1 -> ... -> 15 -> 17

			void setChild(unsigned _i) { child = _i; }
			void setFirstChild() { child = 16; }
			void incrementChild() { child = child == 16 ? 0 : child == 15 ? 17 : (child + 1); }

			bool operator==(Node const& _c) const { return rlp == _c.rlp && key == _c.key && child == _c.child; }
			bool operator!=(Node const& _c) const { return !operator==(_c); }
		};

	protected:
		std::vector<Node> m_trail;
		GenericTrieDB<DB> const* m_that;
	};

	iterator begin() const { return iterator(this); }
	iterator end() const { return iterator(); }

	iterator lower_bound(bytesConstRef _key) const { return iterator(this, _key); }

	/// Used for debugging, scans the whole trie.
	void descendKey(h256 const& _k, h256Hash& _keyMask, bool _wasExt, std::ostream* _out, int _indent = 0) const
	{
		_keyMask.erase(_k);
		if (_k == m_root && _k == c_shaNull)	// root allowed to be empty
			return;
		descendList(RLP(node(_k)), _keyMask, _wasExt, _out, _indent);	// if not, it must be a list
	}

	/// Used for debugging, scans the whole trie.
	void descendEntry(RLP const& _r, h256Hash& _keyMask, bool _wasExt, std::ostream* _out, int _indent) const
	{
		if (_r.isData() && _r.size() == 32)
			descendKey(_r.toHash<h256>(), _keyMask, _wasExt, _out, _indent);
		else if (_r.isList())
			descendList(_r, _keyMask, _wasExt, _out, _indent);
		else
			BOOST_THROW_EXCEPTION(InvalidTrie());
	}

	/// Used for debugging, scans the whole trie.
	void descendList(RLP const& _r, h256Hash& _keyMask, bool _wasExt, std::ostream* _out, int _indent) const
	{
		if (_r.isList() && _r.itemCount() == 2 && (!_wasExt || _out))
		{
			if (_out)
				(*_out) << std::string(_indent * 2, ' ') << (_wasExt ? "!2 " : "2  ") << sha3(_r.data()) << ": " << _r << "\n";
			if (!isLeaf(_r))						// don't go down leaves
				descendEntry(_r[1], _keyMask, true, _out, _indent + 1);
		}
		else if (_r.isList() && _r.itemCount() == 17)
		{
			if (_out)
				(*_out) << std::string(_indent * 2, ' ') << "17 " << sha3(_r.data()) << ": " << _r << "\n";
			for (unsigned i = 0; i < 16; ++i)
				if (!_r[i].isEmpty())				// 16 branches are allowed to be empty
					descendEntry(_r[i], _keyMask, false, _out, _indent + 1);
		}
		else
			BOOST_THROW_EXCEPTION(InvalidTrie());
	}

};

}
