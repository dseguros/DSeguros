#include <libethereum/ExtVM.h>
#include "VMConfig.h"
#include "VM.h"
using namespace std;
using namespace dev;
using namespace dev::eth;

void VM::copyDataToMemory(bytesConstRef _data, u256*_sp)
{
	auto offset = static_cast<size_t>(_sp[0]);
	s512 bigIndex = _sp[1];
	auto index = static_cast<size_t>(bigIndex);
	auto size = static_cast<size_t>(_sp[2]);

	size_t sizeToBeCopied = bigIndex + size > _data.size() ? _data.size() < bigIndex ? 0 : _data.size() - index : size;

	if (sizeToBeCopied > 0)
		std::memcpy(m_mem.data() + offset, _data.data() + index, sizeToBeCopied);
	if (size > sizeToBeCopied)
		std::memset(m_mem.data() + offset + sizeToBeCopied, 0, size - sizeToBeCopied);
}

// consolidate exception throws to avoid spraying boost code all over interpreter

void VM::throwOutOfGas()
{
	if (m_onFail)
		(this->*m_onFail)();
	BOOST_THROW_EXCEPTION(OutOfGas());
}

void VM::throwBadInstruction()
{
	if (m_onFail)
		(this->*m_onFail)();
	BOOST_THROW_EXCEPTION(BadInstruction());
}

void VM::throwBadJumpDestination()
{
	if (m_onFail)
		(this->*m_onFail)();
	BOOST_THROW_EXCEPTION(BadJumpDestination());
}

void VM::throwBadStack(unsigned _removed, unsigned _added)
{
	bigint size = m_stackEnd - m_SPP;
	if (size < _removed)
	{
		if (m_onFail)
			(this->*m_onFail)();
		BOOST_THROW_EXCEPTION(StackUnderflow() << RequirementError((bigint)_removed, size));
	}
	else
	{
		if (m_onFail)
			(this->*m_onFail)();
		BOOST_THROW_EXCEPTION(OutOfStack() << RequirementError((bigint)(_added - _removed), size));
	}
}

void VM::throwRevertInstruction(owning_bytes_ref&& _output)
{
	// We can't use BOOST_THROW_EXCEPTION here because it makes a copy of exception inside and RevertInstruction has no copy constructor 
	throw RevertInstruction(move(_output));
}

int64_t VM::verifyJumpDest(u256 const& _dest, bool _throw)
{
	// check for overflow
	if (_dest <= 0x7FFFFFFFFFFFFFFF) {

		// check for within bounds and to a jump destination
		// use binary search of array because hashtable collisions are exploitable
		uint64_t pc = uint64_t(_dest);
		if (std::binary_search(m_jumpDests.begin(), m_jumpDests.end(), pc))
			return pc;
	}
	if (_throw)
		throwBadJumpDestination();
	return -1;
}

void VM::caseCreate()
{
	m_bounce = &VM::interpretCases;
	m_runGas = toInt63(m_schedule->createGas);
	updateMem(memNeed(m_SP[1], m_SP[2]));
	ON_OP();
	updateIOGas();

	auto const& endowment = m_SP[0];
	uint64_t initOff = (uint64_t)m_SP[1];
	uint64_t initSize = (uint64_t)m_SP[2];

	if (m_ext->balance(m_ext->myAddress) >= endowment && m_ext->depth < 1024)
	{
		*m_io_gas_p = m_io_gas;
		u256 createGas = *m_io_gas_p;
		if (!m_schedule->staticCallDepthLimit())
			createGas -= createGas / 64;
		u256 gas = createGas;
		m_SPP[0] = (u160)m_ext->create(endowment, gas, bytesConstRef(m_mem.data() + initOff, initSize), m_onOp);
		*m_io_gas_p -= (createGas - gas);
		m_io_gas = uint64_t(*m_io_gas_p);
	}
	else
		m_SPP[0] = 0;
	++m_PC;
}

void VM::caseCall()
{
	m_bounce = &VM::interpretCases;
	unique_ptr<CallParameters> callParams(new CallParameters());
	bytesRef output;
	if (caseCallSetup(callParams.get(), output))
	{
		std::pair<bool, owning_bytes_ref> callResult = m_ext->call(*callParams);
		if (callResult.second)
			callResult.second.copyTo(output);

		m_SPP[0] = callResult.first ? 1 : 0;
	}
	else
		m_SPP[0] = 0;
	m_io_gas += uint64_t(callParams->gas);
	++m_PC;
}
