//===-- TensilicaPseudoExpand.h - Tensilica pseudo insn expansion Interface===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the interfaces that Xtensa uses to find the intrinsic
// opcode required to expand a pseudo instruction
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_CODEGEN_TENSILICA_TENSILICAPSEUDOEXPAND_H
#define LLVM_LIB_CODEGEN_TENSILICA_TENSILICAPSEUDOEXPAND_H

namespace llvm {
namespace Tensilica {

// Structure to specify the pseudo instruction opcode and the corresponding
// intrinsic id
struct PseudoExpandInfo {
  // Pseudo instruction opcode
  unsigned PseudoOpc;
  // Intrinsic ID
  unsigned IntrnID;

  bool operator<(const PseudoExpandInfo &RHS) const {
    return PseudoOpc < RHS.PseudoOpc;
  }

  bool operator==(const PseudoExpandInfo &RHS) const {
    return RHS.PseudoOpc == PseudoOpc;
  }
};

} // namespace Tensilica
} // namespace llvm

#endif // LLVM_LIB_CODEGEN_TENSILICA_TENSILICAPSEUDOEXPAND_H
