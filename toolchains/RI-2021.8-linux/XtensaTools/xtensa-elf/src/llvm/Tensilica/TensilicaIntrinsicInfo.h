//===- TensilicaIntrinsicInfo.h - Tensilica Intrinsic Information ---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//
//
// Interface for the Tensilica Implementation of the Intrinsic Info class.
//
//===----------------------------------------------------------------------===//
#ifndef LLVM_LIB_CODEGEN_TENSILICA_TENSILICAINTRINSICINFO_H
#define LLVM_LIB_CODEGEN_TENSILICA_TENSILICAINTRINSICINFO_H

#include "Tensilica/TensilicaIntrnProperty.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/Target/TargetIntrinsicInfo.h"

namespace llvm {

using namespace llvm::TargetIntrinsic;

namespace Tensilica {
class IntrinsicInfo : public TargetIntrinsicInfo {
public:
  IntrinsicInfo();
  std::string getName(unsigned IntrId, Type **Tys = nullptr,
                      unsigned numTys = 0) const final;
  unsigned lookupName(const char *Name, unsigned Len) const final;
  bool isOverloaded(unsigned IID) const final;
  Function *getDeclaration(Module *M, unsigned ID, Type **Tys = nullptr,
                           unsigned numTys = 0) const final;

  unsigned getNumOfFrontendBuiltins() const final;

  const FrontendBuiltinInfo *getFrontendBuiltinInfo() const final;

  const TargetBuiltinImmArgInfoVT *
  getFrontendBuiltinImmArgInfo(unsigned BuiltinID) const final;

  unsigned getRegFormBuiltinID(unsigned BID, int &ArgDiffIdx) const final;

  const Tensilica::IntrnProperty *getIntrnProperty(unsigned ID) const final;
};
} // namespace Tensilica
} // namespace llvm

#endif // LLVM_LIB_CODEGEN_TENSILICA_TENSILICAINTRINSICINFO_H
