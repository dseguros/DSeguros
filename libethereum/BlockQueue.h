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

};

}
}
