// Copyright (c) 1998-2021 Cadence Design Systems, Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// crt1-sm.S -- branch from xtos/crt1.S


// Sets up the C calling environment (sets up stack, clears BSS sections,
// calls __clibrary_init) and then calls secmon_unpack() (instead of main()). 
// Control arrives here from the reset handler.


#include <xtensa/config/core.h>

#if !(XCHAL_HAVE_XEA2)
#error Secure LX configurations require XEA2
#endif
#if !(XCHAL_HAVE_MPU)
#error Secure LX configurations require MPU
#endif
#if !(XCHAL_HAVE_EXCEPTIONS)
#error Secure LX configurations require exceptions
#endif
#if (XCHAL_HW_MIN_VERSION < XTENSA_HWVERSION_RI_2020_5)
#error Secure LX configurations require RI-2020.5 or newer HW
#endif

#if defined (SIMULATOR)
#include <xtensa/simboard.h>
#include <xtensa/simcall.h>
#endif

#include <xtensa/coreasm.h>
#include <xtensa/corebits.h>
#include "xtos-internal.h"

#include <xtensa/mpuasm.h>

# if XCHAL_HAVE_CACHEADRDIS
#  define IFCAD
# else
#  define IFCAD #
# endif


	// Exports
	.global	_start
	.type	_start,@function

	.global	main
	.type	main,@function

	// External Functions
	//
	// secmon_unpack                    from user code
	// secmon_user_init                 from user code
	// secmon_mpu_init_final            from secmon code
	// __memmap_init                    (optional) user code
	// __bss_init                       (optional) user code
	// __stack_init                     (optional) user code

	// External Symbols/Variables
	//
	// __stack                          from linker script or user code
	// _bss_table_start                 from linker script
	// _bss_table_end                   from linker script
	// __xtos_simc                      from linker script or user code
	// __xt_mpu_init_table              from secure MPU table
	// __xt_mpu_init_table_size         from secure MPU table
	// __xt_mpu_init_table_size_secure  from secure MPU table
	// __xt_mpu_init_cacheadrdis        from secure MPU table
	// __xt_mpu_nsm_write_enable        (optional) user code


// Secure Boot: always use call0 ABI
#define CALL	call0
#define CALLX	callx0
#define ARG1	a2	/* 1st outgoing call argument */
#define ARG2	a3	/* 2nd outgoing call argument */
#define ARG3	a4	/* 3rd outgoing call argument */
#define ARG4	a5	/* 4th outgoing call argument */

#define SETARGS_ARGC	a6	/* argc if set from xt-gdb "set-args" */
#define SETARGS_ARGV	a7	/* argv if set from xt-gdb "set-args" */

	.data

#if !defined (SIMULATOR)
	.weak	_start_argc
	.weak	_start_argv

	.align	4
_start_argc:
	.word	1		// one argument (program name)
_start_argv:
	.word	_start_null	// empty program name
#endif

	.weak	_start_envp

	.align	4
_start_envp:
_start_null:
	.word	0		// end of argv array, empty string

	.global	secmon_secure_sp
secmon_secure_sp:
	.word	0		// secure monitor SP for interrupt/exception handling


	.text
	.align	4

_start:
	//  _start is typically NOT at the beginning of the text segment --
	//  it is always called from either the reset vector or other code
	//  that does equivalent initialization (such as crt0-app.S).
	//
	//  Assumptions on entry to _start:
	//	- (XEA1/XEA2) low (level-one) and medium priority interrupts are
	//	  disabled via PS.INTLEVEL and/or INTENABLE (PS.INTLEVEL is expected
	//	  to be zeroed, to potentially enable them, before calling secmon_unpack)
	//	- C calling context not initialized:
	//	  - PS not initialized
	//	  - SP not initialized
	//	- the following are initialized:
	//	  - LITBASE, cache attributes, WindowBase, WindowStart,
	//	    CPENABLE, FP's FCR and FSR, EXCSAVE[n]

	// Keep a0 zero.  It is used to initialize a few things.
	// It is also the return address, where zero indicates
	// that the frame used by _start is the bottommost frame.

	movi	a0, 0			// keep this register zero.

	// Secure Boot (step #5): Initialize MPU.

	// Now setup the memory attributes.  On some cores this "enables" caches,
	// but must only be on nonsecure memory (will be verified later).
	// We do this after unpacking since IRAM is configured without write
	// access for security purposes.  Secure entries are locked by mpu_write_map.

	// There must be a user-provided MPU table; program the MPU
	// with it now.  For romable images, unpacking will have already occurred,
	// so the MPU table values and table size can reside outside of the reset 
	// vector/handler regions.

	// NOTE: only the secure entries are programmed here.  The nonsecure
	// entries will be programmed after unpack, allowing for nonsecure IRAM
	// to be write-protected.

	.weak	__xt_mpu_init_table		// Table of MPU entries
	.weak	__xt_mpu_init_table_size	// Number of entries in table
	.weak	__xt_mpu_init_table_size_secure	// Number of secure entries in table
IFCAD	.weak	__xt_mpu_init_cacheadrdis	// value for cacheadrdis

	movi	a2, __xt_mpu_init_table		// non-zero if user defined
	movi	a3, __xt_mpu_init_table_size	// non-zero if user defined
IFCAD	movi	a4, __xt_mpu_init_cacheadrdis	// non-zero if user defined

	beqz	a2, .Lno_user_mpu
	beqz	a3, .Lno_user_mpu
	l32i	a3, a3, 0
	movi	a5, XCHAL_MPU_ENTRIES+1
	beqz	a3, .Lno_user_mpu		// skip if size = 0
	bge	a3, a5, .Luser_mpu_too_large	// skip if size too big

	// After checking total # entries, program only secure # entries
	movi	a3, __xt_mpu_init_table_size_secure	// non-zero if user defined
	beqz	a3, .Lno_user_mpu
	l32i	a3, a3, 0
	beqz	a3, .Lno_user_mpu		// skip if size = 0
	bge	a3, a5, .Luser_mpu_too_large	// skip if size too big

	mpu_write_map	a2, a3, a9, a10, a11, a12, a13, a14, a15
IFCAD	beqz	a4, .Lmpu_nsm_write_disable
IFCAD	l32i	a4, a4, 0
IFCAD	wsr.cacheadrdis	a4
	j	.Lmpu_nsm_write_disable

	// MPU settings must be specified; trap if not found
.Lno_user_mpu:
#if defined (SIMULATOR)
	// ISS expects exit code in a3
	movi	a3, 14
	mov	a4, a3			// save exitcode for the debugger, as simcall will erase a3
	movi	a2, SYS_exit
	simcall				// exit if in simulator, else NOP
	mov	a2, a4
#endif
#if XCHAL_HAVE_DEBUG
	break	1, 15			// back to debugger, if one is attached
#endif 
#if XCHAL_HAVE_INTERRUPTS
	waiti	15
#endif
	j	.Lno_user_mpu


	// MPU table is too large; trap
.Luser_mpu_too_large:
#if defined (SIMULATOR)
	// ISS expects exit code in a3
	movi	a3, 15
	mov	a4, a3			// save exitcode for the debugger, as simcall will erase a3
	movi	a2, SYS_exit
	simcall				// exit if in simulator, else NOP
	mov	a2, a4
#endif
#if XCHAL_HAVE_DEBUG
	break	1, 15			// back to debugger, if one is attached
#endif 
#if XCHAL_HAVE_INTERRUPTS
	waiti	15
#endif
	j	.Luser_mpu_too_large


	// Set up mpucfg register.  It is possible to override the default
	// behavior of setting MPUCFG.NSWRDIS by defining the optional
	// "__xt_mpu_nsm_write_enable" hook with a non-zero value.
.Lmpu_nsm_write_disable:
	.weak	__xt_mpu_nsm_write_enable
	movi	a2, __xt_mpu_nsm_write_enable
	bnez	a2, 1f
	rsr.mpucfg	a2
	movi	a3, (1 << XTHAL_MPUCFG_NSWRDIR_SHIFT)
	or	a2, a2, a3
	wsr.mpucfg	a2
1:

	// Secure Boot (step #6): Internal state init--secure handlers, secure stack

	// Set up the stack pointer. It is possible to override the default
	// stack location by providing a user hook __stack_init. This hook
	// must not rely on stack and BSS availability.

	.weak	__stack_init
	movi	a2, __stack_init	// optional user hook
	movi	a1, __stack		// default SP
	beqz	a2, 1f
	callx0	a2
	movi	a0, 0			// keep this register zero
1:

#if XCHAL_HAVE_ISL
	// If the Interrupt Stack Limit register exists, then set up stack limit
	// for XTOS. This can be overridden later by applications if needed.
	compute_stack_limit	a3 a4 a5
	wsr.isl	a3
#endif

#if defined (SIMULATOR)
	// Reserve stack space for
	//    - argv array
	//    - argument strings

	movi	a2, SYS_iss_argv_size
	simcall				// returns size of argv[] + its strings in a2

	// The stack only needs 16-byte alignment.
	// However, here we round up the argv size further to 128 byte multiples
	// so that in most cases, variations in argv[0]'s path do not result in
	// different stack allocation.  Otherwise, such variations can impact
	// execution timing (eg. due to cache effects etc) for the same code and data.
	// If we have an outbound bus, it's more likely there's enough memory.

# if XCHAL_HAVE_PIF
	addi	a2, a2, 127
	srli	a2, a2, 7
	slli	a2, a2, 7
# else
	// Keep stack 16-byte aligned.
	addi	a2, a2, 15
	srli	a2, a2, 4
	slli	a2, a2, 4
# endif

	// No need to use MOVSP because we have no caller (we're the base caller).
	// In fact it's better not to use MOVSP in this context, to avoid unnecessary
	// ALLOCA exceptions and copying from undefined memory.

	sub	a1, a1, a2
	movi	a3, _stack_sentry
	bltu	a1, a3, secmon_stack_overflow
	movi	a3, _heap_sentry
	bgeu	a1, a3, secmon_stack_overflow
#endif	// SIMULATOR

	// Secure Boot (step #6, continued):
	// Now that the stack is set, we can set PS for secure monitor execution.
	// (User vector mode, enable secure interrupts, disable window exceptions).

	movi	a3, PS_PNSM | PS_UM | PS_INTLEVEL(XCHAL_EXCM_LEVEL)	// PS.WOE = 0, PS.UM = 1, PS.EXCM = 0, PS.INTLEVEL = EXCMLEVEL
	wsr.ps	a3
	rsync

	// Do any initialization that affects the memory map, such as setting
	// up TLB entries, that needs to be done before we can successfully
	// clear BSS (e.g. if some BSS segments are in remapped areas).
	//
	// NOTE: This hook works where the reset vector does not unpack segments
	// (see "ROM packing" in the LSP manual), or where unpacking of segments
	// is not affected by memory remapping. If ROM unpacking is affected,
	// TLB setup must be done in assembler from the reset handler.
	//
	// The __memmap_init() routine can be a C function, however it does not
	// have BSS initialized! In particular, __memmap_init() cannot set BSS
	// variables, i.e. uninitialized global variables (they'll be wiped out
	// by the following BSS clear), nor can it assume they are yet initialized
	// to zero.

	.weak	__memmap_init
	movi	a4, __memmap_init	// optional memory map init hook
	beqz	a4, 1f
	CALLX	a4
	movi	a0, 0
1:

#if !XCHAL_HAVE_BOOTLOADER

	// Clear the BSS segments. This code supports multiple BSS areas.

	movi	a6, _bss_table_start
	movi	a7, _bss_table_end
	bgeu	a6, a7, .Lnobss		// No BSS areas if a6 >= a7

	// If there is an optional BSS init routine provided, we will call
	// it instead of clearing BSS here. This routine can be a C function,
	// but it must be written to be able to work with BSS not initialized.

	.weak	__bss_init
	movi	a4, __bss_init
	beqz	a4, 2f
	movi	ARG1, _bss_table_start
	movi	ARG2, _bss_table_end
	CALLX	a4
	movi	a0, 0
	j	.Lnobss			// skip default BSS init code
2:

	// If running on simulator and there is simcall support for memset,
	// use it. Else clear memory the hard way.

#if defined (SIMULATOR)
.Lbssloop:
	movi	a2, SYS_memset
	l32i	a3, a6, 0		// arg1 = fill start address
	movi	a4, 0			// arg2 = fill pattern
	l32i	a5, a6, 4		// get end address
	addi	a6, a6, 8		// next bss table entry
	sub	a5, a5, a3		// arg3 = fill size in bytes
	simcall				// memset(a3,a4,a5)
	bltu	a6, a7, .Lbssloop	// loop until end of bss table
#else
.L0zte:
	l32i	a8, a6, 0		// get start address, assumed multiple of 4
	l32i	a9, a6, 4		// get end address, assumed multiple of 4
	addi	a6, a6, 8		// next entry
	sub	a10, a9, a8		// a10 = length, assumed a multiple of 4
	bbci.l	a10, 2, .L1zte
	s32i	a0, a8, 0		// clear 4 bytes to make length multiple of 8
	addi	a8, a8, 4
.L1zte:	bbci.l	a10, 3, .L2zte
	s32i	a0, a8, 0		// clear 8 bytes to make length multiple of 16
	s32i	a0, a8, 4
	addi	a8, a8, 8
.L2zte:	srli	a10, a10, 4		// length is now multiple of 16, divide by 16
	floopnez	a10, clearzte
	s32i	a0, a8,  0		// clear 16 bytes at a time...
	s32i	a0, a8,  4
	s32i	a0, a8,  8
	s32i	a0, a8, 12
	addi	a8, a8, 16
	floopend	a10, clearzte
	bltu	a6, a7, .L0zte		// loop until end of table of *.bss sections
.L3zte:
#endif	// SIMULATOR

.Lnobss:

#endif	// XCHAL_HAVE_BOOTLOADER

	// Optional user-specific secure monitor initialization hook.
	// Called prior to unpacking nonsecure application; implementable in C.
	// Can be used to register secure interrupt handlers, for example.

	.weak	secmon_user_init
	movi	a4, secmon_user_init
	beqz	a4, 3f
	CALLX	a4
	bnez    a2, secmon_post_exit  // If user init failed, stop execution
	movi	a0, 0
3:

	// Secure Boot: removed _init_reent_bss and __clibrary_init libc calls

.Lunpack:
	// Secure Boot (step #7): Locate nonsecure executable image and load it.
	// Call: int secmon_unpack(int argc, char ** argv, char ** environ);
	//
	// NOTE 1: secmon_unpack() is responsible for validating unpack table 
	// destination addresses reside in nonsecure memory.
	//
	// NOTE 2: secmon_unpack() does not writeback the entire cache as 
    // nonsecure MPU mappings are not yet in place during unpacking.

#if defined (SIMULATOR)
	// Get argv with the arguments from the ISS
	movi	a2, SYS_iss_set_argv
	mov	a3, a1			// tell simcall where to write argv[]
	simcall				// write argv[] array at a3
	movi	a2, SYS_iss_argc
	simcall				// put argc in a2

	// __XTENSA_CALL0_ABI__ always enabled so argc already in ARG1 (a2)
	mov	ARG2, a1		// argv
#else
	movi	ARG1, _start_argc
	movi	ARG2, _start_argv	// argv = ["", 0]
	l32i	ARG1, ARG1, 0		// argc = 1
#endif	// SIMULATOR

	movi	SETARGS_ARGC, 0
	movi	SETARGS_ARGV, 0

	movi	ARG3, _start_envp	// envp

	// Define a main symbol here so that xt-gdb can specify ARGC/ARGV via set-args 
	// NOTE: Could add windowed ABI conditional around SETARGS logic to save 6 instructions
	// (3 executed), but this way allows xt-gdb from either ABI to run set-args correctly.
main:
	beqz	SETARGS_ARGC, .Lcall_unpack
	beqz	SETARGS_ARGV, .Lcall_unpack
	mov		ARG1, SETARGS_ARGC
	mov		ARG2, SETARGS_ARGV

.Lcall_unpack:
	CALL	secmon_unpack

	// Preserve NSM entry point across final MPU init
	// a12-a15 are callee-saved ARs, as secure code is always call0
	mov		a12, a2

	// Secure Boot: after unpack, program nonsecure MPU entries and verify full MPU
	movi	a4, secmon_mpu_init_final
	CALLX	a4

	// Secure Boot (steps #8, #9): Wipe state; exit secure mode and jump to nonsecure entry point.
	// Handled in exit-sm.S
	
	mov		ARG1, a12
	CALL	secmon_exit			// exit with secmon_unpack()'s return value (nonsecure entry point)

	// Does not return here.

	.size	_start, . - _start

