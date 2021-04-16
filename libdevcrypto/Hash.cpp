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

namespace rmd160
{

/********************************************************************\
 *
 *      FILE:     rmd160.h
 *      FILE:     rmd160.c
 *
 *      CONTENTS: Header file for a sample C-implementation of the
 *                RIPEMD-160 hash-function.
 *      TARGET:   any computer with an ANSI C compiler
 *
 *      AUTHOR:   Antoon Bosselaers, ESAT-COSIC
 *      DATE:     1 March 1996
 *      VERSION:  1.0
 *
 *      Copyright (c) Katholieke Universiteit Leuven
 *      1996, All Rights Reserved
 *
 \********************************************************************/

// Adapted into "header-only" format by Gav Wood.

/* macro definitions */

#define RMDsize 160

/* collect four bytes into one word: */
#define BYTES_TO_DWORD(strptr)                    \
(((uint32_t) *((strptr)+3) << 24) | \
((uint32_t) *((strptr)+2) << 16) | \
((uint32_t) *((strptr)+1) <<  8) | \
((uint32_t) *(strptr)))

/* ROL(x, n) cyclically rotates x over n bits to the left */
/* x must be of an unsigned 32 bits type and 0 <= n < 32. */
#define ROL(x, n)        (((x) << (n)) | ((x) >> (32-(n))))

/* the five basic functions F(), G() and H() */
#define F(x, y, z)        ((x) ^ (y) ^ (z))
#define G(x, y, z)        (((x) & (y)) | (~(x) & (z)))
#define H(x, y, z)        (((x) | ~(y)) ^ (z))
#define I(x, y, z)        (((x) & (z)) | ((y) & ~(z)))
#define J(x, y, z)        ((x) ^ ((y) | ~(z)))

/* the ten basic operations FF() through III() */
#define FF(a, b, c, d, e, x, s)        {\
(a) += F((b), (c), (d)) + (x);\
(a) = ROL((a), (s)) + (e);\
(c) = ROL((c), 10);\
}
#define GG(a, b, c, d, e, x, s)        {\
(a) += G((b), (c), (d)) + (x) + 0x5a827999UL;\
(a) = ROL((a), (s)) + (e);\
(c) = ROL((c), 10);\
}
#define HH(a, b, c, d, e, x, s)        {\
(a) += H((b), (c), (d)) + (x) + 0x6ed9eba1UL;\
(a) = ROL((a), (s)) + (e);\
(c) = ROL((c), 10);\
}
#define II(a, b, c, d, e, x, s)        {\
(a) += I((b), (c), (d)) + (x) + 0x8f1bbcdcUL;\
(a) = ROL((a), (s)) + (e);\
(c) = ROL((c), 10);\
}
#define JJ(a, b, c, d, e, x, s)        {\
(a) += J((b), (c), (d)) + (x) + 0xa953fd4eUL;\
(a) = ROL((a), (s)) + (e);\
(c) = ROL((c), 10);\
}
#define FFF(a, b, c, d, e, x, s)        {\
(a) += F((b), (c), (d)) + (x);\
(a) = ROL((a), (s)) + (e);\
(c) = ROL((c), 10);\
}
#define GGG(a, b, c, d, e, x, s)        {\
(a) += G((b), (c), (d)) + (x) + 0x7a6d76e9UL;\
(a) = ROL((a), (s)) + (e);\
(c) = ROL((c), 10);\
}
#define HHH(a, b, c, d, e, x, s)        {\
(a) += H((b), (c), (d)) + (x) + 0x6d703ef3UL;\
(a) = ROL((a), (s)) + (e);\
(c) = ROL((c), 10);\
}
#define III(a, b, c, d, e, x, s)        {\
(a) += I((b), (c), (d)) + (x) + 0x5c4dd124UL;\
(a) = ROL((a), (s)) + (e);\
(c) = ROL((c), 10);\
}
#define JJJ(a, b, c, d, e, x, s)        {\
(a) += J((b), (c), (d)) + (x) + 0x50a28be6UL;\
(a) = ROL((a), (s)) + (e);\
(c) = ROL((c), 10);\
}

}
