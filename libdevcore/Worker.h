
#pragma once

#include <string>
#include <thread>
#include <atomic>
#include "Guards.h"

namespace dev
{

enum class IfRunning
{
	Fail,
	Join,
	Detach
};

enum class WorkerState
{
	Starting,
	Started,
	Stopping,
	Stopped,
	Killing
};

class Worker
{
protected:
	Worker(std::string const& _name = "anon", unsigned _idleWaitMs = 30): m_name(_name), m_idleWaitMs(_idleWaitMs) {}

	/// Move-constructor.
	Worker(Worker&& _m) { std::swap(m_name, _m.m_name); }

	/// Move-assignment.
	Worker& operator=(Worker&& _m)
	{
		assert(&_m != this);
		std::swap(m_name, _m.m_name);
		return *this;
	}

	virtual ~Worker() { terminate(); }

	/// Allows changing worker name if work is stopped.
	void setName(std::string _n) { if (!isWorking()) m_name = _n; }

	/// Starts worker thread; causes startedWorking() to be called.
	void startWorking();
	
	/// Stop worker thread; causes call to stopWorking().
	void stopWorking();

	/// Returns if worker thread is present.
	bool isWorking() const { Guard l(x_work); return m_state == WorkerState::Started; }
	
	/// Called after thread is started from startWorking().
	virtual void startedWorking() {}
	
	/// Called continuously following sleep for m_idleWaitMs.
	virtual void doWork() {}

	/// Overrides doWork(); should call shouldStop() often and exit when true.
	virtual void workLoop();
	bool shouldStop() const { return m_state != WorkerState::Started; }
	
	/// Called when is to be stopped, just prior to thread being joined.
	virtual void doneWorking() {}
private:
	/// Stop and never start again.
	void terminate();

	std::string m_name;

	unsigned m_idleWaitMs = 0;
	
	mutable Mutex x_work;						///< Lock for the network existance.
	std::unique_ptr<std::thread> m_work;		///< The network thread.
	std::atomic<WorkerState> m_state = {WorkerState::Starting};
private:
	/// Stop and never start again.
	void terminate();

	std::string m_name;

	unsigned m_idleWaitMs = 0;
	
	mutable Mutex x_work;						///< Lock for the network existance.
	std::unique_ptr<std::thread> m_work;		///< The network thread.
	std::atomic<WorkerState> m_state = {WorkerState::Starting};
};
}
