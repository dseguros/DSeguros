#include <libethereum/ExtVM.h>
#include "VMConfig.h"
#include "VM.h"
using namespace std;
using namespace dev;
using namespace dev::eth;

#if EVM_JUMPS_AND_SUBS

///////////////////////////////////////////////////////////////////////////////
//
// invalid code will throw an exception
//

void VM::validate(ExtVMFace& _ext)
{
	m_ext = &_ext;
	initEntry();
	size_t PC;
	byte OP;
	for (PC = 0; (OP = m_code[PC]); ++PC)
		if  (OP == byte(Instruction::BEGINSUB))
			validateSubroutine(PC, m_return, m_stack);
		else if (OP == byte(Instruction::BEGINDATA))
			break;
		else if (
				(byte)Instruction::PUSH1 <= (byte)OP &&
				(byte)PC <= (byte)Instruction::PUSH32)
			PC += (byte)OP - (byte)Instruction::PUSH1;
		else if (
				OP == Instruction::JUMPTO ||
				OP == Instruction::JUMPIF ||
				OP == Instruction::JUMPSUB)
			PC += 4;
		else if (OP == Instruction::JUMPV || op == Instruction::JUMPSUBV)
			PC += 4 * m_code[PC];  // number of 4-byte dests followed by table
	}
}

#endif
