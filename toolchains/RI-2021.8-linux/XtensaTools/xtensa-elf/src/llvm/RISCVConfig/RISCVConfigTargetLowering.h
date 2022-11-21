//===-- RISCVConfigTargetLowering.h - RISCV Target Lowering Config Info -===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares a helper class for RISCVTargetLowering
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_RISCV_RISCVCONFIG_RISCVCONFIGTARGETLOWERING_H
#define LLVM_LIB_TARGET_RISCV_RISCVCONFIG_RISCVCONFIGTARGETLOWERING_H

#include "Tensilica/TensilicaOperationISelLower.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/ADT/StringMap.h"
#include <vector>


namespace llvm {
// This structure contains target config-specific information that is not
// passed through tablegen, instead it is generated directly by
// llvm-xtensa-config-gen.
class RISCVConfigTargetLowering {
private:
  bool UnsafeFPMath;

public:
  ArrayRef<Tensilica::OpDesc> RISCVOps;
  ArrayRef<llvm::MVT> VshuffleMvts;
  ArrayRef<llvm::MVT> SelectMvts;
  ArrayRef<llvm::MVT> UnalignedMvts;
  ArrayRef<Tensilica::TypeDesc> ConsecutiveTypes;
  StringMap<unsigned> Names2Regs;

  // Calling conventions
  llvm::CCAssignFn *CC_RISCV;
  llvm::CCAssignFn *RetCC_RISCV;

  bool isUnsafeFPMath() const { return UnsafeFPMath; }
  // Constructors
  RISCVConfigTargetLowering(bool UnsafeFPMath, bool CoProc, bool UseMul16);
  RISCVConfigTargetLowering(const RISCVConfigTargetLowering &) = delete;
  RISCVConfigTargetLowering(RISCVConfigTargetLowering &&that) = default;
};
} // namespace llvm
#endif // LLVM_LIB_TARGET_RISCV_RISCVCONFIG_RISCVCONFIGTARGETLOWERING_H
