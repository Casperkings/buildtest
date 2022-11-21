//===-- XtensaTargetTransformInfo.h - Xtensa specific TTI -----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
/// This file a TargetTransformInfo::Concept conforming object specific to the
/// Xtensa target machine. It uses the target's detailed information to
/// provide more precise answers to certain TTI queries, while letting the
/// target independent and default TTI implementations handle the rest.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_XTENSA_XTENSATARGETTRANSFORMINFO_H
#define LLVM_LIB_TARGET_XTENSA_XTENSATARGETTRANSFORMINFO_H

#include "Xtensa.h"
#include "XtensaTargetMachine.h"
#include "Tensilica/TensilicaTargetSIMDInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/CodeGen/BasicTTIImpl.h"
#include "llvm/CodeGen/TargetLowering.h"

namespace llvm {
class XtensaTargetSIMDInfo;
class XtensaTTIImpl : public BasicTTIImplBase<XtensaTTIImpl> {
  typedef BasicTTIImplBase<XtensaTTIImpl> BaseT;
  typedef TargetTransformInfo TTI;
  friend BaseT;

  const XtensaSubtarget *ST;
  const XtensaTargetLowering *TLI;

  const XtensaSubtarget *getST() const { return ST; }
  const XtensaTargetLowering *getTLI() const { return TLI; }

  LLVMContext &Ctx;

public:
  explicit XtensaTTIImpl(const XtensaTargetMachine *TM, const Function &F)
      : BaseT(TM, F.getParent()->getDataLayout()), ST(TM->getSubtargetImpl(F)),
        TLI(ST->getTargetLowering()), Ctx(F.getContext()) {}

  ~XtensaTTIImpl() {}

  // Provide value semantics. MSVC requires that we spell all of these out.
  XtensaTTIImpl(const XtensaTTIImpl &Arg)
      : BaseT(static_cast<const BaseT &>(Arg)), ST(Arg.ST), TLI(Arg.TLI),
        Ctx(Arg.Ctx) {}
  XtensaTTIImpl(XtensaTTIImpl &&Arg)
      : BaseT(std::move(static_cast<BaseT &>(Arg))), ST(std::move(Arg.ST)),
        TLI(std::move(Arg.TLI)), Ctx(Arg.Ctx) {}

  bool areInlineCompatible(const Function *Caller,
                           const Function *Callee) const;

  bool enableInterleavedAccessVectorization() const { return true; }

  unsigned getNumberOfRegisters(bool Vector) const;

  unsigned getRegisterBitWidth(bool Vector) const;

  unsigned getMaxInterleaveFactor(unsigned VF) const;

  unsigned getLoadStoreVecRegBitWidth(unsigned AS) const;

  bool isBBNotForHWLoop(BasicBlock *BB, TargetLibraryInfo *LibInfo) const;

  bool isHardwareLoopProfitable(Loop *L, ScalarEvolution &SE,
                                AssumptionCache &AC,
                                TargetLibraryInfo *LibInfo,
                                HardwareLoopInfo &HWLoopInfo);

  void getUnrollingPreferences(Loop *L, ScalarEvolution &SE,
                               TTI::UnrollingPreferences &UP) const;

  int getVectorInstrCost(unsigned Opcode, Type *Val, unsigned Index) const;

  bool isLegalArithmeticInstr(
      unsigned Opcode, Type *Ty,
      TTI::OperandValueKind Opd1Info = TTI::OK_AnyValue,
      TTI::OperandValueKind Opd2Info = TTI::OK_AnyValue,
      TTI::OperandValueProperties Opd1PropInfo = TTI::OP_None,
      TTI::OperandValueProperties Opd2PropInfo = TTI::OP_None,
      ArrayRef<const Value *> Args = ArrayRef<const Value *>());

  bool isLegalCmpSelInstr(unsigned Opcode, Type *ValTy, Type *CondTy,
                          const Instruction *I);

  bool isLegalCastInstr(unsigned Opcode, Type *Dst, Type *Src,
                        const Instruction *I);

  int getArithmeticInstrCost(
      unsigned Opcode, Type *Ty,
      TTI::OperandValueKind Opd1Info = TTI::OK_AnyValue,
      TTI::OperandValueKind Opd2Info = TTI::OK_AnyValue,
      TTI::OperandValueProperties Opd1PropInfo = TTI::OP_None,
      TTI::OperandValueProperties Opd2PropInfo = TTI::OP_None,
      ArrayRef<const Value *> Args = ArrayRef<const Value *>(),
      const Instruction *CxtI = nullptr);

  int getCmpSelInstrCost(unsigned Opcode, Type *ValTy, Type *CondTy,
                         const Instruction *I);

  int getShuffleCost(TTI::ShuffleKind Kind, Type *Ty, int Index, Type *SubTp);

  unsigned getScalarizationOverhead(Type *Ty, bool Insert, bool Extract);

  int getMemoryOpCost(unsigned Opcode, Type *Src, MaybeAlign Alignment,
                      unsigned AddressSpace,  const Instruction *I = nullptr);

  bool isLegalMaskedLoad(Type *DataType, MaybeAlign Alignment);
  bool isLegalMaskedStore(Type *DataType, MaybeAlign Alignment);
  bool isLegalMaskedGatherInst(CallInst *CI);
  bool isLegalMaskedScatterInst(CallInst *CI);
  bool isLegalInterleavedAccessType(unsigned Opcode, Type *VecTy,
                                    unsigned Factor, bool NeedsSExt);
  int getInterleavedMemoryOpCostWithSignedness(unsigned Opcode, Type *VecTy, unsigned Factor,
                                 ArrayRef<unsigned> Indices, unsigned Alignment,
                                 unsigned AddressSpace,
                                 bool NeedsSExt);
  bool lowerInterleavedLoadValues(SmallVectorImpl<Value *> &InputVectors,
                                  SmallVectorImpl<Value *> &OutputVectors,
                                  unsigned Factor, IRBuilder<> &Builder);
  bool lowerInterleavedStoreValues(SmallVectorImpl<Value *> &InputVectors,
                                   SmallVectorImpl<Value *> &OutputVectors,
                                   unsigned Factor, IRBuilder<> &Builder);
  bool allowsVectorization(std::function<void(const char *)> RemarkCallBack);
  const Type *getTargetComplexFloatType();
  const Type *getTargetComplexDoubleType();
  const TargetComplexOpInfo *getTargetComplexFloatOpInfo();
  const TargetComplexOpInfo *getTargetComplexDoubleOpInfo();
  TargetSIMDInfo *getTargetSIMDInfo();
  int getCastInstrCost(unsigned Opcode, Type *Dst, Type *Src,
                       const Instruction *I = nullptr);
  int getIntrinsicInstrCost(Intrinsic::ID IID, Type *RetTy,
                            ArrayRef<Type *> Tys, FastMathFlags FMF,
                            unsigned VF = 1);
  int getIntrinsicInstrCost(Intrinsic::ID IID, Type *RetTy,
                            ArrayRef<Value *> Args, FastMathFlags FMF,
                            unsigned VF = 1);
  int getUintToFpInstrCost(unsigned Opcode, Type *Dst, Type *Src,
                           const Instruction *I = nullptr);
  bool getTgtMemIntrinsic(IntrinsicInst *Inst, MemIntrinsicInfo &Info) const;

  unsigned getNumberOfParts(Type *Tp);

  bool isLSRCostLess(TargetTransformInfo::LSRCost &C1,
                     TargetTransformInfo::LSRCost &C2);
};

} // end namespace llvm

#endif
