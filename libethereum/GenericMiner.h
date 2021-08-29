#pragma once

#include <libdevcore/Common.h>
#include <libdevcore/Log.h>
#include <libdevcore/Worker.h>
#include <libethcore/Common.h>

namespace dev
{

namespace eth
{

struct MineInfo: public WorkingProgress {};

inline std::ostream& operator<<(std::ostream& _out, WorkingProgress _p)
{
	_out << _p.rate() << " H/s = " <<  _p.hashes << " hashes / " << (double(_p.ms) / 1000) << " s";
	return _out;
}

}
}
