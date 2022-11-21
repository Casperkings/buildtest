//===-- XtensaMachineFuctionInfo.h - Xtensa machine function info ---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares Xtensa-specific per-machine-function information.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_XTENSA_XTENSAMACHINEFUNCTIONINFO_H
#define LLVM_LIB_TARGET_XTENSA_XTENSAMACHINEFUNCTIONINFO_H

#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include <vector>

namespace llvm {

// Forward declarations
class Function;

/// XtensaFunctionInfo - This class is derived from MachineFunction private
/// Xtensa target-specific information for each MachineFunction.
class XtensaFunctionInfo : public MachineFunctionInfo {
  virtual void anchor();
  bool LRSpillSlotSet;
  int LRSpillSlot;
  bool FPSpillSlotSet;
  int FPSpillSlot;
  bool EHSpillSlotSet;
  int EHSpillSlot[2];
  unsigned ReturnStackOffset;
  bool ReturnStackOffsetSet;
  int VarArgsFrameIndex;
  int VarArgsStackOffset;
  int VarArgsFirstOffset;
  mutable int CachedEStackSize;

  // True iff an incoming argument on the stack is ever referenced.
  bool IncomingArgOnStackReferenced;

  // The vreg for the original stack pointer.
  unsigned OSPVReg;

  // Frame Index for the original stack pointer.
  int OSPFI;

  // Frame Index for the stack pointer before stack alignment
  int DwarfOSPFI;

  std::vector<std::pair<MachineBasicBlock::iterator, CalleeSavedInfo>>
      SpillLabels;

  MachineFunction *MF;
  mutable bool MaxCallFrameSizeSet;
  mutable unsigned MaxCallFrameSize;
  int EmergencyARSpillSlot;
  void calculateMaxCallFrameSize() const;
public:
  int getMaxCallFrameSize() const;

  XtensaFunctionInfo()
      : LRSpillSlotSet(false), FPSpillSlotSet(false), EHSpillSlotSet(false),
        ReturnStackOffsetSet(false), VarArgsFrameIndex(0),
        VarArgsStackOffset(0), VarArgsFirstOffset(0), CachedEStackSize(-1),
        IncomingArgOnStackReferenced(false), OSPVReg(0), OSPFI(0), MF(nullptr),
        MaxCallFrameSizeSet(false), MaxCallFrameSize(0),
        EmergencyARSpillSlot(0) {}

  explicit XtensaFunctionInfo(MachineFunction &_MF)
      : LRSpillSlotSet(false), FPSpillSlotSet(false), EHSpillSlotSet(false),
        ReturnStackOffsetSet(false), VarArgsFrameIndex(0),
        VarArgsFirstOffset(0), CachedEStackSize(-1),
        IncomingArgOnStackReferenced(false), OSPVReg (0), OSPFI(0), MF(&_MF),
        MaxCallFrameSizeSet(false), MaxCallFrameSize(0),
        EmergencyARSpillSlot(0) {}

  ~XtensaFunctionInfo() {}

  void setVarArgsFrameIndex(int off) { VarArgsFrameIndex = off; }
  int getVarArgsFrameIndex() const { return VarArgsFrameIndex; }

  void setVarArgsStackOffset(int off) { VarArgsStackOffset = off; }
  int getVarArgsStackOffset() const { return VarArgsStackOffset; }

  void setVarArgsFirstOffset(int off) { VarArgsFirstOffset = off; }
  int getVarArgsFirstOffset() const { return VarArgsFirstOffset; }

  void setIncomingArgOnStackReferenced(bool ref) {
    IncomingArgOnStackReferenced = ref;
  }
  bool getIncomingArgOnStackReferenced() const {
    return IncomingArgOnStackReferenced;
  }

  void setOSPVReg(int vreg) { OSPVReg = vreg; }
  int getOSPVReg() const { return OSPVReg; }

  void setOSPFI(int FI) { OSPFI = FI; }
  int getOSPFI() const { return OSPFI; }

  // This is introduced to avoid modification to unwind-dw2.c that is shared by 
  // both XCC and LLVM. If we generate the call0 prologue same as XCC, it would
  // introduce more instructions and thus degrade the performance. As the extra
  // memory for sp before stack alignment is only used for debugging and
  // exception handling, it should be a good workaround.
  void setDwarfOSPFI(int FI) { DwarfOSPFI = FI; }
  int getDwarfOSPFI() const { return DwarfOSPFI; }

  void setEmergencyARSpillSlot(int N) { 
    EmergencyARSpillSlot = std::max(N, EmergencyARSpillSlot); 
  }
  int emergencyARSpillSlot() { return EmergencyARSpillSlot; }
  int createLRSpillSlot(MachineFunction &MF);
  bool hasLRSpillSlot() { return LRSpillSlotSet; }
  int getLRSpillSlot() const {
    assert(LRSpillSlotSet && "LR Spill slot not set");
    return LRSpillSlot;
  }

  int createFPSpillSlot(MachineFunction &MF);
  bool hasFPSpillSlot() { return FPSpillSlotSet; }
  int getFPSpillSlot() const {
    assert(FPSpillSlotSet && "FP Spill slot not set");
    return FPSpillSlot;
  }

  const int *createEHSpillSlot(MachineFunction &MF);
  bool hasEHSpillSlot() { return EHSpillSlotSet; }
  const int *getEHSpillSlot() const {
    assert(EHSpillSlotSet && "EH Spill slot not set");
    return EHSpillSlot;
  }

  void setReturnStackOffset(unsigned value) {
    assert(!ReturnStackOffsetSet && "Return stack offset set twice");
    ReturnStackOffset = value;
    ReturnStackOffsetSet = true;
  }

  unsigned getReturnStackOffset() const {
    assert(ReturnStackOffsetSet && "Return stack offset not set");
    return ReturnStackOffset;
  }

  bool isLargeFrame(const MachineFunction &MF) const;

  std::vector<std::pair<MachineBasicBlock::iterator, CalleeSavedInfo>> &
  getSpillLabels() {
    return SpillLabels;
  }
};
} // End llvm namespace

#endif // LLVM_LIB_TARGET_XTENSA_XTENSAMACHINEFUNCTIONINFO_H
