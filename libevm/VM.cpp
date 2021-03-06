
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

///////////////////////////////////////////////////////////////////////////////
//
// interpreter entry point

owning_bytes_ref VM::exec(u256& _io_gas, ExtVMFace& _ext, OnOpFunc const& _onOp)
{
	m_io_gas_p = &_io_gas;
	m_io_gas = uint64_t(_io_gas);
	m_ext = &_ext;
	m_schedule = &m_ext->evmSchedule();
	m_onOp = _onOp;
	m_onFail = &VM::onOperation;
	
	try
	{
		// trampoline to minimize depth of call stack when calling out
		m_bounce = &VM::initEntry;
		do
			(this->*m_bounce)();
		while (m_bounce);
		
	}
	catch (...)
	{
		*m_io_gas_p = m_io_gas;
		throw;
	}

	*m_io_gas_p = m_io_gas;
	return std::move(m_output);
}

//
// main interpreter loop and switch
//
void VM::interpretCases()
{
	INIT_CASES
	DO_CASES
	{	
		//
		// Call-related instructions
		//
		
		CASE(CREATE)
		{
			m_bounce = &VM::caseCreate;
		}
		BREAK

		CASE(DELEGATECALL)

			// Pre-homestead
			if (!m_schedule->haveDelegateCall)
				throwBadInstruction();

		CASE(CALL)
		CASE(CALLCODE)
		{
			m_bounce = &VM::caseCall;
		}
		BREAK

		CASE(RETURN)
		{
			m_copyMemSize = 0;
			updateMem(memNeed(m_SP[0], m_SP[1]));
			ON_OP();
			updateIOGas();

			uint64_t b = (uint64_t)m_SP[0];
			uint64_t s = (uint64_t)m_SP[1];
			m_output = owning_bytes_ref{std::move(m_mem), b, s};
			m_bounce = 0;
		}
		BREAK

		CASE(REVERT)
		{
			// Pre-metropolis 
			if (!m_schedule->haveRevert)
				throwBadInstruction();

			m_copyMemSize = 0;
			updateMem(memNeed(m_SP[0], m_SP[1]));
			ON_OP();
			updateIOGas();

			uint64_t b = (uint64_t)m_SP[0];
			uint64_t s = (uint64_t)m_SP[1];
			owning_bytes_ref output{move(m_mem), b, s};
			throwRevertInstruction(move(output));
		}
		BREAK; 

		CASE(SUICIDE)
		{
			m_runGas = toInt63(m_schedule->suicideGas);
			Address dest = asAddress(m_SP[0]);

			// After EIP158 zero-value suicides do not have to pay account creation gas.
			if (m_ext->balance(m_ext->myAddress) > 0 || m_schedule->zeroValueTransferChargesNewAccountGas())
				// After EIP150 hard fork charge additional cost of sending
				// ethers to non-existing account.
				if (m_schedule->suicideChargesNewAccountGas() && !m_ext->exists(dest))
					m_runGas += m_schedule->callNewAccountGas;

			ON_OP();
			updateIOGas();
			m_ext->suicide(dest);
			m_bounce = 0;
		}
		BREAK

		CASE(STOP)
		{
			ON_OP();
			updateIOGas();
			m_bounce = 0;
		}
		BREAK
			
			
		//
		// instructions potentially expanding memory
		//
		
		CASE(MLOAD)
		{
			updateMem(toInt63(m_SP[0]) + 32);
			ON_OP();
			updateIOGas();

			m_SPP[0] = (u256)*(h256 const*)(m_mem.data() + (unsigned)m_SP[0]);
		}
		NEXT

		CASE(MSTORE)
		{
			updateMem(toInt63(m_SP[0]) + 32);
			ON_OP();
			updateIOGas();

			*(h256*)&m_mem[(unsigned)m_SP[0]] = (h256)m_SP[1];
		}
		NEXT

		CASE(MSTORE8)
		{
			updateMem(toInt63(m_SP[0]) + 1);
			ON_OP();
			updateIOGas();

			m_mem[(unsigned)m_SP[0]] = (byte)(m_SP[1] & 0xff);
		}
		NEXT

		CASE(SHA3)
		{
			m_runGas = toInt63(m_schedule->sha3Gas + (u512(m_SP[1]) + 31) / 32 * m_schedule->sha3WordGas);
			updateMem(memNeed(m_SP[0], m_SP[1]));
			ON_OP();
			updateIOGas();

			uint64_t inOff = (uint64_t)m_SP[0];
			uint64_t inSize = (uint64_t)m_SP[1];
			m_SPP[0] = (u256)sha3(bytesConstRef(m_mem.data() + inOff, inSize));
		}
		NEXT

		CASE(LOG0)
		{
			logGasMem();
			ON_OP();
			updateIOGas();

			m_ext->log({}, bytesConstRef(m_mem.data() + (uint64_t)m_SP[0], (uint64_t)m_SP[1]));
		}
		NEXT

		CASE(LOG1)
		{
			logGasMem();
			ON_OP();
			updateIOGas();

			m_ext->log({m_SP[2]}, bytesConstRef(m_mem.data() + (uint64_t)m_SP[0], (uint64_t)m_SP[1]));
		}
		NEXT

		CASE(LOG2)
		{
			logGasMem();
			ON_OP();
			updateIOGas();

			m_ext->log({m_SP[2], m_SP[3]}, bytesConstRef(m_mem.data() + (uint64_t)m_SP[0], (uint64_t)m_SP[1]));
		}
		NEXT

		CASE(LOG3)
		{
			logGasMem();
			ON_OP();
			updateIOGas();

			m_ext->log({m_SP[2], m_SP[3], m_SP[4]}, bytesConstRef(m_mem.data() + (uint64_t)m_SP[0], (uint64_t)m_SP[1]));
		}
		NEXT

		CASE(LOG4)
		{
			logGasMem();
			ON_OP();
			updateIOGas();

			m_ext->log({m_SP[2], m_SP[3], m_SP[4], m_SP[5]}, bytesConstRef(m_mem.data() + (uint64_t)m_SP[0], (uint64_t)m_SP[1]));
		}
		NEXT	

		CASE(EXP)
		{
			u256 expon = m_SP[1];
			m_runGas = toInt63(m_schedule->expGas + m_schedule->expByteGas * (32 - (h256(expon).firstBitSet() / 8)));
			ON_OP();
			updateIOGas();

			u256 base = m_SP[0];
			m_SPP[0] = exp256(base, expon);
		}
		NEXT

		//
		// ordinary instructions
		//

		CASE(ADD)
		{
			ON_OP();
			updateIOGas();

			//pops two items and pushes their sum mod 2^256.
			m_SPP[0] = m_SP[0] + m_SP[1];
		}
		NEXT

		CASE(MUL)
		{
			ON_OP();
			updateIOGas();

#if EVM_HACK_MUL_64
			*(uint64_t*)&m_SP[1] *= *(uint64_t*)&m_SP[0];
#else
			//pops two items and pushes their product mod 2^256.
			m_SPP[0] = m_SP[0] * m_SP[1];
#endif
		}
		NEXT

		CASE(SUB)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_SP[0] - m_SP[1];
		}
		NEXT

		CASE(DIV)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_SP[1] ? divWorkaround(m_SP[0], m_SP[1]) : 0;
		}
		NEXT

		CASE(SDIV)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_SP[1] ? s2u(divWorkaround(u2s(m_SP[0]), u2s(m_SP[1]))) : 0;
			--m_SP;
		}
		NEXT

		CASE(MOD)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_SP[1] ? modWorkaround(m_SP[0], m_SP[1]) : 0;
		}
		NEXT

		CASE(SMOD)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_SP[1] ? s2u(modWorkaround(u2s(m_SP[0]), u2s(m_SP[1]))) : 0;
		}
		NEXT

		CASE(NOT)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = ~m_SP[0];
		}
		NEXT

		CASE(LT)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_SP[0] < m_SP[1] ? 1 : 0;
		}
		NEXT

		CASE(GT)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_SP[0] > m_SP[1] ? 1 : 0;
		}
		NEXT

		CASE(SLT)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = u2s(m_SP[0]) < u2s(m_SP[1]) ? 1 : 0;
		}
		NEXT

		CASE(SGT)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = u2s(m_SP[0]) > u2s(m_SP[1]) ? 1 : 0;
		}
		NEXT

		CASE(EQ)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_SP[0] == m_SP[1] ? 1 : 0;
		}
		NEXT

		CASE(ISZERO)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_SP[0] ? 0 : 1;
		}
		NEXT

		CASE(AND)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_SP[0] & m_SP[1];
		}
		NEXT

		CASE(OR)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_SP[0] | m_SP[1];
		}
		NEXT

		CASE(XOR)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_SP[0] ^ m_SP[1];
		}
		NEXT

		CASE(BYTE)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_SP[0] < 32 ? (m_SP[1] >> (unsigned)(8 * (31 - m_SP[0]))) & 0xff : 0;
		}
		NEXT

		CASE(ADDMOD)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_SP[2] ? u256((u512(m_SP[0]) + u512(m_SP[1])) % m_SP[2]) : 0;
		}
		NEXT

		CASE(MULMOD)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_SP[2] ? u256((u512(m_SP[0]) * u512(m_SP[1])) % m_SP[2]) : 0;
		}
		NEXT

		CASE(SIGNEXTEND)
		{
			ON_OP();
			updateIOGas();

			if (m_SP[0] < 31)
			{
				unsigned testBit = static_cast<unsigned>(m_SP[0]) * 8 + 7;
				u256& number = m_SP[1];
				u256 mask = ((u256(1) << testBit) - 1);
				if (boost::multiprecision::bit_test(number, testBit))
					number |= ~mask;
				else
					number &= mask;
			}
		}
		NEXT

		CASE(ADDRESS)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = fromAddress(m_ext->myAddress);
		}
		NEXT

		CASE(ORIGIN)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = fromAddress(m_ext->origin);
		}
		NEXT

		CASE(BALANCE)
		{
			m_runGas = toInt63(m_schedule->balanceGas);
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_ext->balance(asAddress(m_SP[0]));
		}
		NEXT


		CASE(CALLER)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = fromAddress(m_ext->caller);
		}
		NEXT

		CASE(CALLVALUE)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_ext->value;
		}
		NEXT


		CASE(CALLDATALOAD)
		{
			ON_OP();
			updateIOGas();

			if (u512(m_SP[0]) + 31 < m_ext->data.size())
				m_SP[0] = (u256)*(h256 const*)(m_ext->data.data() + (size_t)m_SP[0]);
			else if (m_SP[0] >= m_ext->data.size())
				m_SP[0] = u256(0);
			else
			{ 	h256 r;
				for (uint64_t i = (uint64_t)m_SP[0], e = (uint64_t)m_SP[0] + (uint64_t)32, j = 0; i < e; ++i, ++j)
					r[j] = i < m_ext->data.size() ? m_ext->data[i] : 0;
				m_SP[0] = (u256)r;
			};
		}
		NEXT


		CASE(CALLDATASIZE)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_ext->data.size();
		}
		NEXT

		CASE(CODESIZE)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_ext->code.size();
		}
		NEXT

		CASE(EXTCODESIZE)
		{
			m_runGas = toInt63(m_schedule->extcodesizeGas);
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_ext->codeSizeAt(asAddress(m_SP[0]));
		}
		NEXT

		CASE(CALLDATACOPY)
		{
			m_copyMemSize = toInt63(m_SP[2]);
			updateMem(memNeed(m_SP[0], m_SP[2]));
			ON_OP();
			updateIOGas();

			copyDataToMemory(m_ext->data, m_SP);
		}
		NEXT

		CASE(CODECOPY)
		{
			m_copyMemSize = toInt63(m_SP[2]);
			updateMem(memNeed(m_SP[0], m_SP[2]));
			ON_OP();
			updateIOGas();

			copyDataToMemory(&m_ext->code, m_SP);
		}
		NEXT

		CASE(EXTCODECOPY)
		{
			m_runGas = toInt63(m_schedule->extcodecopyGas);
			m_copyMemSize = toInt63(m_SP[3]);
			updateMem(memNeed(m_SP[1], m_SP[3]));
			ON_OP();
			updateIOGas();

			Address a = asAddress(m_SP[0]);
			copyDataToMemory(&m_ext->codeAt(a), m_SP + 1);
		}
		NEXT


		CASE(GASPRICE)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_ext->gasPrice;
		}
		NEXT

		CASE(BLOCKHASH)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = (u256)m_ext->blockHash(m_SP[0]);
		}
		NEXT

		CASE(COINBASE)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = (u160)m_ext->envInfo().author();
		}
		NEXT

		CASE(TIMESTAMP)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_ext->envInfo().timestamp();
		}
		NEXT

		CASE(NUMBER)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_ext->envInfo().number();
		}
		NEXT

		CASE(DIFFICULTY)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_ext->envInfo().difficulty();
		}
		NEXT

		CASE(GASLIMIT)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_ext->envInfo().gasLimit();
		}
		NEXT

		CASE(POP)
		{
			ON_OP();
			updateIOGas();

			--m_SP;
		}
		NEXT

		CASE(PUSHC)
		{
#ifdef EVM_USE_CONSTANT_POOL
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_pool[m_code[++m_PC]];
			m_PC += m_code[m_PC];
#else
			throwBadInstruction();
#endif
		}
		CONTINUE

		CASE(PUSH1)
		{
			ON_OP();
			updateIOGas();
			++m_PC;
			m_SPP[0] = m_code[m_PC];
			++m_PC;
		}
		CONTINUE

		CASE(PUSH2)
		CASE(PUSH3)
		CASE(PUSH4)
		CASE(PUSH5)
		CASE(PUSH6)
		CASE(PUSH7)
		CASE(PUSH8)
		CASE(PUSH9)
		CASE(PUSH10)
		CASE(PUSH11)
		CASE(PUSH12)
		CASE(PUSH13)
		CASE(PUSH14)
		CASE(PUSH15)
		CASE(PUSH16)
		CASE(PUSH17)
		CASE(PUSH18)
		CASE(PUSH19)
		CASE(PUSH20)
		CASE(PUSH21)
		CASE(PUSH22)
		CASE(PUSH23)
		CASE(PUSH24)
		CASE(PUSH25)
		CASE(PUSH26)
		CASE(PUSH27)
		CASE(PUSH28)
		CASE(PUSH29)
		CASE(PUSH30)
		CASE(PUSH31)
		CASE(PUSH32)
		{
			ON_OP();
			updateIOGas();

			int numBytes = (int)m_OP - (int)Instruction::PUSH1 + 1;
			m_SPP[0] = 0;
			// Construct a number out of PUSH bytes.
			// This requires the code has been copied and extended by 32 zero
			// bytes to handle "out of code" push data here.
			for (++m_PC; numBytes--; ++m_PC)
				m_SPP[0] = (m_SPP[0] << 8) | m_code[m_PC];
		}
		CONTINUE

		CASE(JUMP)
		{
			ON_OP();
			updateIOGas();
			m_PC = verifyJumpDest(m_SP[0]);
		}
		CONTINUE

		CASE(JUMPI)
		{
			ON_OP();
			updateIOGas();
			if (m_SP[1])
				m_PC = verifyJumpDest(m_SP[0]);
			else
				++m_PC;
		}
		CONTINUE

#if EVM_JUMPS_AND_SUBS
		CASE(JUMPTO)
		{
			ON_OP();
			updateIOGas();
			
			m_PC = decodeJumpDest(m_code.data(), m_PC);
		}
		CONTINUE

		CASE(JUMPIF)
		{
			ON_OP();
			updateIOGas();
			
			if (m_SP[0])
				m_PC = decodeJumpDest(m_code.data(), m_PC);
			else
				++m_PC;
		}
		CONTINUE

		CASE(JUMPV)
		{
			ON_OP();
			updateIOGas();
			m_PC = decodeJumpvDest(m_code.data(), m_PC, byte(m_SP[0]));
		}
		CONTINUE

		CASE(JUMPSUB)
		{
			ON_OP();
			updateIOGas();
			*m_RP++ = m_PC++;
			m_PC = decodeJumpDest(m_code.data(), m_PC);
			}
		}
		CONTINUE

		CASE(JUMPSUBV)
		{
			ON_OP();
			updateIOGas();
			*m_RP++ = m_PC;
			m_PC = decodeJumpvDest(m_code.data(), m_PC, byte(m_SP[0]));
		}
		CONTINUE

		CASE(RETURNSUB)
		{
			ON_OP();
			updateIOGas();
			
			m_PC = *m_RP--;
		}
		NEXT
#else
		CASE(JUMPTO)
		CASE(JUMPIF)
		CASE(JUMPV)
		CASE(JUMPSUB)
		CASE(JUMPSUBV)
		CASE(RETURNSUB)
		{
			throwBadInstruction();
		}
		CONTINUE
#endif

		CASE(JUMPC)
		{
#ifdef EVM_REPLACE_CONST_JUMP
			ON_OP();
			updateIOGas();

			m_PC = uint64_t(m_SP[0]);
#else
			throwBadInstruction();
#endif
		}
		CONTINUE

		CASE(JUMPCI)
		{
#ifdef EVM_REPLACE_CONST_JUMP
			ON_OP();
			updateIOGas();

			if (m_SP[1])
				m_PC = uint64_t(m_SP[0]);
			else
				++m_PC;
#else
			throwBadInstruction();
#endif
		}
		CONTINUE

		CASE(DUP1)
		CASE(DUP2)
		CASE(DUP3)
		CASE(DUP4)
		CASE(DUP5)
		CASE(DUP6)
		CASE(DUP7)
		CASE(DUP8)
		CASE(DUP9)
		CASE(DUP10)
		CASE(DUP11)
		CASE(DUP12)
		CASE(DUP13)
		CASE(DUP14)
		CASE(DUP15)
		CASE(DUP16)
		{
			ON_OP();
			updateIOGas();

			unsigned n = (unsigned)m_OP - (unsigned)Instruction::DUP1;
#if EVM_HACK_DUP_64
			*(uint64_t*)m_SPP = *(uint64_t*)(m_SP + n);
#else
			m_SPP[0] = m_SP[n];
#endif
		}
		NEXT


		CASE(SWAP1)
		CASE(SWAP2)
		CASE(SWAP3)
		CASE(SWAP4)
		CASE(SWAP5)
		CASE(SWAP6)
		CASE(SWAP7)
		CASE(SWAP8)
		CASE(SWAP9)
		CASE(SWAP10)
		CASE(SWAP11)
		CASE(SWAP12)
		CASE(SWAP13)
		CASE(SWAP14)
		CASE(SWAP15)
		CASE(SWAP16)
		{
			ON_OP();
			updateIOGas();

			unsigned n = (unsigned)m_OP - (unsigned)Instruction::SWAP1 + 1;
			std::swap(m_SP[0], m_SP[n]);
		}
		NEXT


		CASE(SLOAD)
		{
			m_runGas = toInt63(m_schedule->sloadGas);
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_ext->store(m_SP[0]);
		}
		NEXT

		CASE(SSTORE)
		{
			if (!m_ext->store(m_SP[0]) && m_SP[1])
				m_runGas = toInt63(m_schedule->sstoreSetGas);
			else if (m_ext->store(m_SP[0]) && !m_SP[1])
			{
				m_runGas = toInt63(m_schedule->sstoreResetGas);
				m_ext->sub.refunds += m_schedule->sstoreRefundGas;
			}
			else
				m_runGas = toInt63(m_schedule->sstoreResetGas);
			ON_OP();
			updateIOGas();
	
			m_ext->setStore(m_SP[0], m_SP[1]);
		}
		NEXT

		CASE(PC)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_PC;
		}
		NEXT

		CASE(MSIZE)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_mem.size();
		}
		NEXT

		CASE(GAS)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_io_gas;
		}
		NEXT

		CASE(JUMPDEST)
		{
			m_runGas = 1;
			ON_OP();
			updateIOGas();
		}
		NEXT

#if EVM_JUMPS_AND_SUBS
		CASE(BEGINSUB)
		{
			m_runGas = 1;
			ON_OP();
			updateIOGas();
		}
		NEXT
#else
		CASE(BEGINSUB)
#endif
		CASE(BEGINDATA)
		CASE(BAD)
		DEFAULT
		{
			throwBadInstruction();
		}
	}	
	WHILE_CASES
}
