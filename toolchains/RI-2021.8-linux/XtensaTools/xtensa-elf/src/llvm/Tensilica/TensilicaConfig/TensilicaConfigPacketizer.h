//===-- TensilicaConfigPacketizer.h - Config info for packetizer ----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file provides Config-specific information for the Tensilica Packetizer
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_CODEGEN_TENSILICA_TENSILICACONFIG_TENSILICACONFIGPACKETIZER_H
#define LLVM_LIB_CODEGEN_TENSILICA_TENSILICACONFIG_TENSILICACONFIGPACKETIZER_H
#include "llvm/ADT/BitVector.h"
#include <map>
#include <vector>
#include <string>

namespace llvm {
class Target;

namespace Tensilica {

struct Format {
  const char *Name;
  int Bytesize;
  int FormatID;
  int NumSlot;
};
struct FunctionalUnitInfo {
  std::string Name;
  int NumCopies;
  // from instruction-class to number of copies
  std::map<int, int> Uses;
};
// This structure contains target config-specific information, used for the
// DFA packetizer initialization.
struct ConfigPacketizer {
  int *OpcodeClasses;
  int *GenericOpcodeTable;
  const struct Format *Formats;
  const struct FunctionalUnitInfo *FunctionalUnits;
  const int *BundlingDFATransitionTable;
  const int NumState;
  const int NumClass;
  const int NumFormat;
  const int InstructionListEnd;
  const unsigned MoveAr;
  const unsigned Nop;
  const unsigned XsrSar; // Some late config-independent instruction.
  int NumFunctionalUnits;
  // Constructors
  ConfigPacketizer();
  ConfigPacketizer(const ConfigPacketizer &) = delete;
  ConfigPacketizer(ConfigPacketizer &&that) = delete;
};

} // namespace Tensilica
} // namespace llvm
#endif // LLVM_LIB_CODEGEN_TENSILICA_TENSILICACONFIG_TENSILICACONFIGPACKETIZER_H
