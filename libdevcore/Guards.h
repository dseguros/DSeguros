#pragma once

#include <mutex>
#include <condition_variable>
#include <atomic>
#pragma warning(push)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <boost/thread.hpp>
#pragma warning(pop)
#pragma GCC diagnostic pop

namespace dev
{

using Mutex = std::mutex;
using RecursiveMutex = std::recursive_mutex;
using SharedMutex = boost::shared_mutex;

using Guard = std::lock_guard<std::mutex>;
using UniqueGuard = std::unique_lock<std::mutex>;
using RecursiveGuard = std::lock_guard<std::recursive_mutex>;
using ReadGuard = boost::shared_lock<boost::shared_mutex>;
using UpgradableGuard = boost::upgrade_lock<boost::shared_mutex>;
using UpgradeGuard = boost::upgrade_to_unique_lock<boost::shared_mutex>;
using WriteGuard = boost::unique_lock<boost::shared_mutex>;

template <class GuardType, class MutexType>
struct GenericGuardBool: GuardType
{
	GenericGuardBool(MutexType& _m): GuardType(_m) {}
	bool b = true;
};
template <class MutexType>
struct GenericUnguardBool
{
	GenericUnguardBool(MutexType& _m): m(_m) { m.unlock(); }
	~GenericUnguardBool() { m.lock(); }
	bool b = true;
	MutexType& m;
};
template <class MutexType>
struct GenericUnguardSharedBool
{
	GenericUnguardSharedBool(MutexType& _m): m(_m) { m.unlock_shared(); }
	~GenericUnguardSharedBool() { m.lock_shared(); }
	bool b = true;
	MutexType& m;
};

/** @brief Simple lock that waits for release without making context switch */
class SpinLock
{
public:
	SpinLock() { m_lock.clear(); }
	void lock() { while (m_lock.test_and_set(std::memory_order_acquire)) {} }
	void unlock() { m_lock.clear(std::memory_order_release); }
private:
	std::atomic_flag m_lock;
};
using SpinGuard = std::lock_guard<SpinLock>;
}
