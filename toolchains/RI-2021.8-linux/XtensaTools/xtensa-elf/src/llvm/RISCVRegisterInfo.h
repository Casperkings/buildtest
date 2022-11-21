//===-- RISCVRegisterInfo.h - RISCV Register Information Impl ---*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the RISCV implementation of the TargetRegisterInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_RISCV_RISCVREGISTERINFO_H
#define LLVM_LIB_TARGET_RISCV_RISCVREGISTERINFO_H

#include "llvm/CodeGen/TargetRegisterInfo.h"

#if defined(TENSILICA) || 1
#include "Tensilica/TensilicaRegisterInfo.h"
#else
#define GET_REGINFO_HEADER
#include "RISCVGenRegisterInfo.inc"
#endif

namespace llvm {

#if defined(TENSILICA) || 1
struct RISCVRegisterInfo : public Tensilica::RegisterInfo {
  template <typename... Args>
  RISCVRegisterInfo(Args... args) : Tensilica::RegisterInfo(args...) {}

  /// Configuration-specific fields, initialized immediately after construction.
  const MCPhysReg *CSR_ILP32D_LP64D_SaveList;
  const uint32_t *CSR_ILP32D_LP64D_RegMask;
  const MCPhysReg *CSR_ILP32F_LP64F_SaveList;
  const uint32_t *CSR_ILP32F_LP64F_RegMask;
  const MCPhysReg *CSR_ILP32_LP64_SaveList;
  const uint32_t *CSR_ILP32_LP64_RegMask;
  const MCPhysReg *CSR_Interrupt_SaveList;
  const uint32_t *CSR_Interrupt_RegMask;
  const MCPhysReg *CSR_NoRegs_SaveList;
  const uint32_t *CSR_NoRegs_RegMask;
  const MCPhysReg *CSR_XLEN_F32_Interrupt_SaveList;
  const uint32_t *CSR_XLEN_F32_Interrupt_RegMask;
  const MCPhysReg *CSR_XLEN_F64_Interrupt_SaveList;
  const uint32_t *CSR_XLEN_F64_Interrupt_RegMask;
  // FIXME: Remove the FP regclasses.
  const TargetRegisterClass *FPR32RegClass;
  const TargetRegisterClass *FPR64RegClass;

#else
struct RISCVRegisterInfo : public RISCVGenRegisterInfo

  RISCVRegisterInfo(unsigned HwMode);
#endif

  const uint32_t *getCallPreservedMask(const MachineFunction &MF,
                                       CallingConv::ID) const override;

  const MCPhysReg *getCalleeSavedRegs(const MachineFunction *MF) const override;

  BitVector getReservedRegs(const MachineFunction &MF) const override;
  bool isAsmClobberable(const MachineFunction &MF,
                        unsigned PhysReg) const override;

  bool isConstantPhysReg(unsigned PhysReg) const override;

  const uint32_t *getNoPreservedMask() const override;

  void eliminateFrameIndex(MachineBasicBlock::iterator MI, int SPAdj,
                           unsigned FIOperandNum,
                           RegScavenger *RS = nullptr) const override;

  Register getFrameRegister(const MachineFunction &MF) const override;

  bool requiresRegisterScavenging(const MachineFunction &MF) const override {
    return true;
  }

  bool requiresFrameIndexScavenging(const MachineFunction &MF) const override {
    return true;
  }

  bool trackLivenessAfterRegAlloc(const MachineFunction &) const override {
    return true;
  }

#if defined(TENSILICA) || 1
  using Tensilica::RegisterInfo::getPointerRegClass;

  // FIXME: The following two should be removed with adding config-gen.
  const TargetRegisterClass *getFPR32RegClass() const { return FPR32RegClass; }
  const TargetRegisterClass *getFPR64RegClass() const { return FPR64RegClass; }

  const TargetRegisterClass *
  getPointerRegClass(const MachineFunction &,
                     unsigned /*Kind*/) const override {
    return getPointerRegClass();
  }

  llvm::Register getStackPointerRegister() const final;
 
  unsigned getNextPhysReg(unsigned PReg, unsigned &Align,
                          MachineFunction &MF) const final;

  void materializeFrameBaseRegister(MachineBasicBlock *MBB, unsigned BaseReg,
                                    int FrameIdx, int64_t Offset) const final;
  
  void eliminateFrameIndex(MachineBasicBlock::instr_iterator II, int SPAdj,
                           unsigned FIOperandNum,
                           RegScavenger *RS = nullptr) const final;

  bool requiresVirtualBaseRegisters(const MachineFunction &MF) const override {
    return true;
  }

#else
  const TargetRegisterClass *
  getPointerRegClass(const MachineFunction &MF,
                     unsigned Kind = 0) const override {
    return &RISCV::GPRRegClass;
  }
#endif
};
}

#endif
