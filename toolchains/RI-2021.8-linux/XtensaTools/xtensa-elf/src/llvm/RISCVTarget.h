//===--- RISCVTarget.h - Declare Tensilica RISCV target feature support ---===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares Tensilica RISCV TargetInfo objects.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_BASIC_TARGETS_TENSILICA_RISCV_H
#define LLVM_CLANG_LIB_BASIC_TARGETS_TENSILICA_RISCV_H

#include "TensilicaCommon.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Support/Compiler.h"

namespace clang {
// RISC-V Target
class TensilicaRISCVTargetInfo : public TensilicaTargetInfo {
protected:
  std::string ABI;
  bool HasM;
  bool HasA;
  bool HasF;
  bool HasD;
  bool HasC;

public:
  TensilicaRISCVTargetInfo(const llvm::Triple &Triple, const TargetOptions &);

  StringRef getABI() const override { return ABI; }

  void getTargetDefines(const LangOptions &Opts,
                        MacroBuilder &Builder) const override;

  BuiltinVaListKind getBuiltinVaListKind() const override {
    return TargetInfo::VoidPtrBuiltinVaList;
  }

  const char *getClobbers() const override { return ""; }

  ArrayRef<const char *> getGCCRegNames() const override;

  bool validateAsmOperandType(StringRef Constraint,
                              QualType Type) const override;

  int getEHDataRegisterNumber(unsigned RegNo) const override {
    if (RegNo == 0)
      return 10;
    else if (RegNo == 1)
      return 11;
    else
      return -1;
  }

  ArrayRef<TargetInfo::GCCRegAlias> getGCCRegAliases() const override;

  bool validateAsmConstraint(const char *&Name,
                             TargetInfo::ConstraintInfo &Info) const override;

  bool hasFeature(StringRef Feature) const override;

  bool handleTargetFeatures(std::vector<std::string> &Features,
                            DiagnosticsEngine &Diags) override;
};
class LLVM_LIBRARY_VISIBILITY TensilicaRISCV32TargetInfo
    : public TensilicaRISCVTargetInfo {
public:
  TensilicaRISCV32TargetInfo(const llvm::Triple &Triple,
                             const TargetOptions &Opts)
      : TensilicaRISCVTargetInfo(Triple, Opts) {
    IntPtrType = SignedInt;
    PtrDiffType = SignedInt;
    SizeType = UnsignedInt;
    resetDataLayout("e-m:e-p:32:32-i64:64-n32-S128");
  }

  bool setABI(const std::string &Name) override {
    if (Name == "ilp32" || Name == "ilp32f" || Name == "ilp32d") {
      ABI = Name;
      return true;
    }
    return false;
  }

  void setMaxAtomicWidth() override {
    MaxAtomicPromoteWidth = 128;

    if (HasA)
      MaxAtomicInlineWidth = 32;
  }
};
class LLVM_LIBRARY_VISIBILITY TensilicaRISCV64TargetInfo
    : public TensilicaRISCVTargetInfo {
public:
  TensilicaRISCV64TargetInfo(const llvm::Triple &Triple,
                             const TargetOptions &Opts)
      : TensilicaRISCVTargetInfo(Triple, Opts) {
    LongWidth = LongAlign = PointerWidth = PointerAlign = 64;
    IntMaxType = Int64Type = SignedLong;
    resetDataLayout("e-m:e-p:64:64-i64:64-i128:128-n64-S128");
  }

  bool setABI(const std::string &Name) override {
    if (Name == "lp64" || Name == "lp64f" || Name == "lp64d") {
      ABI = Name;
      return true;
    }
    return false;
  }

  void setMaxAtomicWidth() override {
    MaxAtomicPromoteWidth = 128;

    if (HasA)
      MaxAtomicInlineWidth = 64;
  }
};
} // namespace clang

#endif // LLVM_CLANG_LIB_BASIC_TARGETS_TENSILICA_RISCV_H
