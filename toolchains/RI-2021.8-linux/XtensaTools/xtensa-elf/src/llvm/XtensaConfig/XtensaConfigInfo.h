//===-- XtensaConfigInfo.h - Xtensa TDK flow Implementation ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_XTENSA_XTENSACONFIG_XTENSACONFIGINFO_H
#define LLVM_LIB_TARGET_XTENSA_XTENSACONFIG_XTENSACONFIGINFO_H
#include "llvm/ADT/StringRef.h"

namespace llvm {
class Target;
class TargetOptions;
class XtensaRegisterInfo;
namespace Tensilica {
struct ConfigPacketizer;
struct ConfigInstrInfo;
struct ConfigTargetLowering;
class ConfigSIMDInfo;
struct ConfigImap;
struct ConfigRewrite;
}
struct XtensaConfigTargetLowering;
}
struct XtensaConfigImap;

struct XtensaConfigInfo {
  bool IsLittleEndian;
  llvm::XtensaRegisterInfo *RegisterInfo;
  llvm::Tensilica::ConfigInstrInfo *InstrInfo;
  llvm::Tensilica::ConfigPacketizer *PacketizerInfo;
  llvm::Tensilica::ConfigTargetLowering *LoweringInfo;
  llvm::XtensaConfigTargetLowering *TargetLoweringInfo;
  llvm::Tensilica::ConfigImap *ImapInfo;
  llvm::Tensilica::ConfigRewrite *RewriteInfo;
  llvm::Tensilica::ConfigSIMDInfo *SIMDInfo;
};

extern "C" XtensaConfigInfo *
LLVMInitializeXtensaConfigInfo(llvm::StringRef FS,
                               const llvm::TargetOptions &Options);

#endif // LLVM_LIB_TARGET_XTENSA_XTENSACONFIG_XTENSACONFIGINFO_H
