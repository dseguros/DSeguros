#include "Log.h"

#include <string>
#include <iostream>
#include <thread>
#ifdef __APPLE__
#include <pthread.h>
#endif
#include "Guards.h"
using namespace std;
using namespace dev;


// Logging
int dev::g_logVerbosity = 5;
mutex x_logOverride;

/// Map of Log Channel types to bool, false forces the channel to be disabled, true forces it to be enabled.
/// If a channel has no entry, then it will output as long as its verbosity (LogChannel::verbosity) is less than
/// or equal to the currently output verbosity (g_logVerbosity).
static map<type_info const*, bool> s_logOverride;

bool dev::isChannelVisible(std::type_info const* _ch, bool _default)
{
	Guard l(x_logOverride);
	if (s_logOverride.count(_ch))
		return s_logOverride[_ch];
	return _default;
}

LogOverrideAux::LogOverrideAux(std::type_info const* _ch, bool _value):
	m_ch(_ch)
{
	Guard l(x_logOverride);
	m_old = s_logOverride.count(_ch) ? (int)s_logOverride[_ch] : c_null;
	s_logOverride[m_ch] = _value;
}
