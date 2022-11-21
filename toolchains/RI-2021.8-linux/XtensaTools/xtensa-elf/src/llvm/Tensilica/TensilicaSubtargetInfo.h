//===-- Tensilica/TensilicaSubtargetInfo.h - Tensilica Subtarget Info -----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the Tensilica common part of implementation of the
// TargetSubtargetInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_CODEGEN_TENSILICA_TENSILICASUBTARGETINFO_H
#define LLVM_LIB_CODEGEN_TENSILICA_TENSILICASUBTARGETINFO_H

#include "TensilicaInstrInfo.h"
#include "TensilicaRegisterInfo.h"
#include "TensilicaISelLowering.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/CodeGen/ScheduleDAG.h"

// HACK includes
#include "llvm/CodeGen/MachineDominators.h"
#include "llvm/CodeGen/MachineLoopInfo.h"

namespace llvm {

namespace Tensilica {

class SubtargetInfo : public TargetSubtargetInfo {
  const TargetMachine &TM;
public:
  /// This constructor initializes the data members to match that
  /// of the specified triple.
  ///
  template <typename... Args>
  SubtargetInfo(const Triple &TT, const std::string &CPU,
                const TargetMachine &TM, const std::string &FS, Args... args)
      : TargetSubtargetInfo(TT, CPU, FS, args...), TM(TM) {}

  // Bundling overlapping load and store allowed in RG-2015.0.
  virtual bool mayBundleOverlappingLoadAndStore() const { return true; }

  // Perform target specific adjustments to the latency of a schedule
  // dependency, for instructions.
  unsigned adjustInstrDependency(MachineInstr *DefMI, unsigned DefReg,
                                 MachineInstr *UseMI, unsigned UseReg,
                                 SDep::Kind Kind, unsigned Latency) const;
  void adjustSchedDependency(SUnit *Def, SUnit *Use, SDep &Dep) const final;

  const Tensilica::InstrInfo *getInstrInfo() const override {
    return getInstrInfo();
  }
  const Tensilica::RegisterInfo *getRegisterInfo() const override {
    return getRegisterInfo();
  }
  const Tensilica::ISelLowering *getTargetLowering() const override {
    return getTargetLowering();
  }

  const TargetMachine &getTargetMachine() const { return TM; }

  bool hasLoop() const { return HasLoop; }
  bool coproc() const { return Coproc; }
  bool enableHiFi2Ops() const { return HasHiFi2Ops && Coproc; }
  bool enableVecMemcpy() const { return HasVecMemcpy && Coproc; }
  bool enableVecMemset() const { return HasVecMemset && Coproc; }
  bool hasBooleans() const { return HasBooleans; }
  bool hasDepbits() const { return HasDepbits; }
  bool hasMul16() const { return HasMul16; }
  bool hasHWF32() const { return HasHWF32; }
  bool hasHWF64() const { return HasHWF64; }
  bool hasMinMax() const { return HasMinMax; }
  bool hasSext() const { return HasSext; }
  bool hasBrdec() const { return HasBrdec; }

protected:
  int LoadStoreWidth = 16;
  int LoadStoreUnitsCount = 1;
  int DataCacheBanks = 1;
  int ISSDataRAMBanks = 1;
  int NumMemoryAccessBanks = 2;
  int GatherLatency = 6;

  // Common features
  bool HasMul16 = false;
  bool HasBooleans = false;
  bool HasDepbits = false;
  bool Coproc = false;
  bool HasHiFi2Ops = false;
  bool HasVecMemcpy = false;
  bool HasVecMemset = false;
  bool HasLoop = false;
  bool HasHWF32 = false;
  bool HasHWF64 = false;
  bool HasMinMax = false;
  bool HasSext = false;
  bool HasBrdec = false;
public:
  int loadStoreWidth() const { return LoadStoreWidth; }
  int loadStoreUnitsCount() const { return LoadStoreUnitsCount; }
  int numMemoryAccessBanks() const { return NumMemoryAccessBanks; }
  int gatherLatency() const { return GatherLatency; }
  bool areMemAccessesBankConflict(int64_t OffsetDiff) const;

  // HACK API, these are temporary hacks to call xtensa api, when the tensilica
  //    part is not ready
  virtual void setEmergencyARSpillSlot(MachineFunction &MF, int N) const {}
  virtual void IMAPBBLoop(MachineBasicBlock *MBB, const MachineLoopInfo *MLI,
                          MachineDominatorTree *MDT, AAResults *AA) const {}

  // END HACK
};

} // namespace Tensilica
} // namespace llvm

#endif // LLVM_LIB_CODEGEN_TENSILICA_TENSILICASUBTARGETINFO_H
