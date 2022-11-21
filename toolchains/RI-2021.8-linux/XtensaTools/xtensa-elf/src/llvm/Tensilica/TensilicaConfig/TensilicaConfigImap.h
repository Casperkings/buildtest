//===-- XtensaConfigImap.h - Xtensa Instruction Maps Config Information ---===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares a helper class to pass configuration-specific IMAP info
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_CONFIG_TENSILICA_TENSILICACONFIG_XTENSACONFIGIMAP_H
#define LLVM_LIB_CONFIG_TENSILICA_TENSILICACONFIG_XTENSACONFIGIMAP_H
#include <map>
#include <set>

namespace llvm {
namespace Tensilica {

struct IMAP;
// This structure contains target config-specific information.
struct ConfigImap {
  const int ImapArraySize;
  const llvm::Tensilica::IMAP **ImapArray;
  const std::map<unsigned, std::set<unsigned>> &GenericsOpcodeTable;
  const std::map<unsigned, std::set<unsigned>> &AliasesOpcodeTable;

  // Constructors
  ConfigImap();
  ConfigImap(const ConfigImap &) = delete;
  ConfigImap(ConfigImap &&that) = default;
};

} // namespace Tensilica
} // namespace llvm
#endif // LLVM_LIB_CONFIG_TENSILICA_TENSILICACONFIG_XTENSACONFIGIMAP_H
