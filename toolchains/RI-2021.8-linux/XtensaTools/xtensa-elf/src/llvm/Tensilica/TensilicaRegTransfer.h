//===-- TensilicaRegTransfer.h - Tensilica register transfer Interface ----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the interfaces that Tensilica uses to find the machine
// opcode required to perform a move/load/store within a register file
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_CODEGEN_TENSILICA_TENSILICAREGTRANSFER_H
#define LLVM_LIB_CODEGEN_TENSILICA_TENSILICAREGTRANSFER_H

#define MULTI_INSTR_REG_MOV true

namespace llvm {
namespace Tensilica {

// Structure to specify the mov/loadi/storei pseudo instr to use for moving
// between registers and memory within a target register file
struct RegTransferInfo {
  // Target register file
  unsigned RegClassID;
  // Pseudo instruction opcodes
  unsigned MoviOpcode;
  unsigned LoadiOpcode;
  unsigned StoreiOpcode;
  unsigned RegMovOpcode;
  unsigned RegLoadOpcode;
  unsigned RegStoreOpcode;
  unsigned RegLoadPhysOpcode;
  bool MultiInstrRegMove;

  bool operator<(const RegTransferInfo &RHS) const {
    return RegClassID < RHS.RegClassID;
  }
  bool operator==(const RegTransferInfo &RHS) const {
    return RHS.RegClassID == RegClassID;
  }
};

} // namespace Tensilica
} // namespace llvm

#endif // LLVM_LIB_CODEGEN_TENSILICA_TENSILICAREGTRANSFER_H
