#include "SHA3.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "RLP.h"
using namespace std;
using namespace dev;

namespace dev
{

h256 EmptySHA3 = sha3(bytesConstRef());
h256 EmptyListSHA3 = sha3(rlpList());

namespace keccak
{

/** libkeccak-tiny
 *
 * A single-file implementation of SHA-3 and SHAKE.
 *
 * Implementor: David Leon Gil
 * License: CC0, attribution kindly requested. Blame taken too,
 * but not liability.
 */

#define decshake(bits) \
  int shake##bits(uint8_t*, size_t, const uint8_t*, size_t);

#define decsha3(bits) \
  int sha3_##bits(uint8_t*, size_t, const uint8_t*, size_t);

decshake(128)
decshake(256)
decsha3(224)
decsha3(256)
decsha3(384)
decsha3(512)

}
}
