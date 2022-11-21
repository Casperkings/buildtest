//===--------- XtensaPipelineEngine.h - Xtensa Pipeline Engine ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains type declaration of software pipelining phase
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_XTENSA_XTENSAPIPELINEENGINE_H
#define LLVM_LIB_TARGET_XTENSA_XTENSAPIPELINEENGINE_H

#include "llvm/CodeGen/MachineScheduler.h"
#include "llvm/CodeGen/ScheduleDAGInstrs.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/RegisterClassInfo.h"
#include "llvm/CodeGen/MachineBranchProbabilityInfo.h"
#include "Tensilica/TensilicaInstrInfo.h"
#include "Tensilica/TensilicaSubtargetInfo.h"
#include "Tensilica/TensilicaDFAPacketizer.h"
#include "llvm/ADT/SmallSet.h"
#include <array>
#include <list>
#include <map>
#include <set>

namespace llvm {
void initializeTensilicaSWPPass(PassRegistry &);
namespace Tensilica {

struct BankConflictPair {
  int IndexA;
  int IndexB;
  enum ConflictType {
    Zero,      // No conflict
    Invariant, // P = InvariantFactor / BankConflictFator
    Linear,    // General linear
    LinearA,   // No-conflict only if Omega == K
    LinearB,   // Conflict if Omega % K == M
    Unknown
  } Type;
  int Step;
  int Diff;
  int Size;
  int K;
  int Inv;
  void setIndex(int A, int B) {
    IndexA = A;
    IndexB = B;
  }
  bool isZero() { return Type == Zero; }
  bool isCritical() const {
    return Type == LinearA || (Type == Invariant && Inv == Banks);
  }
  bool isUnknown() { return Type == Unknown; }
  static BankConflictPair unknown() {
    BankConflictPair P;
    P.Type = Unknown;
    return P;
  }

  static BankConflictPair staticConflict(int Diff) {
    int ADiff = std::abs(Diff);
    BankConflictPair P;
    // Fall back to invariant, this is inacurate
    if (ADiff < LineSize || ADiff % LineSize >= AccessWidth)
      return P;
    P.Type = Invariant;
    P.Inv = Banks;
    return P;
  }

  BankConflictPair() : Type(Zero) {}
  BankConflictPair(int P) : Type(Invariant) { Inv = P; }
  BankConflictPair(int Step, int Diff, int Size)
      : Step(Step), Diff(Diff), Size(Size) {
    // OmegaDiff = (Omega * Step + Diff)
    // if(OmegaDiff < LineSize)
    //  No Conflict
    // else {
    //   R = OmegaDiff % LineSize;
    //   if (R < AccessWidth)
    //     No Conflict = R / AccessWidth;
    //   else
    //     No Conflict
    // }
    //
    // Note that the actual variable is Omega

    int ADiff = std::abs(Diff);
    if (Step % LineSize == 0) {
      if (ADiff % LineSize >= AccessWidth) {
        Type = Zero;
        return;
      } else {
        // if (Omega * Step + Diff) == 0, the same address, conflict otherwise
        if (Diff % Step == 0) {
          Type = LinearA;
          K = -Diff / Step;
          return;
        } else if (Step % Diff == 0) {
          if (Diff % LineSize == 0) {
            Type = Invariant;
            Inv = Banks;
          } else
            Type = Zero;
          return;
        }
        // inaccurate but use staticConflict
      }
    }

#if 0
      if (AccessWidth % Step == 0) {
        if (Diff % Step == 0) {
          // The omega factor is big, so roughly only check omega0
          if (Diff >= Step * 4) {
            if (Diff >= LineSize) {
              // almost conflict
              Type = Invariant;
              Inv = Banks;
            } else
              // almost no conflict
              Type = Zero;
            return;
          }
        }
      }
#endif

    *this = staticConflict(ADiff);
    return;
  }

  bool operator<(const BankConflictPair &A) const {
    bool Full = (Type == Invariant && Inv == Banks);
    bool FullA = (A.Type == Invariant && A.Inv == Banks);
    if (Full && FullA)
      return false;
    else if (Full)
      return true;
    else if (FullA)
      return false;

    if (Type == LinearA) {
      if (A.Type != LinearA)
        return true;
      else
        return false;
    } else if (A.Type == LinearA)
      return false;

    if (Type == Invariant && A.Type == Invariant)
      return Inv > A.Inv;

    return false;
  }

  static int LineSize;
  static int Banks;
  static int AccessWidth;
};

struct InstructionBranchPolicy {
  // If my priority is higher, branch
  int BranchPriority;
  // If we do not branch on this insn, try to assign when the predecessors is
  //   assigned or successor is assigned
  // Note, do not schedule backward if there is no predecessor
  // Also, we need to select the anchors wisely
  SmallVector<int, 4> Predecessors;
  SmallVector<int, 4> Successors;

  int Priority;
  // will be scheduled first,
  //  1. global-reg updates
  //  2. stores
  //  3. gatherd
  //  4. resource critical instructions
  bool Projected;
  // schedule forward or backward
  // there could be second levels and so on

  // toward +, 0, -
  bool Directed;
  int Direction;

  // even without a dependence, schedule an IV update after the last use
  bool RightAfter;
  int Anchor;

  InstructionBranchPolicy()
      : Projected(false), Directed(true), Direction(0), RightAfter(false),
        Anchor(0) {
    Priority = 0;
  }
  bool operator<(const InstructionBranchPolicy &A) const {
    if (Projected != A.Projected)
      return Projected;

    if (Directed != A.Directed)
      return Directed;

    return false;
  }
};

struct CyclicDist {
  // omega == -1 for INVALID
  int omega;
  int latency;
  int hop; // used to get rid of "equal" edges, i.e. a=>b 1, b=>c 1, and a=>c 2
  CyclicDist(int o, int l) : omega(o), latency(l), hop(1){};
  CyclicDist(int o, int l, int h) : omega(o), latency(l), hop(h){};
  CyclicDist() : omega(-1), latency(-1), hop(-1){};
  CyclicDist operator+(const CyclicDist &t) const {
    if (t.omega == -1 || omega == -1)
      return CyclicDist();
    return CyclicDist(omega + t.omega, latency + t.latency, hop + t.hop);
  }

  int dist(int II) const {
    if (omega < 0)
      return -(1 << 14); // avoid overflow
    return latency - omega * II;
  }
  bool isAcyclic() const { return omega == 0 && latency >= 0; }
  bool operator<(const CyclicDist &t) {
    if (omega == -1)
      return false;
    if (t.omega == -1)
      return true;
    if (t.omega < omega)
      return false;
    if (t.omega > omega)
      return true;
    if (t.latency > latency)
      return false;
    if (t.latency < latency)
      return true;
    if (t.hop > hop)
      return false;
    if (t.hop < hop)
      return true;
    return false;
  }
};

struct PressureLiveRange {
  unsigned Reg;
  int Def;
  std::set<std::pair<int, bool>> Uses;
  std::set<int> Updates;
  // TODO for any use1 must precede use2, remove use1 from the uses list
  PressureLiveRange(unsigned Reg, int Def, std::set<std::pair<int, bool>> Uses)
      : Reg(Reg), Def(Def), Uses(Uses) {}
};

typedef std::list<PressureLiveRange> PressureLiveRangeList;

struct RegisterClassPressureInfo {
  const char *Name;
  int NumberOfAvailable;
  std::set<unsigned> Globals;
  // TODO bool IsAR; // AR pressure+1 at II
  PressureLiveRangeList LiveRanges;
};

struct RegisterPressureInfoType {
  int NumberLiveRanges; // Total number
  int NumberOfWrapper;
  std::map<const TargetRegisterClass *, RegisterClassPressureInfo>
      RegclassPressure;
  void count(const TargetRegisterInfo *TRI) {
    NumberLiveRanges = 0;
    NumberOfWrapper = 0;
    for (auto &Iter : RegclassPressure) {
      Iter.second.Name = TRI->getRegClassName(Iter.first);
      NumberLiveRanges += Iter.second.LiveRanges.size();
      for (const auto &Range : Iter.second.LiveRanges)
        for (auto Use : Range.Uses)
          if (Use.first < Range.Def)
            NumberOfWrapper++;
    }
  }
};

struct TripCountInfo {
  Optional<int> Value;
  Optional<int> Min;
  Optional<int> Max;
  Optional<int> Average;
  Optional<int> Factor;
  bool NoUnroll;
  bool NoZcl1iter;
  void unroll(int U) {
    if (Value)
      Value = Value.getValue() / U;
    if (Min)
      Min = Min.getValue() / U;
    if (Max)
      Max = Max.getValue() / U;
    if (Average)
      Average = Average.getValue() / U;
    if (Factor) {
      Factor = Factor.getValue() / U;
      if (Factor.getValue() <= 1)
        Factor.reset();
    }
  }
  TripCountInfo() {
    NoUnroll = false;
    NoZcl1iter = false;
  }
  void setValue(int V) {
    if (V >= 0)
      Value = V;
  }
};

struct CyclicInstructionPosition {
  int Index;
  int Cycle;
  int Omega;
  // need to Copy the operands to avoid a dependence circle
  SmallSet<int, 4> Copy;
  CyclicInstructionPosition(int i = 0, int c = 0, int o = 0)
      : Index(i), Cycle(c), Omega(o) {}
  CyclicInstructionPosition(int i, int c, int o, SmallSet<int, 4> C)
      : Index(i), Cycle(c), Omega(o), Copy(C) {}
};

struct CPCompare {
  bool operator()(const CyclicInstructionPosition &B,
                  const CyclicInstructionPosition &C) {
    if (B.Omega < C.Omega)
      return true;
    if (B.Omega > C.Omega)
      return false;
    if (B.Cycle < C.Cycle)
      return true;
    if (B.Cycle > C.Cycle)
      return false;
    if (B.Index < C.Index)
      return true;
    if (B.Index > C.Index)
      return false;
    return false;
  }
};

struct IVAccess {
  int InstrID;
  unsigned UseOpIdx;

  bool HasDef;
  unsigned DefOpIdx;

  // ADDI/ADDMI is considered as XTENSA_INSTR_POST_INC_IMM
  // for now, only XTENSA_INSTR_BASE_DISP_IMM and XTENSA_INSTR_POST_INC_IMM is
  // handled
  XtensaLdStKind AccessPattern;
  // We can be a bit flexible about the Offset for ADDI/ADDMI, as it is not
  // really using it
  bool IsADDI;
  // Stay at the same ImmValue for ADD
  bool IsADD;

  unsigned ImmOpIdx;
  const XtensaImmRangeSet *ImmRange;

  int Offset;
  struct StageAdjust_t {
    int Offset;
    int ImmValue;
  };
  std::vector<StageAdjust_t> StageAdjust;
  int Cycle;
  int Omega;
  int ImmValue;
  IVAccess()
      : HasDef(false),
        AccessPattern(XtensaLdStKind::XTENSA_INSTR_UNKNOWN_ADDR_MODE),
        IsADDI(false), IsADD(false) {}
};
typedef std::map<unsigned, std::map<int, IVAccess>> IVAccessMap_T;

struct ScheduleResult {
  IVAccessMap_T IVAccessMap;

  // The number stage is increased aggressively, and we want to discard this
  // result if the register pressure is much higher or II not much lower than
  // the result without this flag set
  bool IncreasedStage;
  bool Summarized;
  bool Emittable;
  int II;
  int Stage;
  int VLength;
  double BankConflict;
  double VariableDelay;
  std::vector<CyclicInstructionPosition> Positions;

  // This is to help prologue and epilogue code emittion
  // TODO relax it, say, generate needed COPY instruction for the prologue
  // and epilogue
  std::vector<CyclicInstructionPosition> IssueOrder;

  /// Map for each register and the max difference between its uses and def.
  /// The first element in the pair is the max difference in stages. The
  /// second is true if the register defines a Phi value and loop value is
  /// scheduled before the Phi.
  std::map<unsigned, std::pair<unsigned, bool>> RegToStageDiff;
  std::map<unsigned, std::pair<int, int>> RegToStage;

  // The register allocation result
  bool RegAllocated;
  bool CriticalRegAllocated;
  bool PairedRegAllocated;
  std::map<const TargetRegisterClass *, bool> RegClassAllocated;
  bool AntiDepLayout;
  bool IVDependenceRelaxation;
  std::map<unsigned, unsigned> RegAllocation;
  // copy in, copy out, rename
  std::map<unsigned, std::array<bool, 3>> RegGlueInfo;
  // Do we need a VR or not here
  std::map<unsigned, unsigned> RegRename;
  int NEAR = 0;

  // The definition of Register at all the stages, the definitions are at
  // Index 0, Register Name
  // Index 1, BB-id
  // Index 2, Pipeline stage

  // For prolog BBs, fill in versions from the predecessors and ancestors

  // For kernel or epilog BB, fill in PHIs of -1 versions of the incoming edges,
  // say, the previous BB and some prolog BB

  // 1. The use before def is +1, those after is 0
  // 2. The scheduled stage is p
  // 3. The use is +1 stage, version 1
  // 4. The use is +2 stage, version 2, and so on
  // 5. The live out value is the last def or last PHI
  std::vector<std::vector<std::vector<unsigned>>> RegVersion;
  std::vector<std::vector<unsigned>> PrologRegVersion;

  // 1. Any use in the same stage or -1 uses the current version
  //   Be 0 or -1, the use is after the defintion in kernel
  // 2. Use Phi[0] if stage is +1
  // 3. Use Phi[1] if stage is +2, and so on
  // 3.1 The last Phi use is before the definition
  // If hardware/software moves are allowed, there could be multiple entry-phi
  // for each bb
  // Without these, there is at most one Phi per name

  ScheduleResult(unsigned int size)
      : IncreasedStage(false), Summarized(false), Positions(size),
        RegAllocated(false), AntiDepLayout(false),
        IVDependenceRelaxation(false) {}
  bool isScheduledAtStage(int I, int S) { return Positions[I].Omega == S; }
  int stageScheduled(int I) { return Positions[I].Omega; }
  int cycleScheduled(int I) { return Positions[I].Cycle; }

  /// Return the max. number of stages/iterations that can occur between a
  /// register definition and its uses.
  unsigned getStagesForReg(int Reg, unsigned CurStage) {
    /*
        std::pair<unsigned, bool> Stages = RegToStage[Reg];
        if (CurStage > Stage && Stages.first == 0 && Stages.second)
          return 1;
        return Stages.first;
    */
    auto Stages = RegToStage[Reg];
    int NP = Stages.second - Stages.first;
    // The stage after last definition and before has NP
    // And, NP decreased by 1 for later stages
    int Dec = (int)CurStage - Stage - Stages.first;
    if (Dec > 0)
      NP -= Dec;
    if (NP < 0)
      NP = 0;
    return NP;
  }

  /// The number of stages for a Phi is a little different than other
  /// instructions. The minimum value computed in RegToStageDiff is 1
  /// because we assume the Phi is needed for at least 1 iteration.
  /// This is not the case if the loop value is scheduled prior to the
  /// Phi in the same stage.  This function returns the number of stages
  /// or iterations needed between the Phi definition and any uses.
  unsigned getStagesForPhi(int Reg) {
    std::pair<unsigned, bool> Stages = RegToStageDiff[Reg];
    if (Stages.second)
      return Stages.first;
    return Stages.first - 1;
  }

  bool operator<(const ScheduleResult &S) const {
    bool RR = !RegAllocated;
    bool RCR = !CriticalRegAllocated;
    double VII = II + BankConflict + VariableDelay;
    bool SRR = !S.RegAllocated;
    bool SRCR = !S.CriticalRegAllocated;
    double SVII = S.II + S.BankConflict + S.VariableDelay;
    return std::tie(RR, RCR, VII, Stage) < std::tie(SRR, SRCR, SVII, S.Stage);
  }
};

struct PrecedenceEdge {
  llvm::SDep::Kind Kind;
  int Source;
  int Destination;
  int Latency;
  int Omega;
  int Variable;
  PrecedenceEdge(int Source, int Destination, int Latency, int Omega,
                 llvm::SDep::Kind Kind = llvm::SDep::Data, int Variable = 0)
      : Kind(Kind), Source(Source), Destination(Destination), Latency(Latency),
        Omega(Omega), Variable(Variable) {}
};

struct RegPrecedenceEdge {
  unsigned Reg;
  PrecedenceEdge Edge;
  RegPrecedenceEdge(unsigned Reg, PrecedenceEdge E) : Reg(Reg), Edge(E) {}
};

struct VariablePrecedenceEdge : public PrecedenceEdge {
  int LatencyDescription;
  VariablePrecedenceEdge(int Source, int Destination, int Latency, int Omega,
                         llvm::SDep::Kind Kind = llvm::SDep::Data)
      : PrecedenceEdge(Source, Destination, Latency, Omega, Kind) {}
  // TODO Expected latency distribution
};

typedef std::vector<PrecedenceEdge> PrecedenceVector;
typedef std::vector<VariablePrecedenceEdge> VariablePrecedenceVector;

raw_ostream &operator<<(raw_ostream &os, const InstructionBranchPolicy &P);
raw_ostream &operator<<(raw_ostream &os, const PressureLiveRange &LR);
raw_ostream &operator<<(raw_ostream &os, const RegisterPressureInfoType &RPI);
raw_ostream &operator<<(raw_ostream &os, const ScheduleResult &S);
raw_ostream &operator<<(raw_ostream &os, const PrecedenceEdge &E);

// A simplified dependence graph
struct SchedulePrecedenceGraph {
  // only none loop carried dependences count
  std::vector<std::vector<int>> predecessors;
  std::vector<std::vector<int>> successors;
  std::vector<std::vector<int>> AllPredecessors;
  std::vector<std::vector<int>> AllSuccessors;
  SchedulePrecedenceGraph(const PrecedenceVector &vpe, int size)
      : predecessors(size), successors(size), AllPredecessors(size),
        AllSuccessors(size) {
    for (auto ip = vpe.begin(); ip != vpe.end(); ip++) {
      AllPredecessors[ip->Destination].push_back(ip->Source);
      AllSuccessors[ip->Source].push_back(ip->Destination);
      if (ip->Omega == 0) {
        predecessors[ip->Destination].push_back(ip->Source);
        successors[ip->Source].push_back(ip->Destination);
      }
    }
  }
};

typedef std::vector<SmallVector<RegPrecedenceEdge, 6>> RegPrecedenceVector;

typedef std::map<const TargetRegisterClass *, int> RegisterDeficitType;
// A report about what happened so far
struct SWPScheduleReport {
  // do we ever tried to lower register pressure
  bool FeelRegisterPressure;
  // the maximum register pressure seen
  RegisterDeficitType RegisterDeficit;
  // based on the II ratio, may not want more unroll
  int ResourceMinII;
  int RecurrenceMinII;
  // may not want to spill if so
  bool IsLoadIntensive;
  // may want to stop unrolling
  bool FeelHard;
  SWPScheduleReport(int Res = -1, int Rec = -1)
      : FeelRegisterPressure(false), ResourceMinII(Res), RecurrenceMinII(Rec),
        IsLoadIntensive(false), FeelHard(false) {}
};

struct XtensaScheduleEngineBase {
  int Size;
  const std::vector<SUnit *> &MachineInstructions;
  const std::vector<int> &Classes;
  const Tensilica::InstrInfo *TII;
  PrecedenceVector Precedence;
  PrecedenceVector UnnormalizedPrecedence;
  PrecedenceVector AuxilaryPrecedence;
  // VariablePrecedenceVector VariablePrecedence;
  int NumVariablePrecedence;
  std::vector<int> Earliest;
  std::vector<int> Latest;
  std::vector<int> Slack;
  std::vector<int> CyclicSlack;
  std::vector<std::vector<CyclicDist>> MinDist;
  std::vector<std::vector<CyclicDist>> VirtualMinDist;
  int AcyclicLowerBound;
  std::vector<InstructionBranchPolicy> BranchPolicy;
  std::vector<double> ResourceMerit;
  // Use the following FU for branches
  std::map<const Tensilica::FunctionalUnitInfo *, double> BranchFunctionalUnits;
  // Model the following FU as constraints
  std::set<const Tensilica::FunctionalUnitInfo *> ModelFunctionalUnits;
  int ResMII;
  int RecMII;
  int VirtualRecMII;
  int MinII;
  // for each insn, which cluster it belongs to, -1 be void
  std::vector<int> ClusterID;
  std::vector<std::set<int>> Clusters;
  std::vector<std::vector<int>> ClusterPredecessors;
  std::vector<std::vector<int>> ClusterSuccessors;
  std::vector<std::vector<int>> ClusterPeers;

  // if there is no competation for resource
  bool IsResourceAllocationSimple;

  // All the bit vectors we care about for the kernel
  struct OverUse {
    BitVector Payload;
    int NumGood;
    OverUse(const BitVector &V, int N) : Payload(V), NumGood(N) {}
  };
  // index to overusevector for each insn
  std::vector<int> OverUseIndex;
  // vector of all overuses for the kernel
  std::vector<OverUse> OverUseVector;
  // for each cycle, the load of each overuse
  // std::vector<std::vector<int>>OverUseLoad;

  // Guess which instruction to be scheduled first 
  int HeuristicFirst;
  int HeuristicLast;

  // Make the precedence graph more efficient by removing redundant and "long"
  // edges, also calculate RecMII
  int normalizePrecedence();
  void computeMinDist(bool IsVirtual);
  int computeRecMinII(bool IsVirtual);
  void breakSymmetry(Tensilica::DFAPacketizer *);
  void computeResourceMerit(Tensilica::DFAPacketizer *DFA0);
  XtensaScheduleEngineBase(int Size,
                           const std::vector<SUnit *> &MachineInstructions,
                           const std::vector<int> &Classes,
                           const Tensilica::InstrInfo *TII,
                           const PrecedenceVector &Precedence)
      : Size(Size), MachineInstructions(MachineInstructions), Classes(Classes),
        TII(TII), UnnormalizedPrecedence(Precedence), Earliest(Size),
        Latest(Size), Slack(Size), CyclicSlack(Size), MinDist(Size + 2),
        BranchPolicy(Size), ResourceMerit(Size), ResMII(1), RecMII(1), MinII(1),
        ClusterID(Size), ClusterPredecessors(Size), ClusterSuccessors(Size),
        ClusterPeers(Size) {}

  // try to form clusters using register class with pressure, if RPI is
  // nullptr, everything is considered
  void formClusters(const RegisterPressureInfoType *RPI,
                    const TargetRegisterInfo *TRI);
  void cluster(const std::vector<int> &ClusterID);
  void initOverUse(Tensilica::DFAPacketizer *DFA0);
};

// This class accept an abstract description of a loop, and try to generate
// a software pipeline scheduling of it.
struct XtensaScheduleEngine {
  Tensilica::DFAPacketizer *ResourceModel;
  int Size;
  bool CPSDirection = false;
  const XtensaScheduleEngineBase &Base;
  int BranchPriorityThreshold = 0;
  PressureLiveRangeList PressureLiveRanges;
  const RegPrecedenceVector &RegPredecessor;
  const RegPrecedenceVector &RegSuccessor;
  int NumberOfWrapper;
  RegisterPressureInfoType RegisterPressureInfo;
  int ResMII;
  int RecMII;
  int MinII;

  int MaxII;
  int BestII;

  bool Trace0;
  bool Trace1;
  bool Trace2;
  bool Trace3;

  SchedulePrecedenceGraph PrecedenceGraph;
  int AcyclicScheduleLength;
  int MaxStage;
  int MinStage;

  // The issue cycle for the optimal acyclic schedule
  std::vector<int> AcyclicScheduled;
  std::vector<double> ProjectedCycle;
  // projected_d += ProjectedCycleGap if reversed
  double ProjectedCycleGap;
  std::vector<double> ProjectedCycleP;
  int VLengthRatioA;
  int VLengthRatioB;
  double ProjectedLength;
  double ProjectedLengthInverse;
  double ProjectedLengthP;
  double ProjectedLengthInverseP;

  // TODO register pressure on each class
  // Number of global register pressure
  // Extra AR register pressure at cycle-0 caused by the loop instruction
  // Number of local register pressure
  // Bypass register files with abundant registers, compared to the pressure

  // TODO extra cost inflicted by potential memory bank conflict
  // this may or may not be modeled by some kernel
  bool ReduceVLength;
  int PressureRegress;
  int MaxScheduleIncrement;

  // Increased stage aggressively
  bool IncreasedStage;

public:
  int BankConflictFactor;
  bool AllowBankConflict;
  const std::vector<BankConflictPair> *BankConflict;
  // For some scheduler, it is more efficient to generate multiple results
  // than one by one
  std::list<ScheduleResult> Results;
  enum Phase_T {
    Acyclic,
    Approximate,
    AutoStage,
    Main,
    ReduceMovement,
    ShrinkLiveRange,
    StaticProjection,
    ReducePressure,

    // do tuning to increase latency for gather pairs, and reduce bank conflict
    MainTune 
  } Phase;

  bool acyclic() { return Phase == Acyclic; }

  bool approximate() { return Phase == Approximate; }

  bool autoStage() { return Phase == AutoStage; }

  bool main() { return Phase == Main; }

  bool reduceMovement() { return Phase == ReduceMovement; }

  bool isStatic() { return reduceMovement() || staticProjection(); }

  bool shrinkLiveRange() { return Phase == ShrinkLiveRange; }

  bool reducePressure() { return Phase == ReducePressure; }

  bool staticProjection() { return Phase == StaticProjection; }

  bool mainTune() { return Phase == MainTune; }

  void setPhase(Phase_T P) { Phase = P; }

  const char *Name() const {
    const char *Name = "Modulo Scheduler";
    switch (Phase) {
    case Acyclic:
      Name = "Acyclic";
      break;
    case Approximate:
      Name = "Approximation";
      break;
    case AutoStage:
      Name = "Automatic stage";
      break;
    case Main:
      Name = "Main";
      break;
    case ReduceMovement:
      Name = "Reduce movement";
      break;
    case ShrinkLiveRange:
      Name = "Shrink live range";
      break;
    case ReducePressure:
      Name = "Reduce register pressure";
      break;
    case StaticProjection:
      Name = "Static projection";
      break;
    case MainTune:
      Name = "Main tune";
      break;
    }
    return Name;
  }
  void setProjection(const std::map<MachineInstr *, int> &Cycles, int, bool);
  void setProjection(const ScheduleResult &Schedule, bool Prime, bool HintM1,
                     bool Print);
  void scaleProjection(double R);
  // return the number of schedule found
  virtual int schedule() = 0;
  void newResult(const ScheduleResult &r) {
    Results.push_front(r);
    MaxII = std::min(MaxII, r.II);
    BestII = std::min(BestII, r.II);
  }
  void updateRecMII(int NewII) {
    if (RecMII < NewII) {
      RecMII = NewII;
      MinII = std::max(RecMII, ResMII);
    }
  }
  void updateResMII(int NewII) {
    if (ResMII < NewII) {
      ResMII = NewII;
      MinII = std::max(RecMII, ResMII);
    }
  }
  int ExtraGatherDelay;
  int IssueWidth;
  XtensaScheduleEngine(int Size, const XtensaScheduleEngineBase &Base,
                       PressureLiveRangeList &PressureLiveRanges,
                       const RegPrecedenceVector &RegPredecessor,
                       const RegPrecedenceVector &RegSuccessor,
                       Tensilica::DFAPacketizer *DFA0, int ASL)
      : ResourceModel(DFA0), Size(Size), Base(Base),
        PressureLiveRanges(PressureLiveRanges), RegPredecessor(RegPredecessor),
        RegSuccessor(RegSuccessor), PrecedenceGraph(Base.Precedence, Size),
        AcyclicScheduleLength(1), MaxStage(-1), MinStage(1),
        AcyclicScheduled(Size), ProjectedCycle(Size), ProjectedCycleP(Size),
        ReduceVLength(false), AllowBankConflict(true) {
    // Start with 1
    ResMII = Base.ResMII;
    RecMII = Base.RecMII;
    MinII = Base.MinII;
    MaxII = ASL;
    BestII = INT_MAX;
    Trace0 = true;
    Trace1 = false;
    Trace2 = true;
    Trace3 = true;
  }
  void setMaxStage(int M) { MaxStage = M; }
  void setMinStage(int M) { MinStage = M; }
  void setMaxII(int M) { MaxII = M; }
  void relaxMaxII(double F, int Cap) {
    int M = MaxII * F;
    MaxII = std::min(M, Cap);
  }
  int getMinII() { return MinII; }
  int getRecMinII() { return RecMII; }
  int getResMinII() { return ResMII; }
  void setPressureLiveRanges(PressureLiveRangeList &P) {
    PressureLiveRanges = P;
    NumberOfWrapper = 0;
    for (const auto &Range : PressureLiveRanges)
      for (auto Use : Range.Uses)
        if (Use.first < Range.Def)
          NumberOfWrapper++;
  }
  void setRegisterPressureInfo(RegisterPressureInfoType &P) {
    RegisterPressureInfo = P;
  }
  void setPressureRegress(int R) { PressureRegress = R; }
  virtual void setBankConflict(const std::vector<BankConflictPair> &BCV) {
    BankConflict = &BCV;
  }
  void allowBankConflict() { AllowBankConflict = true; }
  void disallowBankConflict() { AllowBankConflict = false; }
};

// Constraint programming scheduler
class XtensaCPSEngine : public XtensaScheduleEngine {
  int Jobs;
  int GlobalFailLimit;
  int GlobalTimeLimit;
  int Tile;
  int Direction;

public:
  virtual int schedule() override;
  XtensaCPSEngine(int Size, const XtensaScheduleEngineBase &Base,
                  PressureLiveRangeList plr,
                  const RegPrecedenceVector &RegPredecessor,
                  const RegPrecedenceVector &RegSuccessor,
                  // const VariablePrecedenceVector &vpv,
                  Tensilica::DFAPacketizer *DFA0, int ASL, int j)
      : XtensaScheduleEngine(Size, Base, plr, RegPredecessor, RegSuccessor,
                             DFA0, ASL),
        Jobs(j) {}
};

class CyclicSchedulerDAG;
bool PostprocessPipelineSchedule(CyclicSchedulerDAG *Driver,
                                 ScheduleResult &Result);
void UpdateRegisterPressure(CyclicSchedulerDAG *Driver, const TargetRegisterClass *RC, int D);
#include "TensilicaLSMSEngine.h"
// Lifetime sensitive modulo scheduler
class XtensaLSMSEngine : public XtensaScheduleEngine {
  CyclicSchedulerDAG *Driver;
  // All local registers
  const RegisterPressureInfoType *FullRPI;
  // Local register of those register classes with high pressure
  const RegisterPressureInfoType *RPI;

  MachineRegisterInfo *MRI;
  const RootRegisterClassInfo *RootInfo;
  double IncreaseAlpha, IncreaseBeta;
  int OptLevel;
  OPVector V;
  std::vector<int> SchedulingOrder;
  int Budget;
  std::vector<int> VariantRegs;
  int SpecifiedII;
  std::vector<int> Specified;

  int getStageCount(int II);
  int checkRegisterPressure(int II);
  void setupMinLT(std::vector<int> &MinLT, int II, const DistMatrix &M);
  std::vector<OrderHeuristic> Heuristics;
  std::map<std::pair<int, int>, BankConflictPair> BankConflictMap;
  std::map<std::pair<int, int>, BankConflictPair> BankConflictCriticalMap;
  std::map<std::pair<int, int>, BankConflictPair> BankConflictNonCriticalMap;

public:
  virtual int schedule() override;
  XtensaLSMSEngine(const RegisterPressureInfoType *RPI,
                   MachineRegisterInfo *MRI,
                   const RootRegisterClassInfo *RootInfo, int Size,
                   const XtensaScheduleEngineBase &Base,
                   PressureLiveRangeList plr,
                   // const VariablePrecedenceVector &vpv,
                   const RegPrecedenceVector &RegPredecessor,
                   const RegPrecedenceVector &RegSuccessor,
                   Tensilica::DFAPacketizer *DFA0, int ASL)
      : XtensaScheduleEngine(Size, Base, plr, RegPredecessor, RegSuccessor,
                             DFA0, ASL),
        FullRPI(RPI), RPI(RPI), MRI(MRI), RootInfo(RootInfo), V(Size + 2) {
    for (int i = 0; i < Size; i++)
      V[i].MI = Base.MachineInstructions[i]->getInstr();
  }
  void setOptLevel(int O);
  void setDriver(CyclicSchedulerDAG *S) { Driver = S; }
  void setupLinearSchedulingOrder(const ScheduleResult &R);
  void dumpState(raw_ostream &os, int II);
  void setupHeuristics(int S = 0);
  void printResult(int II);
  virtual void
  setBankConflict(const std::vector<BankConflictPair> &BCV) override;
  int getBankConflict(int Candidate, int Cycle, const ReservationTable &RT);
  int linearScheduleOrder(bool Topdown = false);
};

typedef DenseMap<unsigned, unsigned> ValueMapTy;
typedef SmallVectorImpl<MachineBasicBlock *> MBBVectorTy;
typedef DenseMap<MachineInstr *, MachineInstr *> InstrMapTy;
const unsigned NoPreference = UINT_MAX;
bool MIUsesReg(const MachineInstr *MI, unsigned Reg);
bool MIDefinesReg(const MachineInstr *MI, unsigned Reg);
DebugLoc getLoopDebugLoc(const MachineBasicBlock *LoopHeader);

enum SWPPartRedType {
  SWP_PART_RED_ADD,
  SWP_PART_RED_MINMAX,
};

struct PartRed {
  SWPPartRedType Type;
  unsigned ZeroOpc;
  unsigned AddOpc;
  unsigned OrigReg;
  std::set<unsigned> Regs;
};
struct DependenceDistance {
  enum DepTag { Unknown, NonNegative, Negative, Independent } Tag;
  int Value;
  DependenceDistance(DepTag T, int V) : Tag(T), Value(V) {}
  DependenceDistance(int V) {
    if (V >= 0) {
      Value = V;
      Tag = NonNegative;
    } else {
      Value = -V;
      Tag = Negative;
    }
  }
  static DependenceDistance UnknownDep() {
    return DependenceDistance(Unknown, 0);
  }
  static DependenceDistance ZeroDep() {
    return DependenceDistance(NonNegative, 0);
  }
  static DependenceDistance IndependentDep() {
    return DependenceDistance(Independent, 0);
  }
};

inline raw_ostream &operator<<(raw_ostream &os, const DependenceDistance &Dep) {
  if (Dep.Tag == DependenceDistance::Unknown)
    return os << "Unknown Dependence";
  else if (Dep.Tag == DependenceDistance::Independent)
    return os << "Indepdendent";
  else if (Dep.Tag == DependenceDistance::NonNegative)
    return os << Dep.Value;
  else if (Dep.Tag == DependenceDistance::Negative)
    return os << "-" << Dep.Value;
  return os;
}

class SWPDependenceGraph {
public:
  SWPDependenceGraph *RootGraph;
  int UnrollFactor;
  struct IV {
    unsigned Reg;
    int Step;
    IV(unsigned Reg, int Step) : Reg(Reg), Step(Step) {}
  };
  struct Access {
    IV *Index;
    int Offset;
    int Size;
  };
  std::map<MachineInstr *, Access> MIMap;
  std::map<unsigned, IV> IVMap;
  std::map<const MachineInstr *, std::pair<const MachineInstr *, int>>
      UnrollMap;
  SWPDependenceGraph() : RootGraph(nullptr), UnrollFactor(1){};
  SWPDependenceGraph(SWPDependenceGraph *Root, unsigned Unroll)
      : RootGraph(Root), UnrollFactor(Unroll) {
    for (auto &IVI : Root->IVMap) {
      IVMap.insert(std::pair<unsigned, IV>(
          IVI.first, IV(IVI.first, IVI.second.Step * UnrollFactor)));
    }
  };
  void clear() {
    IVMap.clear();
    MIMap.clear();
  }

  // +, -, 0, independent, or unknown
  DependenceDistance distance(MachineInstr *MIa, MachineInstr *MIb);

  void normalizeSize(int &Offseta, int &Offsetb, int Sizea, int Sizeb) {
    auto DivFloor = [](int a, int b) {
      if (a >= 0)
        return a / b;
      int q = a / b;
      int r = a % b;
      if (r != 0)
        return q - 1;
      return q;
    };
    if (Sizea != Sizeb) {
      assert(Sizea % Sizeb == 0 || Sizeb % Sizea == 0);
      if (Sizea > Sizeb) {
        Offsetb = DivFloor(Offsetb - Offseta, Sizea) * Sizea;
        Offseta = 0;
      } else {
        Offseta = DivFloor(Offseta - Offsetb, Sizeb) * Sizeb;
        Offsetb = 0;
      }
    }
  }

  // This is a bit hacky, returning the distance in number of bytes for two
  // accesses
  BankConflictPair bankConflict(MachineInstr *MIa, MachineInstr *MIb) const {
    auto Accessa = depInfo(MIa);
    auto Accessb = depInfo(MIb);
    if (Accessa == nullptr || Accessb == nullptr ||
        Accessa->Index != Accessb->Index) {
      return BankConflictPair::unknown();
    }
    int Step = Accessa->Index->Step;
    int Offseta = Accessa->Offset;
    int Offsetb = Accessb->Offset;
    int Diff = Offseta - Offsetb;
    return BankConflictPair(Step, Diff, std::max(Accessa->Size, Accessb->Size));
  }

  DependenceDistance distance(const Access &Accessa, const Access &Accessb) {
    assert(Accessa.Index == Accessb.Index);
    int Size = std::max(Accessa.Size, Accessb.Size);
    int Offseta = Accessa.Offset;
    int Offsetb = Accessb.Offset;
    normalizeSize(Offseta, Offsetb, Accessa.Size, Accessb.Size);
    int Diff = Offseta - Offsetb;
    if (Diff == 0)
      return DependenceDistance::ZeroDep();

    // Partial overlap
    if (Diff % Size != 0)
      return DependenceDistance::UnknownDep();

    Diff /= Size;
    int Step = Accessa.Index->Step / Size;
    // Partial overlap, say, original step is smaller than size
    if (Step == 0)
      return DependenceDistance::UnknownDep();
    if (Diff % Step == 0) {
      return Diff / Step;
    } else
      return DependenceDistance::IndependentDep();
  }

  const Access *depInfo(MachineInstr *MI) const {
    auto Iter = MIMap.find(MI);
    if (Iter == MIMap.end())
      return nullptr;
    return &Iter->second;
  }

  void addIV(unsigned Reg, int Step) {
    assert(IVMap.find(Reg) == IVMap.end());
    IVMap.insert(std::pair<unsigned, IV>(Reg, IV(Reg, Step)));
  }

  void addMI(MachineInstr *MI, unsigned Reg, int Offset, int Size) {
    auto &Access = MIMap[MI];
    auto IVI = IVMap.find(Reg);
    assert(IVI != IVMap.end());
    Access.Index = (&IVI->second);
    Access.Offset = Offset;
    Access.Size = Size;
  }

  void unrollMI(MachineInstr *MI, MachineInstr *OldMI, int Omega) {
    UnrollMap[MI] = std::pair<const MachineInstr *, int>(OldMI, Omega);
    assert(Omega < UnrollFactor && RootGraph != nullptr);
    auto MII = RootGraph->depInfo(OldMI);
    if (MII != nullptr)
      addMI(MI, MII->Index->Reg, MII->Offset + Omega * MII->Index->Step,
            MII->Size);
  }
  std::pair<const MachineInstr *, int> getUnroll(const MachineInstr *MI) {
    auto Iter = UnrollMap.find(MI);
    if (Iter != UnrollMap.end())
      return Iter->second;
    else
      return std::pair<const MachineInstr *, int>(nullptr, -1);
  }
};

struct SpillCandidate {
  const TargetRegisterClass *RC;
  unsigned Reg;

  // more uses means more reload and higher register pressure after spilling
  int NumReload;
  int NumUses;

  // Cost per load * NumUses
  int LoadCost;
  bool IsReMat;
  int Cost;
  bool operator<(const SpillCandidate &C) const { return Cost < C.Cost; }
  SpillCandidate(const TargetRegisterClass *RC, unsigned Reg, int NumReload,
                 int NumUses, int LoadCost, bool IsReMat, int Cost)
      : RC(RC), Reg(Reg), NumReload(NumReload), NumUses(NumUses),
        LoadCost(LoadCost), IsReMat(IsReMat), Cost(Cost) {}
};


typedef std::map<const TargetRegisterClass *, std::vector<SpillCandidate>>
    SpillCandidateMap;
// The function pass class of the Xtensa software pipeliner
class TensilicaSWP : public MachineFunctionPass {
public:
  MachineFunction *MF;
  CodeGenOpt::Level OptLevel;
  const Tensilica::SubtargetInfo *STI;
  MachineLoopInfo *MLI;
  LoopInfo *LI;
  MachineDominatorTree *MDT;
  MachineRegisterInfo *MRI;
  const MCRegisterInfo *MCRI;
  // const InstrItineraryData *InstrItins;
  const Tensilica::InstrInfo *TII;
  const Tensilica::RegisterInfo *TRI;
  LiveIntervals *LIS;
  RegisterClassInfo RegClassInfo;
  Tensilica::DFAPacketizer *ResourceModel;
  unsigned OpcodeJ;
  // for TargetOpcode::COPY, map from register class to the real opcode
  // expand the op if it is not single op based, otherwise we can keep it to
  // get better prolog epilog code
  std::map<const TargetRegisterClass *, unsigned> CopyOpcodes;

  // a regiter file is critical, if there is no move instructions defined,
  // say, the spilling is very expensive
  std::set<const TargetRegisterClass *> CriticalRegfile;
  // a register file is expensive, if there is no direct load/store instruction
  // say, it is expensive to spill
  std::set<const TargetRegisterClass *> ExpensiveRegfile;
  std::set<const TargetRegisterClass *> RequiredAllocRegfile;
  RootRegisterClassInfo RootRegisterClass;
  bool IsSSA;
  const MachineBranchProbabilityInfo *MBPI;
  std::map<unsigned, bool> LiveOutSet;
  std::map<unsigned, bool> LiveInSet;
  // All the invariant instructions are put aside here
  std::set<MachineInstr *> InvariantInstructions;
  SWPDependenceGraph RootDepGraph;

  // Map of register to the value in the machine function
  std::map<unsigned, int> MapRegConst;
  // Map of int value to register available in the current loop
  std::map<int, unsigned> MapConstReg;

  std::map<unsigned, MachineInstr *> MapRegSingleDef;

  // it is opt for size, so even if SWP is on, no unroll
  bool IsOS;
  bool CBox;
  std::map<unsigned, unsigned> RAPreference;
  Optional<unsigned> getRAPreference(unsigned Reg);

  void setRAPreference(unsigned Reg, unsigned PhyReg) {
    auto IB = RAPreference.insert(std::make_pair(Reg, PhyReg));
    if (IB.first->second == NoPreference)
      IB.first->second = PhyReg;
  }

  // ASSERT overlapping registers of register file and its super file share the
  // same ID
  std::map<unsigned, unsigned> PhysicalRegMap;
  unsigned getPhysicalRegID(unsigned PhyReg, const TargetRegisterClass *RC) {
    auto Iter = PhysicalRegMap.find(PhyReg);
    if (Iter == PhysicalRegMap.end()) {
      for (unsigned RegIter = 0, E = RC->getNumRegs(); RegIter < E; ++RegIter)
        PhysicalRegMap[RC->getRegister(RegIter)] = RegIter;
      Iter = PhysicalRegMap.find(PhyReg);
      assert(Iter != PhysicalRegMap.end());
    }
    return Iter->second;
  }

  // Primary IV have one or more updates in the loop
  // Secondary IV has a single definition based on a primary, but the offset
  //   may be none-immediate
  // There could be opaque uses, like MUL
  //
  // Generally, because of unrolling, we do not record the details, and the
  //  analyzer need to figure out the offset to the entry value, and if it is
  //  opaque, before any other action
  struct IVInfo {
    unsigned Name;
    unsigned Primary;
    int Increment;

    // Secondary = Opcode Primary Addend
    unsigned Opcode; // ADDI, ADD, or SUB
    bool AddendIsReg;
    unsigned AddendReg;
    int AddendImm;

    bool HasMultipleUpdate;

    IVInfo(unsigned Name, unsigned P, int I, unsigned Op, bool IsReg,
           unsigned AddReg, int AddImm, bool IsM = false)
        : Name(Name), Primary(P), Increment(I), Opcode(Op), AddendIsReg(IsReg),
          AddendReg(AddReg), AddendImm(AddImm), HasMultipleUpdate(IsM) {}
  };

  std::map<unsigned, IVInfo> IVInfoMap;
  std::vector<unsigned> PrimaryIVs;

  // Cache the target analysis information about the loop.
  struct LocalLoopInfo {
    MachineBasicBlock *TBB;
    MachineBasicBlock *FBB;
    SmallVector<MachineOperand, 4> BrCond;
    MachineInstr *LoopInductionVar;
    MachineInstr *LoopCompare;
    LocalLoopInfo()
        : TBB(nullptr), FBB(nullptr), LoopInductionVar(nullptr),
          LoopCompare(nullptr) {}
  };
  LocalLoopInfo LLI;

  struct StackSlotInfo {
    int Index;
    int Size;
    int Alignment;
    SmallVector<unsigned, 4> Values;
  };

  std::vector<StackSlotInfo> StackSlots;
  std::map<unsigned, int> StackSlotMap;

  struct VersionedLoop {
    // the version is [EntryBlock, ExitBlock)
    MachineBasicBlock *EntryBlock;
    MachineBasicBlock *ExitBlock;
    // There may be a remainder loop, which we do not care too much
    MachineBasicBlock *MainLoop;
    std::vector<StackSlotInfo> StackSlots;
    std::set<int> ReusedStackSlots;

    int II;
    double BankConflict;
    double EffectiveII;
    int Stage;
    int UnrollFactor;
    int RecMII;
    int VirtualRecMII;
    int ResMII;
    bool RegAllocated;
    int NEAR;
    bool HasSpill;
    // IV Dependence relaxation candidates
    std::set<unsigned> RelaxedIV;
    std::map<unsigned, unsigned> RegAllocation;
    SWPDependenceGraph *DepGraph;
    std::vector<PartRed *> PartReds;
    VersionedLoop(MachineBasicBlock *Entry, MachineBasicBlock *Exit,
                  MachineBasicBlock *Loop, int Factor,
                  const std::set<unsigned> &RelaxedIV,
                  SWPDependenceGraph *Graph,
                  const std::vector<PartRed *> &PartReds)
        : EntryBlock(Entry), ExitBlock(Exit), MainLoop(Loop),
          UnrollFactor(Factor), HasSpill(false), RelaxedIV(RelaxedIV),
          DepGraph(Graph), PartReds(PartReds) {}
    bool overheadTooHigh(int ConstTripCount) {
      if (ConstTripCount == -1)
        return false;
      int KernelTC = ConstTripCount / UnrollFactor + 1 - Stage;
      return KernelTC < 2;
    }

    bool ResourceUtilizationPotential() {
      return (ResMII * 1.3 < RecMII && VirtualRecMII * 1.19 < RecMII);
    }

    double estimatedCycle(bool IsConst, int TripCount, double PII) const {
      int R = TripCount % UnrollFactor;
      int W = UnrollFactor * (Stage - 1);
      int K = (TripCount - R - W);
      // If trip count is not const, add some more overhead for conditional
      // branches, a, cost of branches, b, scheduling overhead due to less
      // instructions per BB
      if (IsConst)
        return (double)K * EffectiveII / UnrollFactor + (1.4 * (R + W)) * PII;
      else
        return (double)K * EffectiveII / UnrollFactor + (1.65 * (R + W)) * PII +
               (Stage - 1 + R) * 2;
    }

    bool isBetter(const VersionedLoop &VL, const TripCountInfo &TCI0);
  };

  static char ID;
  TensilicaSWP()
      : MachineFunctionPass(ID), MF(nullptr), STI(nullptr), MLI(nullptr),
        LI(nullptr), MDT(nullptr), TII(nullptr) {
    initializeTensilicaSWPPass(*PassRegistry::getPassRegistry());
    // XHL = createXtensaHardwareLoops();
  }

  virtual bool runOnMachineFunction(MachineFunction &MF) override;

  virtual void getAnalysisUsage(AnalysisUsage &AU) const override;

  bool hasUseAfterLoop(unsigned Reg) { return LiveOutSet[Reg]; }
  void collectLiveSet(MachineBasicBlock *BB);
  unsigned getShadowOpcode(const MachineInstr *MI, unsigned Opcode) const;
  void analyzeInductionVariable(MachineBasicBlock *BB);
  void buildDependenceGraph(MachineBasicBlock *BB);
  void renameUnrolledLoop(MachineBasicBlock *LoopBlock,
                          std::vector<PartRed *> &PartReds);
  void rescheduleLoop(MachineBasicBlock *LoopBlock,
                      std::set<MachineInstr *> OverlapCopies);

  std::pair<unsigned, int> analyzeIVUpdate(const MachineInstr &MI);
  struct IVUseInfo {
    // for example, A is primary and B is secondary IV
    enum IVUseKind {
      // A = B + 1
      Fork,
      // load.i A, 1
      Use,
      // load.i B, 1
      UseSecond,
      // s32i A, X, 0
      Unknown
    } Kind;
    unsigned Secondary;
    bool UseOffset;
    int Offset;
    unsigned OffsetOpnd;

    IVUseInfo(IVUseKind K, unsigned Reg, bool UseOffset, int Offset,
              unsigned Opnd)
        : Kind(K), Secondary(Reg), UseOffset(UseOffset), Offset(Offset),
          OffsetOpnd(Opnd) {}
  };
  IVUseInfo analyzeIVUse(const MachineInstr &MI, unsigned Reg);

  // < isadd, addimmediate>
  std::pair<bool, bool> analyzeAdd(const MachineInstr &MI) const {
    unsigned Opcode = MI.getOpcode();
    if (TII->isADDImm(MI))
      return std::make_pair(true, true);
    if (TII->isSemantic(Opcode, Tensilica::MapOpcode::ADD) ||
        TII->isSemantic(Opcode, Tensilica::MapOpcode::SUB))
      return std::make_pair(true, false);
    return std::make_pair(false, false);
  }

  Optional<int> getImmediate(unsigned Reg) {
    Optional<int> Result;

    auto I = MapRegConst.find(Reg);
    if (I != MapRegConst.end())
      Result = I->second;
    return Result;
  }

  int getImmediate(const MachineInstr &MI) {
    auto &&MO = MI.getOperand(2);
    if (MO.isImm())
      return MO.getImm();
    if (MO.isReg()) {
      auto R = getImmediate(MO.getReg());
      if (R)
        return R.getValue();
      R = getImmediate(MI.getOperand(1).getReg());
      if (R)
        return R.getValue();
    }
    assert(0);
    return 0;
  }

  unsigned getImmediateReg(MachineBasicBlock *LoopBlock, int Val) {
    auto &&Iter = MapConstReg.find(Val);
    if (Iter != MapConstReg.end())
      return Iter->second;
    const TargetRegisterClass *GPRegClass = TRI->getPointerRegClass();
    unsigned Reg = MRI->createVirtualRegister(GPRegClass);
    TII->loadImmediate(*LoopBlock, LoopBlock->instr_begin(), Reg, Val);
    MapConstReg[Val] = Reg;
    MapRegConst[Reg] = Val;
    return Reg;
  }

  MachineInstr *getSingleDef(unsigned Reg) {
    auto Iter = MapRegSingleDef.find(Reg);
    if (Iter == MapRegSingleDef.end())
      return nullptr;
    return Iter->second;
  }
  void generatePartRedProlog(MachineBasicBlock *PH,
                             std::vector<PartRed *> &PartReds);
  void generatePartRedEpilog(MachineBasicBlock *BB,
                             std::vector<PartRed *> &PartReds);
  int MemWARLatency;

  // The loop instructions are similar
  unsigned LOOPOpc;
  unsigned LOOPGTZOpc;
  unsigned LOOPNEZOpc;

  // For RISCV, LDAWFI does not exist, and ADDI is used directly, this does
  //   not matter too much at the first place
  unsigned LDAWFIOpc;

  unsigned ADDIOpc;

  // In case we need the Target ID
  bool IsRISCV;

private:
  // Return true if the loop can be software pipelined.  The algorithm is
  // restricted to loops with a single basic block that the target is
  // able to analyze.
  bool canPipelineLoop(MachineLoop *Loop);

  // Attempt to schedule the specified loop. Identifies candidate loops, do
  // the preparation, invoke the scheduler, and emit code for the schedule
  // result successful.
  bool scheduleLoop(MachineLoop *Loop);
  bool moduloScheduleLoop(MachineLoop *Loop);
  // Make a copy and unroll the Loop by UnrollFactor
  // Connect the generated basic block between the only predecessor of
  // PreheaderBlock and MergeBlock

  // TODO take unrolled instruction as symmetry to each other and add relaxed
  // precedence constraint among them
  // As the symmtricity is pseudo, the precedence edge is relaxed as X1 > X0 + 2
  // (relaxation degree, a small constant to be determined)
  // Maybe we can do this more generally, say, any instructions X and Y are of
  // same scheduling class, Y > X + 2, if Y is after X, the
  // order is determined by several factors
  // 1. source ordering
  // 2. earliest end, or latest start
  VersionedLoop unrollLoop(MachineBasicBlock *PreheaderBlock,
                           MachineBasicBlock *MergeBlock, MachineLoop *Loop,
                           int UnrollFactor, const TripCountInfo &TCI,
                           int BestII);

  void copyPropagate(MachineBasicBlock *LoopBlock,
                     std::set<MachineInstr *> &OverlapCopies,
                     std::vector<PartRed *> &PartReds,
                     std::set<unsigned> &NewRegs,
                     const std::set<MachineInstr *> &NewCopies);
  bool copyPropagate(MachineBasicBlock *LoopBlock, MachineInstr *MI,
                     bool TiedOnly, std::set<MachineInstr *> &OverlapCopies,
                     std::vector<PartRed *> &PartReds,
                     std::set<unsigned> &NewRegs,
                     const std::set<MachineInstr *> &NewCopies);
  void IMAPUnrolledLoop(MachineBasicBlock *LoopBlock);

  void simplifyInductionVariable(MachineBasicBlock *PreheaderBlock,
                                 MachineBasicBlock *LoopBlock, int Unroll,
                                 std::set<unsigned> &PrimaryIV);

  struct IVInstructionType {
    MachineInstr *MI;
    unsigned UseIV;
    unsigned DefIV;
    // TODO rename
    int IVOffset;
    enum InstructionType {
      Erased,
      AddI,   // f=s+1;
      Update, // p+=1;
      Fork,   // s=p+1;
      MemoryI,
      MemoryIP,
      SecondMemoryI,
      Opaque
    } Type;
    // Use RangeI for ADDI
    const XtensaImmRangeSet *RangeI;
    const XtensaImmRangeSet *RangeIP;
    bool UseOffset;
    int Offset;
    bool isMemoryIP() { return Type == MemoryIP; }
    bool isMemoryI() { return Type == MemoryI; }
    bool isSecondMemoryI() { return Type == SecondMemoryI; }
    bool isOpaque() { return Type == Opaque; }
    bool isUpdate() { return Type == Update; }
    bool isErased() { return Type == Erased; }
    bool isFork() { return Type == Fork; }
    void erase() { Type = Erased; }
    bool immContains(int Imm, const Tensilica::InstrInfo *TII) {
      if (Type == MemoryIP)
        return RangeIP && RangeIP->contains(Imm);
      if (Type == MemoryI || Type == SecondMemoryI)
        return RangeI && RangeI->contains(Imm);
      if (Type == Opaque || Type == Erased)
        return false;
      if (Type == Fork && !UseOffset)
        return false;
      return (RangeI && RangeI->contains(Imm)) || TII->isImmADDI(Imm);
    }

    IVInstructionType(MachineInstr *MI, const Tensilica::InstrInfo *TII)
        : MI(MI), UseIV(0), DefIV(0), IVOffset(0), Type(Opaque),
          RangeI(nullptr), RangeIP(nullptr), UseOffset(true), Offset(0) {
      unsigned Opcode = MI->getOpcode();
      const auto *InstrProp = TII->getInstrProperty(Opcode);
      if (TII->isADDImm(*MI)) {
        RangeI = &InstrProp->getImmRange(2);
        return;
      }
      if (InstrProp->isPostIncImm()) {
        unsigned OffsetOpnd = InstrProp->getOffsetOpnd();
        RangeIP = &InstrProp->getImmRange(OffsetOpnd);
        unsigned IOpcode = TII->mapToAddressMode(
            Opcode, XTENSA_INSTR_POST_INC_IMM, XTENSA_INSTR_BASE_DISP_IMM);
        if (IOpcode != Opcode) {
          const auto *InstrProp = TII->getInstrProperty(IOpcode);
          unsigned OffsetOpnd = InstrProp->getOffsetOpnd();
          RangeI = &InstrProp->getImmRange(OffsetOpnd);
        }
        return;
      }
      if (InstrProp->isBaseDispImm()) {
        unsigned OffsetOpnd = InstrProp->getOffsetOpnd();
        RangeI = &InstrProp->getImmRange(OffsetOpnd);
        unsigned IPOpcode = TII->mapToAddressMode(
            Opcode, XTENSA_INSTR_BASE_DISP_IMM, XTENSA_INSTR_POST_INC_IMM);
        if (IPOpcode != Opcode) {
          const auto *InstrProp = TII->getInstrProperty(IPOpcode);
          unsigned OffsetOpnd = InstrProp->getOffsetOpnd();
          RangeIP = &InstrProp->getImmRange(OffsetOpnd);
        }
        return;
      }
    }
    const char *getTypeString() const {
      switch (Type) {
      case Erased:
        return "Erased";
      case AddI:
        return "AddI";
      case Update:
        return "Update";
      case Fork:
        return "Fork";
      case MemoryI:
        return "MemoryI";
      case MemoryIP:
        return "MemoryIP";
      case SecondMemoryI:
        return "SecondMemoryI";
      case Opaque:
        return "Opaque";
      }
      return nullptr;
    }
  };
  typedef std::vector<IVInstructionType> IVInstructions;
  std::pair<bool, bool> analyzeIV(MachineBasicBlock *PreheaderBlock,
                                  MachineBasicBlock *LoopBlock, int Unroll,
                                  unsigned PrimaryReg, IVInstructions &Action);

  bool buildIVVector(MachineBasicBlock *LoopBlock, unsigned PrimaryReg,
                     IVInstructions &Action, std::set<unsigned> &AllIV,
                     bool &HasLoad, bool &HasStore);
  void collectConst();
  void collectConst(MachineBasicBlock *MBB);
  void addOneConst(unsigned Reg);
  bool analyzeSingleUpdate(int Increment, unsigned PrimaryReg,
                           IVInstructions &Action, int UpdateIdx);
  void reduceUpdates(int Increment, unsigned PrimaryReg, IVInstructions &Action,
                     bool Merge);
  void mergeSecondary(int Increment, unsigned PrimaryReg,
                      IVInstructions &Action);
  bool offsetAccess(IVInstructions &Work, int I, int J, int Offset);
  void reducePostUpdates(int Increment, unsigned PrimaryReg,
                         IVInstructions &Action);
  void applySimplification(unsigned PrimaryReg, MachineBasicBlock *LoopBlock,
                           IVInstructions &Action);

  void eraseVersion(MachineBasicBlock *ForkBlock, MachineBasicBlock *EntryBlock,
                    MachineBasicBlock *ExitBlock, VersionedLoop *VL);
  void collectMembers(MachineBasicBlock *EntryBlock,
                      MachineBasicBlock *ExitBlock,
                      std::set<MachineBasicBlock *> &Set);
  void initRegClass();
  bool isInvariantInstruction(MachineInstr *MI, MachineBasicBlock *MBB) const;
  bool isPartRedInstr(MachineInstr *MI, int MinLat);
  unsigned calcInstrNumInMBB(MachineBasicBlock *LoopBlock);
  void findPartRedCandidates(MachineBasicBlock *LoopBlock,
                             std::vector<PartRed *> &PartReds,
                             std::map<MachineInstr *, PartRed *> &PartRedInstrs,
                             int BestII);
  void renamePartRedInstr(MachineInstr *PI, PartRed *PS);
  void updateRegInPartRed(unsigned OldReg, unsigned NewReg,
                          std::vector<PartRed *> &PartReds);
  friend inline raw_ostream &operator<<(raw_ostream &os,
                                        const IVInstructionType &IVU) {
    IVU.MI->dump();
    os << format("Type %s, IVOffset %d, Offset %d", IVU.getTypeString(),
                 IVU.IVOffset, IVU.Offset)
       << ", DEF-IV " << printReg(IVU.DefIV) << ", USE-IV "
       << printReg(IVU.UseIV) << '\n';
    return os;
  }
  // examination of candidate for expensive register file has to be done
  // before "cheaper" ones, as secondary invariants may be generated.
  SpillCandidateMap collectSpillCandidates(MachineBasicBlock *MBB,
                                           bool Expensive = false);
  // std::map<unsigned, SpillCandidate> refineSpillCandidates(const
  // SpillCandidateMap &, const RegisterDeficitType &);
  void doSpilling(const std::map<unsigned, SpillCandidate> &,
                  MachineBasicBlock *, VersionedLoop &VL);
  void generateSWPFailComment(MachineBasicBlock *, StringRef);
};

// This class derives ScheduleDAGInstrs, and is the driver of the pipeliner
// Say, it interacts with the compiler infrastruction, get the information
// to build an abstract description of the loop, invoke the scheduler
// kernel, and re-construction the loop structure based on the result.
class CyclicSchedulerDAG : public ScheduleDAGInstrs {
public:
  TensilicaSWP *Pass;
private:
  XtensaScheduleEngine *Engine;
  bool Scheduled;
  // MachineLoop *Loop;
  MachineBasicBlock *LoopHeader;
  BranchProbability ProbLoop;
  BranchProbability ProbExit;
  LiveIntervals &LIS;
  const RegisterClassInfo &RegClassInfo;
  PrecedenceVector Precedence;
  // VariablePrecedenceVector VariablePrecedence;
  int NumVariablePrecedence;
  TripCountInfo TCI;
  std::set<unsigned> RelaxedIV;
  PressureLiveRangeList PressureLiveRanges;
  RegisterPressureInfoType RegisterPressureInfo;
  struct Projection {
    std::map<MachineInstr *, int> Cycles;
    int Length;
  } ProjectionReg, Projection, ProjectionASL;

  // Adjustment of IV at the entry and exit of all epilogue BB
  std::map<unsigned, std::vector<std::pair<int, int>>> IVEpilogueAdjust;
  // Adjustment if IV at the top
  std::map<unsigned, int> IVOffset;
  unsigned UnrollFactor;
  // The loop count variable
  int LoopCountReg;

  // Turn it off and retry, if failed to restore
  bool ToRelaxDependence;

  bool HasCall;
  bool HasSideEffects;

  std::vector<SUnit *> RealInstructions;
  // These two are used by minlt heuristic
  RegPrecedenceVector RegPredecessor;
  RegPrecedenceVector RegSuccessor;
  // Need to change some of the pseudo opcode in the kernel, like COPY
  std::vector<unsigned> ShadowOpcodes;
  // Used by packetizer
  std::vector<int> ScheduleClasses;
  // If a frame index is used
  std::vector<Optional<int>> FrameIndexes;

  // FIXME better replacement of implicit_def
  std::set<unsigned> ImplicitDefs;
  std::set<unsigned> Undefs;

  // Keep all AntiDependence, to order instructions in a same cycle if we are
  // not emitting bundles directly
  std::map<std::pair<int, int>, std::pair<int, bool>> AntiDependence;

  // Possible load bank conflict
  // Assume LineSize = AccessWidth * NBanks
  //
  // 1. Y-memory and none Y-memory does not conflict
  // 2. Check if there is known alignment issue, e.g. Type or Numeric info
  //    pX = AccessWidth * NBanks * N
  //    pY = AccessWidth * NBanks * M
  //    pZ = AccessWidth * NBanks * M + AccessWidth
  //    pX and pY is going to conflict while they do not conflict with pZ
  // 2. Two accesses based on a same IV, so the Diff is known
  // 3. Frame index accesses are somewhat flexible TODO
  // 4. A constant and an IV access, P = Step / (LineSize * NBanks)
  // 5. Finally, if not anything above, it is 1/N
  //
  // TODO NYI
  // 1. There are some pointers have bigger alignment factor than access size
  //    or step, say
  //    p = align 64 ...
  //    for i
  //      ... *p
  //      p += 4;
  // 2. There are rare cases, that the step of the two accesses are different,
  // but the initial distance is known. Since the current infrastructure is not
  // able to do so, IGNORED for now.

  // Actual conflict possibility = X / BankConflictFactor
  bool ModelBankConflict;
  int BankConflictFactor;

  //
  std::vector<BankConflictPair> BankConflict;
  // Only model critical ones when trying to lower register pressure
  std::vector<BankConflictPair> BankConflictCritical;

  // Map from phi result to loop input, key and value are both VR
  ValueMapTy PhiVRMap;
  struct PhiInfo {
    bool LiveOut;
    unsigned PreheaderInput;
    unsigned PhiResult;
    unsigned LoopInput;
    // the LoopInput is always previous phi in the chain or the real definition
    PhiInfo(bool L, unsigned I, unsigned R, unsigned Loop)
        : LiveOut(L), PreheaderInput(I), PhiResult(R), LoopInput(Loop) {}
    PhiInfo() { PhiResult = 0; }
  };

  struct SWPVariable {
    // multiple definitions for tied operands only handled with physical
    // register allocation
    int RealDef;
    // The output VR , and used as Identification
    unsigned Output;
    // The input VR to the loop and PHI
    //  name of each phi and if it is live-out
    std::vector<PhiInfo> PhiQueue;
    // unsigned PreheaderInput;
    // unsigned PhiName;
    // this is useful if we do register allocation
    std::vector<unsigned> TiedNames;
    // if the real definition is live-out, the number of phis is at least one
    // for
    //    all kernel-and-epilog BBS
    // if the first phi is live-out, it is at least 2, and so on
    // the live-out value is the 0, 1, 2, def/phi of last epilog BB
    bool LiveOut;
    int PhiLiveOut;
    // It is a Phi, and not only, but also the loop-input is loop invariant
    bool IsInvariant;
    SWPVariable(int d, /*unsigned in, */ unsigned out,
                /*unsigned phi, */ bool l, bool iv)
        : RealDef(d), Output(out),
          /*PreheaderInput(in), PhiName(phi), */ LiveOut(l), IsInvariant(iv) {}
  };

  // For register allocation purpose
  struct CyclicLiveRange {
    unsigned ID;
    int Start;
    int Stop;
    // Relative weight regarding the smallest=1
    int Weight;

    CyclicLiveRange(unsigned i = 0, int start = 0, int stop = 0)
        : ID(i), Start(start), Stop(stop), Weight(0) {}
    bool interfere(const CyclicLiveRange &R) {
      // any of them is full range
      if (Start == -1 || R.Start == -1)
        return true;
      // Interfere if the two defs are on the same cycle, even if they are dead
      // Assuming a variable is either all dead, or each def have at least one
      // use
      if (R.Start == Start)
        return true;
      if (Start <= Stop) {
        if (R.Start <= R.Stop)
          // return !(R.Stop <= Start || Stop <= R.Start);
          return R.Stop > Start && Stop > R.Start;
        else
          return (Start < R.Stop) || (Stop > R.Start);
      } else {
        if (R.Start <= R.Stop)
          return (R.Start < Stop) || (R.Stop > Start);
        else
          return true;
      }
    }
  };

  // The struct to describe the register allocation constraints for one register
  // class
  struct RegAllocDescription {
    // The virtual registers need one whole register across the loop
    // 1. Global read only
    // 2. Updated at each iteration and the live range cover the whole loop
    // 3. Not the above two, but still conflict with all other live ranges
    std::vector<CyclicLiveRange> Allocated;
    // The virtual registers need to be allocated
    std::vector<CyclicLiveRange> ToAllocate;
    bool Weighted;

    unsigned NumLiveLoopBack;
    // TODO general physical register is harder to describe, basically
    // 1. assign allocated physical register to some small value
    // 2. pre-assign variable to these registers, say, x=5 (a7)

    // The interference graph
    std::vector<std::vector<bool>> IG;
    void addLiveRange(unsigned I, int start, int stop) {
      ToAllocate.push_back(CyclicLiveRange(I, start, stop));
    }
    void computeInterferenceGraph() {
      int Size = ToAllocate.size();
      IG.resize(Size);
      for (auto &Row : IG) {
        Row.resize(Size, false);
      }
      NumLiveLoopBack = 0;
      for (int I = 0; I < Size; ++I) {
        // find clique algorithm want the diagonal to be all true
        IG[I][I] = true;
        if (ToAllocate[I].Start > ToAllocate[I].Stop ||
            ToAllocate[I].Start == -1)
          ++NumLiveLoopBack;
        for (int J = 0; J < I; ++J) {
          if (ToAllocate[I].interfere(ToAllocate[J]))
            IG[I][J] = IG[J][I] = true;
        }
      }
      // Recursive, if a live-range conflicts with everything else, put it aside
      bool Change = false;
      std::vector<bool> FullRegister(Size, false);
      do {
        Change = false;
        for (int I = 0; I < Size; ++I) {
          if (FullRegister[I])
            continue;
          bool IsFull = true;
          for (int J = 0; J < Size; ++J) {
            if (FullRegister[J])
              continue;
            if (!IG[I][J]) {
              IsFull = false;
              break;
            }
          }
          if (IsFull) {
            FullRegister[I] = true;
            Change = true;
          }
        }
      } while (Change);
      // compact IG
      int NewSize = 0;
      for (int R = 0; R < Size; ++R) {
        if (FullRegister[R])
          Allocated.push_back(ToAllocate[R]);
        else
          ++NewSize;
      }
      for (int I = 0; I < Size; ++I) {
        int NextReg = 0;
        for (int J = 0; J < Size; ++J) {
          if (!FullRegister[J]) {
            IG[I][NextReg] = IG[I][J];
            ++NextReg;
          }
        }
        IG[I].resize(NewSize);
      }
      int NextReg = 0;
      for (int I = 0; I < Size; ++I) {
        if (!FullRegister[I]) {
          IG[NextReg] = IG[I];
          ToAllocate[NextReg] = ToAllocate[I];
          ++NextReg;
        }
      }
      ToAllocate.resize(NewSize);
      IG.resize(NewSize);
      return;
    }
    void dump() {
      dbgs() << "Allocated: ";
      for (auto &LiveRange : Allocated)
        dbgs() << printReg(LiveRange.ID)
               << format(" %d %d, ", LiveRange.Start, LiveRange.Stop);
      if (ToAllocate.size() > 0) {
        dbgs() << "\nTo Allocate: ";
        for (auto &LiveRange : ToAllocate)
          dbgs() << printReg(LiveRange.ID)
                 << format(" %d %d, ", LiveRange.Start, LiveRange.Stop);
        dbgs() << "\nInterference Graph:\n";
        for (auto &Row : IG) {
          for (auto C : Row)
            dbgs() << C;
          dbgs() << '\n';
        }
      } else
        dbgs() << "\nNothing To Allocate.\n";
    }
  };

  // All VRNames indexed by Output name
  std::vector<SWPVariable> VRName;
  // Map from VR number to VRName and PhiQueue index, index value -1 means
  // realdef
  std::map<unsigned, std::pair<int, int>> VRRename;

  SmallPtrSet<MachineInstr *, 4> NewMIs;

  bool EnableIVDependenceRelaxation;
  bool AvoidMemoryBanking;

  /// TODO Instructions to change when emitting the final schedule.
  // dependence relaxation
  DenseMap<SUnit *, std::pair<unsigned, int64_t>> InstrChanges;

  // Register Pressure
  unsigned renameReg(unsigned Reg) {
    auto VI = VRRename.find(Reg);
    if (VI != VRRename.end())
      return VRName[VI->second.first].Output;
    else
      return Reg;
  }

  int MaxII;
  int EstimatedMaxII;

  int getResourceMinimalII(XtensaScheduleEngineBase *Engine);

  SmallString<1024> getScheduleComment(const ScheduleResult &Schedule); 

  void renameAntiDep(MachineInstr *MI, SmallSet<int, 4> Operands,
                     SmallVector<std::pair<unsigned, unsigned>, 4> &Copies);
  ScheduleResult *scheduleMainLSMS(XtensaLSMSEngine &LSMS, int MaxStage);
  ScheduleResult *scheduleMainCPS(XtensaCPSEngine &Cps);

  XtensaScheduleEngineBase *Base;
  const TensilicaSWP::VersionedLoop *ReferenceVersion;

public:
  int MinII;
  ScheduleResult Result;
  SWPDependenceGraph *DepGraph;
  std::map<SUnit *, int> RealInstructionsMap;
  std::vector<PartRed *> PartReds;

  SWPScheduleReport Report;
  CyclicSchedulerDAG(TensilicaSWP *P, MachineBasicBlock *LoopHeader,
                           LiveIntervals &lis, const RegisterClassInfo &rci,
                           const TripCountInfo &TCI,
                           const std::set<unsigned> &RelaxedIV,
                           unsigned UnrollFactor, SWPDependenceGraph *DG,
                           const std::vector<PartRed *> &PartReds)
      : ScheduleDAGInstrs(*P->MF, P->MLI, false), Pass(P), Scheduled(false),
        LoopHeader(LoopHeader), LIS(lis), RegClassInfo(rci),
        NumVariablePrecedence(0), TCI(TCI), RelaxedIV(RelaxedIV),
        UnrollFactor(UnrollFactor), HasCall(false), HasSideEffects(false), 
        EnableIVDependenceRelaxation(RelaxedIV.size() != 0), MaxII(-1),
        EstimatedMaxII(-1), Base(nullptr), ReferenceVersion(nullptr), Result(1),
        DepGraph(DG), PartReds(PartReds) {
    ProbLoop = Pass->MBPI->getEdgeProbability(LoopHeader, LoopHeader);
    ProbExit = ProbLoop.getCompl();
  }
  ~CyclicSchedulerDAG() { delete Base; }
  virtual void schedule() override;
  void emitCode(ScheduleResult &R);
  void setReference(const TensilicaSWP::VersionedLoop *R) { ReferenceVersion = R; }
  /// Return true if OK to emit
  bool postprocessSchedule(ScheduleResult &R);
  void generateProlog(ScheduleResult &Schedule, unsigned LastStage,
                      MachineBasicBlock *KernelBB, ValueMapTy *VRMap,
                      MBBVectorTy &PrologueBBs);
  void generateEpilog(ScheduleResult &Schedule, unsigned LastStage,
                      MachineBasicBlock *KernelBB, ValueMapTy *VRMap,
                      MBBVectorTy &EpilogBBs, MBBVectorTy &PrologBBs);
  void generatePhis(MachineBasicBlock *NewBB, MachineBasicBlock *BB1,
                    MachineBasicBlock *BB2, MachineBasicBlock *KernelBB,
                    ScheduleResult &Schedule, ValueMapTy *VRMap,
                    InstrMapTy &InstrMap, unsigned LastStageNum,
                    unsigned CurStageNum, bool IsLast);
  void removeDeadInstructions(MachineBasicBlock *KernelBB,
                              MBBVectorTy &EpilogBBs);
  void splitLifetimes(MachineBasicBlock *KernelBB, MBBVectorTy &EpilogBBs,
                      ScheduleResult &Schedule);
  void addBranches(MBBVectorTy &PrologBBs, MachineBasicBlock *KernelBB,
                   MBBVectorTy &EpilogBBs, ScheduleResult &Schedule,
                   ValueMapTy *VRMap);
  bool computeDelta(MachineInstr *MI, unsigned &Delta);
  void updateMemOperands(MachineInstr *NewMI, MachineInstr *OldMI,
                         unsigned Num);
  MachineInstr *cloneInstr(MachineInstr *OldMI, unsigned CurStageNum,
                           unsigned InstStageNum);
  MachineInstr *cloneAndChangeInstr(MachineInstr *OldMI, unsigned CurStageNum,
                                    unsigned InstStageNum, ScheduleResult &);
  void updateInstruction(MachineInstr *NewMI, unsigned CurStageNum,
                         unsigned InstrStageNum, unsigned OriginalIndex,
                         ScheduleResult &Schedule, ValueMapTy *VRMap);
  MachineInstr *findDefInLoop(unsigned Reg);
  unsigned getPrevMapVal(unsigned StageNum, unsigned PhiStage, unsigned LoopVal,
                         ValueMapTy *VRMap, MachineBasicBlock *BB);
  void rewritePhiValues(MachineBasicBlock *NewBB, unsigned StageNum,
                        ScheduleResult &Schedule, ValueMapTy *VRMap,
                        InstrMapTy &InstrMap);
  void rewriteScheduledInstr(MachineBasicBlock *BB, ScheduleResult &Schedule,
                             InstrMapTy &InstrMap, unsigned CurStageNum,
                             unsigned PhiNum, MachineInstr *Phi,
                             unsigned OldReg, unsigned NewReg,
                             unsigned PrevReg = 0);
  bool canUseLastOffsetValue(MachineInstr *MI, unsigned &BasePos,
                             unsigned &OffsetPos, unsigned &NewBase,
                             int64_t &NewOffset);
  MachineInstr *applyInstrChange(MachineInstr *MI, ScheduleResult &Schedule,
                                 bool UpdateDAG = false);

  bool allocateRegister(ScheduleResult &Schedule);

  static bool CheckIVAImm(IVAccess &Access, int ImmValue) {
    if (Access.IsADD)
      return ImmValue != Access.ImmValue;
    else
      return !Access.ImmRange->contains(ImmValue);
  };

  bool restoreIVDependence(ScheduleResult &Schedule);

  typedef std::map<unsigned, std::vector<std::pair<int, int>>>
      NonSSARegisterAccessMap;
  void setupPressureLiveRanges(const NonSSARegisterAccessMap &);

  // Preallocate critical register files
  void preallocate(const NonSSARegisterAccessMap &, int MinII);
  // Collect all real instructions to be scheduled and build the precedence
  // graph.
  // Note that redundancy in the precedence graph does not matter, as it will
  // be normalized, and there is not much related scheduling cost change
  void buildPrecedence(NonSSARegisterAccessMap &);

  void serializeMemoryAccess();

  void computeBankConflict(const std::vector<std::vector<CyclicDist>> &MinDist);

  // Return true if the loop kernel has been scheduled.
  bool hasNewSchedule() {
    return Scheduled && Result.CriticalRegAllocated &&
           !Result.IVDependenceRelaxation && !Result.AntiDepLayout;
  }

  SWPScheduleReport getReport() { return Report; }
  void updateRegisterPressure(const TargetRegisterClass *RC, int D) {
    int &C = Report.RegisterDeficit[RC];
    C = std::max(C, D);
  }
  void getScheduleSummary(TensilicaSWP::VersionedLoop &VL) {
    VL.II = Result.II;
    VL.BankConflict = Result.BankConflict;
    VL.EffectiveII = Result.II + Result.BankConflict;
    VL.Stage = Result.Stage;
    VL.RegAllocated = Result.RegAllocated;
    VL.NEAR = Result.NEAR;
    VL.RegAllocation = Result.RegAllocation;
    VL.RecMII = Base->RecMII;
    VL.VirtualRecMII = Base->VirtualRecMII;
    VL.ResMII = Base->ResMII;
  }
  void addPrecedence(int Source, int Target, int Latency, int Omega = 0,
                     llvm::SDep::Kind Kind = llvm::SDep::Data,
                     int Variable = 0) {
    Precedence.push_back(
        PrecedenceEdge(Source, Target, Latency, Omega, Kind, Variable));
  }

  // I is a memory access before J
  void addLoopCarriedPrecedence(int I, int J, bool IIsStore, bool JIsStore,
                                bool LoopCarried = true) {
    assert(I < J);
    if (IIsStore) {
      addPrecedence(I, J, 1, 0, llvm::SDep::Order);
      if (LoopCarried) {
        if (JIsStore)
          addPrecedence(J, I, 1, 1, llvm::SDep::Order);
        else
          addPrecedence(J, I, Pass->MemWARLatency, 1, llvm::SDep::Order);
      }
    } else {
      addPrecedence(I, J, Pass->MemWARLatency, 0, llvm::SDep::Order);
      if (LoopCarried) {
        if (JIsStore)
          addPrecedence(J, I, 1, 1, llvm::SDep::Order);
        else
          addPrecedence(J, I, Pass->MemWARLatency, 1, llvm::SDep::Order);
      }
    }
  }

  void collectAntidependence();
  void dumpRealInstructions();
  void dumpPrecedence();
  TensilicaSWP *getPass() { return Pass; }

  // This is about SSA
  void finalizeSchedule(ScheduleResult *Schedule);
  bool generateIssueOrder(ScheduleResult &Schedule);
  /// Return true if the scheduled Phi has a loop carried operand.
  bool isLoopCarried(ScheduleResult *Schedule, MachineInstr *Phi);

  void setEstimatedMaxII(int M) { EstimatedMaxII = M; }
  void setMaxII(int M) { MaxII = M; }
  void setBranchPolicy(std::vector<InstructionBranchPolicy> &Policy,
                       ScheduleResult *Schedule);

  RegisterPressureInfoType refineRegisterPressureInfo(
      const RegisterPressureInfoType &ToRefine, ScheduleResult &Schedule,
      const std::vector<std::vector<CyclicDist>> &MinDist,
      bool ExcludeCritical = true);

  PressureLiveRangeList refinePressureLiveRanges(
      const PressureLiveRangeList &ToRefine, ScheduleResult &Schedule,
      const std::vector<std::vector<CyclicDist>> &MinDist);

  struct BaseOffsetSize {
    unsigned Base;
    int Offset;
    int Size;
  };
  BaseOffsetSize getBaseOffset(const MachineInstr *MI) const {
    BaseOffsetSize R = {0, 0, 0};
    const auto *InstrProp = Pass->TII->getInstrProperty(MI->getOpcode());
    if (InstrProp->isLoadStore() && InstrProp->isBaseDispImm()) {
      auto &&RegOperand = MI->getOperand(InstrProp->getBaseOpnd());
      unsigned ImmOperandIdx = InstrProp->getOffsetOpnd();
      auto &&ImmOperand = MI->getOperand(ImmOperandIdx);
      if (!RegOperand.isReg() || !ImmOperand.isImm())
        return R;
      R.Base= RegOperand.getReg();
      R.Offset = ImmOperand.getImm();
      R.Size = InstrProp->getImmRange(ImmOperandIdx).ImmRanges.front().getStep();
    }
    return R;
  }
  void prepare(bool RelaxAntiDep);
  static const BranchProbability ProbOne, ProbLikely, ProbUnlikely;
};
} // end namespace Tensilica 
} // end namespace llvm
#endif // XTENSAPIPELINERENGINE_H
