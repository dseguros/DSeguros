#include "CommonData.h"
#include <random>

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4724) // potential mod by 0, line 78 of boost/random/uniform_int_distribution.hpp (boost 1.55)
#endif
#include <boost/random/uniform_int_distribution.hpp>
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
#include "Exceptions.h"
#include "Log.h"
using namespace std;
using namespace dev;


namespace
{
int fromHexChar(char _i) noexcept
{
	if (_i >= '0' && _i <= '9')
		return _i - '0';
	if (_i >= 'a' && _i <= 'f')
		return _i - 'a' + 10;
	if (_i >= 'A' && _i <= 'F')
		return _i - 'A' + 10;
	return -1;
}
}

}

