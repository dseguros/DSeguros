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
public:
	static const unsigned BasicFields = 13;

	BlockHeader();
	explicit BlockHeader(bytesConstRef _data, BlockDataType _bdt = BlockData, h256 const& _hashWith = h256());
	explicit BlockHeader(bytes const& _data, BlockDataType _bdt = BlockData, h256 const& _hashWith = h256()): BlockHeader(&_data, _bdt, _hashWith) {}


static h256 headerHashFromBlock(bytes const& _block) { return headerHashFromBlock(&_block); }
	static h256 headerHashFromBlock(bytesConstRef _block);
	static RLP extractHeader(bytesConstRef _block);


explicit operator bool() const { return m_timestamp != Invalid256; }

	bool operator==(BlockHeader const& _cmp) const
	{
		return m_parentHash == _cmp.parentHash() &&
			m_sha3Uncles == _cmp.sha3Uncles() &&
			m_author == _cmp.author() &&
			m_stateRoot == _cmp.stateRoot() &&
			m_transactionsRoot == _cmp.transactionsRoot() &&
			m_receiptsRoot == _cmp.receiptsRoot() &&
			m_logBloom == _cmp.logBloom() &&
			m_difficulty == _cmp.difficulty() &&
			m_number == _cmp.number() &&
			m_gasLimit == _cmp.gasLimit() &&
			m_gasUsed == _cmp.gasUsed() &&
			m_timestamp == _cmp.timestamp() &&
			m_extraData == _cmp.extraData();
	}

}

}
}
