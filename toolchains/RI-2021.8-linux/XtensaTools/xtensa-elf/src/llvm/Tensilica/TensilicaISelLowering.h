//===-- Tensilica/TensilicaTargetLowering.h - Tensilica DAG Lowering ------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines Register transfer info, pseudo expansion and other common
// helpers to lower LLVM code into a selection DAG.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_CODEGEN_TENSILICA_TENSILICAISELLOWERING_H
#define LLVM_LIB_CODEGEN_TENSILICA_TENSILICAISELLOWERING_H

#include "llvm/CodeGen/TargetLowering.h"
#include "Tensilica/TensilicaPseudoExpand.h"
#include "Tensilica/TensilicaRegTransfer.h"
#include "Tensilica/TensilicaIntrinsicISelLower.h"

namespace llvm {

namespace Tensilica {

struct ConfigTargetLowering;
class SubtargetInfo;
class OpISelInfo;

class ISelLowering : public llvm::TargetLowering {
public:
  explicit ISelLowering(const TargetMachine &TM,
                        const Tensilica::ConfigTargetLowering &CTL)
      : TargetLowering(TM), TCTL(CTL), TM(TM) {}

  const Tensilica::PseudoExpandInfo *
  getPseudoExpandInfo(unsigned PseudoOpc) const;

  const Tensilica::RegTransferInfo *
  getRegTransferInfo(unsigned RegClassID) const;

  const Tensilica::IntrinsicISelInfo *
  getIntrinsicISelInfo(unsigned IntrinsicID) const;

  bool matchVSelIntrinImmMask(unsigned IntrinID, ArrayRef<int> ShuffleMask,
                              int &Imm) const;

  /// Check if proto immediates are satisfied
  bool checkIntrinsicImmConstraints(
    const SDNode *N, const Tensilica::IntrinsicISelInfo *IntrISelInfo) const;

  Tensilica::OpISelInfo *getOperationISelInfo(unsigned Opcode) const;

  static void getLastValue(SDNode *N, SDValue &LV);
  static void AddIntrnResults(const Tensilica::IntrinsicISelInfo *IntrISelInfo,
                              SDNode *N, std::map<unsigned, SDValue> &Results);
  static TensilicaISelLinkType 
  getOutLinkType(const Tensilica::IntrinsicISelInfo *ISelInfo, bool IsChain);

  static std::string genRToMIntrnName(const SDNode *Intrn, SDNode *Store,
                                      const TargetIntrinsicInfo *TII);

  static bool isRTOR(const SDNode *N, const TargetIntrinsicInfo *TII);

  static std::string convertIntrnName(const SDNode *N,
                                      const TargetIntrinsicInfo *TII,
                                      const char *MemStr);

  // Helper for getTgtMemIntrinsic
  bool getTensilicaTgtMemIntrinsic(IntrinsicInfo &Info, const CallInst &I,
                                   MachineFunction &MF,
                                   unsigned Intrinsic) const;

  const Tensilica::ConfigTargetLowering &TCTL;
  const TargetMachine &TM;
};

} // namespace Tensilica
} // namespace llvm
#endif // LLVM_LIB_CODEGEN_TENSILICA_TENSILICAISELLOWERING_H
