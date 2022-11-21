//===-- TensilicaConfigInstrInfo.h - Config Instruction Information -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares a helper class for Tensilica::InstrInfo
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_CODEGEN_TENSILICA_TENSILICACONFIG_TENSILICACONFIGINSTRINFO_H
#define LLVM_LIB_CODEGEN_TENSILICA_TENSILICACONFIG_TENSILICACONFIGINSTRINFO_H
#include "Tensilica/TensilicaInstrProperty.h"
#include "Tensilica/TensilicaConfig/TensilicaConfigImap.h"
#include "Tensilica/TensilicaConfig/TensilicaConfigRewrite.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include <map>
#include <set>


namespace llvm {
namespace Tensilica {

// This structure contains target config-specific information that is not
// passed through tablegen, instead it is generated directly by
// llvm-xtensa-config-gen.
struct ConfigInstrInfo {
  const unsigned AStage;
  const unsigned EStage;
  const std::map<unsigned, unsigned> PipeAlterMap;
  const std::map<unsigned, unsigned> PipeAlterReverseMap;
  ArrayRef<Tensilica::InstrProperty> InstrProperties;
  ArrayRef<int> InstrPropMap;
  const std::map<unsigned, std::vector<unsigned>> InsnBitsInputMap;
  const std::map<unsigned, std::set<unsigned>> IgnoreStateOutputMap;
  const std::map<unsigned, unsigned> I_X_Map;
  const std::map<unsigned, unsigned> I_IP_Map;
  const std::map<unsigned, unsigned> IP_I_Map;
  const std::map<unsigned, unsigned> X_XP_Map;
  const std::map<unsigned, unsigned> I_IU_Map;
  const std::map<unsigned, unsigned> X_XU_Map;
  const std::map<unsigned, unsigned> XP_I_Map;
  // Mapping between MapOpcode enum and typical target MachineInstr opcode.
  ArrayRef<unsigned> EnumToOpcode;
  // Map to several opcodes for some special instructions, e.g. ADDI/ADDMI.
  const std::map<Tensilica::MapOpcode, std::vector<unsigned>> EnumToVariants;
  const std::map<unsigned, Tensilica::MapOpcode> OpcodeToEnum;
  // TargetInstrInfo initialization helper.
  typedef void (*InitMCInstrInfo)(MCInstrInfo *);
  InitMCInstrInfo initTensilicaMCInstrInfo;
  const ConfigImap *ImapInfo;
  const ConfigRewrite *RewriteInfo;
  // Constructors
  // Using hidden visibility is a workaround to allow both RISCV and Xtensa
  // targets together in standalone build. This should be not required for
  // ybuild.
  LLVM_LIBRARY_VISIBILITY ConfigInstrInfo();
  ConfigInstrInfo(const ConfigInstrInfo &) = delete;
  ConfigInstrInfo(ConfigInstrInfo &&that) = delete;
  LLVM_LIBRARY_VISIBILITY ~ConfigInstrInfo();
};

} // namespace Tensilica
} // namespace llvm
#endif // LLVM_LIB_CODEGEN_TENSILICA_TENSILICACONFIG_TENSILICACONFIGINSTRINFO_H
