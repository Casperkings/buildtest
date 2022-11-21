//===-- Xtensa.h - Top-level interface for Xtensa representation ----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the entry points for global functions defined in the LLVM
// Xtensa back-end.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_XTENSA_XTENSA_H
#define LLVM_LIB_TARGET_XTENSA_XTENSA_H

#include "MCTargetDesc/XtensaMCTargetDesc.h"
#include "llvm/Support/DataTypes.h"
#include "llvm/Support/MachineValueType.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {
class FunctionPass;
class ModulePass;
class TargetMachine;
class XtensaTargetMachine;
class formatted_raw_ostream;

void initializeXtensaLowerThreadLocalPass(PassRegistry &p);

FunctionPass *createXtensaFrameToArgsOffsetEliminationPass();
FunctionPass *createXtensaPacketizer();
FunctionPass *createTensilicaSWP();
FunctionPass *createXtensaISelDag(XtensaTargetMachine &TM,
                                  CodeGenOpt::Level OptLevel);
ModulePass *createXtensaLowerThreadLocalPass();

ImmutablePass *
createXtensaTargetTransformInfoPass(const XtensaTargetMachine *TM);

#define GET_CUSTOM_TYPE_MVT_DEFS
#include "TensilicaGenCustomTypeInfo.inc"
#undef GET_CUSTOM_TYPE_MVT_DEFS

} // end namespace llvm;

#endif
