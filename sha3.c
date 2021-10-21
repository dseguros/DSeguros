/** libkeccak-tiny
*
* A single-file implementation of SHA-3 and SHAKE.
*
* Implementor: David Leon Gil
* License: CC0, attribution kindly requested. Blame taken too,
* but not liability.
*/
#include "sha3.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/******** The Keccak-f[1600] permutation ********/

/*** Constants. ***/
static const uint8_t rho[24] = \
	{ 1,  3,   6, 10, 15, 21,
	  28, 36, 45, 55,  2, 14,
	  27, 41, 56,  8, 25, 43,
	  62, 18, 39, 61, 20, 44};
static const uint8_t pi[24] = \
	{10,  7, 11, 17, 18, 3,
	 5, 16,  8, 21, 24, 4,
	 15, 23, 19, 13, 12, 2,
	 20, 14, 22,  9, 6,  1};
static const uint64_t RC[24] = \
	{1ULL, 0x8082ULL, 0x800000000000808aULL, 0x8000000080008000ULL,
	 0x808bULL, 0x80000001ULL, 0x8000000080008081ULL, 0x8000000000008009ULL,
	 0x8aULL, 0x88ULL, 0x80008009ULL, 0x8000000aULL,
	 0x8000808bULL, 0x800000000000008bULL, 0x8000000000008089ULL, 0x8000000000008003ULL,
	 0x8000000000008002ULL, 0x8000000000000080ULL, 0x800aULL, 0x800000008000000aULL,
	 0x8000000080008081ULL, 0x8000000000008080ULL, 0x80000001ULL, 0x8000000080008008ULL};

