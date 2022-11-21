//===- XTENSACallingConv.h - XTENSA Custom Calling Convention Routines ----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the custom routines for the XTENSA Calling Convention that
// aren't done by tablegen.
//
//===----------------------------------------------------------------------===//


#ifndef LLVM_LIB_TARGET_XTENSA_XTENSACALLINGCONV_H
#define LLVM_LIB_TARGET_XTENSA_XTENSACALLINGCONV_H

#include "Xtensa.h"
#include "XtensaInstrInfo.h"
#include "XtensaSubtarget.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/CodeGen/TargetInstrInfo.h"

namespace llvm {
class XtensaCCState : public CCState {
private:
  /// Records whether the value was a fixed argument.
  /// See ISD::OutputArg::IsFixed,
  SmallVector<bool, 4> ArgIsFixed;
  /// Whether to use FP registers to pass arguments
  bool FPABI;

public:
  XtensaCCState(CallingConv::ID CC, bool isVarArg, MachineFunction &MF,
                SmallVectorImpl<CCValAssign> &locs, LLVMContext &C, bool fpabi)
      : CCState(CC, isVarArg, MF, locs, C) {
    FPABI = fpabi;
  }

  void AnalyzeCallOperands(const SmallVectorImpl<ISD::OutputArg> &Outs,
                           CCAssignFn Fn) {
    ArgIsFixed.clear();
    for (unsigned i = 0; i < Outs.size(); ++i)
      ArgIsFixed.push_back(Outs[i].IsFixed);
    CCState::AnalyzeCallOperands(Outs, Fn);
  }

  void AnalyzeFormalArguments(const SmallVectorImpl<ISD::InputArg> &Ins,
                              CCAssignFn Fn) {
    ArgIsFixed.clear();
    for (unsigned i = 0; i < Ins.size(); ++i)
      ArgIsFixed.push_back(true);
    CCState::AnalyzeFormalArguments(Ins, Fn);
  }

  bool IsCallOperandFixed(unsigned ValNo) { return ArgIsFixed[ValNo]; }

  bool IsFPABI() { return FPABI; }
};

} // End llvm namespace

#endif
