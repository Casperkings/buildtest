//===-- XtensaTargetStreamer.h - Xtensa Target Streamer -------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_XTENSA_XTENSATARGETSTREAMER_H
#define LLVM_LIB_TARGET_XTENSA_XTENSATARGETSTREAMER_H

#include "llvm/MC/MCStreamer.h"

namespace llvm {
class XtensaTargetStreamer : public MCTargetStreamer {
public:
  XtensaTargetStreamer(MCStreamer &S);
  virtual ~XtensaTargetStreamer();
};
}

#endif
