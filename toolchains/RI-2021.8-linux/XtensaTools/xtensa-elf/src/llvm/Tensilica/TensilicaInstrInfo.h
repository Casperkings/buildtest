//===-- Tensilica/TensilicaInstrInfo.h - Tensilica Instruction Information ===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the Tensilica common part of implementation of the
// TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_CODEGEN_TENSILICA_TENSILICAINSTRINFO_H
#define LLVM_LIB_CODEGEN_TENSILICA_TENSILICAINSTRINFO_H

#include "Tensilica/TensilicaInstrProperty.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/Target/TargetIntrinsicInfo.h"
#include <cstdint>
#include <set>
#include <utility>
#include <vector>

namespace llvm {

namespace Tensilica {

struct ConfigInstrInfo;
struct ConfigPacketizer;
struct ConfigImap;
struct ConfigRewrite;
struct IntrinsicISelInfo;
class ISelLowering;

class InstrInfo : public TargetInstrInfo {
public:
  InstrInfo(Tensilica::ConfigInstrInfo &CII, unsigned CFSetupOpcode = ~0u,
            unsigned CFDestroyOpcode = ~0u, unsigned CatchRetOpcode = ~0u,
            unsigned ReturnOpcode = ~0u);

  bool isYmemoryLoad(const MachineInstr &MI) const final {
    if (!MI.mayLoad() || !MI.hasOneMemOperand())
      return false;
    const MachineMemOperand *MMO = *(MI.memoperands_begin());
    return MMO->getAddrSpace() == llvm::XtensaYmemoryAddrSpace;
  }

  bool areMemAccessesDisjoint(const MachineInstr &MIa, const MachineInstr &MIb,
                              AAResults *AA = nullptr) const final;
  bool areMemAccessesTriviallyDisjoint(const MachineInstr &MIa,
                                       const MachineInstr &MIb) const override;
  bool areMemAccessesTriviallyBankConflict(const MachineInstr &MIa,
                                           const MachineInstr &MIb) const;

  /// isLoadFromStackSlot - If the specified machine instruction is a direct
  /// load from a stack slot, return the virtual or physical register number of
  /// the destination along with the FrameIndex of the loaded stack slot.  If
  /// not, return 0.  This predicate must return 0 if the instruction has
  /// any side effects other than loading from the stack slot.
  /// TODO: These helpers should be final.
  unsigned isLoadFromStackSlot(const MachineInstr &MI,
                               int &FrameIndex) const override;

  unsigned isLoadFromStackSlotWithOffset(const MachineInstr &MI,
                                         int &FrameIndex, bool Zero) const;

  /// isStoreToStackSlot - If the specified machine instruction is a direct
  /// store to a stack slot, return the virtual or physical register number of
  /// the source reg along with the FrameIndex of the loaded stack slot.  If
  /// not, return 0.  This predicate must return 0 if the instruction has
  /// any side effects other than storing to the stack slot.
  unsigned isStoreToStackSlot(const MachineInstr &MI,
                              int &FrameIndex) const override;
  unsigned isStoreToStackSlotWithOffset(const MachineInstr &MI, int &FrameIndex,
                                        bool Zero) const;

  void storeRegToStackSlot(MachineBasicBlock &MBB,
                           MachineBasicBlock::iterator MI, unsigned SrcReg,
                           bool isKill, int FrameIndex,
                           const TargetRegisterClass *RC,
                           const TargetRegisterInfo *TRI) const override;

  void loadRegFromStackSlot(MachineBasicBlock &MBB,
                            MachineBasicBlock::iterator MI, unsigned DestReg,
                            int FrameIndex, const TargetRegisterClass *RC,
                            const TargetRegisterInfo *TRI) const override;

  void expandINSERT_SUBREG(MachineInstr &MI) const;
  void expandEXTRACT_SUBREG(MachineInstr &MI) const;

  // Common implementation part of copyPhysReg.
  // This function is overriden (and used) by XtensaInstrInfo::copyPhysReg and
  // RISCVInstrInfo::copyPhysReg.
  void copyPhysReg(MachineBasicBlock &MBB, MachineBasicBlock::iterator I,
                   const DebugLoc &DL, MCRegister DestReg, MCRegister SrcReg,
                   bool KillSrc) const override;

  // Emit code before MI to load immediate value into register Reg.
  // Returns an iterator to the new instruction.
  virtual MachineBasicBlock::iterator
  loadImmediate(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
                Register Reg, int64_t Value,
                MachineInstr::MIFlag Flag = MachineInstr::NoFlags) const = 0;

  // Emit code before MI to add SrcReg and immediate value and write the result
  // to register DestReg. Returns an iterator to the new instruction.
  virtual MachineBasicBlock::iterator
  addImmediate(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
               Register DestReg, Register SrcReg, int64_t Value,
               RegScavenger *RS = nullptr,
               MachineInstr::MIFlag Flag = MachineInstr::NoFlags) const = 0;

  // Query if given immediate value will be emitted as a single ADDI or ADDMI
  // instruction by addImmediate.
  virtual bool isImmADDI(int64_t Imm) const = 0;

  virtual void expandLOAD_CONST(MachineInstr &MI) const = 0;

  void
  getPseudoInstOutOperands(MachineInstr *MI,
                           const Tensilica::IntrinsicISelInfo *IntrISelInfo,
                           MachineInstrBuilder &MIB) const;

  void getPseudoInstInOperands(std::vector<MachineInstr *> &MIList,
                               MachineInstr *MI,
                               const Tensilica::IntrinsicISelInfo *IntrISelInfo,
                               MachineInstrBuilder &MIB) const;

  MachineInstr *replaceExtractSubRegWithCopy(
      std::vector<MachineInstr *> &MIList, MachineInstr *MI,
      const Tensilica::IntrinsicISelInfo *IntrISelInfo) const;

  MachineInstr *
  getPseudoInst(std::vector<MachineInstr *> &MIList, MachineInstr *MI,
                const Tensilica::IntrinsicISelInfo *IntrISelInfo) const;

  void expandPseudoInst(MachineInstr *MI, unsigned IntrinsicID) const;

  Register getRematDstReg(const MachineInstr &MI) const;
  unsigned getRegMovOpcode(const Tensilica::ISelLowering *TLI,
                           const TargetRegisterClass *RC) const;

  const InstrProperty *getInstrProperty(unsigned Opcode) const;

  const XtensaImmRangeSet &getImmediateRange(const MachineInstr *MI,
                                             uint32_t OpndNum) const {
    return getImmediateRange(MI->getOpcode(), OpndNum);
  }

  const XtensaImmRangeSet &getImmediateRange(unsigned Opcode,
                                             uint32_t OpndNum) const {
    const Tensilica::InstrProperty *Prop = getInstrProperty(Opcode);
    if (Prop->Opcode == 0) {
      static const XtensaImmRangeSet RS = {{}};
      return RS;
    }
    return Prop->getImmRange(OpndNum);
  }

  int32_t getFIUpperBound(unsigned Opcode, unsigned N) const {
    const Tensilica::InstrProperty *Prop = getInstrProperty(Opcode);
    if (Prop->Opcode != 0) {
      // most likely it is N-1
      for (unsigned Opnd = N - 1; Opnd > 0; Opnd--) {
        if (Prop->hasImmRange(Opnd)) {
          return Prop->getImmRange(Opnd).getBounds().second;
        }
      }
    }
    return -1;
  }

  virtual unsigned getInstOperandInputBits(const MachineInstr &MI,
                                           unsigned OpNo) const = 0;
  
  unsigned getTargetInstOperandInputBits(const MachineInstr &MI,
                                         unsigned OpNo) const ;
  
  bool ignoreStateOutput(unsigned Opc, unsigned State) const final;

  // TODO memory addressing semantic APIs
  /// Return true if the instruction contains a base register and offset. If
  /// true, the function also sets the operand position in the instruction
  /// for the base register and offset.
  bool getBaseAndOffsetPosition(const MachineInstr &MI, unsigned &BasePos,
                                unsigned &OffsetPos) const final;

  // Return true if we detect base register, constant offset of the memory
  // access and its width, which are returned through corresponding arguments.
  // If XReg is non-null, it's added to BaseReg for accessed address.
  bool getMemOperandWithOffsetWidth(const MachineInstr &LdSt,
                                    const MachineOperand *&BaseReg,
                                    const MachineOperand *&XReg,
                                    int64_t &Offset, unsigned &Width,
                                    const TargetRegisterInfo *TRI) const;

  bool isPostIncrement(const MachineInstr *MI) const {
    return getInstrProperty(MI->getOpcode())->isPostInc();
  }

  bool isBaseDispImmediate(const MachineInstr *MI) const {
    const auto Property = getInstrProperty(MI->getOpcode());
    return Property->isBaseDispImm();
  }

  unsigned mapToAddressMode(unsigned Opcode, XtensaLdStKind FromKind,
                            XtensaLdStKind ToKind) const;
  void convertToAddressMode(MachineInstr *MI, XtensaLdStKind FromKind,
                            XtensaLdStKind ToKind) const;

  bool isAdjustableStage(const MachineInstr *MI, unsigned OpIdx) const;
  unsigned getAStageOpcode(unsigned Opcode) const;
  unsigned getEStageOpcode(unsigned Opcode) const;
  unsigned getAStage() const ;
  unsigned getEStage() const ;
  // Return if the access is at A stage
  bool operandAccessStage(const MachineInstr *MI, unsigned OpIdx) const {
    return false;
  }

  /// Generic conditional branch information:
  /// minimal/maximal label distance, corresponding wide branch opcode.
  struct BranchInfo {
    unsigned MinDist;
    unsigned MaxDist;
    unsigned WideBranchOpcode;
    bool isUnknown() const { return MinDist == INT_MAX && MaxDist == 0; }
    BranchInfo() : MinDist(INT_MAX), MaxDist(0), WideBranchOpcode(0) {}
  };
  /// getGenericTypicalImplementation, get a none density implementation of
  /// the opcode, if it is generic, otherwise return the opcode itself
  unsigned getGenericTypicalImplementation(unsigned) const;
  unsigned getGenericTypicalImplementationBranch(unsigned) const;

  bool isSemantic(unsigned opcodeA, unsigned opcodeB) const {
    return isGeneric(getEStageOpcode(opcodeA), getEStageOpcode(opcodeB));
  }
  bool isGeneric(unsigned Opcode) const {
    return getGenericTypicalImplementation(Opcode) != Opcode;
  }
  bool isGeneric(unsigned Opcode, unsigned Opcode1) const {
    return getGenericTypicalImplementation(Opcode) ==
           getGenericTypicalImplementation(Opcode1);
  }
  unsigned getGeneric(unsigned) const;
  bool isGenericBranch(unsigned Opcode) const {
    return getGenericTypicalImplementationBranch(Opcode) != Opcode;
  }
  bool isGenericBranch(unsigned Opcode, unsigned Opcode1) const {
    return getGenericTypicalImplementationBranch(Opcode) ==
           getGenericTypicalImplementationBranch(Opcode1);
  }
  BranchInfo getBranchInfo(unsigned Opcode) const;
  virtual bool isCondBranch(unsigned BrOpcGeneric) const { return false; }
  virtual bool isLoop(unsigned BrOpcGeneric) const { return false; }
  virtual bool isBrDec(unsigned BrOpcGeneric) const { return false; }
  virtual bool isPseudoInstr(const MachineInstr *I) const { return false; }

  virtual unsigned getBranchTargetOpnd(unsigned BrOpcode) const = 0;
  unsigned branchRange(unsigned Opcode) const;
  unsigned estimateBlockSize(const MachineBasicBlock *MBB) const;
  unsigned estimateBranchDist(const MachineInstr *MI) const;
  int getMaxNumSlot() const;
  int getMidNumSlot() const;

  MapOpcode getSemantic(unsigned Opcode) const;
  unsigned mapToTargetOpcode(MapOpcode M) const;
  const ArrayRef<unsigned> mapToVariants(MapOpcode M) const;

  const MCInstrDesc &map(MapOpcode M) const {
    unsigned Opcode = mapToTargetOpcode(M);
    assert(Opcode != static_cast<unsigned>(-1 /*LLVMXCC_OPC_NULL*/) &&
           "Cannot map to target opcode");
    return get(Opcode);
  }
  bool isSemantic(unsigned opcode, MapOpcode MOP) const {
    return isSemantic(opcode, mapToTargetOpcode(MOP));
  }

  // For CONST16 return 1 for first and 2 for second CONST16 in pair.
  // This may be used for LUI+ADDI pair for risc-v variant.
  virtual int isHalfConst(const MachineInstr &) const { return 0; }
  virtual bool evalConst(const MachineInstr &, int64_t &Val,
                         int Depth = 2) const {
    return false;
  }
  virtual bool evalConstOp(const MachineOperand &, int64_t &Val,
                           int Depth = 2) const {
    return false;
  }
  virtual bool chooseImmADDI(int64_t From, int64_t To, int64_t &Val) const {
    return 0;
  }
  bool isADD(const MachineInstr &MI) const {
    return getSemantic(MI.getOpcode()) == MapOpcode::ADD;
  }
  bool isADDI(unsigned Opcode) const {
    return getSemantic(Opcode) == MapOpcode::ADDI;
  }
  bool isADDImm(const MachineInstr &MI) const {
    return isADDI(MI.getOpcode()) && MI.getOperand(2).isImm() &&
           MI.getOperand(1).isReg();
  }
  bool isMOVImm(const MachineInstr &MI, unsigned &IdxImm) const {
    return isMOVI(MI, IdxImm) && MI.getOperand(IdxImm).isImm();
  }
  virtual bool mayCopyProp(const MachineInstr &MI, int64_t &Offset) const {
    return false;
  }
  bool mayChangeImmediate(const MachineInstr &MI, unsigned ImmOpNo,
                                  int64_t Imm, unsigned &NewOpcode) const;
  virtual bool isMOVI(const MachineInstr &, unsigned &IdxImm) const {
    return false;
  }

  virtual bool isMovArBr(const MachineInstr &) const { return false; }
  virtual bool isMovBrAr(const MachineInstr &) const { return false; }

  virtual bool isEntry(const MachineInstr &) const { return false; }

  // Analyze if the specified branch instruction might be transformed to brdec.
  // Return the register, which is compared to zero in condition.
  virtual Register analyzeBranchForBrdec(MachineInstr &MI) const = 0;
  virtual void convertBranchToBrdec(MachineInstr &MI) const = 0;

  std::pair<int, int> getStrengthReductionPos(const MachineInstr *MI) const;
  std::pair<bool, int> checkStrengthReduction(const MachineInstr *MI,
                                              unsigned Reg, int64_t Diff) const;
  bool applyStrengthReduction(MachineFunction &MF, MachineInstr *MI,
                              unsigned OriginalReg, unsigned NewReg,
                              int64_t Diff) const;

  // Target-specific instruction promotion, in particular to handle cases like
  // BBC/BBS for big-endian.
  virtual bool promoteInstruction(MachineInstr &MI) const { return false; }

  bool generalizeInstruction(MachineInstr &MI) const;
  bool specializeInstruction(MachineInstr &MI) const;

  // Test if the instruction is like a fence for packetizer.
  virtual bool isPacketizerBoundary(const MachineInstr &MI) const {
    return MI.hasUnmodeledSideEffects() || MI.isEHLabel();
  }

  // Test if the given instruction should be considered a scheduling boundary.
  // This primarily includes labels and terminators.
  bool isSchedulingBoundary(const MachineInstr &MI,
                            const MachineBasicBlock *MBB,
                            const MachineFunction &MF) const override;

  MachineInstr *commuteInstructionImpl(MachineInstr &MI, bool NewMI,
                                       unsigned OpIdx1,
                                       unsigned OpIdx2) const override;
  bool isPartRed(const MachineInstr &Inst) const;
  bool isPartRedAdd(const MachineInstr &Inst) const;
  bool isPartRedSub(const MachineInstr &Inst) const;
  bool isPartRedMinMax(const MachineInstr &Inst) const;
  bool isPartRedMadd(const MachineInstr &Inst) const;
  unsigned getPartRedZeroOpcode(const MachineInstr &Inst) const;
  unsigned getPartRedAddOpcode(const MachineInstr &Inst) const;

  MachineBasicBlock *getZCLPreheader(MachineBasicBlock *BB) const;
  virtual bool isLoopInfinite(MachineLoop *Loop) const { return false; }
  unsigned reduceLoopCount(MachineBasicBlock &MBB, MachineInstr *IndVar,
                           MachineInstr *Cmp, MachineInstr *Loop,
                           SmallVectorImpl<MachineOperand> &Cond,
                           SmallVectorImpl<MachineInstr *> &PrevInsts,
                           unsigned Iter, unsigned MaxIter,
                           unsigned MinimalTC) const ;
  virtual void buildBEQZ(MachineBasicBlock &MBB, MachineBasicBlock::iterator I,
                 const DebugLoc &DL, unsigned Reg, 
                 MachineBasicBlock *TargetMBB, bool KillSrc = false) const = 0; 
  virtual void buildBLEZ(MachineBasicBlock &MBB, MachineBasicBlock::iterator I,
                 const DebugLoc &DL, unsigned Reg, 
                 MachineBasicBlock *TargetMBB, bool KillSrc = false) const = 0; 
  virtual void buildBBCI(MachineBasicBlock &MBB, MachineBasicBlock::iterator I,
                 const DebugLoc &DL, unsigned Reg, int64_t Imm,
                 MachineBasicBlock *TargetMBB, bool KillSrc = false) const = 0;
  virtual void buildEXTUI(MachineBasicBlock &MBB, MachineBasicBlock::iterator I,
                 const DebugLoc &DL, unsigned DestReg, unsigned SrcReg,
                 int64_t Imm0, int64_t Imm1, bool KillSrc = false) const = 0;
  virtual void buildSRLI(MachineBasicBlock &MBB, MachineBasicBlock::iterator I,
                 const DebugLoc &DL, unsigned DestReg, unsigned SrcReg,
                 int64_t Imm, bool KillSrc = false) const = 0;
  virtual Tensilica::ConfigImap *getConfigImap() const = 0;
  virtual Tensilica::ConfigRewrite *getConfigRewrite() const = 0;

private:
  const Tensilica::ConfigInstrInfo &CII;
  mutable DenseMap<unsigned /*Branch Opcode*/, BranchInfo> BInfo;
};

} // namespace Tensilica
} // namespace llvm

#endif // LLVM_LIB_CODEGEN_TENSILICA_TENSILICAINSTRINFO_H
