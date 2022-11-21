#include <cfloat>

// Positive Mod
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

struct DistMatrix;
class XtensaLSMSEngine;
typedef std::map<const TargetRegisterClass *,
                 std::pair<const TargetRegisterClass *, int>>
    RootRegisterClassInfo;
struct BranchII;
struct BranchInsn;

// Variable information about an instruction
struct InsnVar {
  int Index;
  bool Placed;
  int Cycle;
  // Information about its domain
  int Early;
  int Late;
  int CurrentCandidate;
  std::vector<int> Candidates;
};

struct InsnVarArray {
  std::vector<InsnVar> Array;
  // on branch, copy the domain information only for those not placed
  void branchCopy(const InsnVarArray &A);
};

// Brancher on instruction
struct BranchInsn {
  // cache
  const DistMatrix *Matrix;
  int II;
  int Size;
  const XtensaDFAPacketizer *DFA0;
  BranchII *BII;

  int Chosen;
  int ChosenCycle;
  int Depth;
  std::vector<int> DFA0State;
  std::vector<SmallVector<int, 6>> Instructions;
  void dump();
  InsnVarArray Insns;
public:
  BranchInsn(const BranchInsn *Parent, int Chosen, int ChosenCycle);
  BranchInsn(BranchII *BII, int Chosen, int ChosenCycle);
  int search();
  bool evaluate();
};

// Potentially any brancher can be turned into a thread
struct BranchII {
  int II;
  int Size; // copied
  XtensaCustomEngine *Engine;
  const RootRegisterClassInfo *RootInfo;
  std::map<int, std::set<int>> RelevantMap;
  int IssueWidth;
  BranchII(XtensaCustomEngine *Engine, int II);
  int search();
};

// The following goes to .cpp
int BranchII::search() {
  int Insn0, Cycle0;
  // choose an initial instruction, which shall 
  // 1. be from the biggest SCC
  // 2. define a local register with pressured register class
  // 3. early
  BranchInsn RootBII(this, Insn0, Cycle0);
}

// evaluate the status for the newly scheduled instruction 
bool BranchInsn::evaluate() {
  for (auto &Insn : Insns) {
    if (Insn.Placed)
      continue;
  }
}

int BranchInsn::search() {

}
