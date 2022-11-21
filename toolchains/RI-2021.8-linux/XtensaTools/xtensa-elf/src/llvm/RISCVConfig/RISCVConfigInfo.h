//===-- RISCVConfigInfo.h - RISCV TDK flow Implementation -----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_RISCV_RISCVCONFIG_RISCVCONFIGINFO_H
#define LLVM_LIB_TARGET_RISCV_RISCVCONFIG_RISCVCONFIGINFO_H

namespace llvm {
class Target;
class Triple;
class TargetOptions;
class RISCVRegisterInfo;
namespace Tensilica {
struct ConfigPacketizer;
struct ConfigInstrInfo;
struct ConfigTargetLowering;
class ConfigSIMDInfo;
struct ConfigImap;
}
struct RISCVConfigTargetLowering;
}
struct XtensaConfigImap;

struct RISCVConfigInfo {
  llvm::RISCVRegisterInfo *RegisterInfo;
  llvm::Tensilica::ConfigInstrInfo *InstrInfo;
  llvm::Tensilica::ConfigPacketizer *PacketizerInfo;
  llvm::Tensilica::ConfigTargetLowering *LoweringInfo;
  llvm::RISCVConfigTargetLowering *TargetLoweringInfo;
  llvm::Tensilica::ConfigImap *ImapInfo;
  llvm::Tensilica::ConfigSIMDInfo *SIMDInfo;
};

extern "C" RISCVConfigInfo *
LLVMInitializeRISCVConfigInfo(const llvm::Triple &TargetTriple,
                              llvm::StringRef CPU, llvm::StringRef FS,
                              llvm::StringRef ABIName, unsigned HwMode);

#endif // LLVM_LIB_TARGET_RISCV_RISCVCONFIG_RISCVCONFIGINFO_H
