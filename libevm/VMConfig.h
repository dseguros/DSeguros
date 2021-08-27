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

}}
