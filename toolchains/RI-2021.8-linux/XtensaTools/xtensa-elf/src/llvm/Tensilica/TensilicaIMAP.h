//===-- TensilicaIMAP.h - Tensilica IMAP pattern representation in LLVM  --===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the interfaces that Tensilica uses to query about the
// custom/TIE instructions immediate range
//
//===----------------------------------------------------------------------===//

/*
  These data structures represent an IMAP pattern in the compiler and
config-gen.

  Structures generally resemble IMAP specification in TIE reference manual
(chapter 25.1).

  Notes that in,out,in-out and temporary arguments are held together in array of
IMAPArg.

  Example of usage (See imap_absssub16 for Vision P5/P6 configurations):

IMAPArg args1[4] = {
  {
    ARG_OUT, //rx
    0,
    Xtensa::XTRF_VECRegClassID //TypeId
  },
  {
    ARG_IN, //v0
    1,
    Xtensa::XTRF_VECRegClassID
  },
  {
    ARG_IN, //v1
    2,
    Xtensa::XTRF_VECRegClassID
  },
  {
    ARG_TMP, //t
    3,
    Xtensa::XTRF_VECRegClassID
  },
};


IMAPCodeArg CodeArgs[] = {
  // output
  {false, 0,  0},
  {false, 1,  0},
  {false, 2,  0},
  // input
  {false, 3,  0},
  {false, 1,  0},
  {false, 2,  0},
  {false, 0,  0},
  {false, 3,  0},
};

IMAPArgList arg_list[] = {
    {
      4,    //number of arguments
      args1 //list of arguments
    },
};

IMAPCode code_imap_absssub16_output[1] = {
  {
    Xtensa::IVP_ABSSSUBNX16, //Opcode
    3, //num args
    &CodeArgs[0], //Code args
  }
};

IMAPCode code_imap_absssub16_input[2] = {
  {
    Xtensa::IVP_SUBSNX16, //Opcode
    3, //num args
    &CodeArgs[3], //Code args
  },
  {
    Xtensa::IVP_ABSSNX16, //Opcode
    2, //num args
    &CodeArgs[6], //Code args
  }
};

IMAP imap_absssub16 = {
  {
    "imap_absssub16", //Name
    &arg_list[0], //arg-list
    {1, code_imap_absssub16_output},
    {2, code_imap_absssub16_input}
  },
};

*/

#ifndef LLVM_LIB_CONFIG_XTENSA_XTENSAIMAP_H
#define LLVM_LIB_CONFIG_XTENSA_XTENSAIMAP_H

#include "TensilicaConfig/TensilicaConfigImap.h"
#include "Tensilica/TensilicaInstrInfo.h"
#include "Tensilica/TensilicaRegisterInfo.h"
#include "Tensilica/TensilicaSubtargetInfo.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/CodeGen/MachineBranchProbabilityInfo.h"
#include "llvm/CodeGen/MachineDominators.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/TargetSchedule.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include <algorithm>
#include <map>
#include <set>
#include <utility>
#include <vector>

namespace llvm {
namespace Tensilica {
struct IMAP;
struct IMAPArgList;
struct IMAPArg;

enum IMAPArgDir {
  ARG_IN = 0,
  ARG_OUT,
  ARG_INOUT,
  ARG_TMP,
};

enum IMAPPatternProperties {
  IMAP_PROP_NONE = 0,
  IMAP_PROP_NONEXACT,
  IMAP_PROP_FUSED_MADD,
};

struct IMAPArg {
  IMAPArgDir Dir;
  unsigned Id;
  unsigned TypeId; // TypeId = 0 for immediate
};

struct IMAPArgList {
  unsigned NumArgs;
  const IMAPArg *args;
};

struct IMAPCodeArg {
  bool IsConst;
  unsigned Id;
  long long Cnst; // constant

  // This is relevant to the particular instruction within the pattern (not to
  // IMAP arguments definition).
  // It is easier to fill out this info in config-gen rather than extract it
  // from llvm MIs.
  IMAPArgDir CodeArgDir;
};

struct IMAPCode {
  unsigned Opcode; // Opcode = 0 for assignment
  unsigned NumArgs;
  const IMAPCodeArg *CodeArgs;
};

struct IMAPCodePattern {
  unsigned NumCodeInstrs;
  const IMAPCode *Code;
};

struct IMAP {
  const char *Name;
  const IMAPArgList *AgrList;
  IMAPCodePattern OutputPattern;
  IMAPCodePattern InputPattern;
  IMAPPatternProperties Prop;
};

struct TensilicaIMAPImpl {
  TensilicaIMAPImpl(MachineFunction &MF, const TargetMachine *tm,
                 MachineRegisterInfo *mri,
                 const MachineLoopInfo *MLI, MachineDominatorTree *MDT,
                 AAResults *AA, bool CopyProp, bool MultBBs);

  // Propagate trivial copies between virtual registers of same RC.
  // Returns true if code was changed, otherwise returns false.
  bool propagateCopies(MachineBasicBlock &MBB);

  // Build instruction order.
  void buildInstrOrder(const MachineBasicBlock &MBB);

  // Applies IMAP optimizations to a BB. Returns true of code was changed.
  bool workOnBB(MachineBasicBlock &MBB);

private:
  typedef std::map<unsigned, unsigned> MatchedRegsTy; // ArgId : Vreg
  typedef std::map<unsigned, int64_t> MatchedImmsTy;
  typedef std::map<MCPhysReg, const MachineInstr *>
      MatchedInputStatesTy; // state Reg :  state def Instr
  typedef std::map<unsigned, const MachineInstr *> MatchedInstrsTy;
  bool DoCopyProp;
  bool EnabledMultBBs;
  bool IsRISCV;

  MachineRegisterInfo *MRI;
  const TargetRegisterInfo *TRI;
  const Tensilica::InstrInfo *TII;
  const TargetMachine *TM;
  const MachineLoopInfo *MLI;

  MachineDominatorTree *MDT;
  AAResults *AA;
  MCSchedModel SchedModel;
  TargetSchedModel TSchedModel;

  // To represent states
  struct ImpUseArg {
    MCPhysReg Reg; // physical register
    int DefIdx;    // -1 means outside the pattern, operation number otherwise.
  };

  // Describes IMAP instruction argument in SSA form.
  struct SSACodeArg {
    unsigned DefIdx;
    unsigned DefOpndIdx;
    unsigned Id;
    unsigned OrigId;

    bool IsConst;
    long long Cnst;

    SSACodeArg(unsigned _DefIdx, unsigned _DefOpndIdx, unsigned _Id,
               unsigned _OrigId, bool _IsConst = false)
        : DefIdx(_DefIdx), DefOpndIdx(_DefOpndIdx), Id(_Id), OrigId(_OrigId),
          IsConst(_IsConst) {}
  };

  // Internal representation of an IMAP pattern.
  struct PatternInfoTy {
    // Collection of data structures for all patterns.
    SmallVector<SSACodeArg, 8> SSACodeArgsVect;
    SmallVector<SmallVector<unsigned, 8>, 8> PatternSSACodeArgs;
    SmallVector<SmallVector<ImpUseArg, 8>, 8> PatternImplicitUseArgs;
    SmallVector<std::set<MCPhysReg>, 8> PatternImplicitDefArgs;
    SmallVector<std::pair<unsigned, SmallVector<unsigned, 8>>, 8>
        SSACodeArgVersions;
    SmallVector<std::pair<unsigned, const XtensaImmRangeSet *>, 8> ImmRanges;
  };

  // Represents an actually matched pattern.
  struct MatchInfoTy {
    MatchInfoTy(unsigned _MatchedIMAP, const MatchedRegsTy &_MatchedRegs,
                const MatchedImmsTy &_MatchedImms,
                const MatchedInstrsTy &_MatchedInstrs,
                const MatchedInputStatesTy &_MatchedInputStates,
                const std::map<unsigned, bool> &_IsOutsideMasterBB,
                MachineInstr *_InsertionPoint)
        : MatchedIMAP(_MatchedIMAP), MatchedRegs(_MatchedRegs),
          MatchedImms(_MatchedImms), MatchedInstrs(_MatchedInstrs),
          MatchedInputStates(_MatchedInputStates),
          IsOutsideMasterBB(_IsOutsideMasterBB),
          InsertionPoint(_InsertionPoint), Disabled(false) {}

    unsigned MatchedIMAP;
    MatchedRegsTy MatchedRegs;
    MatchedImmsTy MatchedImms;
    MatchedInstrsTy MatchedInstrs;
    MatchedInputStatesTy MatchedInputStates;
    std::map<unsigned, bool> IsOutsideMasterBB;
    MachineInstr *InsertionPoint;
    bool Disabled;
  };

  // These conventions must be balanced with config-gen.
  static const unsigned IMM_TYPE = 0xFFFFFFFF;
  static const unsigned ASSIGNMENT_OPCODE = 0xFFFFFFFF;

  // Get original (non SSA) IMAP argument by its Id.
  const IMAPArg *getIMAPArg(const IMAP *Imap, unsigned ArgId);

  // Get a vector of SSA arguments for a given original IMAP argument.
  SmallVector<unsigned, 8> &getArgVersions(unsigned ArgId,
                                           bool CreatNew = false);

  // Construct a new SSA register argument
  // DefIdx - IMAP instruction index that defines the argument.
  // DefOpndIdx - def operand index.
  unsigned getNewSSAArg(unsigned ArgId, unsigned DefIdx, unsigned DefOpndIdx);

  // Construct a new const argument for patern's SSA form.
  int getNewSSAConst(long long Cnst);

  // Build SSA representation of an IMAP pattern.
  void buildIMAPCodePatternSSA(const IMAP *Imap);

  // Build def-Use chains for implicit operands.
  void buildIMAPImplicitOperands(const IMAP *Imap);


  // Retrieve SSA argument by its ID.
  const SSACodeArg &GetSSACodeArg(unsigned SSAArgId);

  // For a given LLVM MI Opcode get operand number that corresponds to
  // its number in TIE version of the instruction.
  unsigned getOpndIdx(const MCInstrDesc &MC, unsigned ArgN);

  // Check if some actual number is assigned to immediate argument in IMAP SSA
  // form and retrieve it.
  bool getMatchedImm(const std::map<unsigned, int64_t> &MatchedImms,
                     unsigned SSAArgId, int64_t &Imm);

  // Check if a virtual register is assigned to an argument in IMAP SSA form and
  // retrieve it.
  bool getMatchedVreg(const std::map<unsigned, unsigned> &MatchedRegs,
                      unsigned SSAArgId, unsigned &Vreg);

  // Check if Opcode in IMAP pattern is assignment operation.
  bool isOpAssignment(unsigned Opcode);

  // Check if MI is an assignment operation
  bool isMIAssignment(const MachineInstr &MI);

  // Estimate instruction cost.
  unsigned getInstrCost(const MachineInstr &MI);

  // Check if argument type in IMAP patter is an immediate.
  bool isImmType(unsigned TypeId);

  // Check if MI's Opcode is a "generic" instruction (e.g. ADD_N_GENERIC).
  bool isOpcodeGeneric(unsigned Opcode);

  // Check if Opcode has aliases.
  bool hasAliases(unsigned NodeOpcode);

  // Check if enabled patterns with instructions from multiple BBs 
  bool isMultBBs();

  // Expand LOAD_CONST instructions
  void expandLoadConstInstrs(MachineBasicBlock &MBB);

  // Initialize.
  void initialize(const llvm::Tensilica::ConfigImap *XCIMAP);

  // Get an Opcode for output instruction considering aliasing.
  bool getOutputOpcode(const IMAP *Imap, unsigned InstrN,
                       const MatchedRegsTy &MatchedRegs,
                       unsigned &OutputOpcode);

  // Check if in output instruction an argument is a final output
  // (in other words, it is not redefined by following instructions in the
  // output pattern).
  bool isFinalOutputArg(const IMAP *Imap, unsigned InstrN, unsigned ArgN);

  // Construct an output instruction for a matched pattern.
  // Populate RCCopiesBefore and RCCopiesAfter vectors
  // with copies between and after output instruction if they are required.
  MachineInstr *getNewInstr(const IMAP *Imap, unsigned InstrN,
                            const MatchedRegsTy &MatchedRegs,
                            const MatchedImmsTy &MatchedImms,
                            const MatchedInstrsTy &MatchedInstrs,
                            MachineInstr *InsertionPointMI,
                            SmallVector<MachineInstr *, 8> &RCCopiesBefore,
                            SmallVector<MachineInstr *, 8> &RCCopiesAfter,
                            std::map<unsigned, unsigned> &VregTemps);

  // Check if an immediate number potentially fits into immediate IMAP argument.
  bool isFitImm(const IMAP *Imap, unsigned SSAArgId, int64_t Imm);

  // Match "generic" MI Opcode (e.g. ADD_N_GENERIC) with an Opcode in IMAP
  // pattern.
  bool matchGeneric(unsigned NodeOpcode, unsigned InstrOpcode);

  // Match instruction aliases.
  bool matchAlias(unsigned NodeOpcode, unsigned InstrOpcode);

  // Match MI Opcode with an Opcode in IMAP pattern.
  bool matchOpcode(const IMAP *Imap, unsigned NodeOpcode,
                   const MachineInstr &MI);

  // Get next (from the bottom) instruction from input IMAP pattern that is not
  // matched yet.
  int getNextUnmatchedInstr(const IMAP *Imap,
                            const MatchedInstrsTy &MatchedInstr);

  // Scan IMAP output pattern and figure out actual restrictions on arguments
  // with immediate type.
  void setImmediateRanges(const IMAP *Imap);

  // Return a vector of argument's IDs that are assigned to a virtual register.
  SmallVector<unsigned, 8> getArgsForReg(const MatchedRegsTy &MatchedRegs,
                                         unsigned Vreg);

  // Match MI operands with pattern  (specified by Idx).
  bool matchInstrOpnds(const IMAP *Imap, unsigned PatternCandInstr,
                       const MachineInstr &MI, MatchedRegsTy &MatchedRegs,
                       MatchedImmsTy &MatchedImms);

  // Match instruction's subtree (including all Reg operands and states).
  bool matchInstrTree(const IMAP *Imap, unsigned PatternCandInstr,
                      const MachineInstr &CandMI, MatchedRegsTy &MatchedRegs,
                      MatchedImmsTy &MatchedImms,
                      MatchedInstrsTy &MatchedInstrs,
                      MatchedInputStatesTy &MatchedInputStates);

  // Match MI with and instruction from the pattern (specified by Idx).
  bool matchInstr(const IMAP *Imap, unsigned PatternCandInstr,
                  const MachineInstr &CandMI, MatchedRegsTy &MatchedRegs,
                  MatchedImmsTy &MatchedImms, MatchedInstrsTy &MatchedInstrs,
                  MatchedInputStatesTy &MatchedInputStates);

  // Check if matched instruction doesn't violate ordering rules.
  bool isOrderLegal(const IMAP *Imap, unsigned PatternCandInstr,
                    const MachineInstr &CandMI,
                    const MatchedRegsTy &MatchedRegs,
                    const MatchedImmsTy &MatchedImms,
                    const MatchedInstrsTy &MatchedInstrs);

  // Check if matched pattern has legal arguments
  bool isArgsLegal(const IMAP *Imap, const MatchedInstrsTy &MatchedInstrs);

  // Match an IMAP pattern with MBB.
  bool matchPattern(const IMAP *Imap, const MachineBasicBlock &MBB,
                    MatchedRegsTy &MatchedRegs, MatchedImmsTy &MatchedImms,
                    MatchedInstrsTy &MatchedInstrs,
                    MatchedInputStatesTy &MatchedInputStates);

  // Test if it legal to insert output instruction right before IP_MI.
  bool tryInsertionPoint(const IMAP *Imap, const MatchedInstrsTy &MatchedInstrs,
                         const std::set<const MachineInstr *> &MatchedMIs,
                         const MatchedRegsTy &MatchedRegs,
                         const MatchedInputStatesTy &MatchedInputStates,
                         const MachineInstr *IP_MI, std::string &ReasonStr);

  // Helper to determine correctness of transformation for patterns
  // that contain memory access instructions.
  bool
  isSafeToMoveMemInstr(const MachineInstr *MI, const MachineInstr *IP_MI,
                       const SmallVector<const MachineInstr *, 8> &Loads,
                       const SmallVector<const MachineInstr *, 8> &Stores,
                       const SmallVector<const MachineInstr *, 8> &Unmodeled);

  // Check if pattern replacement is legal.
  // Return MI insertion point that is legal and null otherwise.
  MachineInstr *
  IsReplacementLegal(const IMAP *Imap, const MatchedInstrsTy &MatchedInstrs,
                     const MatchedRegsTy &MatchedRegs,
                     const MatchedInputStatesTy &MatchedInputStates,
                     const std::map<unsigned, bool> &IsOutsideMasterBB,
                     std::string &ReasonStr);

  // Remove matched and replaced patterns.
  // Keep instructions from AllTempInstrs set.
  void removeInstructions(const std::vector<MatchInfoTy> &MatchedPatterns,
                          std::set<MachineInstr *> AllTempInstrs);

  // Returns pattern's operation index for a bottom instruction in the matched
  // pattern.
  unsigned getBottomPatternInstr(const MatchedInstrsTy &MatchedInstrs,
                                 const MachineBasicBlock &MasterBB);

  // Returns pattern's operation index for a top instruction in the matched
  // pattern.
  unsigned getTopPatternInstr(const MatchedInstrsTy &MatchedInstrs,
                              const MachineBasicBlock &MasterBB);

  // Debug functions
  void dumpMatched(const IMAP *Imap, MatchedRegsTy &MatchedRegs,
                   MatchedImmsTy &MatchedImms, MatchedInstrsTy &MatchedInstrs);

  void setCurrentPattern(unsigned n) { CurrentPattern = n; }

#if 0
  void dumpSSAPattern();
#endif

  // IMAP SSA argument's counter.
  unsigned SSACodeArgID = 0;

  // An IMAP pattern that is currently proceeded.
  unsigned CurrentPattern;

  std::set<MatchedInstrsTy> AlreadyTriedCandidates; // Matched instructions that
                                                    // already have been tried.

  std::map<const MachineInstr *, unsigned>
      InstrMap; // Map that used to figure out instruction order in BB.

  const std::map<unsigned, std::set<unsigned>>
      *GenericsMap; // Map with generics.

  const std::map<unsigned, std::set<unsigned>>
      *AliasesMap; // Map with instruction aliases.

  // Target-specific IMAP patterns.
  int ImapArraySize;
  const llvm::Tensilica::IMAP **ImapArray;

  // A vector with all supported patterns for a given configuration.
  std::vector<PatternInfoTy> PatternsInfo;
};
}
}
#endif

