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

bool VM::caseCallSetup(CallParameters *callParams, bytesRef& o_output)
{
	m_runGas = toInt63(m_schedule->callGas);

	if (m_OP == Instruction::CALL && !m_ext->exists(asAddress(m_SP[1])))
		if (m_SP[2] > 0 || m_schedule->zeroValueTransferChargesNewAccountGas())
			m_runGas += toInt63(m_schedule->callNewAccountGas);

	if (m_OP != Instruction::DELEGATECALL && m_SP[2] > 0)
		m_runGas += toInt63(m_schedule->callValueTransferGas);

	size_t sizesOffset = m_OP == Instruction::DELEGATECALL ? 2 : 3;
	u256 inputOffset  = m_SP[sizesOffset];
	u256 inputSize    = m_SP[sizesOffset + 1];
	u256 outputOffset = m_SP[sizesOffset + 2];
	u256 outputSize   = m_SP[sizesOffset + 3];
	uint64_t inputMemNeed = memNeed(inputOffset, inputSize);
	uint64_t outputMemNeed = memNeed(outputOffset, outputSize);

	m_newMemSize = std::max(inputMemNeed, outputMemNeed);
	updateMem(m_newMemSize);
	updateIOGas();

	// "Static" costs already applied. Calculate call gas.
	if (m_schedule->staticCallDepthLimit())
	{
		// With static call depth limit we just charge the provided gas amount.
		callParams->gas = m_SP[0];
	}
	else
	{
		// Apply "all but one 64th" rule.
		u256 maxAllowedCallGas = m_io_gas - m_io_gas / 64;
		callParams->gas = std::min(m_SP[0], maxAllowedCallGas);
	}

	m_runGas = toInt63(callParams->gas);
	ON_OP();
	updateIOGas();

	if (m_OP != Instruction::DELEGATECALL && m_SP[2] > 0)
		callParams->gas += m_schedule->callStipend;

	callParams->codeAddress = asAddress(m_SP[1]);

	unsigned inOutOffset = 0;
	if (m_OP == Instruction::DELEGATECALL)
	{
		callParams->apparentValue = m_ext->value;
		callParams->valueTransfer = 0;
	}
	else
	{
		callParams->apparentValue = callParams->valueTransfer = m_SP[2];
		inOutOffset = 1;
	}

	uint64_t inOff = (uint64_t)m_SP[inOutOffset + 2];
	uint64_t inSize = (uint64_t)m_SP[inOutOffset + 3];
	uint64_t outOff = (uint64_t)m_SP[inOutOffset + 4];
	uint64_t outSize = (uint64_t)m_SP[inOutOffset + 5];

	if (m_ext->balance(m_ext->myAddress) >= callParams->valueTransfer && m_ext->depth < 1024)
	{
		callParams->onOp = m_onOp;
		callParams->senderAddress = m_OP == Instruction::DELEGATECALL ? m_ext->caller : m_ext->myAddress;
		callParams->receiveAddress = m_OP == Instruction::CALL ? callParams->codeAddress : m_ext->myAddress;
		callParams->data = bytesConstRef(m_mem.data() + inOff, inSize);
		o_output = bytesRef(m_mem.data() + outOff, outSize);
		return true;
	}
	else
		return false;
}
