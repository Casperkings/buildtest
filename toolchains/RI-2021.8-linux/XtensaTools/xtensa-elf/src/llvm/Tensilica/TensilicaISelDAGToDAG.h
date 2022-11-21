//===-- Tensilica/TensilicaISelDAGToDAG.h - Tensilica DAG inst selector ---===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines common helpers for DAG instruction selector.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_CODEGEN_TENSILICA_TENSILICAISELDAGTODAG_H
#define LLVM_LIB_CODEGEN_TENSILICA_TENSILICAISELDAGTODAG_H

#include "llvm/CodeGen/SelectionDAGISel.h"
#include "Tensilica/TensilicaIntrinsicISelLower.h"

namespace llvm {

namespace Tensilica {

class SDAGISel : public llvm::SelectionDAGISel {
public:
  explicit SDAGISel(TargetMachine &TM, CodeGenOpt::Level OptLevel)
      : SelectionDAGISel(TM, OptLevel) {}

  /// getI32Imm - Return a target constant with the specified value, of type
  /// i32.
  inline SDValue getI32Imm(unsigned Imm, const SDLoc &dl) {
    return CurDAG->getTargetConstant(Imm, dl, MVT::i32);
  }

  static void getLastValue(SDNode *N, SDValue &LV);

  static TensilicaISelLinkType 
  getOutLinkType(const Tensilica::IntrinsicISelInfo *ISelInfo, bool IsChain);
};

} // namespace Tensilica
} // namespace llvm
#endif // LLVM_LIB_CODEGEN_TENSILICA_TENSILICAISELDAGTODAG_H
