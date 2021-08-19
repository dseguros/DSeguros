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
