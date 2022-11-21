//===-- XtensaTargetInfo.cpp - Xtensa Target Implementation----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Xtensa.h"
#include "XtensaIntrinsicInfo.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Support/TargetRegistry.h"
#include "Tensilica/TensilicaConfig/TensilicaTargetCustomType.h"
#include "Tensilica/TensilicaConfig/TensilicaTargetIntrinsicRegistry.h"

namespace llvm {
Target TheXtensaTarget;
} // end namespace llvm

extern "C" void LLVMInitializeXtensaConfigTargetInfo() {

  RegisterTarget<Triple::xtensa> X(TheXtensaTarget, "xtensa", "Xtensa",
                                   "xtensa-backend");

  initCustomTypeInfo();
  initCustomTypeToVecTypeInfo();
  initTargetIntrinsicRegistry(); // FIXME: move it to XtensaIntrinsicInfo.cpp?
}

namespace llvm {
// Return true if the config lib is present. This is used by lto plug-in and
// defined in XtensaConfigurable.cpp. This definition is for standalone build.
extern bool hasTensilicaConfigLib(void) { return true; }
} // namespace llvm
