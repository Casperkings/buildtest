//===------------------------ TensilicaRewrite.h --------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef TENSILICACONFIGREWRITE_H
#define TENSILICACONFIGREWRITE_H
#include "Tensilica/TensilicaRewrite.h"

namespace llvm {
namespace Tensilica {

struct ConfigRewrite {
  const std::map<unsigned, SubregTransferInfo> &OpcodeSubregTransfer;
  const std::vector<TensilicaConfigEQTable> &ConfigEQTable;
  const std::vector<IntDTypes> &DTypes;
  const std::map<unsigned, TargetOps> &BaseTargetEquivalentTable;

  ConfigRewrite();

  ConfigRewrite(const std::map<unsigned, SubregTransferInfo> &M,
                const std::vector<TensilicaConfigEQTable> &E,
                const std::vector<IntDTypes> &R,
                const std::map<unsigned, TargetOps> &O)
      : OpcodeSubregTransfer(M), ConfigEQTable(E), DTypes(R),
        BaseTargetEquivalentTable(O) {}

  SubregTransferInfo GetSubregTransfer(unsigned opcode) const {
    auto Iter = OpcodeSubregTransfer.find(opcode);
    if (Iter == OpcodeSubregTransfer.end())
      return SubregTransferInfo::InvalidInfo();
    return Iter->second;
  }

  TargetOps getTargetOpts(unsigned Opc) const {
    auto Iter = BaseTargetEquivalentTable.find(Opc);
    if (Iter == BaseTargetEquivalentTable.end()) {
      return TargetOps::InvalidTargetOps();
    }
    return Iter->second;
  }

  bool isEmpty() const {
    return OpcodeSubregTransfer.size() == 0;
  }
};
}
}
#endif
