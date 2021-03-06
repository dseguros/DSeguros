/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	cpp-ethereum is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file Ethash.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "Ethash.h"
#include <libethash/ethash.h>
#include <libethash/internal.h>
#include <libethereum/Interface.h>
#include <libethcore/ChainOperationParams.h>
#include <libethcore/CommonJS.h>
#include "EthashCPUMiner.h"
using namespace std;
using namespace dev;
using namespace eth;

void Ethash::init()
{
	ETH_REGISTER_SEAL_ENGINE(Ethash);
}


