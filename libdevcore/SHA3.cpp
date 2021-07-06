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

}
