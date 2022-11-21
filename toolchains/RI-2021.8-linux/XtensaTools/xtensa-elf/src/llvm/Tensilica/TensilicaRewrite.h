//===------------------------ TensilicaRewrite.h --------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef TENSILICAREWRITE_H
#define TENSILICAREWRITE_H
#include "llvm/CodeGen/TargetRegisterInfo.h"


namespace llvm {
namespace Tensilica {

  struct SubregTransferInfo {
  bool ToAR;
  const llvm::TargetRegisterClass *TRC;
  unsigned Size;
  SubregTransferInfo(bool ToAR, const llvm::TargetRegisterClass *TRC,
                     unsigned Size)
      : ToAR(ToAR), TRC(TRC), Size(Size) {}
  static SubregTransferInfo InvalidInfo() {
    return SubregTransferInfo(true, nullptr, 0);
  }
  bool isValid() { return TRC != nullptr; }
};

struct TensilicaConfigEQTable {
  unsigned Size;
  bool IsEq;
  unsigned CmpOpcode;
  const llvm::TargetRegisterClass *RRC;
  const llvm::TargetRegisterClass *ARC;
  unsigned MovTOpcode;
  unsigned MovFOpcode;
  TensilicaConfigEQTable(unsigned Size, bool IsEq, unsigned CmpOpcode,
                      const llvm::TargetRegisterClass *RRC,
                      const llvm::TargetRegisterClass *ARC,
                      unsigned MovTOpcode,
                      unsigned MovFOpcode)
      : Size(Size), IsEq(IsEq), CmpOpcode(CmpOpcode), RRC(RRC), ARC(ARC),
        MovTOpcode(MovTOpcode), MovFOpcode(MovFOpcode) {}
  static TensilicaConfigEQTable InvalidEQT() {
    return TensilicaConfigEQTable(0, 0, 0, nullptr, nullptr, 0, 0);
  }
  bool isValid() { return Size != 0; }
};

struct IntDTypes {
  unsigned Size;
  const llvm::TargetRegisterClass *TRC;
  IntDTypes(unsigned S, const llvm::TargetRegisterClass *RC) : Size(S), TRC(RC) {}
};

struct TargetOps {
//  unsigned Opcode;
  unsigned Op8;
  unsigned Op16;
  unsigned Op32;
  unsigned Op64;
  TargetOps(unsigned Op8, unsigned Op16, unsigned Op32,
            unsigned Op64)
      : Op8(Op8), Op16(Op16), Op32(Op32), Op64(Op64) {}
  static TargetOps InvalidTargetOps() {
    return TargetOps(0u, 0u, 0u, 0u);
  }
  bool isValid() {
    return Op8 != 0 || Op16 != 0 || Op32 != 0;
  }
};

}
}
#endif
