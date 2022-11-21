//===-- XtensaInstrInfo.h - Xtensa Instruction Information ----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the Xtensa implementation of the TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_XTENSA_XTENSAINSTRINFO_H
#define LLVM_LIB_TARGET_XTENSA_XTENSAINSTRINFO_H

#include "XtensaRegisterInfo.h"
#include "Tensilica/TensilicaInstrInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/RegisterScavenging.h"

namespace llvm {

class XtensaSubtarget;

class XtensaInstrInfo final : public Tensilica::InstrInfo {
  const XtensaRegisterInfo &RI;
  const XtensaSubtarget &Subtarget;
  const TargetRegisterClass *ARRegClass;

private:
  virtual void anchor();

public:
  explicit XtensaInstrInfo(const XtensaSubtarget &STI);

  /// getRegisterInfo - TargetInstrInfo is a superset of MRegister info.  As
  /// such, whenever a client has an instance of instruction info, it should
  /// always be able to get register info as well (through this method).
  ///
  const XtensaRegisterInfo &getRegisterInfo() const { return RI; }

  std::pair<unsigned, unsigned>
  decomposeMachineOperandsTargetFlags(unsigned TF) const override;

  ArrayRef<std::pair<unsigned, const char *>>
  getSerializableDirectMachineOperandTargetFlags() const override;

  /// MachineCombiner support.
  bool useMachineCombiner() const override { return true; }

  /// Return true when there is potentially a faster code sequence
  /// for an instruction chain ending in <Root>. All potential patterns are
  /// listed in the <Patterns> array.
  bool getMachineCombinerPatterns(
      MachineInstr &Root,
      SmallVectorImpl<MachineCombinerPattern> &P) const override;

  /// Return true when a code sequence can improve throughput. It
  /// should be called only for instructions in loops.
  /// \param Pattern - combiner pattern.
  bool isThroughputPattern(MachineCombinerPattern Pattern) const override;

  /// When getMachineCombinerPatterns() finds patterns, this function generates
  /// the instructions that could replace the original code sequence.
  void genAlternativeCodeSequence(
      MachineInstr &Root, MachineCombinerPattern Pattern,
      SmallVectorImpl<MachineInstr *> &InsInstrs,
      SmallVectorImpl<MachineInstr *> &DelInstrs,
      DenseMap<unsigned, unsigned> &InstrIdxForVirtReg) const override;

  /// Specifies instructions for reassociation.
  bool isAssociativeAndCommutative(const MachineInstr &Inst) const override;

  // Xtensa special reassociation patterns support (XTENSA_REASSOC_*).

  /// Specifies special instructions for reassociation.
  bool isSpecialAssociativeAndCommutative(const MachineInstr &Inst,
                                          unsigned &AssocOpcode) const;

  // IdxOp0 is result, IdxOp1 and IdxOp2 are associative and commutative.
  using SpecialOps =
      std::tuple<unsigned /*IdxOp0*/, unsigned /*IdxOp1*/, unsigned /*IdxOp2*/>;
  SpecialOps getSpecialReassociableOperands(const MachineInstr &Inst) const;
  bool hasSpecialReassociableOperands(const MachineInstr &Inst,
                                      const MachineBasicBlock *MBB,
                                      bool SingleUse) const;
  bool hasSpecialReassociableSibling(const MachineInstr &Inst,
                                     unsigned AssocOpcode,
                                     bool &Commuted) const;
  /// Check if there is a special reassociation pattern.
  bool isSpecialReassociationCandidate(const MachineInstr &Inst,
                                       bool &Commuted) const;

  void
  reassociateSpecial(MachineInstr &Root, MachineInstr &Prev,
                     MachineCombinerPattern Pattern,
                     SmallVectorImpl<MachineInstr *> &InsInstrs,
                     SmallVectorImpl<MachineInstr *> &DelInstrs,
                     DenseMap<unsigned, unsigned> &InstrIdxForVirtReg) const;

  void ExtractFromCondBranch(MachineInstr *CondBranch, MachineBasicBlock *&TBB,
                             SmallVectorImpl<MachineOperand> &Cond) const;

  void BuildCondBranchFromCond(MachineBasicBlock &MBB, MachineBasicBlock *TBB,
                               ArrayRef<MachineOperand> Cond,
                               DebugLoc DL) const;

  bool analyzeBranch(MachineBasicBlock &MBB, MachineBasicBlock *&TBB,
                     MachineBasicBlock *&FBB,
                     SmallVectorImpl<MachineOperand> &Cond,
                     bool AllowModify) const override;

  bool analyzeSelect(const MachineInstr &MI,
                     SmallVectorImpl<MachineOperand> &Cond, unsigned &TrueOp,
                     unsigned &FalseOp, bool &Optimizable) const override;

  MachineInstr *optimizeSelect(MachineInstr &MI,
                               SmallPtrSetImpl<MachineInstr *> &SeenMIs,
                               bool) const override;

  bool optimizeTargetOp(MachineInstr &MI) const override;

  bool optimizeMoveBoolRegister(MachineInstr &MI) const override;

  bool optimizeCondBranch(MachineInstr &MI) const override;

  unsigned insertBranch(MachineBasicBlock &MBB, MachineBasicBlock *TBB,
                        MachineBasicBlock *FBB, ArrayRef<MachineOperand> Cond,
                        const DebugLoc &DL,
                        int *BytesAdded = nullptr) const override;

  unsigned removeBranch(MachineBasicBlock &MBB,
                        int *BytesRemoved = nullptr) const override;

  bool canInsertSelect(const MachineBasicBlock &, ArrayRef<MachineOperand> Cond,
                       unsigned, unsigned, int &, int &, int &) const override;
  void insertSelect(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
                    const DebugLoc &DL, unsigned DstReg,
                    ArrayRef<MachineOperand> Cond, unsigned TrueReg,
                    unsigned FalseReg) const override;

  void copyPhysReg(MachineBasicBlock &MBB, MachineBasicBlock::iterator I,
                   const DebugLoc &DL, MCRegister DestReg, MCRegister SrcReg,
                   bool KillSrc) const final;

  /// Test if the instruction is like a fence for packetizer.
  bool isPacketizerBoundary(const MachineInstr &MI) const final;

  bool
  reverseBranchCondition(SmallVectorImpl<MachineOperand> &Cond) const override;

  MachineBasicBlock::iterator loadGlobalAddress(MachineBasicBlock &MBB,
                                                MachineBasicBlock::iterator MI,
                                                unsigned Reg,
                                                const MachineOperand &MO) const;

  // Emit code before MI to load immediate value into Reg. Returns an iterator
  // to the new instruction.
  MachineBasicBlock::iterator
  loadImmediate(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
                Register Reg, int64_t Value,
                MachineInstr::MIFlag Flag = MachineInstr::NoFlags) const final;

  // Emit code before MI to add SrcReg and immediate value and write the result
  // to DestReg. Returns an iterator to the new instruction.
  MachineBasicBlock::iterator
  addImmediate(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
               Register DestReg, Register SrcReg, int64_t Value,
               RegScavenger *RS = nullptr,
               MachineInstr::MIFlag Flag = MachineInstr::NoFlags) const final;

  // Query if given immediate value will be emitted as a single ADDI instruction
  // by addImmediate.
  bool isImmADDI(int64_t Imm) const final;

  unsigned
  getInstOperandInputBits(const MachineInstr &MI, unsigned OpNo) const final;

  unsigned getMachineCSELookAheadLimit() const override { return 50; }

  unsigned GetOppositeBranchOpcode(unsigned opcGeneric) const;
  unsigned getBranchTargetOpnd(unsigned BrOpcode) const override;

  bool isCondBranch(unsigned BrOpcGeneric) const override;
  bool isLoop(unsigned BrOpcGeneric) const override;
  bool isBrDec(unsigned BrOpcGeneric) const override;
  bool isBBCIOrBBSI(unsigned Opcode) const;
  bool isPseudoInstr(const MachineInstr *I) const override;
  void analyzeBRReg(unsigned Reg, unsigned &Offset, unsigned &Size) const;
  void expandMOVARBR(MachineInstr &MI) const;
  void expandMOVBRAR(MachineInstr &MI) const;
  void expandLOAD_CONST(MachineInstr &MI) const final;
  void expandFRAMEADDR(MachineInstr &MI) const;
  bool expandPostRAPseudo(MachineInstr &MI) const override;
  bool hasEndLoop(const MachineBasicBlock &MBB) const override;
  bool isBPredict(const MachineFunction &MF) const override;
  bool isUnsafeToSpeculate(unsigned Opcode) const override;
  bool mayCopyProp(const MachineInstr &MI, int64_t &Offset) const final;
  bool evalConst(const MachineInstr &, int64_t &Val, int Depth) const final;
  bool chooseImmADDI(int64_t From, int64_t To, int64_t &Val) const final;
  bool isMOVI(const MachineInstr &MI, unsigned &IdxImm) const override;
  int isHalfConst(const MachineInstr &) const override;
  bool isMovArBr(const MachineInstr &) const override;
  bool isMovBrAr(const MachineInstr &) const override;
  bool isEntry(const MachineInstr &) const override;
  Register analyzeBranchForBrdec(MachineInstr &MI) const override;
  void convertBranchToBrdec(MachineInstr &MI) const override;
  bool promoteInstruction(MachineInstr &MI) const override;
  bool isReallyTriviallyReMaterializable(const MachineInstr &MI,
                                         AAResults *AA) const override;

  bool isLoopInfinite(MachineLoop *Loop) const override;
  bool getIncrementValue(const MachineInstr *MI, int &D) const;

  enum MulAlgArg { ORIGINAL = 0, DELEGATE = 32 };
  struct MulAlg {
    unsigned Opcode;
    unsigned Arg0;
    unsigned Arg1;
  };

  void synthMulImpl(int32_t m, SmallVectorImpl<MulAlg> &BestSeq,
                    SmallVectorImpl<MulAlg> &CurrentSeq) const;

  // Returns true if we find a multiplication sequence.
  bool synthMul(int32_t m, SmallVectorImpl<MulAlg> &BestSeq) const;
  void buildBEQZ(MachineBasicBlock &MBB, MachineBasicBlock::iterator I,
                 const DebugLoc &DL, unsigned Reg, 
                 MachineBasicBlock *TargetMBB, bool KillSrc = false) const override; 
  void buildBLEZ(MachineBasicBlock &MBB, MachineBasicBlock::iterator I,
                 const DebugLoc &DL, unsigned Reg, 
                 MachineBasicBlock *TargetMBB, bool KillSrc = false) const override; 
  void buildBBCI(MachineBasicBlock &MBB, MachineBasicBlock::iterator I,
                 const DebugLoc &DL, unsigned Reg, int64_t Imm,
                 MachineBasicBlock *TargetMBB, bool KillSrc = false) const override; 
  void buildEXTUI(MachineBasicBlock &MBB, MachineBasicBlock::iterator I,
                  const DebugLoc &DL, unsigned DestReg, unsigned SrcReg,
                  int64_t Imm0, int64_t Imm1, bool KillSrc = false) const override ;
  void buildSRLI(MachineBasicBlock &MBB, MachineBasicBlock::iterator I,
                 const DebugLoc &DL, unsigned DestReg, unsigned SrcReg,
                 int64_t Imm, bool KillSrc ) const override;
  Tensilica::ConfigImap *getConfigImap() const override;
  Tensilica::ConfigRewrite *getConfigRewrite() const override;

};
}

#endif
