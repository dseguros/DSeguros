#pragma once

#include <array>
#include "GasPricer.h"

namespace dev
{
namespace eth
{

class BasicGasPricer: public GasPricer
{
public:
	explicit BasicGasPricer(u256 _weiPerRef, u256 _refsPerBlock): m_weiPerRef(_weiPerRef), m_refsPerBlock(_refsPerBlock) {}

};
}
}

