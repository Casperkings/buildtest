//===-- XtensaConfigTargetLowering.h - Xtensa Target Lowering Config Info -===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares a helper class for XtensaTargetLowering
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_XTENSA_XTENSACONFIG_XTENSACONFIGTARGETLOWERING_H
#define LLVM_LIB_TARGET_XTENSA_XTENSACONFIG_XTENSACONFIGTARGETLOWERING_H

#include "XtensaCallingConv.h"
#include "Tensilica/TensilicaOperationISelLower.h"
#include "llvm/ADT/StringMap.h"
#include <vector>

namespace llvm {
// This structure contains target config-specific information that is not
// passed through tablegen, instead it is generated directly by
// llvm-xtensa-config-gen.
class XtensaConfigTargetLowering {
private:
  bool UnsafeFPMath;

public:
  ArrayRef<Tensilica::OpDesc> XtensaOps;
  ArrayRef<llvm::MVT> VshuffleMvts;
  ArrayRef<llvm::MVT> SelectMvts;
  ArrayRef<llvm::MVT> UnalignedMvts;
  ArrayRef<Tensilica::TypeDesc> ConsecutiveTypes;
  StringMap<unsigned> Names2Regs;

  // Calling conventions
  llvm::CCAssignFn *CC_Xtensa;
  llvm::CCAssignFn *CC_Xtensa_OldFPABI;
  llvm::CCAssignFn *CC_Xtensa_VarArg;
  llvm::CCAssignFn *CC_Xtensa_VecVarArg;
  llvm::CCAssignFn *RetCC_Xtensa;
  llvm::CCAssignFn *RetCC_Xtensa_OldFPABI;

  bool isUnsafeFPMath() const { return UnsafeFPMath; }
  // Constructors
  XtensaConfigTargetLowering(bool UnsafeFPMath, bool CoProc, bool UseMul16);
  XtensaConfigTargetLowering(const XtensaConfigTargetLowering &) = delete;
  XtensaConfigTargetLowering(XtensaConfigTargetLowering &&that) = default;
};
} // namespace llvm
#endif // LLVM_LIB_TARGET_XTENSA_XTENSACONFIG_XTENSACONFIGTARGETLOWERING_H
