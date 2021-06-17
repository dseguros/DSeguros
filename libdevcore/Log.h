#pragma once

#include <ctime>
#include <chrono>
#include "vector_ref.h"
#include "Common.h"
#include "CommonIO.h"
#include "CommonData.h"
#include "FixedHash.h"
#include "Terminal.h"

namespace boost { namespace asio { namespace ip { template<class T>class basic_endpoint; class tcp; } } }

namespace dev
{

/// The null output stream. Used when logging is disabled.
class NullOutputStream
{
public:
	template <class T> NullOutputStream& operator<<(T const&) { return *this; }
};

/// A simple log-output function that prints log messages to stdout.
void simpleDebugOut(std::string const&, char const*);

/// The logging system's current verbosity.
extern int g_logVerbosity;

/// The current method that the logging system uses to output the log messages. Defaults to simpleDebugOut().
extern std::function<void(std::string const&, char const*)> g_logPost;

class LogOverrideAux
{
protected:
	LogOverrideAux(std::type_info const* _ch, bool _value);
	~LogOverrideAux();

private:
	std::type_info const* m_ch;
	static const int c_null = -1;
	int m_old;
};

template <class Channel>
class LogOverride: LogOverrideAux
{
public:
	LogOverride(bool _value): LogOverrideAux(&typeid(Channel), _value) {}
};

bool isChannelVisible(std::type_info const* _ch, bool _default);
template <class Channel> bool isChannelVisible() { return isChannelVisible(&typeid(Channel), Channel::verbosity <= g_logVerbosity); }

/// Temporary changes system's verbosity for specific function. Restores the old verbosity when function returns.
/// Not thread-safe, use with caution!
struct VerbosityHolder
{
	VerbosityHolder(int _temporaryValue, bool _force = false): oldLogVerbosity(g_logVerbosity) { if (g_logVerbosity >= 0 || _force) g_logVerbosity = _temporaryValue; }
	~VerbosityHolder() { g_logVerbosity = oldLogVerbosity; }
	int oldLogVerbosity;
};

#define ETH_THREAD_CONTEXT(name) for (std::pair<dev::ThreadContext, bool> __eth_thread_context(name, true); p.second; p.second = false)

class ThreadContext
{
public:
	ThreadContext(std::string const& _info) { push(_info); }
	~ThreadContext() { pop(); }

	static void push(std::string const& _n);
	static void pop();
	static std::string join(std::string const& _prior);
};

}
}

