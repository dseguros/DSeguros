#pragma once
#include <utility>
#include <queue>
#include <condition_variable>
#include <mutex>

namespace dev
{

/// Concurrent queue.
/// You can push and pop elements to/from the queue. Pop will block until the queue is not empty.
/// The default backend (_QueueT) is std::queue. It can be changed to any type that has
/// proper push(), pop(), empty() and front() methods.
template<typename _T, typename _QueueT = std::queue<_T>>
class concurrent_queue
{

};
}
