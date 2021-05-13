#include "Common.h"
#include "Exceptions.h"
#include "Log.h"
#include "BuildInfo.h"
using namespace std;
using namespace dev;

namespace dev
{

char const* Version = ETH_PROJECT_VERSION;

const u256 Invalid256 = ~(u256)0;

void InvariantChecker::checkInvariants(HasInvariants const* _this, char const* _fn, char const* _file, int _line, bool _pre)
{
	if (!_this->invariants())
	{
		cwarn << (_pre ? "Pre" : "Post") << "invariant failed in" << _fn << "at" << _file << ":" << _line;
		::boost::exception_detail::throw_exception_(FailedInvariant(), _fn, _file, _line);
	}
}

struct TimerChannel: public LogChannel { static const char* name(); static const int verbosity = 0; };

#if defined(_WIN32)
const char* TimerChannel::name() { return EthRed " ! "; }
#else
const char* TimerChannel::name() { return EthRed " ⚡ "; }
#endif

TimerHelper::~TimerHelper()
{
	auto e = std::chrono::high_resolution_clock::now() - m_t;
	if (!m_ms || e > chrono::milliseconds(m_ms))
		clog(TimerChannel) << m_id << chrono::duration_cast<chrono::milliseconds>(e).count() << "ms";
}

}

