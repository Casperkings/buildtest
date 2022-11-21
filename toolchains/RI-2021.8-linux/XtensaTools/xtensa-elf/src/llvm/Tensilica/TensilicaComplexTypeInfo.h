#ifndef LLVM_LIB_CODEGEN_TENSILICA_TENSILICACOMPLEXTYPEINFO_H
#define LLVM_LIB_CODEGEN_TENSILICA_TENSILICACOMPLEXTYPEINFO_H

#include "llvm/Analysis/TargetTransformInfo.h"

namespace llvm {
namespace Tensilica {
struct TensilicaComplexOpInfoImpl {
  unsigned OpAdd;
  unsigned OpSub;
  unsigned OpMul;
  unsigned OpDiv;
  unsigned OpMAdd;
  unsigned OpMSub;
  unsigned OpMulConj;
  unsigned OpMAddConj;
  unsigned OpMSubConj;
  unsigned OpEq;
  unsigned OpNeg;
  unsigned OpNeq;
  unsigned OpConj;
  unsigned OpRecip;
  unsigned OpExtract0;
  unsigned OpExtract1;
  unsigned OpPack2;
  unsigned OpConst2;
  unsigned OpConst4;
};

class TensilicaComplexFloatInfo : public TargetComplexOpInfo {
public:
  TensilicaComplexFloatInfo(const TensilicaComplexOpInfoImpl &Info)
      : Info(Info) {}

  Intrinsic::ID getIntrinsicForAdd() const override {
    if (!Info.OpAdd)
      return Intrinsic::not_intrinsic;
    return Info.OpAdd + llvm::Intrinsic::num_intrinsics;
  }

  Intrinsic::ID getIntrinsicForSub() const override {
    if (!Info.OpSub)
      return Intrinsic::not_intrinsic;
    return Info.OpSub + llvm::Intrinsic::num_intrinsics;
  }

  Intrinsic::ID getIntrinsicForMul() const override {
    if (!Info.OpMul)
      return Intrinsic::not_intrinsic;
    return Info.OpMul + llvm::Intrinsic::num_intrinsics;
  }

  Intrinsic::ID getIntrinsicForDiv() const override {
    if (!Info.OpDiv)
      return Intrinsic::not_intrinsic;
    return Info.OpDiv + llvm::Intrinsic::num_intrinsics;
  }

  Intrinsic::ID getIntrinsicForMAdd() const override {
    if (!Info.OpMAdd)
      return Intrinsic::not_intrinsic;
    return Info.OpMAdd + llvm::Intrinsic::num_intrinsics;
  }

  Intrinsic::ID getIntrinsicForMSub() const override {
    if (!Info.OpMSub)
      return Intrinsic::not_intrinsic;
    return Info.OpMSub + llvm::Intrinsic::num_intrinsics;
  }

  Intrinsic::ID getIntrinsicForMulConj() const override {
    if (!Info.OpMulConj)
      return Intrinsic::not_intrinsic;
    return Info.OpMulConj + llvm::Intrinsic::num_intrinsics;
  }

  Intrinsic::ID getIntrinsicForCmpEq() const override {
    if (!Info.OpEq)
      return Intrinsic::not_intrinsic;
    return Info.OpEq + llvm::Intrinsic::num_intrinsics;
  }

  Intrinsic::ID getIntrinsicForNeg() const override {
    if (!Info.OpNeg)
      return Intrinsic::not_intrinsic;
    return Info.OpNeg + llvm::Intrinsic::num_intrinsics;
  }

  Intrinsic::ID getIntrinsicForCmpNeq() const override {
    if (!Info.OpNeq)
      return Intrinsic::not_intrinsic;
    return Info.OpNeq + llvm::Intrinsic::num_intrinsics;
  }

  Intrinsic::ID getIntrinsicForConj() const override {
    if (!Info.OpConj)
      return Intrinsic::not_intrinsic;
    return Info.OpConj + llvm::Intrinsic::num_intrinsics;
  }

  Intrinsic::ID getIntrinsicForRecip() const override {
    if (!Info.OpRecip)
      return Intrinsic::not_intrinsic;
    return Info.OpRecip + llvm::Intrinsic::num_intrinsics;
  }

  Intrinsic::ID getIntrinsicForExtractReal() const override {
    if (!Info.OpExtract0)
      return Intrinsic::not_intrinsic;
    return Info.OpExtract0 + llvm::Intrinsic::num_intrinsics;
  }

  Intrinsic::ID getIntrinsicForExtractImag() const override {
    if (!Info.OpExtract1)
      return Intrinsic::not_intrinsic;
    return Info.OpExtract1 + llvm::Intrinsic::num_intrinsics;
  }

  Intrinsic::ID getIntrinsicForPack() const override {
    if (!Info.OpPack2)
      return Intrinsic::not_intrinsic;
    return Info.OpPack2 + llvm::Intrinsic::num_intrinsics;
  }

private:
  const TensilicaComplexOpInfoImpl &Info;
};
} // namespace Tensilica
} // namespace llvm
#endif