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

/// Set the current thread's log name.
///
/// It appears that there is not currently any cross-platform way of setting
/// thread names either in Boost or in the C++11 runtime libraries.   What is
/// more, the API for 'pthread_setname_np' is not even consistent across
/// platforms which implement it.
///
/// A proposal to add such functionality on the Boost mailing list, which
/// I assume never happened, but which I should follow-up and ask about.
/// http://boost.2283326.n4.nabble.com/Adding-an-option-to-set-the-name-of-a-boost-thread-td4638283.html
///
/// man page for 'pthread_setname_np', including this crucial snippet of
/// information ... "These functions are nonstandard GNU extensions."
/// http://man7.org/linux/man-pages/man3/pthread_setname_np.3.html
///
/// Stack Overflow "Can I set the name of a thread in pthreads / linux?"
/// which includes useful information on the minor API differences between
/// Linux, BSD and OS X.
/// http://stackoverflow.com/questions/2369738/can-i-set-the-name-of-a-thread-in-pthreads-linux/7989973#7989973
///
/// musl mailng list posting "pthread set name on MIPs" which includes the
/// information that musl doesn't currently implement 'pthread_setname_np'
/// https://marc.info/?l=musl&m=146171729013062&w=1
void setThreadName(std::string const& _n);

/// Set the current thread's log name.
std::string getThreadName();

/// The default logging channels. Each has an associated verbosity and three-letter prefix (name() ).
/// Channels should inherit from LogChannel and define name() and verbosity.
struct LogChannel { static const char* name(); static const int verbosity = 1; static const bool debug = true; };
struct LeftChannel: public LogChannel { static const char* name(); };
struct RightChannel: public LogChannel { static const char* name(); };
struct WarnChannel: public LogChannel { static const char* name(); static const int verbosity = 0; static const bool debug = false; };
struct NoteChannel: public LogChannel { static const char* name(); static const bool debug = false; };
struct DebugChannel: public LogChannel { static const char* name(); static const int verbosity = 0; };
struct TraceChannel: public LogChannel { static const char* name(); static const int verbosity = 4; static const bool debug = true; };

enum class LogTag
{
	None,
	Url,
	Error,
	Special
};

}
}

