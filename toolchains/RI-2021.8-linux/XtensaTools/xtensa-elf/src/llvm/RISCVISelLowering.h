//===-- RISCVISelLowering.h - RISCV DAG Lowering Interface ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines the interfaces that RISCV uses to lower LLVM code into a
// selection DAG.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_RISCV_RISCVISELLOWERING_H
#define LLVM_LIB_TARGET_RISCV_RISCVISELLOWERING_H

#include "RISCV.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/TargetLowering.h"

#if defined(TENSILICA) || 1
#include "llvm/CodeGen/CallingConvLower.h"
#include "Tensilica/TensilicaISelLowering.h"
#endif

namespace llvm {
class RISCVSubtarget;
#if defined(TENSILICA) || 1
struct RISCVConfigTargetLowering;
#endif
namespace RISCVISD {
enum NodeType : unsigned {
  FIRST_NUMBER = ISD::BUILTIN_OP_END,
  RET_FLAG,
  URET_FLAG,
  SRET_FLAG,
  MRET_FLAG,
  CALL,
  SELECT_CC,
  BuildPairF64,
  SplitF64,
  TAIL,
  // RV64I shifts, directly matching the semantics of the named RISC-V
  // instructions.
  SLLW,
  SRAW,
  SRLW,
  // 32-bit operations from RV64M that can't be simply matched with a pattern
  // at instruction selection time.
  DIVW,
  DIVUW,
  REMUW,
  // FPR32<->GPR transfer operations for RV64. Needed as an i32<->f32 bitcast
  // is not legal on RV64. FMV_W_X_RV64 matches the semantics of the FMV.W.X.
  // FMV_X_ANYEXTW_RV64 is similar to FMV.X.W but has an any-extended result.
  // This is a more convenient semantic for producing dagcombines that remove
  // unnecessary GPR->FPR->GPR moves.
  FMV_W_X_RV64,
  FMV_X_ANYEXTW_RV64,
  // READ_CYCLE_WIDE - A read of the 64-bit cycle CSR on a 32-bit target
  // (returns (Lo, Hi)). It takes a chain operand.
  READ_CYCLE_WIDE
#if defined(TENSILICA) || 1
  ,
  // Corresponds to ABS instruction
  ABS,

  // Average with no rounding (x + y) >> 1
  AVG,

  // Floating point reciprocal
  FRECIP,

  // Floating point rsqrt
  FRSQRT,

  // Reduction ADD
  RADD,

  // Reduction MAX
  RMAX,

  // Reduction MIN
  RMIN,

  // Reduction AND
  RBAND,

  // Reduction OR
  RBOR,

  // Reduction XOR
  RBXOR,

  // Signed integer vector madd
  MADD,

  // Unsigned integer vector madd
  MADDU,

  // Signed integer vector msub
  MSUB,

  // Unsigned integer vector msub
  MSUBU,

  // Signed integer multiplication with signed extension
  SMULE,

  // Unsigned integer multiplication with unsigned extension
  UMULE,

  // Floating-point scalar/vector msub
  FMSUB,

  // Special case of ISD::EXTRACT_SUBVECTOR(VECTOR, IDX), where IDX=0, and the
  // result vector length is half of VECTOR.
  EXTRACT_SUBVECTOR_LO_HALF,

  // Special case of ISD::EXTRACT_SUBVECTOR(VECTOR, IDX), where
  // IDX=(half of VECTOR - 1), and the result vector length is half of VECTOR.
  EXTRACT_SUBVECTOR_UP_HALF,

  // Special case of CONCAT_VECTORS(VECTOR0, VECTOR1, ...), where only two
  // vectors with the same length to be concatenated.
  CONCAT_HALF_VECTORS
#endif
};
}

#if defined(TENSILICA) || 1
class RISCVTargetLowering : public Tensilica::ISelLowering {
#else
class RISCVTargetLowering : public TargetLowering {
#endif
  const RISCVSubtarget &Subtarget;
#if defined(TENSILICA) || 1
  const RISCVConfigTargetLowering &RCTL;
#endif

public:
  explicit RISCVTargetLowering(const TargetMachine &TM,
                               const RISCVSubtarget &STI);

  bool getTgtMemIntrinsic(IntrinsicInfo &Info, const CallInst &I,
                          MachineFunction &MF,
                          unsigned Intrinsic) const override;
#if defined(TENSILICA) || 1
  /// Use the IntrinsicISelInfo table to generate a SDNode when expanding
  /// an instruction within a proto sequence
  SDNode *getIntrinsicInst(std::vector<SDNode *> &Nodes, const SDNode *N,
                           const SDValue Link, SelectionDAG *DAG,
                           const Tensilica::IntrinsicISelInfo *IntrISelInfo,
                           Tensilica::TensilicaISelLinkType LT) const;
  bool isRISCVOpSupported(unsigned Op, EVT VT) const;

  /// Returns true if a cast between SrcAS and DestAS is a noop.
  bool isNoopAddrSpaceCast(unsigned SrcAS, unsigned DestAS) const override {
    // Addrspacecasts are always noops.
    return true;
  }

#endif
  bool isLegalAddressingMode(const DataLayout &DL, const AddrMode &AM, Type *Ty,
                             unsigned AS,
                             Instruction *I = nullptr) const override;
  bool isLegalICmpImmediate(int64_t Imm) const override;
  bool isLegalAddImmediate(int64_t Imm) const override;
  bool isTruncateFree(Type *SrcTy, Type *DstTy) const override;
  bool isTruncateFree(EVT SrcVT, EVT DstVT) const override;
  bool isZExtFree(SDValue Val, EVT VT2) const override;
  bool isSExtCheaperThanZExt(EVT SrcVT, EVT DstVT) const override;

  bool hasBitPreservingFPLogic(EVT VT) const override;

  // Provide custom lowering hooks for some operations.
  SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) const override;
  void ReplaceNodeResults(SDNode *N, SmallVectorImpl<SDValue> &Results,
                          SelectionDAG &DAG) const override;

  SDValue PerformDAGCombine(SDNode *N, DAGCombinerInfo &DCI) const override;

  unsigned ComputeNumSignBitsForTargetNode(SDValue Op,
                                           const APInt &DemandedElts,
                                           const SelectionDAG &DAG,
                                           unsigned Depth) const override;

  // This method returns the name of a target specific DAG node.
  const char *getTargetNodeName(unsigned Opcode) const override;

  ConstraintType getConstraintType(StringRef Constraint) const override;

  unsigned getInlineAsmMemConstraint(StringRef ConstraintCode) const override;

  std::pair<unsigned, const TargetRegisterClass *>
  getRegForInlineAsmConstraint(const TargetRegisterInfo *TRI,
                               StringRef Constraint, MVT VT) const override;

  void LowerAsmOperandForConstraint(SDValue Op, std::string &Constraint,
                                    std::vector<SDValue> &Ops,
                                    SelectionDAG &DAG) const override;

  MachineBasicBlock *
  EmitInstrWithCustomInserter(MachineInstr &MI,
                              MachineBasicBlock *BB) const override;

  EVT getSetCCResultType(const DataLayout &DL, LLVMContext &Context,
                         EVT VT) const override;

  bool convertSetCCLogicToBitwiseLogic(EVT VT) const override {
    return VT.isScalarInteger();
  }

  bool shouldInsertFencesForAtomic(const Instruction *I) const override {
    return isa<LoadInst>(I) || isa<StoreInst>(I);
  }
  Instruction *emitLeadingFence(IRBuilder<> &Builder, Instruction *Inst,
                                AtomicOrdering Ord) const override;
  Instruction *emitTrailingFence(IRBuilder<> &Builder, Instruction *Inst,
                                 AtomicOrdering Ord) const override;

  ISD::NodeType getExtendForAtomicOps() const override {
    return ISD::SIGN_EXTEND;
  }

  bool shouldExpandShift(SelectionDAG &DAG, SDNode *N) const override {
    if (DAG.getMachineFunction().getFunction().hasMinSize())
      return false;
    return true;
  }
  bool isDesirableToCommuteWithShift(const SDNode *N,
                                     CombineLevel Level) const override;

  /// If a physical register, this returns the register that receives the
  /// exception address on entry to an EH pad.
  unsigned
  getExceptionPointerRegister(const Constant *PersonalityFn) const override;

  /// If a physical register, this returns the register that receives the
  /// exception typeid on entry to a landing pad.
  unsigned
  getExceptionSelectorRegister(const Constant *PersonalityFn) const override;

  bool shouldExtendTypeInLibCall(EVT Type) const override;

  /// Returns the register with the specified architectural or ABI name. This
  /// method is necessary to lower the llvm.read_register.* and
  /// llvm.write_register.* intrinsics. Allocatable registers must be reserved
  /// with the clang -ffixed-xX flag for access to be allowed.
  Register getRegisterByName(const char *RegName, LLT VT,
                             const MachineFunction &MF) const override;

private:
  void analyzeInputArgs(MachineFunction &MF, CCState &CCInfo,
                        const SmallVectorImpl<ISD::InputArg> &Ins,
                        bool IsRet) const;
  void analyzeOutputArgs(MachineFunction &MF, CCState &CCInfo,
                         const SmallVectorImpl<ISD::OutputArg> &Outs,
                         bool IsRet, CallLoweringInfo *CLI) const;
  // Lower incoming arguments, copy physregs into vregs
  SDValue LowerFormalArguments(SDValue Chain, CallingConv::ID CallConv,
                               bool IsVarArg,
                               const SmallVectorImpl<ISD::InputArg> &Ins,
                               const SDLoc &DL, SelectionDAG &DAG,
                               SmallVectorImpl<SDValue> &InVals) const override;
  bool CanLowerReturn(CallingConv::ID CallConv, MachineFunction &MF,
                      bool IsVarArg,
                      const SmallVectorImpl<ISD::OutputArg> &Outs,
                      LLVMContext &Context) const override;
  SDValue LowerReturn(SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
                      const SmallVectorImpl<ISD::OutputArg> &Outs,
                      const SmallVectorImpl<SDValue> &OutVals, const SDLoc &DL,
                      SelectionDAG &DAG) const override;
  SDValue LowerCall(TargetLowering::CallLoweringInfo &CLI,
                    SmallVectorImpl<SDValue> &InVals) const override;
#if defined(TENSILICA) || 1
  SDValue unpackFromRegLoc(SelectionDAG &DAG, SDValue Chain,
                                const CCValAssign &VA, const SDLoc &DL) const;
  bool CC_RISCV(const DataLayout &DL, RISCVABI::ABI ABI, unsigned ValNo,
                MVT ValVT, MVT LocVT, CCValAssign::LocInfo LocInfo,
                ISD::ArgFlagsTy ArgFlags, CCState &State, bool IsFixed,
                bool IsRet, Type *OrigTy) const;
  /// Places new result values for the node in Results (their number
  /// and types must exactly match those of the original return values of
  /// the node), or leaves Results empty, which indicates that the node is not
  /// to be custom lowered after all.
  void LowerOperationWrapper(SDNode *N,
                             SmallVectorImpl<SDValue> &Results,
                             SelectionDAG &DAG) const override;
  SDValue LowerINTRINSIC_WO_CHAIN(SDValue Op, SmallVectorImpl<SDValue> &Results,
                                  SelectionDAG &DAG) const;
  SDValue LowerINTRINSIC_W_CHAIN(SDValue Op, SmallVectorImpl<SDValue> &Results,
                                 SelectionDAG &DAG) const;
  SDValue LowerOp(SDValue Op, SmallVectorImpl<SDValue> &Results,
                  SelectionDAG &DAG) const;
  SDValue LowerBUILD_VECTOR(SDValue Op, SelectionDAG &DAG) const;
  bool allowsMisalignedMemoryAccesses(EVT VT, unsigned AddrSpace,
                                      unsigned Align,
                                      MachineMemOperand::Flags Flags,
                                      bool *Fast) const override;
  bool isSelectSupported(EVT ResVT) const override;
  SDValue LowerMul(SDValue Op, SelectionDAG &DAG) const;
#endif
  bool shouldConvertConstantLoadToIntImm(const APInt &Imm,
                                         Type *Ty) const override {
    return true;
  }

  template <class NodeTy>
  SDValue getAddr(NodeTy *N, SelectionDAG &DAG, bool IsLocal = true) const;

  SDValue getStaticTLSAddr(GlobalAddressSDNode *N, SelectionDAG &DAG,
                           bool UseGOT) const;
  SDValue getDynamicTLSAddr(GlobalAddressSDNode *N, SelectionDAG &DAG) const;

  bool shouldConsiderGEPOffsetSplit() const override { return true; }
  SDValue lowerGlobalAddress(SDValue Op, SelectionDAG &DAG) const;
  SDValue lowerBlockAddress(SDValue Op, SelectionDAG &DAG) const;
  SDValue lowerConstantPool(SDValue Op, SelectionDAG &DAG) const;
  SDValue lowerGlobalTLSAddress(SDValue Op, SelectionDAG &DAG) const;
  SDValue lowerSELECT(SDValue Op, SelectionDAG &DAG) const;
  SDValue lowerVASTART(SDValue Op, SelectionDAG &DAG) const;
  SDValue lowerFRAMEADDR(SDValue Op, SelectionDAG &DAG) const;
  SDValue lowerRETURNADDR(SDValue Op, SelectionDAG &DAG) const;
  SDValue lowerShiftLeftParts(SDValue Op, SelectionDAG &DAG) const;
  SDValue lowerShiftRightParts(SDValue Op, SelectionDAG &DAG, bool IsSRA) const;

  bool isEligibleForTailCallOptimization(
      CCState &CCInfo, CallLoweringInfo &CLI, MachineFunction &MF,
      const SmallVector<CCValAssign, 16> &ArgLocs) const;

  TargetLowering::AtomicExpansionKind
  shouldExpandAtomicRMWInIR(AtomicRMWInst *AI) const override;
  virtual Value *emitMaskedAtomicRMWIntrinsic(
      IRBuilder<> &Builder, AtomicRMWInst *AI, Value *AlignedAddr, Value *Incr,
      Value *Mask, Value *ShiftAmt, AtomicOrdering Ord) const override;
  TargetLowering::AtomicExpansionKind
  shouldExpandAtomicCmpXchgInIR(AtomicCmpXchgInst *CI) const override;
  virtual Value *
  emitMaskedAtomicCmpXchgIntrinsic(IRBuilder<> &Builder, AtomicCmpXchgInst *CI,
                                   Value *AlignedAddr, Value *CmpVal,
                                   Value *NewVal, Value *Mask,
                                   AtomicOrdering Ord) const override;

  /// Generate error diagnostics if any register used by CC has been marked
  /// reserved.
  void validateCCReservedRegs(
      const SmallVectorImpl<std::pair<llvm::Register, llvm::SDValue>> &Regs,
      MachineFunction &MF) const;

#if defined(TENSILICA) || 1
  void setCustomOperationActions();

public:
  unsigned getMaxSupportedInterleaveFactor() const override {
    // This can be bigger, but need adjust MaxInterleaveGroupFactor in
    // InterleavedAccessInfo.cpp as well. Leave it for now.
    return 8;
  }

  bool lowerInterleavedLoad(LoadInst *LI,
                            ArrayRef<ShuffleVectorInst *> Shuffles,
                            ArrayRef<unsigned> Indices,
                            unsigned Factor) const override;
  bool lowerInterleavedStore(StoreInst *SI, ShuffleVectorInst *SVI,
                             unsigned Factor) const override;

#endif

};
}

#endif
