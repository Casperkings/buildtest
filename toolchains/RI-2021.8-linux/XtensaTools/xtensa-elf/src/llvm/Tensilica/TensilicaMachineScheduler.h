//===-- TensilicaMachineScheduler.h - Custom Tensilica MI scheduler.   ----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Custom Xtensa MI scheduler.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_CODEGEN_TENSILICA_TENSILICAMACHINESCHEDULER_H
#define LLVM_LIB_CODEGEN_TENSILICA_TENSILICAMACHINESCHEDULER_H

#include "llvm/ADT/PriorityQueue.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/CodeGen/LiveIntervals.h"
#include "llvm/CodeGen/MachineScheduler.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/RegisterClassInfo.h"
#include "llvm/CodeGen/RegisterPressure.h"
#include "llvm/CodeGen/ResourcePriorityQueue.h"
#include "llvm/CodeGen/ScheduleDAGInstrs.h"
#include "llvm/CodeGen/ScheduleHazardRecognizer.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "Tensilica/TensilicaInstrInfo.h"
#include "Tensilica/TensilicaDFAPacketizer.h"
#include "Tensilica/TensilicaSubtargetInfo.h"

using namespace llvm;

namespace llvm {
//===----------------------------------------------------------------------===//
// ConvergingFLIXScheduler - Implementation of the standard
// MachineSchedStrategy.
//===----------------------------------------------------------------------===//

class FLIXResourceModel {
  Tensilica::DFAPacketizer XtensaResourcesModel;

  const TargetSchedModel *SchedModel;

  /// Local packet/bundle model. Purely
  /// internal to the MI schedulre at the time.
  std::vector<SUnit*> Packet;

  /// Total packets created.
  unsigned TotalPackets;

public:
  FLIXResourceModel(const TargetSubtargetInfo &STI, const TargetSchedModel *SM)
      : SchedModel(SM), TotalPackets(0) {
    Packet.resize(SchedModel->getIssueWidth());
    Packet.clear();
    XtensaResourcesModel.clearResources();
  }

  ~FLIXResourceModel() {
  }

  void resetPacketState() {
    Packet.clear();
  }

  void resetDFA() {
    XtensaResourcesModel.clearResources();
  }

  void reset() {
    Packet.clear();
    XtensaResourcesModel.clearResources();
  }

  bool isResourceAvailable(SUnit *SU);
  bool reserveResources(SUnit *SU);
  bool isResourceAvailable(unsigned Opcode);
  bool reserveResources(unsigned Opcode);
  unsigned getTotalPackets() const { return TotalPackets; }
};

/// Extend the standard ScheduleDAGMI to provide more context and override the
/// top-level schedule() driver.
class FLIXMachineScheduler : public ScheduleDAGMILive {
public:
  FLIXMachineScheduler(MachineSchedContext *C,
                       std::unique_ptr<MachineSchedStrategy> S)
      : ScheduleDAGMILive(C, std::move(S)) {}

  /// Schedule - This is called back from ScheduleDAGInstrs::Run() when it's
  /// time to do some work.
  void schedule() override;
  /// Perform platform-specific DAG postprocessing.
  void postprocessDAG();
};

/// ConvergingFLIXScheduler shrinks the unscheduled zone using heuristics
/// to balance the schedule.
class ConvergingFLIXScheduler : public MachineSchedStrategy {
public:
  enum SchedulerPhase{
    SWPEstimate, PreRA, PostRA
  };

private:
  /// Store the state used by ConvergingFLIXScheduler heuristics, required
  ///  for the lifetime of one invocation of pickNode().
  struct SchedCandidate {
    // The best SUnit candidate.
    SUnit *SU;

    // Register pressure values for the best candidate.
    RegPressureDelta RPDelta;

    // Best scheduling cost.
    int SCost;

    SchedCandidate(): SU(nullptr), SCost(0) {}
  };

  SchedulerPhase Phase;
  bool ForceTopDown;
  bool ForceBottomUp;

  /// Represent the type of SchedCandidate found within a single queue.
  enum CandResult {
    NoCand, NodeOrder, SingleExcess, SingleCritical, SingleMax, MultiPressure,
    BestCost};

  /// Each Scheduling boundary is associated with ready queues. It tracks the
  /// current cycle in whichever direction at has moved, and maintains the state
  /// of "hazards" and other interlocks at the current cycle.
  struct FLIXSchedBoundary {
    FLIXMachineScheduler *DAG;
    const TargetSchedModel *SchedModel;

    ReadyQueue Available;
    ReadyQueue Pending;
    bool CheckPending;

    ScheduleHazardRecognizer *HazardRec;
    FLIXResourceModel *ResourceModel;

    unsigned CurrCycle;
    unsigned IssueCount;

    /// MinReadyCycle - Cycle of the soonest available instruction.
    unsigned MinReadyCycle;

    // Remember the greatest min operand latency.
    unsigned MaxMinLatency;

    /// Pending queues extend the ready queues with the same ID and the
    /// PendingFlag set.
    FLIXSchedBoundary(unsigned ID, const Twine &Name):
      DAG(nullptr), SchedModel(nullptr), Available(ID, Name+".A"),
      Pending(ID << ConvergingFLIXScheduler::LogMaxQID, Name+".P"),
      CheckPending(false), HazardRec(nullptr), ResourceModel(nullptr),
      CurrCycle(0), IssueCount(0),
      MinReadyCycle(UINT_MAX), MaxMinLatency(0) {}

    ~FLIXSchedBoundary() {
      delete ResourceModel;
      delete HazardRec;
    }

    void init(FLIXMachineScheduler *dag, const TargetSchedModel *smodel) {
      DAG = dag;
      SchedModel = smodel;
    }

    bool isTop() const {
      return Available.getID() == ConvergingFLIXScheduler::TopQID;
    }

    bool checkHazard(SUnit *SU);

    void releaseNode(SUnit *SU, unsigned ReadyCycle);

    void bumpCycle();

    void bumpNode(SUnit *SU);

    void releasePending();

    void removeReady(SUnit *SU);

    SUnit *pickOnlyChoice();
  };

  FLIXMachineScheduler *DAG;
  const TargetSchedModel *SchedModel;

  // State of the top and bottom scheduled instruction boundaries.
  FLIXSchedBoundary Top;
  FLIXSchedBoundary Bot;

  std::set<unsigned>CriticalPSets;
  std::set<unsigned>ExpensivePSets;

public:
  /// SUnit::NodeQueueId: 0 (none), 1 (top), 2 (bot), 3 (both)
  enum {
    TopQID = 1,
    BotQID = 2,
    LogMaxQID = 2
  };

  ConvergingFLIXScheduler( SchedulerPhase Phase)
    : Phase(Phase), DAG(nullptr), SchedModel(nullptr), Top(TopQID, "TopQ"),
      Bot(BotQID, "BotQ") {}

  void initialize(ScheduleDAGMI *dag) override;

  SUnit *pickNode(bool &IsTopNode) override;

  void schedNode(SUnit *SU, bool IsTopNode) override;

  void releaseTopNode(SUnit *SU) override;

  void releaseBottomNode(SUnit *SU) override;

  unsigned ReportPackets() {
    return Top.ResourceModel->getTotalPackets() +
           Bot.ResourceModel->getTotalPackets();
  }

protected:
  SUnit *pickNodeBidirectional(bool &IsTopNode);

  int SchedulingCost(ReadyQueue &Q,
                     SUnit *SU, SchedCandidate &Candidate,
                     RegPressureDelta &Delta, bool verbose);

  CandResult pickNodeFromQueue(FLIXSchedBoundary &Zone,
                               const RegPressureTracker &RPTracker,
                               SchedCandidate &Candidate);
#ifndef NDEBUG
  void traceCandidate(const char *Label, const ReadyQueue &Q, SUnit *SU,
                      PressureChange P = PressureChange());
#endif
};

/// FLIXScheduler is specialized GenericScheduler for Xtensa
class FLIXScheduler : public GenericScheduler {

  // State of the top and bottom scheduled instruction boundaries.
  FLIXResourceModel *TopModel;
  FLIXResourceModel *BotModel;

  std::set<unsigned>CriticalPSets;
  std::set<unsigned>ExpensivePSets;
  std::set<unsigned>CriticalOrExpensivePSets;

  struct AffinityType {
    double Top;
    double Bottom;
    AffinityType(double T = 0.0, double B = 0.0) : Top(T), Bottom(B) {}
  };

  std::vector<AffinityType> Affinity;
  std::vector<std::vector<AffinityType>> AffinityMatrix;
  void updateAffinity(SUnit *SU, bool IsTop);

public:

  FLIXScheduler(const MachineSchedContext *C)
    : GenericScheduler(C), TopModel(nullptr), BotModel(nullptr) {}
  ~FLIXScheduler() {
    if (TopModel)
      delete TopModel;
    if (BotModel)
      delete BotModel;
  }

  void initialize(ScheduleDAGMI *dag) override;
  void schedNode(SUnit *SU, bool IsTopNode) override;

protected:
  void tryCandidate(SchedCandidate &Cand,
                    SchedCandidate &TryCand,
                    SchedBoundary *Zone) const override;
  void pickNodeFromQueue(SchedBoundary &Zone,
                         const CandPolicy &ZonePolicy,
                         const RegPressureTracker &RPTracker,
                         SchedCandidate &Candidate) override;

  CandReason pickNodeFromQueue(ReadyQueue &Q,
                               const RegPressureTracker &RPTracker,
                               SchedCandidate &Candidate);
};
#if 0
bool tryLatency(GenericSchedulerBase::SchedCandidate &TryCand,
                GenericSchedulerBase::SchedCandidate &Cand,
                SchedBoundary &Zone);
bool tryGreater(int TryVal, int CandVal,
                GenericSchedulerBase::SchedCandidate &TryCand,
                GenericSchedulerBase::SchedCandidate &Cand,
                GenericSchedulerBase::CandReason Reason);
bool tryLess(int TryVal, int CandVal,
             GenericSchedulerBase::SchedCandidate &TryCand,
             GenericSchedulerBase::SchedCandidate &Cand,
             GenericSchedulerBase::CandReason Reason);
#endif

extern char &TensilicaPostRASchedulerID;
void initializeTensilicaPostRASchedulerPass(PassRegistry &);
FunctionPass *createTensilicaPostRAScheduler();

} // namespace llvm

#endif // LLVM_LIB_CODEGEN_TENSILICA_TENSILICAMACHINESCHEDULER_H
