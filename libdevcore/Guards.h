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

template <class N>
class Notified
{
public:
	Notified() {}
	Notified(N const& _v): m_value(_v) {}
	Notified(Notified const&) = delete;
	Notified& operator=(N const& _v) { UniqueGuard l(m_mutex); m_value = _v; m_cv.notify_all(); return *this; }

	operator N() const { UniqueGuard l(m_mutex); return m_value; }

	void wait() const { N old; { UniqueGuard l(m_mutex); old = m_value; } waitNot(old); }
	void wait(N const& _v) const { UniqueGuard l(m_mutex); m_cv.wait(l, [&](){return m_value == _v;}); }
	void waitNot(N const& _v) const { UniqueGuard l(m_mutex); m_cv.wait(l, [&](){return m_value != _v;}); }
	template <class F> void wait(F const& _f) const { UniqueGuard l(m_mutex); m_cv.wait(l, _f); }

	template <class R, class P> void wait(std::chrono::duration<R, P> _d) const { N old; { UniqueGuard l(m_mutex); old = m_value; } waitNot(_d, old); }
	template <class R, class P> void wait(std::chrono::duration<R, P> _d, N const& _v) const { UniqueGuard l(m_mutex); m_cv.wait_for(l, _d, [&](){return m_value == _v;}); }
	template <class R, class P> void waitNot(std::chrono::duration<R, P> _d, N const& _v) const { UniqueGuard l(m_mutex); m_cv.wait_for(l, _d, [&](){return m_value != _v;}); }
	template <class R, class P, class F> void wait(std::chrono::duration<R, P> _d, F const& _f) const { UniqueGuard l(m_mutex); m_cv.wait_for(l, _d, _f); }

};

}
