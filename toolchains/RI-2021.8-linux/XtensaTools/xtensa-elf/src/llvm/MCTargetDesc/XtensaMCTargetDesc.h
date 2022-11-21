/*SWARM_REVIEW_REQUEST_DUMMY_COMMENT*/
//===-- XtensaMCTargetDesc.h - Xtensa Target Descriptions -----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file provides Xtensa specific target descriptions.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_XTENSA_MCTARGETDESC_XTENSAMCTARGETDESC_H
#define LLVM_LIB_TARGET_XTENSA_MCTARGETDESC_XTENSAMCTARGETDESC_H
#include "llvm/Support/DataTypes.h"

namespace llvm {
class Target;

extern Target TheXtensaTarget;

} // End llvm namespace

// Defines symbolic names for Xtensa registers.  This defines a mapping from
// register name to register number.
//
#define GET_REGINFO_ENUM
#include "XtensaGenRegisterInfo.inc"

// Defines symbolic names for the Xtensa instructions.
//
#define GET_INSTRINFO_ENUM
#include "XtensaGenInstrInfo.inc"

#define GET_SUBTARGETINFO_ENUM
#include "XtensaGenSubtargetInfo.inc"

#endif
