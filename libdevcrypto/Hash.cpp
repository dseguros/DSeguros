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
/** @file Hash.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "Hash.h"
#include <secp256k1_sha256.h>

using namespace dev;

namespace dev
{

h256 sha256(bytesConstRef _input) noexcept
{
	secp256k1_sha256_t ctx;
	secp256k1_sha256_initialize(&ctx);
	secp256k1_sha256_write(&ctx, _input.data(), _input.size());
	h256 hash;
	secp256k1_sha256_finalize(&ctx, hash.data());
	return hash;
}
}