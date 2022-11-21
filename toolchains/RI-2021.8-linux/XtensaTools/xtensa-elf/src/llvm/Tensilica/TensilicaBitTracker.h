//===--- TensilicaBitTracker.h --------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_CODEGEN_TENSILICA_TENSILICABITTRACKER_H
#define LLVM_LIB_CODEGEN_TENSILICA_TENSILICABITTRACKER_H

#include "Tensilica/BitTracker.h"
#include "llvm/ADT/DenseMap.h"

namespace llvm {
namespace Tensilica {
struct TensilicaEvaluator : public BitTracker::MachineEvaluator {
  typedef BitTracker::CellMapType CellMapType;
  typedef BitTracker::RegisterRef RegisterRef;
  typedef BitTracker::RegisterCell RegisterCell;
  typedef BitTracker::BranchTargetList BranchTargetList;
  TensilicaEvaluator(const RegisterInfo &tri, MachineRegisterInfo &mri,
                  const InstrInfo &tii, MachineFunction &mf);
  bool evaluate(const MachineInstr &MI, const CellMapType &Inputs,
                CellMapType &Outputs) const final;
  bool evaluate(const MachineInstr &BI, const CellMapType &Inputs,
                BranchTargetList &Targets, bool &FallsThru) final;

  MachineFunction &MF;
  MachineFrameInfo &MFI;
  const InstrInfo &TII;
  void setBlockScanned(int BBN) override {
    BlockScanned[BBN] = true;
  }
  bool isBlockScanned(int BBN) const override {
    return BlockScanned[BBN];
  }
  void setBlockScanned(MachineBasicBlock &BB) override {
    setBlockScanned(BB.getNumber());
  }
  bool isBlockScanned(MachineBasicBlock &BB) const override {
    return isBlockScanned(BB.getNumber());
  }
  void alwaysGoto(MachineBasicBlock *From, MachineBasicBlock *To) override {
    AlwaysGoto[From->getNumber()] = To;
  }
  MachineBasicBlock *alwaysGoto(MachineBasicBlock *From) const override {
    return AlwaysGoto[From->getNumber()];
  }

private:
  // Keep track of visited blocks.
  BitVector BlockScanned;
  std::vector<MachineBasicBlock*> AlwaysGoto;
  bool evaluateLoad(const MachineInstr &MI, const CellMapType &Inputs,
                    CellMapType &Outputs) const;
  bool evaluateFormalCopy(const MachineInstr &MI, const CellMapType &Inputs,
                          CellMapType &Outputs) const;
  unsigned getVirtRegFor(unsigned PReg) const;
  // Type of formal parameter extension.
  struct ExtType {
    enum { SExt, ZExt };
    char Type;
    uint16_t Width;
    ExtType() : Type(0), Width(0) {}
    ExtType(char t, uint16_t w) : Type(t), Width(w) {}
  };
  // Map VR -> extension type.
  typedef DenseMap<unsigned, ExtType> RegExtMap;
  RegExtMap VRX;
};

} // end namespace Tensilica
} // end namespace llvm
#endif // LLVM_LIB_CODEGEN_TENSILICA_TENSILICABITTRACKER_H
