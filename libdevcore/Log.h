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

}
}

