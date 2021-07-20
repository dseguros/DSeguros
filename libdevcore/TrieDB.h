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

	/// Used for debugging, scans the whole trie.
	h256Hash leftOvers(std::ostream* _out = nullptr) const
	{
		h256Hash k = m_db->keys();
		descendKey(m_root, k, false, _out);
		return k;
	}

	/// Used for debugging, scans the whole trie.
	void debugStructure(std::ostream& _out) const
	{
		leftOvers(&_out);
	}

	/// Used for debugging, scans the whole trie.
	/// @param _requireNoLeftOvers if true, requires that all keys are reachable.
	bool check(bool _requireNoLeftOvers) const
	{
		try
		{
			return leftOvers().empty() || !_requireNoLeftOvers;
		}
		catch (...)
		{
			cwarn << boost::current_exception_diagnostic_information();
			return false;
		}
	}

	/// Get the underlying database.
	/// @warning This can be used to bypass the trie code. Don't use these unless you *really*
	/// know what you're doing.
	DB const* db() const { return m_db; }
	DB* db() { return m_db; }

private:
	RLPStream& streamNode(RLPStream& _s, bytes const& _b);

	std::string atAux(RLP const& _here, NibbleSlice _key) const;

	void mergeAtAux(RLPStream& _out, RLP const& _replace, NibbleSlice _key, bytesConstRef _value);
	bytes mergeAt(RLP const& _replace, NibbleSlice _k, bytesConstRef _v, bool _inLine = false);
	bytes mergeAt(RLP const& _replace, h256 const& _replaceHash, NibbleSlice _k, bytesConstRef _v, bool _inLine = false);

	bool deleteAtAux(RLPStream& _out, RLP const& _replace, NibbleSlice _key);
	bytes deleteAt(RLP const& _replace, NibbleSlice _k);

	// in: null (DEL)  -- OR --  [_k, V] (DEL)
	// out: [_k, _s]
	// -- OR --
	// in: [V0, ..., V15, S16] (DEL)  AND  _k == {}
	// out: [V0, ..., V15, _s]
	bytes place(RLP const& _orig, NibbleSlice _k, bytesConstRef _s);

	// in: [K, S] (DEL)
	// out: null
	// -- OR --
	// in: [V0, ..., V15, S] (DEL)
	// out: [V0, ..., V15, null]
	bytes remove(RLP const& _orig);

	// in: [K1 & K2, V] (DEL) : nibbles(K1) == _s, 0 < _s <= nibbles(K1 & K2)
	// out: [K1, H] ; [K2, V] => H (INS)  (being  [K1, [K2, V]]  if necessary)
	bytes cleve(RLP const& _orig, unsigned _s);

	// in: [K1, H] (DEL) ; H <= [K2, V] (DEL)  (being  [K1, [K2, V]] (DEL)  if necessary)
	// out: [K1 & K2, V]
	bytes graft(RLP const& _orig);

	// in: [V0, ... V15, S] (DEL)
	// out1: [k{i}, Vi]    where i < 16
	// out2: [k{}, S]      where i == 16
	bytes merge(RLP const& _orig, byte _i);

	// in: [k{}, S] (DEL)
	// out: [null ** 16, S]
	// -- OR --
	// in: [k{i}, N] (DEL)
	// out: [null ** i, N, null ** (16 - i)]
	// -- OR --
	// in: [k{i}K, V] (DEL)
	// out: [null ** i, H, null ** (16 - i)] ; [K, V] => H (INS)  (being [null ** i, [K, V], null ** (16 - i)]  if necessary)
	bytes branch(RLP const& _orig);

	bool isTwoItemNode(RLP const& _n) const;
	std::string deref(RLP const& _n) const;

	std::string node(h256 const& _h) const { return m_db->lookup(_h); }

	// These are low-level node insertion functions that just go straight through into the DB.
	h256 forceInsertNode(bytesConstRef _v) { auto h = sha3(_v); forceInsertNode(h, _v); return h; }
	void forceInsertNode(h256 const& _h, bytesConstRef _v) { m_db->insert(_h, _v); }
	void forceKillNode(h256 const& _h) { m_db->kill(_h); }

	// This are semantically-aware node insertion functions that only kills when the node's
	// data is < 32 bytes. It can safely be used when pruning the trie but won't work correctly
	// for the special case of the root (which is always looked up via a hash). In that case,
	// use forceKillNode().
	void killNode(RLP const& _d) { if (_d.data().size() >= 32) forceKillNode(sha3(_d.data())); }
	void killNode(RLP const& _d, h256 const& _h) { if (_d.data().size() >= 32) forceKillNode(_h); }

	h256 m_root;
	DB* m_db = nullptr;
};

template <class DB>
std::ostream& operator<<(std::ostream& _out, GenericTrieDB<DB> const& _db)
{
	for (auto const& i: _db)
		_out << escaped(i.first.toString(), false) << ": " << escaped(i.second.toString(), false) << std::endl;
	return _out;
}

/**
 * Different view on a GenericTrieDB that can use different key types.
 */
template <class Generic, class _KeyType>
class SpecificTrieDB: public Generic
{
public:
	using DB = typename Generic::DB;
	using KeyType = _KeyType;

	SpecificTrieDB(DB* _db = nullptr): Generic(_db) {}
	SpecificTrieDB(DB* _db, h256 _root, Verification _v = Verification::Normal): Generic(_db, _root, _v) {}

	std::string operator[](KeyType _k) const { return at(_k); }

	bool contains(KeyType _k) const { return Generic::contains(bytesConstRef((byte const*)&_k, sizeof(KeyType))); }
	std::string at(KeyType _k) const { return Generic::at(bytesConstRef((byte const*)&_k, sizeof(KeyType))); }
	void insert(KeyType _k, bytesConstRef _value) { Generic::insert(bytesConstRef((byte const*)&_k, sizeof(KeyType)), _value); }
	void insert(KeyType _k, bytes const& _value) { insert(_k, bytesConstRef(&_value)); }
	void remove(KeyType _k) { Generic::remove(bytesConstRef((byte const*)&_k, sizeof(KeyType))); }

	class iterator: public Generic::iterator
	{
	public:
		using Super = typename Generic::iterator;
		using value_type = std::pair<KeyType, bytesConstRef>;

		iterator() {}
		iterator(Generic const* _db): Super(_db) {}
		iterator(Generic const* _db, bytesConstRef _k): Super(_db, _k) {}

		value_type operator*() const { return at(); }
		value_type operator->() const { return at(); }

		value_type at() const;
	};

	iterator begin() const { return this; }
	iterator end() const { return iterator(); }
	iterator lower_bound(KeyType _k) const { return iterator(this, bytesConstRef((byte const*)&_k, sizeof(KeyType))); }
};

template <class Generic, class KeyType>
std::ostream& operator<<(std::ostream& _out, SpecificTrieDB<Generic, KeyType> const& _db)
{
	for (auto const& i: _db)
		_out << i.first << ": " << escaped(i.second.toString(), false) << std::endl;
	return _out;
}

template <class _DB>
class HashedGenericTrieDB: private SpecificTrieDB<GenericTrieDB<_DB>, h256>
{
	using Super = SpecificTrieDB<GenericTrieDB<_DB>, h256>;

public:
	using DB = _DB;

	HashedGenericTrieDB(DB* _db = nullptr): Super(_db) {}
	HashedGenericTrieDB(DB* _db, h256 _root, Verification _v = Verification::Normal): Super(_db, _root, _v) {}

	using Super::open;
	using Super::init;
	using Super::setRoot;

	/// True if the trie is uninitialised (i.e. that the DB doesn't contain the root node).
	using Super::isNull;
	/// True if the trie is initialised but empty (i.e. that the DB contains the root node which is empty).
	using Super::isEmpty;

	using Super::root;
	using Super::db;

	using Super::leftOvers;
	using Super::check;
	using Super::debugStructure;

	std::string at(bytesConstRef _key) const { return Super::at(sha3(_key)); }
	bool contains(bytesConstRef _key) { return Super::contains(sha3(_key)); }
	void insert(bytesConstRef _key, bytesConstRef _value) { Super::insert(sha3(_key), _value); }
	void remove(bytesConstRef _key) { Super::remove(sha3(_key)); }

	// empty from the PoV of the iterator interface; still need a basic iterator impl though.
	class iterator
	{
	public:
		using value_type = std::pair<bytesConstRef, bytesConstRef>;

		iterator() {}
		iterator(HashedGenericTrieDB const*) {}
		iterator(HashedGenericTrieDB const*, bytesConstRef) {}

		iterator& operator++() { return *this; }
		value_type operator*() const { return value_type(); }
		value_type operator->() const { return value_type(); }

		bool operator==(iterator const&) const { return true; }
		bool operator!=(iterator const&) const { return false; }

		value_type at() const { return value_type(); }
	};
	iterator begin() const { return iterator(); }
	iterator end() const { return iterator(); }
	iterator lower_bound(bytesConstRef) const { return iterator(); }
};

// Hashed & Hash-key mapping
template <class _DB>
class FatGenericTrieDB: private SpecificTrieDB<GenericTrieDB<_DB>, h256>
{
	using Super = SpecificTrieDB<GenericTrieDB<_DB>, h256>;

public:
	using DB = _DB;
	FatGenericTrieDB(DB* _db = nullptr): Super(_db) {}
	FatGenericTrieDB(DB* _db, h256 _root, Verification _v = Verification::Normal): Super(_db, _root, _v) {}

	using Super::init;
	using Super::isNull;
	using Super::isEmpty;
	using Super::root;
	using Super::leftOvers;
	using Super::check;
	using Super::open;
	using Super::setRoot;
	using Super::db;
	using Super::debugStructure;

	std::string at(bytesConstRef _key) const { return Super::at(sha3(_key)); }
	bool contains(bytesConstRef _key) { return Super::contains(sha3(_key)); }
	void insert(bytesConstRef _key, bytesConstRef _value)
	{
		h256 hash = sha3(_key);
		Super::insert(hash, _value);
		Super::db()->insertAux(hash, _key);
	}

	void remove(bytesConstRef _key) { Super::remove(sha3(_key)); }

	// iterates over <key, value> pairs
	class iterator: public GenericTrieDB<_DB>::iterator
	{
	public:
		using Super = typename GenericTrieDB<_DB>::iterator;

		iterator() { }
		iterator(FatGenericTrieDB const* _trie) : Super(_trie) { }

		typename Super::value_type at() const
		{
			auto hashed = Super::at();
			m_key = static_cast<FatGenericTrieDB const*>(Super::m_that)->db()->lookupAux(h256(hashed.first));
			return std::make_pair(&m_key, std::move(hashed.second));
		}

	private:
		mutable bytes m_key;
	};

	iterator begin() const { return iterator(); }
	iterator end() const { return iterator(); }

	// iterates over <hashedKey, value> pairs
	class HashedIterator: public GenericTrieDB<_DB>::iterator
	{
	public:
		using Super = typename GenericTrieDB<_DB>::iterator;

		HashedIterator() {}
		HashedIterator(FatGenericTrieDB const* _trie) : Super(_trie) {}

		bytes key() const
		{
			auto hashed = Super::at();
			return static_cast<FatGenericTrieDB const*>(Super::m_that)->db()->lookupAux(h256(hashed.first));
		}
	};

	HashedIterator hashedBegin() const { return HashedIterator(this); }
	HashedIterator hashedEnd() const { return HashedIterator(); }
};

template <class KeyType, class DB> using TrieDB = SpecificTrieDB<GenericTrieDB<DB>, KeyType>;

}

// Template implementations...
namespace dev
{

template <class DB> GenericTrieDB<DB>::iterator::iterator(GenericTrieDB const* _db)
{
	m_that = _db;
	m_trail.push_back({_db->node(_db->m_root), std::string(1, '\0'), 255});	// one null byte is the HPE for the empty key.
	next();
}

template <class DB> GenericTrieDB<DB>::iterator::iterator(GenericTrieDB const* _db, bytesConstRef _fullKey)
{
	m_that = _db;
	m_trail.push_back({_db->node(_db->m_root), std::string(1, '\0'), 255});	// one null byte is the HPE for the empty key.
	next(_fullKey);
}

template <class DB> typename GenericTrieDB<DB>::iterator::value_type GenericTrieDB<DB>::iterator::at() const
{
	assert(m_trail.size());
	Node const& b = m_trail.back();
	assert(b.key.size());
	assert(!(b.key[0] & 0x10));	// should be an integer number of bytes (i.e. not an odd number of nibbles).

	RLP rlp(b.rlp);
	return std::make_pair(bytesConstRef(b.key).cropped(1), rlp[rlp.itemCount() == 2 ? 1 : 16].payload());
}

template <class DB> void GenericTrieDB<DB>::iterator::next(NibbleSlice _key)
{
	NibbleSlice k = _key;
	while (true)
	{
		if (m_trail.empty())
		{
			m_that = nullptr;
			return;
		}

		Node const& b = m_trail.back();
		RLP rlp(b.rlp);

		if (m_trail.back().child == 255)
		{
			// Entering. Look for first...
			if (rlp.isEmpty())
			{
				// Kill our search as soon as we hit an empty node.
				k.clear();
				m_trail.pop_back();
				continue;
			}
			if (!rlp.isList() || (rlp.itemCount() != 2 && rlp.itemCount() != 17))
			{
#if ETH_PARANOIA
				cwarn << "BIG FAT ERROR. STATE TRIE CORRUPTED!!!!!";
				cwarn << b.rlp.size() << toHex(b.rlp);
				cwarn << rlp;
				auto c = rlp.itemCount();
				cwarn << c;
				BOOST_THROW_EXCEPTION(InvalidTrie());
#else
				m_that = nullptr;
				return;
#endif
			}
			if (rlp.itemCount() == 2)
			{
				// Just turn it into a valid Branch
				auto keyOfRLP = keyOf(rlp);

				// TODO: do something different depending on how keyOfRLP compares to k.mid(0, std::min(k.size(), keyOfRLP.size()));
				// if == all is good - continue descent.
				// if > discard key and continue descent.
				// if < discard key and skip node.

				if (!k.contains(keyOfRLP))
				{
					if (!k.isEarlierThan(keyOfRLP))
					{
						k.clear();
						m_trail.pop_back();
						continue;
					}
					k.clear();
				}

				k = k.mid(std::min(k.size(), keyOfRLP.size()));
				m_trail.back().key = hexPrefixEncode(keyOf(m_trail.back().key), keyOfRLP, false);
				if (isLeaf(rlp))
				{
					// leaf - exit now.
					if (k.empty())
					{
						m_trail.back().child = 0;
						return;
					}
					// Still data in key we're supposed to be looking for when we're at a leaf. Go for next one.
					k.clear();
					m_trail.pop_back();
					continue;
				}

				// enter child.
				m_trail.back().rlp = m_that->deref(rlp[1]);
				// no need to set .child as 255 - it's already done.
				continue;
			}
			else
			{
				// Already a branch - look for first valid.
				if (k.size())
				{
					m_trail.back().setChild(k[0]);
					k = k.mid(1);
				}
				else
					m_trail.back().setChild(16);
				// run through to...
			}
		}
		else
		{
			// Continuing/exiting. Look for next...
			if (!(rlp.isList() && rlp.itemCount() == 17))
			{
				k.clear();
				m_trail.pop_back();
				continue;
			}
			// else run through to...
			m_trail.back().incrementChild();
		}

		// ...here. should only get here if we're a list.
		assert(rlp.isList() && rlp.itemCount() == 17);
		for (;; m_trail.back().incrementChild())
			if (m_trail.back().child == 17)
			{
				// finished here.
				k.clear();
				m_trail.pop_back();
				break;
			}
			else if (!rlp[m_trail.back().child].isEmpty())
			{
				if (m_trail.back().child == 16)
					return;	// have a value at this node - exit now.
				else
				{
					// lead-on to another node - enter child.
					// fixed so that Node passed into push_back is constructed *before* m_trail is potentially resized (which invalidates back and rlp)
					Node const& back = m_trail.back();
					m_trail.push_back(Node{
						m_that->deref(rlp[back.child]),
						 hexPrefixEncode(keyOf(back.key), NibbleSlice(bytesConstRef(&back.child, 1), 1), false),
						 255
						});
					break;
				}
			}
		else
			k.clear();
	}
}

}
