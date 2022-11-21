//===-- RISCVTargetInfo.cpp - RISCV Target Implementation -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RISCV.h"
#include "RISCVIntrinsicInfo.h"
#include "TargetInfo/RISCVTargetInfo.h"
#include "llvm/Support/TargetRegistry.h"
#include "Tensilica/TensilicaConfig/TensilicaTargetCustomType.h"
#include "Tensilica/TensilicaConfig/TensilicaTargetIntrinsicRegistry.h"
using namespace llvm;

Target &llvm::getTheRISCV32Target() {
  static Target TheRISCV32Target;
  return TheRISCV32Target;
}

Target &llvm::getTheRISCV64Target() {
  static Target TheRISCV64Target;
  return TheRISCV64Target;
}

#if defined(TENSILICA) || 1
extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeRISCVConfigTargetInfo() {
#else
extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeRISCVTargetInfo() {
#endif
  RegisterTarget<Triple::riscv32> X(getTheRISCV32Target(), "riscv32",
                                    "32-bit RISC-V", "RISCV");
  RegisterTarget<Triple::riscv64> Y(getTheRISCV64Target(), "riscv64",
                                    "64-bit RISC-V", "RISCV");

  initCustomTypeInfo();
  initCustomTypeToVecTypeInfo();
  initTargetIntrinsicRegistry(); // FIXME: move it to XtensaIntrinsicInfo.cpp?
}

namespace llvm {
// Return true if the config lib is present. This is used by lto plug-in and
// defined in RISCVConfigurable.cpp. This definition is for standalone build.
extern bool hasTensilicaConfigLib(void) { return true; }
} // namespace llvm
