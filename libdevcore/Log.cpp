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

LogOutputStreamBase::LogOutputStreamBase(char const* _id, std::type_info const* _info, unsigned _v, bool _autospacing):
	m_autospacing(_autospacing),
	m_verbosity(_v)
{
	Guard l(x_logOverride);
	auto it = s_logOverride.find(_info);
	if ((it != s_logOverride.end() && it->second == true) || (it == s_logOverride.end() && (int)_v <= g_logVerbosity))
	{
		time_t rawTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		unsigned ms = chrono::duration_cast<chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() % 1000;
		char buf[24];
		if (strftime(buf, 24, "%X", localtime(&rawTime)) == 0)
			buf[0] = '\0'; // empty if case strftime fails
		static char const* c_begin = "  " EthViolet;
		static char const* c_sep1 = EthReset EthBlack "|" EthNavy;
		static char const* c_sep2 = EthReset EthBlack "|" EthTeal;
		static char const* c_end = EthReset "  ";
		m_sstr << _id << c_begin << buf << "." << setw(3) << setfill('0') << ms;
		m_sstr << c_sep1 << getThreadName() << ThreadContext::join(c_sep2) << c_end;
	}
}

/// Associate a name with each thread for nice logging.
struct ThreadLocalLogName
{
	ThreadLocalLogName(std::string const& _name) { m_name.reset(new string(_name)); }
	boost::thread_specific_ptr<std::string> m_name;
};
