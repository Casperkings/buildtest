//===-- XtensaFrameLowering.h - Frame info for Xtensa Target --------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains Xtensa frame information that doesn't fit anywhere else
// cleanly...
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_XTENSA_XTENSAFRAMELOWERING_H
#define LLVM_LIB_TARGET_XTENSA_XTENSAFRAMELOWERING_H

#include "llvm/CodeGen/TargetFrameLowering.h"
#include "llvm/Target/TargetMachine.h"

#include "XtensaMachineFunctionInfo.h"

namespace llvm {
class XtensaSubtarget;

class XtensaFrameLowering : public TargetFrameLowering {
public:
  XtensaFrameLowering(const XtensaSubtarget &STI);

  /// emitProlog/emitEpilog - These methods insert prolog and epilog code into
  /// the function.
  void emitPrologueWindowed(MachineFunction &MF, MachineBasicBlock &MBB) const;
  void adjustStack(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
                   int64_t Amount, MachineInstr::MIFlag Flag) const;
  unsigned genNewBase(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
                      unsigned DstReg, unsigned SrcReg, unsigned TempReg,
                      unsigned Offset, MachineInstr::MIFlag Flag) const;
  void emitPrologueCall0(MachineFunction &MF, MachineBasicBlock &MBB) const;
  void emitPrologue(MachineFunction &MF, MachineBasicBlock &MBB) const override;
  void emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const override;

  bool spillCalleeSavedRegisters(MachineBasicBlock &MBB,
                                 MachineBasicBlock::iterator MI,
                                 const std::vector<CalleeSavedInfo> &CSI,
                                 const TargetRegisterInfo *TRI) const override;
  bool
  restoreCalleeSavedRegisters(MachineBasicBlock &MBB,
                              MachineBasicBlock::iterator MI,
                              std::vector<CalleeSavedInfo> &CSI,
                              const TargetRegisterInfo *TRI) const override;

  MachineBasicBlock::iterator
  eliminateCallFramePseudoInstr(MachineFunction &MF, MachineBasicBlock &MBB,
                                MachineBasicBlock::iterator I) const override;

  bool hasFP(const MachineFunction &MF) const override;

  void determineCalleeSaves(MachineFunction &MF, BitVector &SavedRegs,
                            RegScavenger *RS = nullptr) const override;

  bool
  assignCalleeSavedSpillSlots(MachineFunction &F, const TargetRegisterInfo *TRI,
                              std::vector<CalleeSavedInfo> &CSI) const override;

  void processFunctionBeforeFrameFinalized(
      MachineFunction &MF, RegScavenger *RS = nullptr) const override;

  //! Stack slot size (4 bytes)
  static int stackSlotSize() { return 4; }

  /// Order the symbols in the local stack.
  /// Smaller objects at lower addresses, and sort objects of the same size by
  /// static frequency
  void
  orderFrameObjects(const MachineFunction &MF,
                    SmallVectorImpl<int> &ObjectsToAllocate) const override;
};
}

#endif // LLVM_LIB_TARGET_XTENSA_XTENSAFRAMELOWERING_H
