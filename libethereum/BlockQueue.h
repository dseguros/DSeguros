#pragma once

#include <condition_variable>
#include <thread>
#include <deque>
#include <boost/thread.hpp>
#include <libdevcore/Common.h>
#include <libdevcore/Log.h>
#include <libethcore/Common.h>
#include <libdevcore/Guards.h>
#include <libethcore/Common.h>
#include <libethcore/BlockHeader.h>
#include "VerifiedBlock.h"

namespace dev
{

namespace eth
{

class BlockChain;

struct BlockQueueChannel: public LogChannel { static const char* name(); static const int verbosity = 4; };
struct BlockQueueTraceChannel: public LogChannel { static const char* name(); static const int verbosity = 7; };
#define cblockq dev::LogOutputStream<dev::eth::BlockQueueChannel, true>()

struct BlockQueueStatus
{
	size_t importing;
	size_t verified;
	size_t verifying;
	size_t unverified;
	size_t future;
	size_t unknown;
	size_t bad;
};

enum class QueueStatus
{
	Ready,
	Importing,
	UnknownParent,
	Bad,
	Unknown
};

std::ostream& operator<< (std::ostream& os, QueueStatus const& obj);

template<class T>
class SizedBlockQueue
{
public:
	std::size_t count() const { return m_queue.size(); }

	std::size_t size() const { return m_size; }
	
	bool isEmpty() const { return m_queue.empty(); }

	h256 nextHash() const { return m_queue.front().verified.info.sha3Uncles(); }

	T const& next() const { return m_queue.front(); }

	void clear()
	{
		m_queue.clear();
		m_size = 0;
	}

	void enqueue(T&& _t)
	{
		m_queue.emplace_back(std::move(_t));
		m_size += m_queue.back().blockData.size();
	}

	T dequeue()
	{
		T t;
		std::swap(t, m_queue.front());
		m_queue.pop_front();
		m_size -= t.blockData.size();

		return t;
	}

	std::vector<T> dequeueMultiple(std::size_t _n)
	{
		return removeRange(m_queue.begin(), m_queue.begin() + _n);
	}

	bool remove(h256 const& _hash)
	{
		std::vector<T> removed = removeIf(sha3UnclesEquals(_hash));
		return !removed.empty();
	}

	template<class Pred>
	std::vector<T> removeIf(Pred _pred)
	{
		auto const removedBegin = std::remove_if(m_queue.begin(), m_queue.end(), _pred);

		return removeRange(removedBegin, m_queue.end());
	}

	bool replace(h256 const& _hash, T&& _t)
	{
		auto const it = std::find_if(m_queue.begin(), m_queue.end(), sha3UnclesEquals(_hash));

		if (it == m_queue.end())
			return false;

		m_size -= it->blockData.size();
		m_size += _t.blockData.size();
		*it = std::move(_t);

		return true;
	}

private:
	static std::function<bool(T const&)> sha3UnclesEquals(h256 const& _hash)
	{
		return [&_hash](T const& _t) { return _t.verified.info.sha3Uncles() == _hash; };
	}

	std::vector<T> removeRange(typename std::deque<T>::iterator _begin, typename std::deque<T>::iterator _end)
	{
		std::vector<T> ret(std::make_move_iterator(_begin), std::make_move_iterator(_end));

		for (auto it = ret.begin(); it != ret.end(); ++it)
			m_size -= it->blockData.size();

		m_queue.erase(_begin, _end);
		return ret;
	}

	std::deque<T> m_queue;
	std::atomic<size_t> m_size = {0};	///< Tracks total size in bytes
};

template<class KeyType>
class SizedBlockMap
{
public:
	std::size_t count() const { return m_map.size(); }

	std::size_t size() const { return m_size; }

	bool isEmpty() const { return m_map.empty(); }

	KeyType firstKey() const { return m_map.begin()->first; }

	void clear()
	{
		m_map.clear();
		m_size = 0;
	}

	void insert(KeyType const& _key, h256 const& _hash, bytes&& _blockData)
	{
		auto hashAndBlock = std::make_pair(_hash, std::move(_blockData));
		auto keyAndValue = std::make_pair(_key, std::move(hashAndBlock));
		m_map.insert(std::move(keyAndValue));
		m_size += _blockData.size();
	}

	std::vector<std::pair<h256, bytes>> removeByKeyEqual(KeyType const& _key)
	{
		auto const equalRange = m_map.equal_range(_key);
		return removeRange(equalRange.first, equalRange.second);
	}

	std::vector<std::pair<h256, bytes>> removeByKeyNotGreater(KeyType const& _key)
	{
		return removeRange(m_map.begin(), m_map.upper_bound(_key));
	}

private:
	using BlockMultimap = std::multimap<KeyType, std::pair<h256, bytes>>;

	std::vector<std::pair<h256, bytes>> removeRange(typename BlockMultimap::iterator _begin, typename BlockMultimap::iterator _end)
	{
		std::vector<std::pair<h256, bytes>> removed;
		std::size_t removedSize = 0;
		for (auto it = _begin; it != _end; ++it)
		{
			removed.push_back(std::move(it->second));
			removedSize += removed.back().second.size();
		}

		m_size -= removedSize;
		m_map.erase(_begin, _end);

		return removed;
	}
		
	BlockMultimap m_map;
	std::atomic<size_t> m_size = {0};	///< Tracks total size in bytes
};

}
}
