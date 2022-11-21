//===- TensilicaRDFInfo.h -------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Helper for providing rdf-based info to packetizer, e.g. pointer difference.
//

#ifndef LLVM_LIB_CODEGEN_TENSILICA_TENSILICARDFINFO_H
#define LLVM_LIB_CODEGEN_TENSILICA_TENSILICARDFINFO_H

#include "llvm/CodeGen/MachineDominators.h"
#include "llvm/CodeGen/MachineDominanceFrontier.h"
#include <memory>

namespace llvm {
namespace Tensilica {

struct RDFInfoImpl;
class RDFInfo {
  std::unique_ptr<RDFInfoImpl> Impl;
public:
  RDFInfo();
  ~RDFInfo();
  bool enabled() const { return Impl.get() != nullptr; }
  void run(MachineFunction &MF, MachineDominatorTree &MDT,
           MachineDominanceFrontier &MDF);
  bool areMemAccessesTriviallyBankConflict(const MachineInstr &MIa,
                                           const MachineInstr &MIb) const;
  std::string getComment(const MachineInstr &MI) const;
};

} // end namespace Tensilica
} // end namespace llvm
#endif // LLVM_LIB_CODEGEN_TENSILICA_TENSILICARDFINFO_H
