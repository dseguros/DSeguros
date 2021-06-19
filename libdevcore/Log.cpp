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

LogOverrideAux::~LogOverrideAux()
{
	Guard l(x_logOverride);
	if (m_old == c_null)
		s_logOverride.erase(m_ch);
	else
		s_logOverride[m_ch] = (bool)m_old;
}

#if defined(_WIN32)
const char* LogChannel::name() { return EthGray "..."; }
const char* LeftChannel::name() { return EthNavy "<--"; }
const char* RightChannel::name() { return EthGreen "-->"; }
const char* WarnChannel::name() { return EthOnRed EthBlackBold "  X"; }
const char* NoteChannel::name() { return EthBlue "  i"; }
const char* DebugChannel::name() { return EthWhite "  D"; }
const char* TraceChannel::name() { return EthGray "..."; }
#else
const char* LogChannel::name() { return EthGray "···"; }
const char* LeftChannel::name() { return EthNavy "◀▬▬"; }
const char* RightChannel::name() { return EthGreen "▬▬▶"; }
const char* WarnChannel::name() { return EthOnRed EthBlackBold "  ✘"; }
const char* NoteChannel::name() { return EthBlue "  ℹ"; }
const char* DebugChannel::name() { return EthWhite "  ◇"; }
const char* TraceChannel::name() { return EthGray "..."; }
#endif
std::string dev::logFileName(const char *file, int line, const char *fun, const char *t)
{
	const char *p = ::strrchr((char*)file, '/');
	p = p ? p+1 : file;

	char buf[1024];
	snprintf(buf, sizeof(buf), "%s:%d:%s:%s::\t", p, line, fun, t);
	
	/*
    int len = strlen(file);
    while(--len >= 0){
            if(file[len] == '/')
                    return file + len + 1;
    }*/

    return std::string(buf);
}
