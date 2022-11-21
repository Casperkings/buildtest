//===- XtensaIntrinsicInfo.h - Xtensa Intrinsic Information ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//
//
// Interface for the Xtensa Implementation of the Intrinsic Info class.
//
//===----------------------------------------------------------------------===//
#ifndef LLVM_LIB_TARGET_XTENSA_XTENSAINTRINSICINFO_H
#define LLVM_LIB_TARGET_XTENSA_XTENSAINTRINSICINFO_H

#include "Tensilica/TensilicaIntrnProperty.h"
#include "Tensilica/TensilicaIntrinsicInfo.h"

namespace llvm {
class TargetMachine;

namespace TensilicaIntrinsic {
enum ID {
  last_non_tensilica_intrinsic = Intrinsic::num_intrinsics - 1,

#define GET_INTRINSIC_ENUM_VALUES
#include "XtensaGenIntrinsicEnums.inc"
#undef GET_INTRINSIC_ENUM_VALUES

  num_tensilica_intrinsics
};
} // end namespace TensilicaIntrinsic

class XtensaIntrinsicInfo : public Tensilica::IntrinsicInfo {
public:
  XtensaIntrinsicInfo() : Tensilica::IntrinsicInfo() {}
};

} // end namespace llvm

#endif
