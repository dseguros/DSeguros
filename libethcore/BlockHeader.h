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
/** @file BlockHeader.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include <algorithm>
#include <libdevcore/Common.h>
#include <libdevcore/RLP.h>
#include <libdevcore/SHA3.h>
#include "Common.h"
#include "ChainOperationParams.h"
#include "Exceptions.h"

namespace dev
{
namespace eth
{

enum IncludeSeal
{
	WithoutSeal = 0,
	WithSeal = 1,
	OnlySeal = 2
};

enum Strictness
{
	CheckEverything,
	JustSeal,
	QuickNonce,
	IgnoreSeal,
	CheckNothingNew
};

// TODO: for implementing soon.
/*enum Check
{
	CheckBasic,
	CheckExtended,
	CheckBlock,
	CheckParent,
	CheckSeal,
	CheckSealQuickly,
	CheckAll = CheckBasic | CheckExtended | CheckBlock | CheckParent | CheckSeal,
};
using Checks = FlagSet<Check>;*/

enum BlockDataType
{
	HeaderData,
	BlockData
};

DEV_SIMPLE_EXCEPTION(NoHashRecorded);
DEV_SIMPLE_EXCEPTION(GenesisBlockCannotBeCalculated);

/** @brief Encapsulation of a block header.
 * Class to contain all of a block header's data. It is able to parse a block header and populate
 * from some given RLP block serialisation with the static fromHeader(), through the method
 * populate(). This will not conduct any verification above basic formating. In this case extra
 * verification can be performed through verify().
 *
 * The object may also be populated from an entire block through the explicit
 * constructor BlockHeader(bytesConstRef) and manually with the populate() method. These will
 * conduct verification of the header against the other information in the block.
 *
 * The object may be populated with a template given a parent BlockHeader object with the
 * populateFromParent() method. The genesis block info may be retrieved with genesis() and the
 * corresponding RLP block created with createGenesisBlock().
 *
 * To determine the header hash without the nonce (for sealing), the method hash(WithoutNonce) is
 * provided.
 *
 * The default constructor creates an empty object, which can be tested against with the boolean
 * conversion operator.
 */
class BlockHeader
{
	friend class BlockChain;

}

}
}
