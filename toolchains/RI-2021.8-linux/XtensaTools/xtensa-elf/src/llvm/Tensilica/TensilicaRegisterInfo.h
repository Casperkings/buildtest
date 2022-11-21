//===-- Tensilica/TensilicaRegisterInfo.h - Tensilica Register Information ===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the Tensilica common part of implementation of the
// TargetRegisterInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_CODEGEN_TENSILICA_TENSILICAREGISTERINFO_H
#define LLVM_LIB_CODEGEN_TENSILICA_TENSILICAREGISTERINFO_H

#include "llvm/CodeGen/TargetRegisterInfo.h"
#include <set>

namespace llvm {

namespace Tensilica {

class RegisterInfo : public TargetRegisterInfo {
public:
  template <typename... Args>
  RegisterInfo(Args... args) : TargetRegisterInfo(args...) {}

  /// Configuration-specific fields, initialized immediately after construction.
  const TargetRegisterClass *PtrRegClass;
  const std::set<int> *SharedOrSet;

  const TargetRegisterClass *getPointerRegClass() const { return PtrRegClass; }

  bool isSharedOr(const TargetRegisterClass *RC) const {
    return SharedOrSet->count(RC->getID());
  }

  virtual llvm::Register getStackPointerRegister() const = 0;

  // If possible, find a subreg index of the given register class, which allows
  // to get its subregister with specified size and offset; otherwise return 0.
  unsigned findSubRegIdx(const TargetRegisterClass *RC, unsigned Offset,
                         unsigned Size) const;

  virtual unsigned getNextPhysReg(unsigned PReg, unsigned &Align,
                                  MachineFunction &MF) const = 0;

  virtual void eliminateFrameIndex(MachineBasicBlock::instr_iterator II,
                                   int SPAdj, unsigned FIOperandNum,
                                   RegScavenger *RS = nullptr) const = 0;
};
} // namespace Tensilica
} // namespace llvm

#endif // LLVM_LIB_CODEGEN_TENSILICA_TENSILICAREGISTERINFO_H
