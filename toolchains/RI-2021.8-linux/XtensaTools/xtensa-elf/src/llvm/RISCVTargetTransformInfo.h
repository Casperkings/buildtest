//===- RISCVTargetTransformInfo.h - RISC-V specific TTI ---------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file
/// This file defines a TargetTransformInfo::Concept conforming object specific
/// to the RISC-V target machine. It uses the target's detailed information to
/// provide more precise answers to certain TTI queries, while letting the
/// target independent and default TTI implementations handle the rest.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_RISCV_RISCVTARGETTRANSFORMINFO_H
#define LLVM_LIB_TARGET_RISCV_RISCVTARGETTRANSFORMINFO_H

#include "RISCVSubtarget.h"
#include "RISCVTargetMachine.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/CodeGen/BasicTTIImpl.h"
#include "llvm/IR/Function.h"
#if defined(TENSILICA) || 1
#include "Tensilica/TensilicaTargetSIMDInfo.h"
#endif

namespace llvm {

class RISCVTTIImpl : public BasicTTIImplBase<RISCVTTIImpl> {
  using BaseT = BasicTTIImplBase<RISCVTTIImpl>;
  using TTI = TargetTransformInfo;

  friend BaseT;

  const RISCVSubtarget *ST;
  const RISCVTargetLowering *TLI;

  const RISCVSubtarget *getST() const { return ST; }
  const RISCVTargetLowering *getTLI() const { return TLI; }

#if defined(TENSILICA) || 1
  LLVMContext &Ctx;
#endif
public:
  explicit RISCVTTIImpl(const RISCVTargetMachine *TM, const Function &F)
      : BaseT(TM, F.getParent()->getDataLayout()), ST(TM->getSubtargetImpl(F)),
#if defined(TENSILICA) || 1
        TLI(ST->getTargetLowering()), Ctx(F.getContext()) {}
#else
        TLI(ST->getTargetLowering()) {}
#endif

  int getIntImmCost(const APInt &Imm, Type *Ty);
  int getIntImmCostInst(unsigned Opcode, unsigned Idx, const APInt &Imm, Type *Ty);
  int getIntImmCostIntrin(Intrinsic::ID IID, unsigned Idx, const APInt &Imm,
                          Type *Ty);

#if defined(TENSILICA) || 1
  TargetSIMDInfo *getTargetSIMDInfo();

  bool enableInterleavedAccessVectorization() const { return true; }

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
  bool isBBNotForHWLoop(BasicBlock *BB,
                        TargetLibraryInfo *LibInfo) const;
  bool isHardwareLoopProfitable(Loop *L, ScalarEvolution &SE,
                                AssumptionCache &AC,
                                TargetLibraryInfo *LibInfo,
                                HardwareLoopInfo &HWLoopInfo);

  bool areInlineCompatible(const Function *Caller,
                           const Function *Callee) const;
  unsigned getRegisterBitWidth(bool Vector) const;
  unsigned getMaxInterleaveFactor(unsigned VF) const;
  bool getTgtMemIntrinsic(IntrinsicInst *Inst, MemIntrinsicInfo &Info) const;
  unsigned getLoadStoreVecRegBitWidth(unsigned AS) const;
  unsigned getNumberOfRegisters(bool Vector) const;
  int getVectorInstrCost(unsigned Opcode, Type *Val, unsigned Index) const;
  bool isLegalArithmeticInstr(
      unsigned Opcode, Type *Ty,
      TTI::OperandValueKind Opd1Info = TTI::OK_AnyValue,
      TTI::OperandValueKind Opd2Info = TTI::OK_AnyValue,
      TTI::OperandValueProperties Opd1PropInfo = TTI::OP_None,
      TTI::OperandValueProperties Opd2PropInfo = TTI::OP_None,
      ArrayRef<const Value *> Args = ArrayRef<const Value *>());
  int getArithmeticInstrCost(
      unsigned Opcode, Type *Ty,
      TTI::OperandValueKind Opd1Info = TTI::OK_AnyValue,
      TTI::OperandValueKind Opd2Info = TTI::OK_AnyValue,
      TTI::OperandValueProperties Opd1PropInfo = TTI::OP_None,
      TTI::OperandValueProperties Opd2PropInfo = TTI::OP_None,
      ArrayRef<const Value *> Args = ArrayRef<const Value *>(),
      const Instruction *CxtI = nullptr);
  int getShuffleCost(TTI::ShuffleKind Kind, Type *Ty, int Index, Type *SubTp);
  int getCmpSelInstrCost(unsigned Opcode, Type *ValTy, Type *CondTy,
                         const Instruction *I);
  bool isLegalCastInstr(unsigned Opcode, Type *Dst, Type *Src,
                        const Instruction *I);
  bool isLegalCmpSelInstr(unsigned Opcode, Type *ValTy, Type *CondTy,
                          const Instruction *I);
  unsigned getScalarizationOverhead(Type *Ty, bool Insert, bool Extract);
  int getMemoryOpCost(unsigned Opcode, Type *Src, MaybeAlign Alignment,
                      unsigned AddressSpace, const Instruction *I = nullptr);
  bool isLegalMaskedLoad(Type *DataType, MaybeAlign Alignment);
  bool isLegalMaskedStore(Type *DataType, MaybeAlign Alignment);
  bool allowsVectorization(std::function<void(const char *)> RemarkCallBack);
  const Type *getTargetComplexFloatType();
  const Type *getTargetComplexDoubleType();
  const TargetComplexOpInfo *getTargetComplexFloatOpInfo();
  const TargetComplexOpInfo *getTargetComplexDoubleOpInfo();
  bool isLegalMaskedGatherInst(CallInst *CI);
  bool isLegalMaskedScatterInst(CallInst *CI);
  int getIntrinsicInstrCost(Intrinsic::ID IID, Type *RetTy,
                            ArrayRef<Type *> Tys, FastMathFlags FMF,
                            unsigned VF = 1);
  int getIntrinsicInstrCost(Intrinsic::ID IID, Type *RetTy,
                            ArrayRef<Value *> Args, FastMathFlags FMF,
                            unsigned VF = 1);
  int getUintToFpInstrCost(unsigned Opcode, Type *Dst, Type *Src,
                           const Instruction *I = nullptr);
  int getCastInstrCost(unsigned Opcode, Type *Dst, Type *Src,
                       const Instruction *I = nullptr);
  unsigned getNumberOfParts(Type *Tp);
  bool isLSRCostLess(TargetTransformInfo::LSRCost &C1,
                     TargetTransformInfo::LSRCost &C2);
  void getUnrollingPreferences(Loop *L, ScalarEvolution &SE,
                               TTI::UnrollingPreferences &UP);
#endif
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_RISCV_RISCVTARGETTRANSFORMINFO_H
