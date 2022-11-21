//===- XtensaCallingConv.td - Calling Conventions for Xtensa - tablegen ---===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// This describes the calling conventions for Xtensa architecture.
//===----------------------------------------------------------------------===//

/// CCIfAlign - Match of the original alignment of the arg
class CCIfAlign<string Align, CCAction A>:
  CCIf<!strconcat("ArgFlags.getOrigAlign() == ", Align), A>;

/// Match if this specific argument is a vararg.
/// This is slightly different fro CCIfIsVarArg which matches if any argument is
/// a vararg.
class CCIfArgIsVarArg<CCAction A>
    : CCIf<"!static_cast<XtensaCCState *>(&State)->IsCallOperandFixed(ValNo)", A>;

/// Match if old FPABI used
class CCIfOldFPABI<CCAction A>
    : CCIf<"!static_cast<XtensaCCState *>(&State)->IsFPABI()", A>;

//===----------------------------------------------------------------------===//
// Xtensa Return Old FP ABI Calling Convention
//===----------------------------------------------------------------------===//
def RetCC_Xtensa_OldFPABI : CallingConv<[
  // Double precision arguments go to a2, a4, a6, or the stack.
  CCIfType<[f64], CCCustom<"RetCC_Xtensa_ARF64">>,
  CCIfType<[f64], CCAssignToStackWithShadow<8, 8, [a2, a3, a4, a5]>>,

  CCIfType<[f32], CCAssignToReg<[a2, a3, a4, a5]>>,
  CCIfType<[f32], CCAssignToStack<4, 4>>
]>;

//===----------------------------------------------------------------------===//
// Xtensa Return Value Calling Convention
//===----------------------------------------------------------------------===//
def RetCC_Xtensa : CallingConv<[
  CCIfOldFPABI<CCDelegateTo<RetCC_Xtensa_OldFPABI>>,

  // i32 are returned in registers a0, a1, a2, a3
  // CCIfType<[i32], CCAssignToReg<[a0, a1, a2, a3]>>,
  CCIfType<[i32], CCAssignToReg<[a2, a3, a4, a5]>>,

  // Integer values get stored in stack slots that are 4 bytes in
  // size and 4-byte aligned.
  CCIfType<[i32], CCAssignToStack<4, 4>>

include "TensilicaCustomCallingConvRet.td"
]>;

//===----------------------------------------------------------------------===//
// Xtensa Varargs Calling Convention
//===----------------------------------------------------------------------===//
def CC_Xtensa_VarArg : CallingConv<[
  // Double precision arguments go to a2, a4, a6, or the stack.
  CCIfType<[f64], CCCustom<"CC_Xtensa_ARF64">>,
  CCIfType<[f64], CCAssignToStackWithShadow<8, 8, [a2, a3, a4, a5, a6, a7]>>,

  CCIfType<[f32], CCAssignToReg<[a2, a3, a4, a5, a6, a7]>>,
  CCIfType<[f32], CCAssignToStack<4, 4>>
]>;

//===----------------------------------------------------------------------===//
// Xtensa Old FP ABI Calling Convention
//===----------------------------------------------------------------------===//
def CC_Xtensa_OldFPABI : CallingConv<[
  // Double precision arguments go to a2, a4, a6, or the stack.
  CCIfType<[f64], CCCustom<"CC_Xtensa_ARF64">>,
  CCIfType<[f64], CCAssignToStackWithShadow<8, 8, [a2, a3, a4, a5, a6, a7]>>,

  CCIfType<[f32], CCAssignToReg<[a2, a3, a4, a5, a6, a7]>>,
  CCIfType<[f32], CCAssignToStack<4, 4>>
]>;

def CC_Xtensa_VecVarArg : CallingConv<[
  // Dummy.
  CCIf<"false", CCAssignToStack<4, 4>>

include "TensilicaCustomCallingConvVararg.td"
]>;

//===----------------------------------------------------------------------===//
// Xtensa Argument Calling Conventions 
//===----------------------------------------------------------------------===//
def CC_Xtensa : CallingConv<[
  // Promote i8/i16 arguments to i32.
  CCIfType<[i8, i16], CCPromoteToType<i32>>,

  // The 'nest' parameter, if any, is passed in a11.
  CCIfNest<CCAssignToReg<[a11]>>,

  // Handle varargs of vector and TIE types before handling aggregate
  // arguments.  This way, a wide vector type like v32f16 will always
  // be passed in memory.
  CCIfVarArg<CCIfArgIsVarArg<CCDelegateTo<CC_Xtensa_VecVarArg>>>,

  // In old FP ABI float and double arguments go to AR
  CCIfOldFPABI<CCDelegateTo<CC_Xtensa_OldFPABI>>,

  // Handle aggregate arguments, which require consecutive registers.
  CCIfConsecutiveRegs<CCCustom<"CC_Xtensa_Custom_Aggregate">>,

  // Arguments with 8-byte alignment go to a2, a4, a6, or the stack.
  CCIfType<[i32], CCIfAlign<"8", CCAssignToRegWithShadow<[a2, a4, a6], [a2, a3, a5]>>>,
  CCIfType<[i32], CCIfAlign<"8", CCAssignToStackWithShadow<4, 8, [a2, a3, a4, a5, a6, a7]>>>,

  // The first 6 integer arguments are passed in integer registers.
  CCIfType<[i32], CCAssignToReg<[a2, a3, a4, a5, a6, a7]>>,

  // Integer values get stored in stack slots that are 4 bytes in
  // size and 4-byte aligned.
  CCIfType<[i32], CCAssignToStack<4, 4>>,

  // Calling convention for varargs
  CCIfVarArg<CCDelegateTo<CC_Xtensa_VarArg>>

include "TensilicaCustomCallingConvCall.td"
]>;
