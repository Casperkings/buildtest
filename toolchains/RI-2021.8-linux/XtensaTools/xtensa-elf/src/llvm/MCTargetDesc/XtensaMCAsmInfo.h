/*SWARM_REVIEW_REQUEST_DUMMY_COMMENT*/
//===-- XtensaMCAsmInfo.h - Xtensa asm properties -------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of the XtensaMCAsmInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_XTENSA_MCTARGETDESC_XTENSAMCASMINFO_H
#define LLVM_LIB_TARGET_XTENSA_MCTARGETDESC_XTENSAMCASMINFO_H

#include "llvm/MC/MCAsmInfoELF.h"

namespace llvm {
class Triple;

class XtensaMCAsmInfo : public MCAsmInfoELF {
  void anchor() override;

public:
  explicit XtensaMCAsmInfo(const Triple &TT);
};

} // namespace llvm

#endif
