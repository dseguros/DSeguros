namespace dev
{
namespace eth
{

///////////////////////////////////////////////////////////////////////////////
//
// interpreter configuration macros for optimizations and tracing
//
// EVM_SWITCH_DISPATCH    - dispatch via loop and switch
// EVM_JUMP_DISPATCH      - dispatch via a jump table - available only on GCC
//
// EVM_USE_CONSTANT_POOL  - 256 constants unpacked and ready to assign to stack
//
// EVM_REPLACE_CONST_JUMP - with pre-verified jumps to save runtime lookup
//
// EVM_TRACE              - provides various levels of tracing

#ifndef EVM_JUMP_DISPATCH
	#ifdef __GNUC__
		#define EVM_JUMP_DISPATCH false
	#else
		#define EVM_JUMP_DISPATCH false
	#endif
#endif
#if EVM_JUMP_DISPATCH
	#ifndef __GNUC__
		#error "address of label extension avaiable only on Gnu"
	#endif
#else
	#define EVM_SWITCH_DISPATCH
#endif

#ifndef EVM_OPTIMIZE
	#define EVM_OPTIMIZE false
#endif
#if EVM_OPTIMIZE
	#define EVM_REPLACE_CONST_JUMP false
	#define EVM_USE_CONSTANT_POOL false
	#define EVM_DO_FIRST_PASS_OPTIMIZATION ( \
				EVM_REPLACE_CONST_JUMP || \
				EVM_USE_CONSTANT_POOL \
			)
#endif

#define EVM_JUMPS_AND_SUBS false

///////////////////////////////////////////////////////////////////////////////
//
// set EVM_TRACE to 2, 1, or 0 for more, less, or no tracing to cerr
//
#ifndef EVM_TRACE
	#define EVM_TRACE 0
#endif
#if EVM_TRACE > 0

	#undef ON_OP
	#if EVM_TRACE > 1
		#define ON_OP() \
			(cerr <<"### "<< ++m_nSteps <<" @"<< m_PC <<" "<< instructionInfo(m_OP).name <<endl)
	#else
		#define ON_OP() onOperation()
	#endif
	
	#define TRACE_STR(level, str) \
		if ((level) <= EVM_TRACE) \
			cerr <<"$$$ "<< (str) <<endl;
			
	#define TRACE_VAL(level, name, val) \
		if ((level) <= EVM_TRACE) \
			cerr <<"=== "<< (name) <<" "<<hex<< (val) <<endl;
	#define TRACE_OP(level, pc, op) \
		if ((level) <= EVM_TRACE) \
			cerr <<"*** "<< (pc) <<" "<< instructionInfo(op).name <<endl;
			
	#define TRACE_PRE_OPT(level, pc, op) \
		if ((level) <= EVM_TRACE) \
			cerr <<"@@@ "<< (pc) <<" "<< instructionInfo(op).name <<endl;
			
	#define TRACE_POST_OPT(level, pc, op) \
		if ((level) <= EVM_TRACE) \
			cerr <<"... "<< (pc) <<" "<< instructionInfo(op).name <<endl;
#else
	#define TRACE_STR(level, str)
	#define TRACE_VAL(level, name, val)
	#define TRACE_OP(level, pc, op)
	#define TRACE_PRE_OPT(level, pc, op)
	#define TRACE_POST_OPT(level, pc, op)
	#define ON_OP() onOperation()
#endif

// Executive swallows exceptions in some circumstances
#if 0
	#define THROW_EXCEPTION(X) \
		((cerr << "!!! EVM EXCEPTION " << (X).what() << endl), abort())
#else
	#if EVM_TRACE > 0
		#define THROW_EXCEPTION(X) \
			((cerr << "!!! EVM EXCEPTION " << (X).what() << endl), BOOST_THROW_EXCEPTION(X))
	#else
		#define THROW_EXCEPTION(X) BOOST_THROW_EXCEPTION(X)
	#endif
#endif

///////////////////////////////////////////////////////////////////////////////
//
// build a simple loop-and-switch interpreter
//
#if defined(EVM_SWITCH_DISPATCH)

	#define INIT_CASES if (!m_caseInit) { m_PC = 0; m_caseInit = true; return; }
	#define DO_CASES for(;;) { fetchInstruction(); switch(m_OP) {
	#define CASE(name) case Instruction::name:
	#define NEXT ++m_PC; break;
	#define CONTINUE continue;
	#define BREAK return;
	#define DEFAULT default:
	#define WHILE_CASES } }

}}
