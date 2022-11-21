//===-- RISCVTargetTransformInfo.cpp - RISC-V specific TTI ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RISCVTargetTransformInfo.h"
#include "Utils/RISCVMatInt.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/CodeGen/BasicTTIImpl.h"
#include "llvm/CodeGen/TargetLowering.h"
#if defined(TENSILICA) || 1
#include "RISCVConfig/RISCVConfigInfo.h"
#include "Tensilica/TensilicaInterleavedAccess.h"
#include "Tensilica/TensilicaComplexTypeInfo.h"
#include "Tensilica/TensilicaIntrnProperty.h"
#include "llvm/CodeGen/Analysis.h"
#include "llvm/CodeGen/CostTable.h"
#include "llvm/CodeGen/TargetSchedule.h"
#include "llvm/Analysis/CodeMetrics.h"
#endif
using namespace llvm;

#define DEBUG_TYPE "riscvtti"

int RISCVTTIImpl::getIntImmCost(const APInt &Imm, Type *Ty) {
  assert(Ty->isIntegerTy() &&
         "getIntImmCost can only estimate cost of materialising integers");

  // We have a Zero register, so 0 is always free.
  if (Imm == 0)
    return TTI::TCC_Free;

  // Otherwise, we check how many instructions it will take to materialise.
  const DataLayout &DL = getDataLayout();
  return RISCVMatInt::getIntMatCost(Imm, DL.getTypeSizeInBits(Ty),
                                    getST()->is64Bit());
}

int RISCVTTIImpl::getIntImmCostInst(unsigned Opcode, unsigned Idx, const APInt &Imm,
                                Type *Ty) {
  assert(Ty->isIntegerTy() &&
         "getIntImmCost can only estimate cost of materialising integers");

  // We have a Zero register, so 0 is always free.
  if (Imm == 0)
    return TTI::TCC_Free;

  // Some instructions in RISC-V can take a 12-bit immediate. Some of these are
  // commutative, in others the immediate comes from a specific argument index.
  bool Takes12BitImm = false;
  unsigned ImmArgIdx = ~0U;

  switch (Opcode) {
  case Instruction::GetElementPtr:
    // Never hoist any arguments to a GetElementPtr. CodeGenPrepare will
    // split up large offsets in GEP into better parts than ConstantHoisting
    // can.
    return TTI::TCC_Free;
  case Instruction::Add:
  case Instruction::And:
  case Instruction::Or:
  case Instruction::Xor:
  case Instruction::Mul:
    Takes12BitImm = true;
    break;
  case Instruction::Sub:
  case Instruction::Shl:
  case Instruction::LShr:
  case Instruction::AShr:
    Takes12BitImm = true;
    ImmArgIdx = 1;
    break;
  default:
    break;
  }

  if (Takes12BitImm) {
    // Check immediate is the correct argument...
    if (Instruction::isCommutative(Opcode) || Idx == ImmArgIdx) {
      // ... and fits into the 12-bit immediate.
      if (Imm.getMinSignedBits() <= 64 &&
          getTLI()->isLegalAddImmediate(Imm.getSExtValue())) {
        return TTI::TCC_Free;
      }
    }

    // Otherwise, use the full materialisation cost.
    return getIntImmCost(Imm, Ty);
  }

  // By default, prevent hoisting.
  return TTI::TCC_Free;
}

int RISCVTTIImpl::getIntImmCostIntrin(Intrinsic::ID IID, unsigned Idx,
                                      const APInt &Imm, Type *Ty) {
  // Prevent hoisting in unknown cases.
  return TTI::TCC_Free;
}

#if defined(TENSILICA) || 1
TargetSIMDInfo *RISCVTTIImpl::getTargetSIMDInfo() {
  return TensilicaTargetSIMDInfo::getInstance(
      Ctx, TLI, ST->getRISCVConfigInfo().SIMDInfo);
}

bool RISCVTTIImpl::lowerInterleavedLoadValues(
    SmallVectorImpl<Value *> &InputVectors,
    SmallVectorImpl<Value *> &OutputVectors, unsigned Factor,
    IRBuilder<> &Builder) {
  Tensilica::TensilicaInterleavedAccess IA;
  return IA.lowerInterleavedLoadValues(InputVectors, OutputVectors, Factor,
                                       Builder, getTargetSIMDInfo());
}

bool RISCVTTIImpl::lowerInterleavedStoreValues(
    SmallVectorImpl<Value *> &InputVectors,
    SmallVectorImpl<Value *> &OutputVectors, unsigned Factor,
    IRBuilder<> &Builder) {
  Tensilica::TensilicaInterleavedAccess IA;
  return IA.lowerInterleavedStoreValues(InputVectors, OutputVectors, Factor,
                                        Builder, getTargetSIMDInfo());
}

bool RISCVTTIImpl::isLegalInterleavedAccessType(unsigned Opcode, Type *VecTy,
                                                unsigned Factor,
                                                bool NeedsSExt) {
  assert(Factor >= 2 && "Invalid interleave factor");
  assert(isa<VectorType>(VecTy) && "Expect a vector type");
  if (Factor > TLI->getMaxSupportedInterleaveFactor())
    return false;
  VectorType *SubVecTy = cast<VectorType>(VecTy);
  Tensilica::TensilicaInterleavedAccess IA;
  return IA.isInterleavedAccessTypeSupported(
      Opcode, SubVecTy, Factor, NeedsSExt, TLI, getTargetSIMDInfo());
}

int RISCVTTIImpl::getInterleavedMemoryOpCostWithSignedness(
    unsigned Opcode, Type *VecTy, unsigned Factor, ArrayRef<unsigned> Indices,
    unsigned Alignment, unsigned AddressSpace, bool NeedsSExt) {
  assert(Factor >= 2 && "Invalid interleave factor");
  assert(isa<VectorType>(VecTy) && "Expect a vector type");

  unsigned NumElts = VecTy->getVectorNumElements();
  auto *SubVecTy = VectorType::get(VecTy->getScalarType(), NumElts / Factor);

  Tensilica::TensilicaInterleavedAccess IA;

  if (isLegalInterleavedAccessType(Opcode, SubVecTy, Factor, NeedsSExt)) {
    // FIXME: need consider if the loads/stores are aligned or not.
    return Factor /* number of loads/stores*/ +
           IA.getInterleavedAccessCost(Opcode, SubVecTy, Factor, Indices,
                                       Alignment, getTargetSIMDInfo());
  }
  assert(!VecTy->getVectorElementType()->isCustomTy() &&
         "vector element type cannot be custom type!");
  return BaseT::getInterleavedMemoryOpCost(Opcode, VecTy, Factor, Indices,
                                           Alignment, AddressSpace);
}

bool RISCVTTIImpl::isBBNotForHWLoop(BasicBlock *BB,
                                    TargetLibraryInfo *LibInfo) const {
  auto isLargeIntegerTy = [](Type *Ty) {
    if (IntegerType *ITy = dyn_cast<IntegerType>(Ty))
      return ITy->getBitWidth() > 32U;

    return false;
  };

  for (BasicBlock::iterator J = BB->begin(), JE = BB->end();
       J != JE; ++J) {
    if (CallInst *CI = dyn_cast<CallInst>(J)) {
      if (CI->isInlineAsm()) {
        return true;
      }

      if (Function *F = CI->getCalledFunction()) {
        // Most intrinsics don't become function calls, but some might.
        // sin, cos, exp and log are always calls.
        unsigned Opcode = 0;
        if (F->getIntrinsicID() != Intrinsic::not_intrinsic) {
          switch (F->getIntrinsicID()) {
          default: continue;
          case Intrinsic::set_loop_iterations:
          case Intrinsic::loop_decrement:
            return true;

          // Exclude eh_sjlj_setjmp; we don't need to exclude eh_sjlj_longjmp
          // because, although it does clobber the counter register, the
          // control can't then return to inside the loop unless there is also
          // an eh_sjlj_setjmp.
          case Intrinsic::eh_sjlj_setjmp:

          case Intrinsic::powi:
          case Intrinsic::log:
          case Intrinsic::log2:
          case Intrinsic::log10:
          case Intrinsic::exp:
          case Intrinsic::exp2:
          case Intrinsic::pow:
          case Intrinsic::sin:
          case Intrinsic::cos:
            return true;
          case Intrinsic::sqrt:               Opcode = ISD::FSQRT;      break;
          case Intrinsic::floor:              Opcode = ISD::FFLOOR;     break;
          case Intrinsic::ceil:               Opcode = ISD::FCEIL;      break;
          case Intrinsic::trunc:              Opcode = ISD::FTRUNC;     break;
          case Intrinsic::rint:               Opcode = ISD::FRINT;      break;
          case Intrinsic::lrint:              Opcode = ISD::LRINT;      break;
          case Intrinsic::llrint:             Opcode = ISD::LLRINT;     break;
          case Intrinsic::nearbyint:          Opcode = ISD::FNEARBYINT; break;
          case Intrinsic::round:              Opcode = ISD::FROUND;     break;
          case Intrinsic::lround:             Opcode = ISD::LROUND;     break;
          case Intrinsic::llround:            Opcode = ISD::LLROUND;    break;
          case Intrinsic::minnum:             Opcode = ISD::FMINNUM;    break;
          case Intrinsic::maxnum:             Opcode = ISD::FMAXNUM;    break;
          case Intrinsic::umul_with_overflow: Opcode = ISD::UMULO;      break;
          case Intrinsic::smul_with_overflow: Opcode = ISD::SMULO;      break;
          }
        }

        // PowerPC does not use [US]DIVREM or other library calls for
        // operations on regular types which are not otherwise library calls
        // (i.e. soft float or atomics). If adapting for targets that do,
        // additional care is required here.

        LibFunc Func;
        if (!F->hasLocalLinkage() && F->hasName() && LibInfo &&
            LibInfo->getLibFunc(F->getName(), Func) &&
            LibInfo->hasOptimizedCodeGen(Func)) {
          // Non-read-only functions are never treated as intrinsics.
          if (!CI->onlyReadsMemory())
            return true;

          // Conversion happens only for FP calls.
          if (!CI->getArgOperand(0)->getType()->isFloatingPointTy())
            return true;

          switch (Func) {
          default: return true;
          case LibFunc_copysign:
          case LibFunc_copysignf:
            continue; // ISD::FCOPYSIGN is never a library call.
          case LibFunc_copysignl:
            return true;
          case LibFunc_fabs:
          case LibFunc_fabsf:
          case LibFunc_fabsl:
            continue; // ISD::FABS is never a library call.
          case LibFunc_sqrt:
          case LibFunc_sqrtf:
          case LibFunc_sqrtl:
            Opcode = ISD::FSQRT; break;
          case LibFunc_floor:
          case LibFunc_floorf:
          case LibFunc_floorl:
            Opcode = ISD::FFLOOR; break;
          case LibFunc_nearbyint:
          case LibFunc_nearbyintf:
          case LibFunc_nearbyintl:
            Opcode = ISD::FNEARBYINT; break;
          case LibFunc_ceil:
          case LibFunc_ceilf:
          case LibFunc_ceill:
            Opcode = ISD::FCEIL; break;
          case LibFunc_rint:
          case LibFunc_rintf:
          case LibFunc_rintl:
            Opcode = ISD::FRINT; break;
          case LibFunc_round:
          case LibFunc_roundf:
          case LibFunc_roundl:
            Opcode = ISD::FROUND; break;
          case LibFunc_trunc:
          case LibFunc_truncf:
          case LibFunc_truncl:
            Opcode = ISD::FTRUNC; break;
          case LibFunc_fmin:
          case LibFunc_fminf:
          case LibFunc_fminl:
            Opcode = ISD::FMINNUM; break;
          case LibFunc_fmax:
          case LibFunc_fmaxf:
          case LibFunc_fmaxl:
            Opcode = ISD::FMAXNUM; break;
          }
        }

        if (Opcode) {
          EVT EVTy =
              TLI->getValueType(DL, CI->getArgOperand(0)->getType(), true);

          if (EVTy == MVT::Other)
            return true;

          if (TLI->isOperationLegalOrCustom(Opcode, EVTy))
            continue;
          else if (EVTy.isVector() &&
                   TLI->isOperationLegalOrCustom(Opcode, EVTy.getScalarType()))
            continue;

          return true;
        }
      }

      return true;
    } else if (isa<UIToFPInst>(J) || isa<SIToFPInst>(J) ||
               isa<FPToUIInst>(J) || isa<FPToSIInst>(J)) {
        return true;
    } else if (isLargeIntegerTy(J->getType()->getScalarType()) &&
               (J->getOpcode() == Instruction::UDiv ||
                J->getOpcode() == Instruction::SDiv ||
                J->getOpcode() == Instruction::URem ||
                J->getOpcode() == Instruction::SRem)) {
      return true;
    } else if (isa<IndirectBrInst>(J) || isa<InvokeInst>(J)) {
      // On PowerPC, indirect jumps use the counter register.
      return true;
    } else if (SwitchInst *SI = dyn_cast<SwitchInst>(J)) {
      if (SI->getNumCases() + 1 >= (unsigned)TLI->getMinimumJumpTableEntries())
        return true;
    }

    if ((J->getType()->isFloatTy() && !ST->hasHWF32()) ||
        (J->getType()->isDoubleTy() && !ST->hasHWF64())) {
      switch(J->getOpcode()) {
      case Instruction::FAdd:
      case Instruction::FSub:
      case Instruction::FMul:
      case Instruction::FDiv:
      case Instruction::FPTrunc:
      case Instruction::FPExt:
      case Instruction::FPToUI:
      case Instruction::FPToSI:
      case Instruction::UIToFP:
      case Instruction::SIToFP:
      case Instruction::FCmp:
        return true;
      }
    }
    if (J->getOpcode() == Instruction::FDiv ||
        J->getOpcode() == Instruction::FCmp) {
    int ISDOpcode = TLI->InstructionOpcodeToISD(J->getOpcode());
    if (!ISDOpcode)
      continue;
    if (!TLI->isOperationLegalOrCustom(ISDOpcode, 
            TLI->getValueType(DL, J->getType(), true)))
      return true;
    }
  }

  return false;
}

bool RISCVTTIImpl::isHardwareLoopProfitable(Loop *L, ScalarEvolution &SE,
                                            AssumptionCache &AC,
                                            TargetLibraryInfo *LibInfo,
                                            HardwareLoopInfo &HWLoopInfo) {
  for (Loop::block_iterator I = L->block_begin(), IE = L->block_end();
       I != IE; ++I) {
    if (isBBNotForHWLoop(*I, LibInfo))
      return false;
  }
  SmallPtrSet<const Value *, 32> EphValues;
  CodeMetrics::collectEphemeralValues(L, &AC, EphValues);
  CodeMetrics Metrics;
  for (BasicBlock *BB : L->blocks())
    Metrics.analyzeBasicBlock(BB, *this, EphValues);
//  dbgs() << "Num insts " << Metrics.NumInsts << "\n";
  if (Metrics.NumInsts > 1000)
    return false;

  // If there is an exit edge known to be frequently taken,
  // we should not transform this loop.
  SmallVector<BasicBlock*, 4> ExitingBlocks;
  L->getExitingBlocks(ExitingBlocks);
  for (auto &BB : ExitingBlocks) {
    Instruction *TI = BB->getTerminator();
    if (!TI) continue;

    if (BranchInst *BI = dyn_cast<BranchInst>(TI)) {
      uint64_t TrueWeight = 0, FalseWeight = 0;
      if (!BI->isConditional() ||
          !BI->extractProfMetadata(TrueWeight, FalseWeight))
        continue;

      // If the exit path is more frequent than the loop path,
      // we return here without further analysis for this loop.
      bool TrueIsExit = !L->contains(BI->getSuccessor(0));
      if (( TrueIsExit && FalseWeight < TrueWeight) ||
          (!TrueIsExit && FalseWeight > TrueWeight))
        return false;
    }
  }

  LLVMContext &C = L->getHeader()->getContext();
  HWLoopInfo.CountType = Type::getInt32Ty(C);
  HWLoopInfo.LoopDecrement = ConstantInt::get(HWLoopInfo.CountType, 1);
  HWLoopInfo.CounterInReg = true;
  return true;
}

static constexpr int IllegalInstrCost = 999;

bool RISCVTTIImpl::areInlineCompatible(const Function *Caller,
                                       const Function *Callee) const {
  // Check for overlays compatibility.

  if (Callee->hasFnAttribute(Attribute::AlwaysInline))
    return true;

  // todo: Implement cost model for various scenarios with overlays,
  // e.g. inlining of tiny functions.

  // Avoid code size bloating if an overlay function is involved.
  if (Caller->hasFnAttribute("xtensa-overlay")) {
    return false;
  }

  if (Callee->hasFnAttribute("xtensa-overlay")) {
    return false;
  }

  return true;
}

unsigned RISCVTTIImpl::getRegisterBitWidth(bool Vector) const {
  if (!Vector)
    return 32;

  unsigned MaxBitWidth = 0;
  // Compute the maximimum bitwidth across all register files
  // that support the vector types
  const TargetRegisterInfo *TRI = ST->getRegisterInfo();
  for (auto RC : make_range(TRI->regclass_begin(), TRI->regclass_end())) {
    if (!RC->isAllocatable())
      continue;

    // MaxBitWidth should be decided by the WVEC register class. Otherwise,
    // vectorization on xb_in40 may not be able to find suitable vectorization
    // factors. Do we really need go through register classes to get the
    // getRegisterBitWidth
    MaxBitWidth = std::max(MaxBitWidth, TRI->getRegSizeInBits(*RC));
#if 0
    TargetRegisterClass::vt_iterator VI , VE;
    for (VI = RC->vt_begin(), VE = RC->vt_end(); VI != VE; ++VI) {
      if (*VI >= MVT::FIRST_VECTOR_VALUETYPE &&
          *VI <= MVT::LAST_VECTOR_VALUETYPE) {
        MaxBitWidth = std::max(MaxBitWidth, MVT(*VI).getSizeInBits()); 
        break;
      }
    }
#endif
  }

  return MaxBitWidth;
}

unsigned RISCVTTIImpl::getMaxInterleaveFactor(unsigned VF) const { return 1; }

bool RISCVTTIImpl::getTgtMemIntrinsic(IntrinsicInst *Inst,
                                      MemIntrinsicInfo &Info) const {
  const TargetIntrinsicInfo *II = ST->getTargetMachine().getIntrinsicInfo();
  auto Intrinsic = Inst->getIntrinsicID();
  const Tensilica::IntrnProperty *IntrnProp = II->getIntrnProperty(Intrinsic);
  if (!IntrnProp->isMemIntrn())
    return false;
  if (!IntrnProp->isBaseDispImm())
    return false;
  auto OffsetOpnd = Inst->getArgOperand(IntrnProp->getOffsetOpnd());
  auto OffsetVal = dyn_cast<ConstantInt>(OffsetOpnd);
  if (!OffsetVal || OffsetVal->getSExtValue() != 0)
    return false;
  Info.PtrVal = Inst->getArgOperand(IntrnProp->getBaseOpnd());
  Info.MatchingId = -1;
  if (IntrnProp->isLoad() && IntrnProp->isStore()) {
    Info.ReadMem = true;
    Info.WriteMem = true;
  } else if (IntrnProp->isLoad()) {
    Info.ReadMem = true;
    Info.WriteMem = false;
  } else if (IntrnProp->isStore()) {
    Info.ReadMem = false;
    Info.WriteMem = true;
  }
  return true;
}

unsigned RISCVTTIImpl::getLoadStoreVecRegBitWidth(unsigned AS) const {
  return ST->loadStoreWidth() * 8;
}

unsigned RISCVTTIImpl::getNumberOfRegisters(bool Vector) const {
  if (!Vector)
    return 16;

  // Compute the maximimum number of registers across all register files
  // that support the vector types
  unsigned MaxNumRegs = 0;
  const TargetRegisterInfo *TRI = ST->getRegisterInfo();
  for (auto RC : make_range(TRI->regclass_begin(), TRI->regclass_end())) {
    if (!RC->isAllocatable())
      continue;
    for (auto &VI : make_range(TRI->legalclasstypes_begin(*RC),
                               TRI->legalclasstypes_end(*RC))) {
      if (VI >= MVT::FIRST_VECTOR_VALUETYPE &&
          VI <= MVT::LAST_VECTOR_VALUETYPE) {
        MaxNumRegs = std::max(MaxNumRegs, RC->getNumRegs());
        break;
      }
    }
  }
  return MaxNumRegs;
}

int RISCVTTIImpl::getVectorInstrCost(unsigned Opcode, Type *Val,
                                     unsigned Index) const {
  assert(Val->isVectorTy() && "This must be a vector type");
  int ISD = TLI->InstructionOpcodeToISD(Opcode);

  EVT MTy = TLI->getValueType(DL, Val);
  if (!(TLI->isTypeLegal(MTy) &&
        TLI->getOperationAction(ISD, MTy) == TargetLoweringBase::Custom))
    return IllegalInstrCost;

  if (Index != -1U) {
    // Legalize the type.
    std::pair<int, MVT> LT = TLI->getTypeLegalizationCost(DL, Val);

    // This type is legalized to a scalar type.
    if (!LT.second.isVector())
      return 0;

    // The type may be split. Normalize the index to the new type.
    unsigned Width = LT.second.getVectorNumElements();
    Index = Index % Width;

    // The element at index zero is already inside the vector.
    if (Index == 0)
      return 0;
  }

  // All other insert/extracts cost this much.
  return 3;
}

static const CostTblEntry InstCostTbl[] = {
    {ISD::VSELECT, MVT::i32, 1}, // dummy cost
#define GET_INST_COST_TABLE
#include "TensilicaGenOpAction.inc"
};

static const CostTblEntry CmpCostTbl[] = {
    {ISD::VSELECT, MVT::i32, 1}, // dummy cost
#define GET_CMP_COST_TABLE
#include "TensilicaGenOpAction.inc"
};

bool RISCVTTIImpl::isLegalArithmeticInstr(
    unsigned Opcode, Type *Ty, TTI::OperandValueKind Opd1Info,
    TTI::OperandValueKind Opd2Info, TTI::OperandValueProperties Opd1PropInfo,
    TTI::OperandValueProperties Opd2PropInfo, ArrayRef<const Value *> Args) {
  int Cost = getArithmeticInstrCost(Opcode, Ty, Opd1Info, Opd2Info,
                                    Opd1PropInfo, Opd2PropInfo, Args);
  if (Cost >= IllegalInstrCost)
    return false;

  return true;
}

// TODO: make it more accurate with cost table (CostTable.h).
int RISCVTTIImpl::getArithmeticInstrCost(
    unsigned Opcode, Type *Ty, TTI::OperandValueKind Opd1Info,
    TTI::OperandValueKind Opd2Info, TTI::OperandValueProperties Opd1PropInfo,
    TTI::OperandValueProperties Opd2PropInfo, ArrayRef<const Value *> Args,
    const Instruction *CxtI) {

  int ISD = TLI->InstructionOpcodeToISD(Opcode);
  assert(ISD && "Invalid opcode");

  // Handling illegal i1 vectors can be expensive.
  EVT ValVT = TLI->getValueType(DL, Ty);
  if (ValVT.isVector() && ValVT.getVectorElementType() == MVT::i1) {
    if (!TLI->isTypeLegal(ValVT))
      return IllegalInstrCost; // Splitting i1 vectors is expensive
  }

  // Legalize the type.
  std::pair<int, MVT> LT = TLI->getTypeLegalizationCost(DL, Ty);

  if (Ty->isVectorTy() &&
      TLI->getOperationAction(ISD, LT.second) != TargetLoweringBase::Custom)
    return IllegalInstrCost;

  return BaseT::getArithmeticInstrCost(Opcode, Ty, Opd1Info, Opd2Info,
                                       Opd1PropInfo, Opd2PropInfo, CxtI);
}

int RISCVTTIImpl::getShuffleCost(TTI::ShuffleKind Kind, Type *Ty, int Index,
                                 Type *SubTp) {
  EVT MTy = TLI->getValueType(DL, Ty);
  if (MTy.isSimple() && TLI->getOperationAction(ISD::VECTOR_SHUFFLE, MTy) ==
                            TargetLoweringBase::Custom)
    return 1;

  return BaseT::getShuffleCost(Kind, Ty, Index, SubTp);
}

int RISCVTTIImpl::getCmpSelInstrCost(unsigned Opcode, Type *ValTy, Type *CondTy,
                                     const Instruction *I) {
  if (!ValTy->isVectorTy())
    return BaseT::getCmpSelInstrCost(Opcode, ValTy, CondTy, I);

  EVT ValVT = TLI->getValueType(DL, ValTy);
  if (!ValVT.isSimple())
    return IllegalInstrCost;

  // Legalize the type.
  std::pair<int, MVT> LT = TLI->getTypeLegalizationCost(DL, ValTy);
  if (!LT.second.isVector())
    return IllegalInstrCost;

  // Check CondTy
  EVT CondVT;
  if (CondTy)
    CondVT = TLI->getValueType(DL, CondTy);
  else
    CondVT = EVT::getVectorVT(ValTy->getContext(), MVT::i1,
                              ValVT.getVectorNumElements());
  // Note, assume no non-view struct i1 vector types.
  if (!TLI->isTypeLegal(CondVT)) {
    return IllegalInstrCost;
  }

  int ISD = TLI->InstructionOpcodeToISD(Opcode);

  EVT NewCondVT = CondVT;
  if (ValVT != LT.second) {
    unsigned LTNumElems = LT.second.getVectorNumElements();
    NewCondVT = MVT::getVectorVT(MVT::i1, LTNumElems);
    if (!TLI->isTypeLegal(NewCondVT) ||
        LTNumElems * 2 != ValVT.getVectorNumElements())
      return IllegalInstrCost;
  }

  if (ISD == ISD::SELECT) {
    ISD = ISD::VSELECT;
    int CondCost = 0;
    if (NewCondVT != CondVT) {
      MVT NewCondMVT = NewCondVT.getSimpleVT();
      const auto *Entry = CostTableLookup(
          InstCostTbl, RISCVISD::EXTRACT_SUBVECTOR_LO_HALF, NewCondMVT);
      if (!Entry)
        return IllegalInstrCost;

      CondCost += Entry->Cost;

      Entry = CostTableLookup(InstCostTbl, RISCVISD::EXTRACT_SUBVECTOR_UP_HALF,
                              NewCondMVT);
      if (!Entry)
        return IllegalInstrCost;

      CondCost += Entry->Cost;
    }

    if (const auto *Entry = CostTableLookup(InstCostTbl, ISD, LT.second))
      return Entry->Cost * LT.first + CondCost;
    return IllegalInstrCost;
  }

  assert(ISD == ISD::SETCC && "expect SETCC");
  if (!I)
    return IllegalInstrCost;

  int Cond;
  if (const ICmpInst *IC = dyn_cast<ICmpInst>(I)) {
    ICmpInst::Predicate Pred = IC->getPredicate();
    Cond = getICmpCondCode(Pred);
  } else {
    const FCmpInst *FC = cast<FCmpInst>(I);
    FCmpInst::Predicate Pred = FC->getPredicate();
    Cond = getFCmpCondCode(Pred);
  }

  int CondCost = 0;
  if (NewCondVT != CondVT) {
    MVT CondMVT = CondVT.getSimpleVT();
    const auto *Entry =
        CostTableLookup(InstCostTbl, RISCVISD::CONCAT_HALF_VECTORS, CondMVT);
    if (!Entry)
      return IllegalInstrCost;
    CondCost += Entry->Cost;
  }

  if (const auto *Entry = CostTableLookup(CmpCostTbl, Cond, LT.second))
    return Entry->Cost * LT.first + CondCost;
  return IllegalInstrCost;
}

bool RISCVTTIImpl::isLegalCastInstr(unsigned Opcode, Type *Dst, Type *Src,
                                    const Instruction *I) {
  return getCastInstrCost(Opcode, Dst, Src, I) < IllegalInstrCost;
}

bool RISCVTTIImpl::isLegalCmpSelInstr(unsigned Opcode, Type *ValTy,
                                      Type *CondTy, const Instruction *I) {
  return getCmpSelInstrCost(Opcode, ValTy, CondTy, I) < IllegalInstrCost;
}

unsigned RISCVTTIImpl::getScalarizationOverhead(Type *Ty, bool Insert,
                                                bool Extract) {
  assert(Ty->isVectorTy() && "Can only scalarize vectors");
  int Cost = 0;

  for (int i = 0, e = Ty->getVectorNumElements(); i < e; ++i) {
    if (Insert)
      Cost += getVectorInstrCost(Instruction::InsertElement, Ty, i);
    if (Extract)
      Cost += getVectorInstrCost(Instruction::ExtractElement, Ty, i);
  }

  return Cost;
}

int RISCVTTIImpl::getMemoryOpCost(unsigned Opcode, Type *Src,
                                  MaybeAlign Alignment, unsigned AddressSpace,
                                  const Instruction *I) {
  assert((Opcode == Instruction::Load || Opcode == Instruction::Store) &&
         "Invalid Opcode");
  if (!Src->isVectorTy())
    return BaseT::getMemoryOpCost(Opcode, Src, Alignment, AddressSpace);

  TargetSIMDInfo *TSI = getTargetSIMDInfo();
  TargetSIMDType *TST = TSI->getTargetSIMDTypeForVecType(Src);
  if (!TST && Src->isIntOrIntVectorTy())
    TST = TSI->getTargetSIMDTypeForVecType(Src, true);

  if (!TST)
    return IllegalInstrCost;

  Type *MemType = TST->MemType;
  Type *RegType = TST->RegType;

  // FIXME: add cost of load/store, here assume cost is 1
  int Cost = 1;

  auto A = Alignment.valueOrOne().value();
  if (Alignment && A % DL.getABITypeAlignment(MemType) == 0)
    return Cost;

  Type *AlignRegType = TSI->getAlignType(MemType);
  if (!AlignRegType)
    return IllegalInstrCost;

  if (Opcode == Instruction::Load) {
    // Need intrinsic for prime align register and alignment load
    unsigned PrimeIntrinId =
        TSI->getIntrinsicForLoadapPost(MemType, AlignRegType);
    unsigned AlignLdIntrinId = TSI->getIntrinsicForLoadauPost(MemType, RegType);
    if (PrimeIntrinId == Intrinsic::not_intrinsic ||
        AlignLdIntrinId == Intrinsic::not_intrinsic)
      return IllegalInstrCost;

    return Cost + 1;
  }

  assert(Opcode == Instruction::Store);
  // Need intrinsic for clear align register, align store, and flush align
  // reg
  unsigned ClearAlignRegIntrinId =
      TSI->getIntrinsicForClearAlignReg(AlignRegType);
  unsigned AlignStIntrinId = TSI->getIntrinsicForStoreauPost(MemType, RegType);
  unsigned FlushAlignRegIntrinId =
      TSI->getIntrinsicForStoreafPost(MemType, RegType);
  if (ClearAlignRegIntrinId == Intrinsic::not_intrinsic ||
      AlignStIntrinId == Intrinsic::not_intrinsic ||
      FlushAlignRegIntrinId == Intrinsic::not_intrinsic)
    return IllegalInstrCost;
  return Cost + 2;
}

bool RISCVTTIImpl::isLegalMaskedLoad(Type *DataTy, MaybeAlign Alignment) {
  static const MVT VecMVTs[] = {MVT::i32
#define GET_MLOAD_MVT
#include "TensilicaGenOpAction.inc"
  };

  if (!DataTy->isVectorTy())
    return false;

  // For aligned types
  auto A = Alignment.valueOrOne().value();
  if (!Alignment || A * 8 >= DL.getABITypeAlignment(DataTy)) {
    EVT MTy = TLI->getValueType(DL, DataTy);
    for (const MVT &VT : VecMVTs) {
      if (MTy == VT)
        return true;
    }
  }

  return false;
}

bool RISCVTTIImpl::isLegalMaskedStore(Type *DataTy, MaybeAlign Alignment) {
  static const MVT VecMVTs[] = {MVT::i32
#define GET_MSTORE_MVT
#include "TensilicaGenOpAction.inc"
  };

  if (!DataTy->isVectorTy())
    return false;

  // For aligned types
  auto A = Alignment.valueOrOne().value();
  if (!Alignment || A * 8 >= DL.getABITypeAlignment(DataTy)) {
    EVT MTy = TLI->getValueType(DL, DataTy);
    for (const MVT &VT : VecMVTs) {
      if (MTy == VT)
        return true;
    }
  }

  return false;
}

static bool checkValidScatterGatherAddr(const Value *Ptr,
                                        const BasicBlock *CIBB) {
  const GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(Ptr);
  if (!GEP || GEP->getNumOperands() > 2)
    return false;
  if (GEP->getParent() != CIBB)
    return false;

  const Value *GEPPtr = GEP->getPointerOperand();
  if (GEPPtr->getType()->isVectorTy())
    return false;

  Value *IndexVal = GEP->getOperand(1);
  if (!IndexVal->getType()->isVectorTy())
    return false;

  return true;
}

bool RISCVTTIImpl::allowsVectorization(
    std::function<void(const char *)> RemarkCallBack) {

  // If -mno-coproc is specified, don't allow vectorization
  if (!ST->coproc()) {
    if (RemarkCallBack)
      RemarkCallBack("Vectorization turned off by -mno-coproc");
    return false;
  }
  return true;
}

const Type *RISCVTTIImpl::getTargetComplexFloatType() {
  unsigned ComplexFloatCtypeID = 0;
#define GET_COMPLEX_FLOAT_CTYPE_ID
#include "TensilicaGenComplexTypeInfo.inc"
#undef GET_COMPLEX_FLOAT_CTYPE_ID

  if (!ComplexFloatCtypeID)
    return nullptr;
  return CustomType::get(Ctx, ComplexFloatCtypeID);
}

const Type *RISCVTTIImpl::getTargetComplexDoubleType() {
  unsigned ComplexDoubleCtypeID = 0;
#define GET_COMPLEX_DOUBLE_CTYPE_ID
#include "TensilicaGenComplexTypeInfo.inc"
#undef GET_COMPLEX_DOUBLE_CTYPE_ID

  if (!ComplexDoubleCtypeID)
    return nullptr;
  return CustomType::get(Ctx, ComplexDoubleCtypeID);
}

static Tensilica::TensilicaComplexOpInfoImpl ComplexFloatOpInfo = {
#define GET_COMPLEX_FLOAT_INFO
#define GEN_COMPLEX_FLOAT_INFO(OP_NAME, IID) IID,
#include "TensilicaGenComplexTypeInfo.inc"
#undef GET_COMPLEX_FLOAT_INFO
};

static Tensilica::TensilicaComplexOpInfoImpl ComplexDoubleOpInfo = {
#define GET_COMPLEX_DOUBLE_INFO
#define GEN_COMPLEX_DOUBLE_INFO(OP_NAME, IID) IID,
#include "TensilicaGenComplexTypeInfo.inc"
#undef GET_COMPLEX_DOUBLE_INFO
};

const TargetComplexOpInfo *RISCVTTIImpl::getTargetComplexFloatOpInfo() {
  static Tensilica::TensilicaComplexFloatInfo Info(ComplexFloatOpInfo);
  return &Info;
}
const TargetComplexOpInfo *RISCVTTIImpl::getTargetComplexDoubleOpInfo() {
  static Tensilica::TensilicaComplexFloatInfo Info(ComplexDoubleOpInfo);
  return &Info;
}

bool RISCVTTIImpl::isLegalMaskedGatherInst(CallInst *CI) {
  if (!checkValidScatterGatherAddr(CI->getArgOperand(0), CI->getParent()))
    return false;

  Type *DataTy = CI->getType();
  static const MVT VecMVTs[] = {MVT::i32
#define GET_MGATHER_MVT
#include "TensilicaGenOpAction.inc"
  };

  EVT MTy = TLI->getValueType(DL, DataTy);
  for (const MVT &VT : VecMVTs) {
    if (MTy.isVector()) {
      if (MTy == VT)
        return true;
    } else {
      if (VT.isVector() && MTy == VT.getVectorElementType())
        return true;
    }
  }
  return false;
}

bool RISCVTTIImpl::isLegalMaskedScatterInst(CallInst *CI) {
  if (!checkValidScatterGatherAddr(CI->getArgOperand(1), CI->getParent()))
    return false;

  Type *DataTy = CI->getArgOperand(0)->getType();
  static const MVT VecMVTs[] = {MVT::i32
#define GET_MSCATTER_MVT
#include "TensilicaGenOpAction.inc"
  };

  EVT MTy = TLI->getValueType(DL, DataTy);
  for (const MVT &VT : VecMVTs) {
    if (MTy.isVector()) {
      if (MTy == VT)
        return true;
    } else {
      if (VT.isVector() && MTy == VT.getVectorElementType())
        return true;
    }
  }
  return false;
}

int RISCVTTIImpl::getIntrinsicInstrCost(Intrinsic::ID IID, Type *RetTy,
                                        ArrayRef<Type *> Tys, FastMathFlags FMF,
                                        unsigned VF) {
  if (IID >= llvm::Intrinsic::num_intrinsics) {
    // FIXME: check the types of ret and param types and cost; set the correct
    //        cost
    return 1;
  }
  return BaseT::getIntrinsicInstrCost(IID, RetTy, Tys, FMF, VF);
}

int RISCVTTIImpl::getIntrinsicInstrCost(Intrinsic::ID IID, Type *RetTy,
                                        ArrayRef<Value *> Args,
                                        FastMathFlags FMF, unsigned VF) {
  if (IID >= llvm::Intrinsic::num_intrinsics) {
    // FIXME: check the types of ret and param types and cost; set the correct
    //        cost
    return 1;
  }
  return BaseT::getIntrinsicInstrCost(IID, RetTy, Args, FMF, VF);
}

int RISCVTTIImpl::getUintToFpInstrCost(unsigned Opcode, Type *Dst, Type *Src,
                                       const Instruction *I) {
  assert(Opcode == Instruction::UIToFP && "expect Instruction::UIToFP");
  if (Src->isVectorTy() &&
      Src->getVectorElementType() == IntegerType::get(Ctx, 1)) {
    // Cost = zero extend cost + cast instr cost
    Type *CvtElemTy = IntegerType::get(Ctx, Dst->getScalarSizeInBits());
    VectorType *CvtVecTy =
        VectorType::get(CvtElemTy, Src->getVectorNumElements());
    return getCastInstrCost(Instruction::ZExt, CvtVecTy, Src) +
           getCastInstrCost(Opcode, Dst, CvtVecTy);
  }

  int ISD = TLI->InstructionOpcodeToISD(Opcode);
  assert(ISD && "Invalid opcode");
  std::pair<int, MVT> LT = TLI->getTypeLegalizationCost(DL, Src);
  if (Src->isVectorTy() &&
      TLI->getOperationAction(ISD, LT.second) == TargetLoweringBase::Custom)
    return 1;

  return BaseT::getCastInstrCost(Opcode, Dst, Src, I);
}

int RISCVTTIImpl::getCastInstrCost(unsigned Opcode, Type *Dst, Type *Src,
                                   const Instruction *I) {
  static const TypeConversionCostTblEntry ConversionTbl[] = {
      {ISD::SIGN_EXTEND, MVT::i32, MVT::i32, 1}, // dummy cost
#define GET_TYPE_CONVERSION_COST_TABLE
#include "TensilicaGenOpAction.inc"
  };

  int ISD = TLI->InstructionOpcodeToISD(Opcode);
  assert(ISD && "Invalid opcode");

  if (Src->isVectorTy() &&
      (Opcode == Instruction::ZExt || Opcode == Instruction::SExt)) {
    if (Src == Dst)
      return 0;

    EVT SrcVT = TLI->getValueType(DL, Src);
    EVT DstVT = TLI->getValueType(DL, Dst);
    if (!SrcVT.isSimple() || !DstVT.isSimple())
      return IllegalInstrCost;

    if (const auto *Entry = ConvertCostTableLookup(
            ConversionTbl, ISD, DstVT.getSimpleVT(), SrcVT.getSimpleVT()))
      return Entry->Cost;
    return IllegalInstrCost;
  }

  if (Opcode == Instruction::UIToFP && Src->isVectorTy())
    return getUintToFpInstrCost(Opcode, Dst, Src, I);

  // For other cases.
  Type *CheckTy;
  if (Opcode == Instruction::SIToFP)
    CheckTy = Src;
  else
    CheckTy = Dst;

  std::pair<int, MVT> LT = TLI->getTypeLegalizationCost(DL, CheckTy);
  if (CheckTy->isVectorTy() &&
      TLI->getOperationAction(ISD, LT.second) == TargetLoweringBase::Custom)
    return 1;

  return BaseT::getCastInstrCost(Opcode, Dst, Src, I);
}

unsigned RISCVTTIImpl::getNumberOfParts(Type *Tp) {
  if (Tp->isVectorTy() &&
      cast<VectorType>(Tp)->getElementType()->isCustomTy()) {
    TargetSIMDInfo *TSI = this->getTargetSIMDInfo();
    if (TSI->getTargetSIMDTypeForVecType(Tp))
      return 1;
  }
  return BaseT::getNumberOfParts(Tp);
}

bool RISCVTTIImpl::isLSRCostLess(TargetTransformInfo::LSRCost &C1,
                                 TargetTransformInfo::LSRCost &C2) {
  return std::tie(C1.NumRegs, C1.NumIVMuls, C1.ScaleCost, C1.NumBaseAdds,
                  C1.AddRecCost, C1.ImmCost, C1.SetupCost) <
         std::tie(C2.NumRegs, C2.NumIVMuls, C2.ScaleCost, C2.NumBaseAdds,
                  C2.AddRecCost, C2.ImmCost, C2.SetupCost);
}

// Unrolling is handled by software pipelining phase
void RISCVTTIImpl::getUnrollingPreferences(Loop *L, ScalarEvolution &SE,
                                           TTI::UnrollingPreferences &UP) {
  // Partial unroll only by SWP
  // UP.Runtime = UP.Partial = false; // it is default anyway
  UP.MaxCount = 1;

  // TODO list
  // count the ratio of intrinsics, and decrease the threshold if it is
  //   high, say, user tuned part should be unrolled less
  // check TIE proto cost for intrinsics, see getIntrinsicInstrCost
  // illegal vector operation can be very expensive
  // unroll more for NX as latency is longer
  // auto XII = ST->getInstrInfo();
  // if (XII->getAStage() == XII->getEStage())
  //   UP.Threshold *= 0.7;

  // Default is 400, the boost is a function of the cost saving ratio,
  // and this is the upper limit
  UP.MaxPercentThresholdBoost = 200;

  // Default of Threshold is 150, while XCC is at 128
  // Default of FullUnrollMaxCount is INT_MAX, say, as much as cost model likes.
  // This may be problematic, mostly want to be a bit similar to xcc, which
  // effectively has a limit of 16 by default
  // Unroll more for wider machine
  TargetSchedModel SchedModel;
  SchedModel.init(ST);
  int IssueWidth = SchedModel.getIssueWidth();
  double Factor = IssueWidth;
  if (IssueWidth > 4) {
    Factor = 5.4;
  } else if (IssueWidth == 4) {
    Factor = 4.5;
  } else if (IssueWidth == 3) {
    Factor = 4;
  } else
    Factor = 3;

  if (L->getParentLoop()) {
    UP.Threshold = Factor * 30;
    // UP.Threshold = 120;
    UP.FullUnrollMaxCount = 30;
  } else {
    UP.Threshold = Factor * 18;
    // UP.Threshold = 80;
    UP.FullUnrollMaxCount = 7;
  }
}
#endif
