//===-- XtensaRegisterInfo.h - Xtensa Register Information Impl -*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the Xtensa implementation of the TargetRegisterInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_XTENSA_XTENSAREGISTERINFO_H
#define LLVM_LIB_TARGET_XTENSA_XTENSAREGISTERINFO_H

#include "Tensilica/TensilicaRegisterInfo.h"

namespace llvm {

class TargetInstrInfo;

class XtensaRegisterInfo : public Tensilica::RegisterInfo {
public:
  template <typename... Args>
  XtensaRegisterInfo(Args... args) : Tensilica::RegisterInfo(args...) {}

  /// Configuration-specific fields, initialized immediately after construction.
  const MCPhysReg *CALL0_CSRs_SaveList;
  const uint32_t *CALL0_CSRs_RegMask;
  const MCPhysReg *CALL8_CSRs_SaveList;
  const uint32_t *CALL8_CSRs_RegMask;

  /// Code Generation virtual methods...
  const MCPhysReg *
  getCalleeSavedRegs(const MachineFunction *MF = nullptr) const override;

  const uint32_t *getCallPreservedMask(const MachineFunction &MF,
                                       CallingConv::ID CC) const override;

  BitVector getReservedRegs(const MachineFunction &MF) const override;

  bool requiresRegisterScavenging(const MachineFunction &MF) const override;

  bool trackLivenessAfterRegAlloc(const MachineFunction &MF) const override;

  bool useFPForScavengingIndex(const MachineFunction &MF) const override;

  void eliminateFrameIndex(MachineBasicBlock::iterator II, int SPAdj,
                           unsigned FIOperandNum,
                           RegScavenger *RS = nullptr) const override;

  void eliminateFrameIndex(MachineBasicBlock::instr_iterator II, int SPAdj,
                           unsigned FIOperandNum,
                           RegScavenger *RS = nullptr) const override;

  Register getStackPointerRegister() const override;

  // Debug information queries.
  Register getFrameRegister(const MachineFunction &MF) const override;

  // Return whether to emit frame moves.
  static bool needsFrameMoves(const MachineFunction &MF);

  // Register pressure limit
  unsigned getRegPressureLimit(const TargetRegisterClass *RC,
                               MachineFunction &MF) const override;

  bool requiresVirtualBaseRegisters(const MachineFunction &MF) const override {
    return true;
  }

  bool needsFrameBaseReg(MachineInstr *MI, int64_t Offset) const override;

  void materializeFrameBaseRegister(MachineBasicBlock *MBB, unsigned BaseReg,
                                    int FrameIdx,
                                    int64_t Offset) const override;

  bool isFrameOffsetLegal(const MachineInstr *MI, unsigned BaseReg,
                          int64_t Offset) const override;

  int64_t getFrameIndexInstrOffset(const MachineInstr *MI,
                                   int Idx) const override;

  using Tensilica::RegisterInfo::getPointerRegClass;

  const TargetRegisterClass *
  getPointerRegClass(const MachineFunction &,
                     unsigned /*Kind*/) const override {
    return getPointerRegClass();
  }

  void resolveFrameIndex(MachineInstr &MI, unsigned BaseReg,
                         int64_t Offset) const override;

  bool LookForFoldingCandidates(SmallVectorImpl<MachineInstr *> &MemOps,
                                const MachineBasicBlock::instr_iterator II,
                                unsigned Reg) const;

  const TargetRegisterClass *
  getLargestLegalSuperClass(const TargetRegisterClass *RC,
                            const MachineFunction &MF) const override;
  unsigned getRegPressureSetScore(const MachineFunction &MF,
                                  unsigned PSetID) const override;
  unsigned getDwarfRegForOldSp() const override { return 16; }
  unsigned getNextPhysReg(unsigned PReg, unsigned &Align,
                          MachineFunction &MF) const final;
};

} // end namespace llvm

#endif
