#include <libethereum/ExtVM.h>
#include "VMConfig.h"
#include "VM.h"
using namespace std;
using namespace dev;
using namespace dev::eth;

void VM::reportStackUse()
{
	static intptr_t p = 0;
	intptr_t q = intptr_t(&q);
	if (p)
		cerr << "STACK: " << p << " - " << q << " = " << (p - q) << endl;
	p = q;
}

std::array<InstructionMetric, 256> VM::c_metrics;
void VM::initMetrics()
{
	static bool done=false;
	if (!done)
	{
		for (unsigned i = 0; i < 256; ++i)
		{
			InstructionInfo op = instructionInfo((Instruction)i);
			c_metrics[i].gasPriceTier = op.gasPriceTier;
			c_metrics[i].args = op.args;
			c_metrics[i].ret = op.ret;
		}
	}
	done = true;
}

void VM::copyCode(int _extraBytes)
{
	// Copy code so that it can be safely modified and extend code by
	// _extraBytes zero bytes to allow reading virtual data at the end
	// of the code without bounds checks.
	auto extendedSize = m_ext->code.size() + _extraBytes;
	m_code.reserve(extendedSize);
	m_code = m_ext->code;
	m_code.resize(extendedSize);
}
