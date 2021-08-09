
#include <libethereum/ExtVM.h>
#include "VMConfig.h"
#include "VM.h"
using namespace std;
using namespace dev;
using namespace dev::eth;

uint64_t VM::memNeed(u256 _offset, u256 _size)
{
	return toInt63(_size ? u512(_offset) + _size : u512(0));
}

template <class S> S divWorkaround(S const& _a, S const& _b)
{
	return (S)(s512(_a) / s512(_b));
}

template <class S> S modWorkaround(S const& _a, S const& _b)
{
	return (S)(s512(_a) % s512(_b));
}


//
// for decoding destinations of JUMPTO, JUMPV, JUMPSUB and JUMPSUBV
//

uint64_t VM::decodeJumpDest(const byte* const _code, uint64_t& _pc)
{
	// turn 4 MSB-first bytes in the code into a native-order integer
	uint64_t dest      = _code[_pc++];
	dest = (dest << 8) | _code[_pc++];
	dest = (dest << 8) | _code[_pc++];
	dest = (dest << 8) | _code[_pc++];
	return dest;
}

uint64_t VM::decodeJumpvDest(const byte* const _code, uint64_t& _pc, byte _voff)
{
	// Layout of jump table in bytecode...
	//     byte opcode
	//     byte n_jumps
	//     byte table[n_jumps][4]
	//	
	uint64_t pc = _pc;
	byte n = _code[++pc];           // byte after opcode is number of jumps
	if (_voff >= n) _voff = n - 1;  // if offset overflows use default jump
	pc += _voff * 4;                // adjust inout pc before index destination in table
	
	uint64_t dest = decodeJumpDest(_code, pc);
	
	_pc += 1 + n * 4;               // adust inout _pc to opcode after table 
	return dest;
}

//
// for tracing, checking, metering, measuring ...
//
void VM::onOperation()
{
	if (m_onOp)
		(m_onOp)(++m_nSteps, m_PC, m_OP,
			m_newMemSize > m_mem.size() ? (m_newMemSize - m_mem.size()) / 32 : uint64_t(0),
			m_runGas, m_io_gas, this, m_ext);
}
#if EVM_HACK_ON_OPERATION
	#define onOperation()
#endif

//
// set current SP to SP', adjust SP' per _removed and _added items
//
void VM::adjustStack(unsigned _removed, unsigned _added)
{
	m_SP = m_SPP;
#if EVM_HACK_STACK
	m_SPP += _removed;
	m_SPP -= _added;
#else
	// adjust stack and check bounds
	m_SPP += _removed;
	if (m_stackEnd < m_SPP)
		throwBadStack(_removed, _added);
	m_SPP -= _added;
	if (m_SPP < m_stack)
		throwBadStack(_removed, _added);
#endif
}


uint64_t VM::gasForMem(u512 _size)
{
	u512 s = _size / 32;
	return toInt63((u512)m_schedule->memoryGas * s + s * s / m_schedule->quadCoeffDiv);
}

void VM::updateIOGas()
{
	if (m_io_gas < m_runGas)
		throwOutOfGas();
	m_io_gas -= m_runGas;
}

void VM::updateGas()
{
	if (m_newMemSize > m_mem.size())
		m_runGas += toInt63(gasForMem(m_newMemSize) - gasForMem(m_mem.size()));
	m_runGas += (m_schedule->copyGas * ((m_copyMemSize + 31) / 32));
	if (m_io_gas < m_runGas)
		throwOutOfGas();
}

void VM::updateMem(uint64_t _newMem)
{
	m_newMemSize = (_newMem + 31) / 32 * 32;
	updateGas();
	if (m_newMemSize > m_mem.size())
		m_mem.resize(m_newMemSize);
}

void VM::logGasMem()
{
	unsigned n = (unsigned)m_OP - (unsigned)Instruction::LOG0;
	m_runGas = toInt63(m_schedule->logGas + m_schedule->logTopicGas * n + u512(m_schedule->logDataGas) * m_SP[1]);
	updateMem(memNeed(m_SP[0], m_SP[1]));
}

void VM::fetchInstruction()
{
	m_OP = Instruction(m_code[m_PC]);
	const InstructionMetric& metric = c_metrics[static_cast<size_t>(m_OP)];
	adjustStack(metric.args, metric.ret);

	// FEES...
	m_runGas = toInt63(m_schedule->tierStepGas[static_cast<unsigned>(metric.gasPriceTier)]);
	m_newMemSize = m_mem.size();
	m_copyMemSize = 0;
}

#if EVM_HACK_ON_OPERATION
	#define onOperation()
#endif
#if EVM_HACK_UPDATE_IO_GAS
	#define updateIOGas()
#endif
