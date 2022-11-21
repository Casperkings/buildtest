//===-- Tensilica/TensilicaDFAPacketizer.h - Tensilica DFA Information ----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the Tensilica common part of implementation of the
// DFAPacketizer class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_CODEGEN_TENSILICA_TENSILICADFAPACKETIZER_H
#define LLVM_LIB_CODEGEN_TENSILICA_TENSILICADFAPACKETIZER_H
#include "llvm/ADT/BitVector.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include <map>
#include <set>
#include <vector>

namespace llvm {

namespace Tensilica {

enum { LLVMXCC_OPC_NULL = -1 };
struct ConfigInstrInfo;
struct ConfigPacketizer;
struct Format;
struct FunctionalUnitInfo;

struct GenericOpcodeInfo {
  int CodeNormal;
  int CodeAll;

  // just for convenience, Variants = NormalVariants + DensityVariants
  std::vector<int> Variants;
  std::vector<int> NormalVariants;
  std::vector<int> DensityVariants;
};

struct OpcodeClassBundleInfoType {
  std::vector<bool> LegalFormats;
  std::vector<std::vector<int>> LegalSlots;
  BitVector SlotMask;
};

// A deterministic finite automaton (DFA) based FLIX packetizing info.
class DFAPacketizer {
  static constexpr int ZeroState = 0;
  static constexpr int IllegalState = -1;
  int State;

public:
  int getDfaState() const { return State; }
  void setDfaState(int S) { State = S; }
  DFAPacketizer() {
    State = ZeroState;
    DFAPacketizer::tryInitialize();
  }
  int reserveResources(int State, int Class) const {
    return BundlingDFATransitionTable[State * NumClass + Class];
  }
  static bool isLegal(int State) { return State != IllegalState; }
  bool canReserveResources(unsigned Opcode) const {
    return isLegal(reserveResources(State, getOpcodeClass(Opcode)));
  }
  bool canReserveResources(MachineInstr &MI) const {
    return isLegal(reserveResources(State, getOpcodeClass(&MI)));
  }
  void reserveResources(MachineInstr *MI) {
    State = reserveResources(State, getOpcodeClass(MI));
  }
  void reserveResources(unsigned Opcode) {
    State = reserveResources(State, getOpcodeClass(Opcode));
  }
  void clearResources() { State = ZeroState; }
  int getOpcodeClass(MachineInstr *MI) const {
    return OpcodeTable[MI->getOpcode()];
  }
  int getOpcodeClass(unsigned opcode) const { return OpcodeTable[opcode]; }
  BitVector getVSlot(int Class) const {
    return OpcodeClassBundleInfo[Class].SlotMask;
  }
  bool preferClass(int Classa, int Classb) {
    auto &&Ma = OpcodeClassBundleInfo[Classa].SlotMask;
    auto &&Mb = OpcodeClassBundleInfo[Classb].SlotMask;
    if (Ma == Mb)
      return false;
    auto U = Ma;
    U |= Mb;
    if (U == Mb)
      return true;
    return false;
  }

  bool slotCovered(int Classa, int Classb) {
    auto &&Ba = OpcodeClassBundleInfo[Classa];
    auto &&Bb = OpcodeClassBundleInfo[Classb];
    for (int F = 0; F < NumFormat; F++) {
      if (!Bb.LegalFormats[F])
        continue;
      if (!Ba.LegalFormats[F])
        return false;
      // TODO check slots
    }
    return true;
  }

  int isRelevant(int Classa, int Classb) const {
    auto &&Ma = OpcodeClassBundleInfo[Classa].SlotMask;
    auto &&Mb = OpcodeClassBundleInfo[Classb].SlotMask;
    if (Ma == Mb)
      return 3;
    auto J = Ma;
    J &= Mb;
    if (J.count() == 0)
      return 0;
    auto U = Ma;
    U |= Mb;
    if (U == Ma)
      return 1;
    if (U == Mb)
      return 4;
    return 2;
  }

  int specificScore(int Class) const {
    auto &&M = OpcodeClassBundleInfo[Class].SlotMask;
    return M.count();
  }

  static void preInitialize(const Tensilica::ConfigPacketizer *XCPK);
  static void tryInitialize();
  static bool initialize();

  static bool Initialized;
  static ManagedStatic<sys::SmartMutex<true>> InitMutex;
  // Fields, pre-initialized from Tensilica::ConfigPacketizer:
  static int *OpcodeClasses;
  static int *GenericOpcodeTable;
  static const struct Tensilica::Format *Formats;
  static llvm::ArrayRef<FunctionalUnitInfo> FunctionalUnits;
  static const int *BundlingDFATransitionTable;
  static int NumState;
  static int NumClass;
  static int NumFormat;
  static int NumVSlot;
  static int MidNumVSlot;
  static int MaxFormatSize;
  static int InstructionListEnd;
  static int MoveAr;
  static int Nop;
  // Fields, calculated during initialization:
  // the instruction class info
  static int *OpcodeTable;
  static int FullState;
  static int SoloClass;
  // the transition table
  static std::vector<OpcodeClassBundleInfoType> OpcodeClassBundleInfo;
  static std::map<int, GenericOpcodeInfo> GenericOpcodeMap;
  static std::map<int, std::pair<int, int>> SpecialOpcodeMap;
  static std::set<unsigned> MissingOpcodes;
};

// to determine the final format and slot of a flix-bundle
class Bundler {
public:
  Bundler() { Tensilica::DFAPacketizer::tryInitialize(); }

  int getFormat(llvm::ArrayRef<MachineInstr *> MIs);
  std::pair<int, std::vector<MCInst>> bundle(const MCInstrInfo &MCII,
                                             std::vector<MCInst> &MIV);
  int getFormatSize(int Fmt);
  const char *getFormatName(int Fmt);
private:
  std::vector<bool> getLegalFormats(llvm::ArrayRef<int> OCs);

  bool checkFormatRecursively(int Format, const ArrayRef<int> OCs,
                              std::vector<int> &WorkSet, unsigned IOC);

  int getFormat(llvm::ArrayRef<int> OCs, std::vector<bool> LegalFormats,
                std::vector<int> *Indices);

  void specializeGenericInstruction(const MCInstrInfo &MCII, MCInst &MCI,
                                    int AltOpcode);
  void specializeGenericInstruction(const MCInstrInfo &MCII, MCInst &Inst,
                                    int Format, int Slot);
};

} // namespace Tensilica
} // namespace llvm

#endif // LLVM_LIB_CODEGEN_TENSILICA_TENSILICADFAPACKETIZER_H
