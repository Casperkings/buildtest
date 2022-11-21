//===-- XtensaISelLowering.h - Xtensa DAG Lowering Interface ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the interfaces that Xtensa uses to lower LLVM code into a
// selection DAG.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_XTENSA_XTENSAISELLOWERING_H
#define LLVM_LIB_TARGET_XTENSA_XTENSAISELLOWERING_H

#include "Tensilica/TensilicaInstrInfo.h"
#include "Tensilica/TensilicaISelLowering.h"
#include "Xtensa.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include <vector>
#include <unordered_set>

namespace llvm {

namespace Tensilica {
  struct IntrinsicISelInfo;
}
// Forward delcarations
class XtensaSubtarget;
class XtensaTargetMachine;
struct XtensaConfigTargetLowering;
class TargetSIMDInfo;

namespace XtensaISD {
enum NodeType {
  // Start the numbering where the builtin ops and target ops leave off.
  FIRST_NUMBER = ISD::BUILTIN_OP_END,

  // Branch and link (call)
  BL,

  // Call to _overlay _call_out_x stub
  CALL4_OVRL_OUT,

  // pc relative address
  PCRelativeWrapper,

  // Load word from stack
  LDWSP,

  // Corresponds to RET instruction
  RET,

  // Corresponds to ABS instruction
  ABS,

  // Average with no rounding (x + y) >> 1
  AVG,

  // Jumptable branch.
  // BR_JT,

  // Jumptable branch using long branches for each entry.
  // BR_JT32,

  // Caller's frame addr, aka SP for Xtensa
  FRAMEADDR,

  // Offset from frame pointer to the first (possible) on-stack argument
  FRAME_TO_ARGS_OFFSET,

  // Exception handler return. The stack is restored to the first
  // followed by a jump to the second argument.
  EH_RETURN,

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
};

}

//===--------------------------------------------------------------------===//
// TargetLowering Implementation
//===--------------------------------------------------------------------===//
class XtensaTargetLowering final : public Tensilica::ISelLowering {
public:
  explicit XtensaTargetLowering(const TargetMachine &TM,
                                const XtensaSubtarget &Subtarget);

  using TargetLowering::isZExtFree;
  bool isZExtFree(SDValue Val, EVT VT2) const override;

  // unsigned getJumpTableEncoding() const override;
  MVT getScalarShiftAmountTy(const DataLayout &DL, EVT LHSTy) const override {
    return MVT::i32;
  }

  /// Returns true if a cast between SrcAS and DestAS is a noop.
  bool isNoopAddrSpaceCast(unsigned SrcAS, unsigned DestAS) const override {
    // Addrspacecasts are always noops.
    return true;
  }

  /// Return true if folding a constant offset with the given GlobalAddress is
  /// legal.
  bool isOffsetFoldingLegal(const GlobalAddressSDNode *GA) const override;

  virtual SDValue
  BuildSDIVPow2(SDNode *N, const APInt &Divisor, SelectionDAG &DAG,
                SmallVectorImpl<SDNode *> &Created) const override;

  /// LowerOperation - Provide custom lowering hooks for some operations.
  SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) const override;

  /// Places new result values for the node in Results (their number
  /// and types must exactly match those of the original return values of
  /// the node), or leaves Results empty, which indicates that the node is not
  /// to be custom lowered after all.
  void LowerOperationWrapper(SDNode *N,
                             SmallVectorImpl<SDValue> &Results,
                             SelectionDAG &DAG) const override;

  /// ReplaceNodeResults - Replace the results of node with an illegal result
  /// type with new values built out of custom code.
  ///
  void ReplaceNodeResults(SDNode *N, SmallVectorImpl<SDValue>&Results,
                          SelectionDAG &DAG) const override;

  /// getTargetNodeName - This method returns the name of a target specific
  //  DAG node.
  const char *getTargetNodeName(unsigned Opcode) const override;


  SDValue getRecipEstimate(SDValue Operand, SelectionDAG &DAG,
                           int Enabled, int &RefinementSteps) const override;

  SDValue getSqrtEstimate(SDValue Operand, SelectionDAG &DAG,
                          int Enabled, int &RefinementSteps,
                          bool &UseOneConstNR, bool Reciprocal) const override;

  bool reduceSelectOfFPConstantLoads(EVT CmpOpVT) const override;

  MachineBasicBlock *
  EmitInstrWithCustomInserter(MachineInstr &MI,
                              MachineBasicBlock *MBB) const override;

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

  bool isLegalAddressingMode(const DataLayout &DL, const AddrMode &AM,
                             Type *Ty, unsigned AddrSpace,
                             Instruction *I = nullptr) const override;

  bool isLegalAddImmediate(int64_t) const override;

  /// Return true if an FMA operation is faster than a pair of fmul and fadd
  /// instructions. fmuladd intrinsics will be expanded to FMAs when this method
  /// returns true, otherwise fmuladd is expanded to fmul + fadd.
  bool isFMAFasterThanFMulAndFAdd(const MachineFunction &,
                                  EVT VT) const override;
  bool isFMAFasterThanFMulAndFAdd(EVT VT) const;

  bool enableAggressiveFMAFusion (EVT VT) const override;

  bool isSelectSupported(EVT ResVT) const override;

  bool isVectorShiftByScalarCheap(Type *Ty) const override;

  bool storeOfVectorConstantIsCheap(EVT MemVT,
                                    unsigned NumElem,
                                    unsigned AS) const override;

  bool shouldFormOverflowOp(unsigned Opcode, EVT VT) const override {
    return false;
  }

  /// If a physical register, this returns the register that receives the
  /// exception address on entry to an EH pad.
  unsigned
  getExceptionPointerRegister(const Constant *PersonalityFn) const override;

  /// If a physical register, this returns the register that receives the
  /// exception typeid on entry to a landing pad.
  unsigned
  getExceptionSelectorRegister(const Constant *PersonalityFn) const override;

  /// Returns the name of the function that implements llvm.eh_unwind_init
  const char *getFnNameForEhUnwindInit() const override;

  EVT getSetCCResultType(const DataLayout &DL, LLVMContext& Context,
                         EVT VT) const override;

  /// Return the memory information for a target load/store intrinsic
  bool getTgtMemIntrinsic(IntrinsicInfo &Info, const CallInst &I,
                          MachineFunction &MF,
                          unsigned Intrinsic) const override;

  /// Use the IntrinsicISelInfo table to generate a SDNode when expanding
  /// an instruction within a proto sequence
  SDNode *getIntrinsicInst(std::vector<SDNode *> &Nodes, const SDNode *N,
                           const SDValue Link, SelectionDAG *DAG,
                           const Tensilica::IntrinsicISelInfo *IntrISelInfo,
                           Tensilica::TensilicaISelLinkType LT) const;

  void setCustomOperationActions();

  bool shouldInsertFencesForAtomic(const Instruction *I) const override;

  bool isAtomicSizeSupported(const Instruction *I, unsigned Size,
                             unsigned Align) const override;

  TargetLoweringBase::AtomicExpansionKind
  shouldExpandAtomicRMWInIR(AtomicRMWInst *AI) const override;

  TargetLoweringBase::AtomicExpansionKind
  shouldExpandAtomicCmpXchgInIR(AtomicCmpXchgInst *AI) const override;

  Value *emitLoadLinked(IRBuilder<> &Builder, Value *Addr,
                        AtomicOrdering Ord) const override;
  Value *emitStoreConditional(IRBuilder<> &Builder, Value *Val,
                              Value *Addr, AtomicOrdering Ord) const override;
  void XtensaFixupA7LiveIn(MachineFunction &MF) const override;
  void AdjustInstrPostInstrSelection(MachineInstr &MI,
                                     SDNode *Node) const override;
  bool isXtensaOpSupported(unsigned Op, EVT VT) const;
  bool isMultiRegVectorType(VectorType *) const;

  // Insert C++ string into ExtSymbols set and return a permanent pointer to
  // its C-string equivalent after the insertion.
  const char* InsertExtSymbol(const std::string &str) const;
  bool isSuitableForJumpTable(const SwitchInst *SI, uint64_t NumCases,
                              uint64_t Range, ProfileSummaryInfo *,
                              BlockFrequencyInfo *) const override {
    const bool OptForSize = SI->getParent()->getParent()->hasOptSize();
    const unsigned MinDensity = OptForSize? 60: 30;
    const unsigned MaxJumpTableSize =
        OptForSize || getMaximumJumpTableSize() == 0
            ? UINT_MAX
            : getMaximumJumpTableSize();
    // Check whether a range of clusters is dense enough for a jump table.
    if (Range <= MaxJumpTableSize &&
        (NumCases * 100 >= Range * MinDensity)) {
      return true;
    }
    return false;
  }

private:
  const XtensaSubtarget &Subtarget;
  const XtensaConfigTargetLowering &XCTL;
  const TargetRegisterClass *ARRegClass;
  mutable unsigned A7VReg;
  mutable std::unordered_set<std::string> ExtSymbols;

  // Lower Operand helpers
  SDValue LowerCCCArguments(SDValue Chain, CallingConv::ID CallConv,
                            bool isVarArg,
                            const SmallVectorImpl<ISD::InputArg> &Ins,
                            const SDLoc &dl, SelectionDAG &DAG,
                            SmallVectorImpl<SDValue> &InVals) const;
  SDValue LowerCCCCallTo(SDValue Chain, SDValue Callee,
                         CallingConv::ID CallConv, bool isVarArg,
                         bool isTailCall,
                         const SmallVectorImpl<ISD::OutputArg> &Outs,
                         const SmallVectorImpl<SDValue> &OutVals,
                         const SmallVectorImpl<ISD::InputArg> &Ins,
                         const SDLoc &dl, SelectionDAG &DAG,
                         SmallVectorImpl<SDValue> &InVals) const;
  SDValue getReturnAddressFrameIndex(SelectionDAG &DAG) const;
  unsigned toCallerWindow(unsigned Reg) const;

  // Lower Operand specifics
  SDValue LowerEH_RETURN(SDValue Op, SelectionDAG &DAG) const;
  // SDValue LowerGlobalAddress(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerGlobalTLSAddress(SDValue Op, SelectionDAG &DAG) const;
  // SDValue LowerBlockAddress(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerMul(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerConstantPool(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerVAARG(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerVACOPY(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerVASTART(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerFRAMEADDR(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerFRAME_TO_ARGS_OFFSET(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerRETURNADDR(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerINIT_TRAMPOLINE(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerADJUST_TRAMPOLINE(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerINTRINSIC_W_CHAIN(SDValue Op, SmallVectorImpl<SDValue> &Results,
                                 SelectionDAG &DAG) const;
  SDValue LowerINTRINSIC_WO_CHAIN(SDValue Op, SmallVectorImpl<SDValue> &Results,
                                  SelectionDAG &DAG) const;
  SDValue LowerATOMIC_LOAD(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerATOMIC_STORE(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerBR_JT(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerDYNAMIC_STACKALLOC(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSTACKRESTORE(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerBUILD_VECTOR(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerCMP_SWAP(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerOp(SDValue Op, SmallVectorImpl<SDValue> &Results,
                  SelectionDAG &DAG) const;
  SDValue LowerStrictFpOp(SDValue Op, SmallVectorImpl<SDValue> &Results,
                          SelectionDAG &DAG) const;
  Register getRegisterByName(const char* RegName, LLT,
                             const MachineFunction &) const override;
  SDValue LowerEXTRACT_SUBVECTOR(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerCONCAT_VECTORS(SDValue Op, SelectionDAG &DAG) const;

  // Inline asm support
  ConstraintType getConstraintType(StringRef Constraint) const override;
  std::pair<unsigned, const TargetRegisterClass *>
  getRegForInlineAsmConstraint(const TargetRegisterInfo *TRI,
                               StringRef Constraint, MVT VT) const override;

  // Expand specifics

  SDValue PerformDAGCombine(SDNode *N, DAGCombinerInfo &DCI) const override;

  // void computeKnownBitsForTargetNode(const SDValue Op,
  //                                    APInt &KnownZero,
  //                                    APInt &KnownOne,
  //                                    const SelectionDAG &DAG,
  //                                    unsigned Depth = 0) const override;

  SDValue LowerFormalArguments(SDValue Chain, CallingConv::ID CallConv,
                               bool isVarArg,
                               const SmallVectorImpl<ISD::InputArg> &Ins,
                               const SDLoc &dl, SelectionDAG &DAG,
                               SmallVectorImpl<SDValue> &InVals) const override;

  SDValue LowerCall(TargetLowering::CallLoweringInfo &CLI,
                    SmallVectorImpl<SDValue> &InVals) const override;

  SDValue LowerCallResult(SDValue Chain, SDValue InFlag,
                          const SmallVectorImpl<CCValAssign> &RVLocs,
                          SDLoc dl, SelectionDAG &DAG,
                          SmallVectorImpl<SDValue> &InVals) const;

  SDValue LowerReturn(SDValue Chain, CallingConv::ID CallConv, bool isVarArg,
                      const SmallVectorImpl<ISD::OutputArg> &Outs,
                      const SmallVectorImpl<SDValue> &OutVals,
                      const SDLoc &dl, SelectionDAG &DAG) const override;

  bool CanLowerReturn(CallingConv::ID CallConv, MachineFunction &MF,
                      bool isVarArg,
                      const SmallVectorImpl<ISD::OutputArg> &ArgsFlags,
                      LLVMContext &Context) const override;

  bool shouldExpandBuildVectorWithShuffles(
                        EVT VT, unsigned DefinedValues) const override;

  bool functionArgumentNeedsConsecutiveRegisters(
    Type *Ty, CallingConv::ID CallConv, bool isVarArg) const override;

  SDValue prepareVolatileInst(SDValue Chain, const SDLoc &DL,
                              SelectionDAG &DAG) const override;

  unsigned wrapA7(MachineRegisterInfo &RegInfo,
                  unsigned IncomingReg,
                  const TargetRegisterClass *RC) const;

  bool allowsMisalignedMemoryAccesses(EVT VT, unsigned AddrSpace,
                                      unsigned Align,
                                      MachineMemOperand::Flags Flags,
                                      bool *Fast) const override;

  bool shouldSinkSelectOperand(Instruction *I) const override;
};
} // namespace llvm

#endif // LLVM_LIB_TARGET_XTENSA_XTENSAISELLOWERING_H
