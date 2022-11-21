//===-- RISCVInstrInfo.h - RISCV Instruction Information --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the RISCV implementation of the TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_RISCV_RISCVINSTRINFO_H
#define LLVM_LIB_TARGET_RISCV_RISCVINSTRINFO_H

#include "RISCVRegisterInfo.h"
#include "llvm/CodeGen/TargetInstrInfo.h"

#if defined(TENSILICA) || 1
#include "Tensilica/TensilicaInstrInfo.h"
#else
#define GET_INSTRINFO_HEADER
#include "RISCVGenInstrInfo.inc"
#endif

namespace llvm {

class RISCVSubtarget;

#if defined(TENSILICA) || 1
class RISCVInstrInfo final : public Tensilica::InstrInfo {
  const RISCVRegisterInfo &RI;
  const TargetRegisterClass &GPRRegClass;
  // FIXME: The following fields should be removed, once we generate
  // Tensilica::RegTransferInfo for RISCV.
  const TargetRegisterClass &FPR32RegClass;
  const TargetRegisterClass &FPR64RegClass;

private:
  virtual void anchor();
#else
class RISCVInstrInfo : public RISCVGenInstrInfo
#endif

public:
  explicit RISCVInstrInfo(RISCVSubtarget &STI);

#if defined(TENSILICA) || 1
  const RISCVRegisterInfo &getRegisterInfo() const { return RI; }
  unsigned getBranchTargetOpnd(unsigned BrOpcode) const override;

  // Emit code before MI to load immediate value into register Reg.
  // Returns an iterator to the new instruction.
  MachineBasicBlock::iterator
  loadImmediate(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
                Register Reg, int64_t Value,
                MachineInstr::MIFlag Flag = MachineInstr::NoFlags) const final;

  // Emit code before MI to add SrcReg and immediate value and write the result
  // to DestReg.  Returns an iterator to the new instruction.
  MachineBasicBlock::iterator
  addImmediate(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
               Register DestReg, Register SrcReg, int64_t Value,
               RegScavenger *RS = nullptr,
               MachineInstr::MIFlag Flag = MachineInstr::NoFlags) const final;

  // Query if given immediate value will be emitted as a single ADDI instruction
  // by addImmediate.
  bool isImmADDI(int64_t Imm) const final { return isInt<12>(Imm); }
  void expandLOAD_CONST(MachineInstr &MI) const final;

  unsigned
  getInstOperandInputBits(const MachineInstr &MI, unsigned OpNo) const final;

  bool mayCopyProp(const MachineInstr &MI, int64_t &Offset) const final;
  bool evalConst(const MachineInstr &, int64_t &Val, int Depth = 2) const final;
  bool evalConstOp(const MachineOperand &, int64_t &Val,
                   int Depth = 2) const final;
  bool chooseImmADDI(int64_t From, int64_t To, int64_t &Val) const final;

  // Early ifcvt
  bool canInsertSelect(const MachineBasicBlock &, ArrayRef<MachineOperand> Cond,
                       unsigned, unsigned, int &, int &, int &) const final;
  void insertSelect(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
                    const DebugLoc &DL, unsigned DstReg,
                    ArrayRef<MachineOperand> Cond, unsigned TrueReg,
                    unsigned FalseReg) const final;
  
  // Peephole opt
  bool analyzeSelect(const MachineInstr &MI,
                     SmallVectorImpl<MachineOperand> &Cond, unsigned &TrueOp,
                     unsigned &FalseOp, bool &Optimizable) const final;
  MachineInstr *optimizeSelect(MachineInstr &MI,
                               SmallPtrSetImpl<MachineInstr *> &SeenMIs,
                               bool PreferFalse) const final;

  bool optimizeTargetOp(MachineInstr &MI) const final;

  bool expandPostRAPseudo(MachineInstr &MI) const final;

  bool isCondBranch(unsigned BrOpcGeneric) const final;
  bool isLoop(unsigned BrOpcGeneric) const final;
  bool isBrDec(unsigned BrOpcGeneric) const final;
  bool hasEndLoop(const MachineBasicBlock &MBB) const final;

  /// Test if the instruction is like a fence for packetizer.
  bool isPacketizerBoundary(const MachineInstr &MI) const final;

  Register analyzeBranchForBrdec(MachineInstr &MI) const final;
  void convertBranchToBrdec(MachineInstr &MI) const final;
#endif

  unsigned isLoadFromStackSlot(const MachineInstr &MI,
                               int &FrameIndex) const override;
  unsigned isStoreToStackSlot(const MachineInstr &MI,
                              int &FrameIndex) const override;

  void copyPhysReg(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
                   const DebugLoc &DL, MCRegister DstReg, MCRegister SrcReg,
                   bool KillSrc) const override;

  void storeRegToStackSlot(MachineBasicBlock &MBB,
                           MachineBasicBlock::iterator MBBI, unsigned SrcReg,
                           bool IsKill, int FrameIndex,
                           const TargetRegisterClass *RC,
                           const TargetRegisterInfo *TRI) const override;

  void loadRegFromStackSlot(MachineBasicBlock &MBB,
                            MachineBasicBlock::iterator MBBI, unsigned DstReg,
                            int FrameIndex, const TargetRegisterClass *RC,
                            const TargetRegisterInfo *TRI) const override;

  // Materializes the given integer Val into DstReg.
  void movImm(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
              const DebugLoc &DL, Register DstReg, uint64_t Val,
              MachineInstr::MIFlag Flag = MachineInstr::NoFlags) const;

  unsigned getInstSizeInBytes(const MachineInstr &MI) const override;

  bool analyzeBranch(MachineBasicBlock &MBB, MachineBasicBlock *&TBB,
                     MachineBasicBlock *&FBB,
                     SmallVectorImpl<MachineOperand> &Cond,
                     bool AllowModify) const override;

  unsigned insertBranch(MachineBasicBlock &MBB, MachineBasicBlock *TBB,
                        MachineBasicBlock *FBB, ArrayRef<MachineOperand> Cond,
                        const DebugLoc &dl,
                        int *BytesAdded = nullptr) const override;

  unsigned insertIndirectBranch(MachineBasicBlock &MBB,
                                MachineBasicBlock &NewDestBB,
                                const DebugLoc &DL, int64_t BrOffset,
                                RegScavenger *RS = nullptr) const override;

  unsigned removeBranch(MachineBasicBlock &MBB,
                        int *BytesRemoved = nullptr) const override;

  bool
  reverseBranchCondition(SmallVectorImpl<MachineOperand> &Cond) const override;

  MachineBasicBlock *getBranchDestBlock(const MachineInstr &MI) const override;

  bool isBranchOffsetInRange(unsigned BranchOpc,
                             int64_t BrOffset) const override;

  bool isAsCheapAsAMove(const MachineInstr &MI) const override;

  bool verifyInstruction(const MachineInstr &MI,
                         StringRef &ErrInfo) const override;

  bool getMemOperandWithOffsetWidth(const MachineInstr &LdSt,
                                    const MachineOperand *&BaseOp,
                                    int64_t &Offset, unsigned &Width,
                                    const TargetRegisterInfo *TRI) const;

  bool areMemAccessesTriviallyDisjoint(const MachineInstr &MIa,
                                       const MachineInstr &MIb) const override;


  std::pair<unsigned, unsigned>
  decomposeMachineOperandsTargetFlags(unsigned TF) const override;

  ArrayRef<std::pair<unsigned, const char *>>
  getSerializableDirectMachineOperandTargetFlags() const override;

  // Return true if the function can safely be outlined from.
  virtual bool
  isFunctionSafeToOutlineFrom(MachineFunction &MF,
                              bool OutlineFromLinkOnceODRs) const override;

  // Return true if MBB is safe to outline from, and return any target-specific
  // information in Flags.
  virtual bool isMBBSafeToOutlineFrom(MachineBasicBlock &MBB,
                                      unsigned &Flags) const override;

  // Calculate target-specific information for a set of outlining candidates.
  outliner::OutlinedFunction getOutliningCandidateInfo(
      std::vector<outliner::Candidate> &RepeatedSequenceLocs) const override;

  // Return if/how a given MachineInstr should be outlined.
  virtual outliner::InstrType
  getOutliningType(MachineBasicBlock::iterator &MBBI,
                   unsigned Flags) const override;

  // Insert a custom frame for outlined functions.
  virtual void
  buildOutlinedFrame(MachineBasicBlock &MBB, MachineFunction &MF,
                     const outliner::OutlinedFunction &OF) const override;

  // Insert a call to an outlined function into a given basic block.
  virtual MachineBasicBlock::iterator
  insertOutlinedCall(Module &M, MachineBasicBlock &MBB,
                     MachineBasicBlock::iterator &It, MachineFunction &MF,
                     const outliner::Candidate &C) const override;

  void buildBEQZ(MachineBasicBlock &MBB, MachineBasicBlock::iterator I,
                 const DebugLoc &DL, unsigned Reg, 
                 MachineBasicBlock *TargetMBB, bool KillSrc = false)
                 const override;
  void buildBLEZ(MachineBasicBlock &MBB, MachineBasicBlock::iterator I,
                 const DebugLoc &DL, unsigned Reg, 
                 MachineBasicBlock *TargetMBB, bool KillSrc = false)
                 const override;
  void buildBBCI(MachineBasicBlock &MBB, MachineBasicBlock::iterator I,
                 const DebugLoc &DL, unsigned Reg, int64_t Imm,
                 MachineBasicBlock *TargetMBB, bool KillSrc = false)
                 const override;
  void buildEXTUI(MachineBasicBlock &MBB, MachineBasicBlock::iterator I,
                  const DebugLoc &DL, unsigned DestReg, unsigned SrcReg,
                  int64_t Imm0, int64_t Imm1, bool KillSrc = false) const
                  override;
  void buildSRLI(MachineBasicBlock &MBB, MachineBasicBlock::iterator I,
                 const DebugLoc &DL, unsigned DestReg, unsigned SrcReg,
                 int64_t Imm, bool KillSrc ) const override;
  Tensilica::ConfigImap *getConfigImap() const override;
  Tensilica::ConfigRewrite *getConfigRewrite() const override;

protected:
  const RISCVSubtarget &STI;
};
}
#endif
