//===----- XtensaLSMSEngine.h - Life Time Sensitive Modulo Scheduling -----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  Lifetime sensitive modulo scheduler, based on the following paper
//
//  Richard Huff,
//  "Lifetime-Sensitive Modulo Scheduling",
//  in Programming Language Design and Implementation. SIGPLAN, 1993.
//  http://www.cs.cornell.edu/Info/Projects/Bernoulli/home.html#PLDI93-2
//
//  Update the resource scale implemenatation for Xtensa
//===----------------------------------------------------------------------===//

#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include <cfloat>
enum ScheduleDirection { SDUnknown, SDTopdown, SDBottomup };

struct SWP_OP {
  ScheduleDirection Direction;
  MachineInstr *MI;
  bool Placed;
  double Merit;
  int Trials;
  int Cycle;
  void place(int C) {
    Placed = true;
    Cycle = C;
  }
  void unplace() { Placed = false; }
  SWP_OP()
      : Direction(SDUnknown), MI(nullptr), Placed(false), Merit(1.0), Trials(0),
        Cycle(0) {}
};

static inline int PMod(int Q, int D) {
  if (Q < 0) {
    do
      Q += D;
    while (Q < 0);
  } else {
    while (Q >= 0)
      Q -= D;
    Q += D;
  }
  return Q;
}

typedef std::vector<SWP_OP> OPVector;

typedef std::vector<std::vector<CyclicDist>> MinDist;

// Flatten the cyclic min-dist to acyclic, based on specific II
struct DistMatrix {
  int Size;
  int II;
  int *Matrix;
  bool OK;
  DistMatrix(const PrecedenceVector &PV, int Size, int II);
  int &operator()(int X, int Y) { return Matrix[X * Size + Y]; }
  int operator()(int X, int Y) const { return Matrix[X * Size + Y]; }
  void setup();
  void finish();
  ~DistMatrix() { delete[] Matrix; }
};

class Slack {
  int Start, Stop;
  std::vector<int> EStart, LStart;

public:
  int start() const { return Start; }
  int stop() const { return Stop; }
  int eStart(int i) const { return EStart[i]; }
  int lStart(int i) const { return LStart[i]; }
  int operator()(int i) const { return LStart[i] - EStart[i]; }
  bool setLastCycle(const OPVector &V, int ScheduleLength,
                    const DistMatrix &Matrix);
  void relaxPrecedence(const OPVector &V, int unplaced,
                       const DistMatrix &Matrix,
                       const SchedulePrecedenceGraph &PrecedenceGraph);
  void relaxPrecedence(const OPVector &V, const std::vector<int> &Unplaced,
                       const DistMatrix &Matrix,
                       const SchedulePrecedenceGraph &PrecedenceGraph);

  // TODO there should be some optimizations for the following 3, say
  // for each instruction, should only need to consider a small set of
  // instructions, which are "tight" successors and/or predecessors.
  void updateSlackFromOp(int C, const OPVector &V, const DistMatrix &Matrix);
  bool updateSlackFromPlacedOp(int C, const OPVector &V,
                               const DistMatrix &Matrix);
  void updateSlackFromPlacedOps(const OPVector &V, const DistMatrix &Matrix);

  Slack(const OPVector &V, int StartIdx, int StopIdx, int II,
        const DistMatrix &Matrix, int ASL)
      : Start(StartIdx), Stop(StopIdx), EStart(Stop + 1, 0),
        LStart(Stop + 1, 0) {
    int Len = II * (int)std::ceil((Matrix(Start, Stop) + 1.0) / II);
#if 0
    // figure out a better lower bound may not help, as it looks like with
    // an illegal lower bound, the search select better candidate, we may want
    // to try even smaller lower bound
    Len = std::max(Len, ASL);
#endif
    setLastCycle(V, Len - 1, Matrix);
  }
  void dump(raw_ostream &, const OPVector &V);
};

class XtensaLSMSEngine;
struct ReservationTable {
  XtensaLSMSEngine *Engine;
  const Tensilica::DFAPacketizer *DFA0;
  int II;
  std::vector<int> DFA0State;
  std::vector<SmallVector<int, 6>> Instructions;
  int IssueWidth;
  // if we want to count
  // std::map<unsigned, int> Opcodes;
  std::set<int> OpcodeClasses;
  std::map<int, std::set<int>> RelevantMap;
  ReservationTable(XtensaLSMSEngine *Engine,
                   const Tensilica::DFAPacketizer *DFA0, int W, int II,
                   const std::vector<int> &Classes);
  // TODO DFAN status
  void reserveResource(int I, int IClass, int Cycle);
  void unreserveResource(int I, int IClass, int Cycle);

  bool resourceAvailable(int I, int IClass, int Cycle) const;

  int findResourceInRange(int I, int IClass, int Earliest, int Latest,
                          bool Topdown) const;
  bool noResourceInRange(int Cand, int IClass, int Earliest, int Latest) const;
  bool almostNoResourceInRange(int Cand, int IClass, int Earliest,
                               int Latest) const;
  int countResourceInRange(int Cand, int IClass, int Earliest,
                           int Latest) const;

  bool resourceRelevant(int IClassa, int IClassb) {
    // FIXME to be verified
    const auto &M = RelevantMap;
    auto Iter = M.find(IClassa);
    if (Iter == M.end())
      return false;
    const auto &S = Iter->second;
    return S.count(IClassb);
  }

  void clear();
  void dump();
  const SmallVector<int, 6> &getInstructions(int C) const;
};

// map to the root
// For example, if we declare 
// vec is 64 bit register, vec32 is the RC to describe 32 bit data types here
//    vec_2 are pairs of 64 bit, and vec_2_32 are pairs of 32 bit
// vec is the class root, while vec is root of vec32, and vec_2 is root of
//   vec_2_32
struct RootRegisterInfo {
  const TargetRegisterClass *ClassRoot;
  int Width;
  const TargetRegisterClass *Root;
};
typedef std::map<const TargetRegisterClass *, RootRegisterInfo> RootRegisterClassInfo;

enum OrderHeuristic {
  //   schedule clusters
  HCluster,
  //   input order
  HInputOrder,
  //   smallest slack
  HSlack,
  //   smallest slack scaled by critical resource
  HSlackScale,
  //   smallest latest start
  HLStart,
  //   largest earliest start
  HEStart,
  //   the first un-placed in forward order
  HUnplacedForward,
  //   the first un-placed in backward order
  HUnplacedBackward
};

class ScheduleHeuristic {
  XtensaLSMSEngine *Engine;
  const RegisterPressureInfoType *RPI;
  MachineRegisterInfo *MRI;
  const RootRegisterClassInfo *RootInfo;
  int II;
  int ScheduleLengthChanges;
  std::vector<int> MinLT;
  OrderHeuristic Heuristic;
  bool Topdown;
  const std::vector<int> &Order;
  bool MinRetry;
  int RestartCount;
  int MaxScheduleLength;
  int Size;
  int SizeP2;
  // if set, avoid all possible bank conflict
  bool AvoidAllBankConflict;
  bool ScheduleTopdown(int Candidate, OPVector &V, const Slack &slack);
  void recalculateTopdown(OPVector &V, const Slack &slack);

public:
  std::vector<int> Unplaced;
  void initOpState(OPVector &V, const Slack &slack);
  int chooseOP(const OPVector &V, const Slack &slack,
               const ReservationTable &RT);
  // return std::pair<precedence might violated, resource might viloated>
  std::pair<bool, bool> chooseIssueCycle(int I, OPVector &V, const Slack &slack,
                                         const ReservationTable &RT);

  // Eject all instructions that violate the precedence constraint with the
  // candidate
  void ejectPrecedenceConflict(int I, OPVector &V, const Slack &slack,
                               const DistMatrix &Matrix);

  void ejectResourceConflict(int I, OPVector &V, ReservationTable &RT);
  void updateResource(int I, OPVector &V, ReservationTable &RT);
  bool updatePrecedence(int C, OPVector &V, Slack &slack,
                        const DistMatrix &Matrix, ReservationTable &RT);

  ScheduleHeuristic(XtensaLSMSEngine *Engine,
                    const RegisterPressureInfoType *RPI,
                    MachineRegisterInfo *MRI,
                    const RootRegisterClassInfo *RootInfo, int i,
                    const std::vector<int> &l, OrderHeuristic h, bool t,
                    const std::vector<int> &O, bool MinRetry, int r = 8)
      : Engine(Engine), RPI(RPI), MRI(MRI), RootInfo(RootInfo), II(i),
        ScheduleLengthChanges(0), MinLT(l), Heuristic(h), Topdown(t), Order(O),
        MinRetry(MinRetry), RestartCount(r), Size(l.size() - 2),
        SizeP2(Size + 2) {
    Unplaced.reserve(SizeP2);
  }
};

// TODO list
// 1. bank conflict
// 2. check heuristic

// TODO
/*
 * The design from Huff's paper
 * 1. Model of the search state
 * 2. Branch of II, track the distance of instructions, and reservation table
 * 3. Branch of schedule length, and the number of stages is automatic. The
 *    slack is calculated based on schedule length.
 * There is some rough heuristic here to reuse the state when the schedule
 * length is enlarged. It is called restart in the context.
 * 4. A set of heuristics to choose next instruction to schedule, and to choose
 *    a cycle. These two are typically coupled.
 * 5. There is a heuristic here to eject instructions and have a limitation of
 *    number of trials per instruction
 *
 * Generalize the search engine.
 * 3. We do not have to branch on the schedule length
 *   3.1 Calculate the Early, Late, and Slack only when the instruction is
 *       limitted in the two directions.
 *   3.2 For example, when there is only an upper bound, we may want to schedule
 *       bottom up. Otherwise, top down.
 *   3.3 In case to reduce register pressure, try to choose instructions of
 *       stretchable live range earlier. As such, the live range will be short
 *       at least.
 * 5. To create a real branch and bound engine.
 *   4.1 the branch of the state is usually cheap, especially we are not trying
 *       to calculate many auxilary variables if any here. Also, we can use a
 *       branch-NVALUES scheme.
 *   4.2 Remove the static trial limitation per instruction, but to have a
 *       estimation of number of trials based on the remaining number of
 *       instruction unscheduled yet.
 */
