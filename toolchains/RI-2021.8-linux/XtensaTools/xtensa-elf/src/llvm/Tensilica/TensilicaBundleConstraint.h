//===---- XtensaBundleConstaint.h - Xtensa Bundle Constraint --------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contain the bundling constraint and some branchers used by
// constraint programming scheduler
//
//===----------------------------------------------------------------------===//

// note that except some constructors and statics, all functions in this file
// are virtual override
#ifndef LLVM_LIB_TARGET_XTENSA_XTENSABUNDLECONSTRAINT_H
#define LLVM_LIB_TARGET_XTENSA_XTENSABUNDLECONSTRAINT_H

#include "TensilicaPipelineEngine.h"
#include <gecode/driver.hh>
#include <gecode/int.hh>
#include <gecode/minimodel.hh>
namespace Gecode {

struct Readyness {
  bool assigned;
  int pred_count;
  int succ_count;
  Readyness() : assigned(false), pred_count(0), succ_count(0) {}
};
typedef std::vector<Readyness, space_allocator<Readyness>> vector_readyness;

enum Branch_Direction {
  BD_Topdown = 1,
  BD_Bottomup = 2,
  BD_Bidirectional = 3 // Bidirectional == Topdown | Bottomup
};

const int MAX_BRANCH_TILE = 16;

// Use advisor to reduce execution
// Use some state to reduce propagation cost
// Use assigned cycles to trim values from un-assigned variables
// Maybe the constant opcode information should be carried by variables
class XtensaBundleConstraint : public Propagator {
protected:
  // Schedule length
  Int::IntView length;
  // The advisor council
  class CycleAdvisor : public Advisor {
  public:
    Int::IntView cycle;
    int iclass;
    int id; // position
    CycleAdvisor(Space &home, Propagator &p, Council<CycleAdvisor> &c,
                 Int::IntView cycle0, int iclass0, int id0)
        : Advisor(home, p, c), cycle(cycle0), iclass(iclass0), id(id0) {
      cycle.subscribe(home, *this);
    }
    CycleAdvisor(Space &home, CycleAdvisor &a)
        : Advisor(home, a), iclass(a.iclass), id(a.id) {
      cycle.update(home, a.cycle);
    }
    void dispose(Space &home, Council<CycleAdvisor> &c) {
      cycle.cancel(home, *this);
      Advisor::dispose(home, c);
    }
  };
  Council<CycleAdvisor> council;
  // ISA Characteristic
  const Tensilica::DFAPacketizer *DFA0;
  // TOP information
  const XtensaScheduleEngine *ScheduleSpecification;
  // already assigned
  int num_assigned;
  int size;

  // State of the propagator
  int *cycles_state;
  struct scheduled_information {
    int iclass;
    int cycle;
    scheduled_information(int t, int c) : iclass(t), cycle(c) {}
    scheduled_information() {}
  };
  typedef std::vector<scheduled_information,
                      space_allocator<scheduled_information>>
      new_assigned_vector;
  typedef std::set<int, std::less<int>, space_allocator<int>> space_int_set;
  space_int_set touched_cycles;
  new_assigned_vector newly_assigned;
  int *already_assigned;
  int n_cycles;

public:
  XtensaBundleConstraint(Space &home, ViewArray<Int::IntView> issue,
                         Int::IntView length, const Tensilica::DFAPacketizer *DFA0,
                         const XtensaScheduleEngine *ScheduleSpecification)
      : Propagator(home), length(length), council(home), DFA0(DFA0),
        ScheduleSpecification(ScheduleSpecification), num_assigned(0),
        touched_cycles(std::less<int>(), space_int_set::allocator_type(home)),
        newly_assigned(new_assigned_vector::allocator_type(home)), n_cycles(0) {
    size = issue.size();
    newly_assigned.reserve(size);
    length.subscribe(home, *this, Int::PC_INT_VAL);
    already_assigned = home.alloc<int>(size);
    for (int i = 0; i < size; i++) {
      already_assigned[i] = issue[i].assigned();
      if (issue[i].assigned()) {
        newly_assigned.push_back(scheduled_information(
            ScheduleSpecification->Base.Classes[i], issue[i].val()));
        touched_cycles.insert(i);
      } else
        (void)new (home)
            CycleAdvisor(home, *this, council, issue[i],
                         ScheduleSpecification->Base.Classes[i], i);
    }
    home.notice(*this, AP_DISPOSE);
  }
  // posting
  static ExecStatus post(Space &home, ViewArray<Int::IntView> issue,
                         Int::IntView length, const Tensilica::DFAPacketizer *DFA0,
                         const XtensaScheduleEngine *ScheduleSpecification) {
    (void)new (home) XtensaBundleConstraint(home, issue, length, DFA0,
                                            ScheduleSpecification);
    return ES_OK;
  }
  virtual size_t allocated(Space &home) { return size * 8 + n_cycles * 8; }
  // disposal
  virtual size_t dispose(Space &home) {
    home.ignore(*this, AP_DISPOSE);
    length.cancel(home, *this, Int::PC_INT_VAL);
    if (n_cycles != 0)
      home.free<int>(cycles_state, n_cycles);
    home.free<int>(already_assigned, size);
    council.dispose(home);
    (void)Propagator::dispose(home);
    return sizeof(*this);
  }
  // copying
  XtensaBundleConstraint(Space &home, XtensaBundleConstraint &p)
      : Propagator(home, p), length(p.length), DFA0(p.DFA0),
        ScheduleSpecification(p.ScheduleSpecification),
        num_assigned(p.num_assigned), size(p.size),
        touched_cycles(std::less<int>(), space_int_set::allocator_type(home)),
        newly_assigned(new_assigned_vector::allocator_type(home)),
        n_cycles(p.n_cycles) {
    council.update(home, p.council);
    length.update(home, p.length);
    already_assigned = home.alloc<int>(size);
    memcpy(already_assigned, p.already_assigned, sizeof(int) * size);
    newly_assigned.reserve(size);
    std::copy(p.newly_assigned.begin(), p.newly_assigned.end(),
              std::inserter(newly_assigned, newly_assigned.end()));
    std::copy(p.touched_cycles.begin(), p.touched_cycles.end(),
              std::inserter(touched_cycles, touched_cycles.end()));
    if (n_cycles != 0) {
      cycles_state = home.alloc<int>(n_cycles);
      memcpy(cycles_state, p.cycles_state, sizeof(int) * n_cycles);
    } else
      cycles_state = nullptr;
  }
  virtual Propagator *copy(Space &home) {
    return new (home) XtensaBundleConstraint(home, *this);
  }
  // cost computation
  virtual PropCost cost(const Space &, const ModEventDelta &) const {
    return PropCost::quadratic(PropCost::HI, size);
  }
  // advise
  // update state of the propagator
  virtual ExecStatus advise(Space &, Advisor &a, const Delta &delta) {
    CycleAdvisor &ca = static_cast<CycleAdvisor &>(a);
    if (!ca.cycle.assigned())
      return ES_FIX;
    newly_assigned.push_back(scheduled_information(ca.iclass, ca.cycle.val()));
    already_assigned[ca.id] = 1;
    return ES_NOFIX;
  }
  // propagation
  virtual ExecStatus propagate(Space &home, const ModEventDelta &) {
    if (n_cycles == 0) {
      n_cycles = length.max();
      cycles_state = home.alloc<int>(n_cycles);
      memset(cycles_state, 0, sizeof(int) * n_cycles);
    } else if (n_cycles < length.max()) {
      int nn_cycles = length.max();
      cycles_state = home.realloc<int>(cycles_state, n_cycles, nn_cycles);
      memset(cycles_state + n_cycles, 0, sizeof(int) * (nn_cycles - n_cycles));
      n_cycles = nn_cycles;
    }

    bool change = false;
    bool full_pass = true;
// alternatively we check for consistency and return ES_NOFIX
// if called again, say, no newly_assigned, try to actually propagate
#if 0
    full_pass = newly_assigned.empty();
#endif
    do {
      change = false;
      for (auto it = newly_assigned.rbegin(); it != newly_assigned.rend();
           it = newly_assigned.rbegin()) {
        num_assigned++;
        int cycle = it->cycle;
        int iclass = it->iclass;
        newly_assigned.pop_back();
        if (cycle >= n_cycles) {
          // some other "actor" should notice this earlier, but anyway
          return ES_FAILED;
        }
        int cycle_state = cycles_state[cycle];
        cycle_state = DFA0->reserveResources(cycle_state, iclass);
        if (!Tensilica::DFAPacketizer::isLegal(cycle_state))
          return ES_FAILED;
        cycles_state[cycle] = cycle_state;
        touched_cycles.insert(cycle);
      }
      if (!full_pass)
        break;
      for (int cycle : touched_cycles) {

        for (Advisors<CycleAdvisor> a(council); a(); ++a) {
          int cycle_state = cycles_state[cycle];
          if (!already_assigned[a.advisor().id] &&
              a.advisor().cycle.in(cycle)) {
            if (!Tensilica::DFAPacketizer::isLegal(
                    DFA0->reserveResources(cycle_state, a.advisor().iclass))) {
              GECODE_ME_CHECK(a.advisor().cycle.nq(home, cycle));
              change = true;
            }
          }
        }
      }
      touched_cycles.clear();
    } while (change);

    if (num_assigned == size)
      return home.ES_SUBSUMED(*this);
    if (full_pass)
      return ES_FIX;
    return ES_NOFIX;
  }
  virtual void reschedule(Space &home) {
    length.reschedule(home, *this, IPL_VAL);
    // TODO deal with the advisor council
    for (Advisors<CycleAdvisor> a(council); a(); ++a) {
      a.advisor().cycle.reschedule(home, *this, IPL_VAL);
    }
  }
};

class Xtensa_II_Stage_Brancher : public Brancher {
protected:
  const XtensaScheduleEngine *ScheduleSpecification;
  const SchedulePrecedenceGraph &PrecedenceGraph;
  Int::IntView ii;
  Int::IntView stage;

  class II_Stage_Choice : public Choice {
  public:
    bool is_stage;
    bool stage_p1only;
    int value;
    II_Stage_Choice(const Xtensa_II_Stage_Brancher &b, bool iss, bool p1only,
                    int v)
        : Choice(b, 2 /*number of alternative choices*/), is_stage(iss),
          stage_p1only(p1only), value(v) {}
    virtual size_t size(void) const { return sizeof(*this); }
    virtual void archive(Archive &a) const {
      Choice::archive(a);
      a << is_stage << stage_p1only << value;
      return;
    }
  };

public:
  Xtensa_II_Stage_Brancher(Space &home, Int::IntView ii, Int::IntView stage,
                           const XtensaScheduleEngine *ScheduleSpecification)
      : Brancher(home), ScheduleSpecification(ScheduleSpecification),
        PrecedenceGraph(ScheduleSpecification->PrecedenceGraph), ii(ii),
        stage(stage) {}
  Xtensa_II_Stage_Brancher(Space &home, Xtensa_II_Stage_Brancher &c)
      : Brancher(home, c), ScheduleSpecification(c.ScheduleSpecification),
        PrecedenceGraph(c.PrecedenceGraph) {
    ii.update(home, c.ii);
    stage.update(home, c.stage);
  }
  virtual bool status(const Space &home) const {
    return !ii.assigned() || !stage.assigned();
  }
  virtual Choice *choice(Space &home) {
    if (ii.assigned()) {
      bool p1only = false;
      int val = stage.min();
      if (ScheduleSpecification->Results.size() > 0) {
        int val1 = ScheduleSpecification->Results.front().Stage;
        if (val1 > 1 && stage.in(val1)) {
          val = val1;
          p1only = true;
        }
      }

      if (!p1only) {
        if (stage.in(2))
          val = 2;
      }
      return new II_Stage_Choice(*this, true, p1only, val);
    }
#if 0 
    int val = (ii.max() + ii.min())/2;
    while(!ii.in(val)) {
      if (val == ii.max() - 1)
        val = ii.max();
      else
        val = (ii.max() + val) / 2;
    }
    return new II_Stage_Choice(*this, false, false, val);
#endif
    return new II_Stage_Choice(*this, false, false, ii.max());
  }
  virtual Choice *choice(const Space &, Archive &e) {
    bool is_s;
    bool p1only;
    int value;
    e >> is_s >> p1only >> value;
    return new II_Stage_Choice(*this, is_s, p1only, value);
  }
  virtual ExecStatus commit(Space &home, const Choice &c, unsigned int alt) {
    const II_Stage_Choice &isc = static_cast<const II_Stage_Choice &>(c);
    bool is_stage = isc.is_stage;
    bool p1only = isc.stage_p1only;
    int value = isc.value;
    if (is_stage) {
      if (alt == 0)
        return me_failed(stage.eq(home, value)) ? ES_FAILED : ES_OK;
      if (p1only)
        return me_failed(stage.eq(home, value + 1)) ? ES_FAILED : ES_OK;
      return me_failed(stage.nq(home, value)) ? ES_FAILED : ES_OK;
    }
    if (alt == 0)
      return me_failed(ii.eq(home, value)) ? ES_FAILED : ES_OK;
    return me_failed(ii.nq(home, value)) ? ES_FAILED : ES_OK;
  }
  virtual void print(const Space &home, const Choice &c, unsigned int alt,
                     std::ostream &o) const {
    const II_Stage_Choice &isc = static_cast<const II_Stage_Choice &>(c);
    bool is_stage = isc.is_stage;
    bool p1only = isc.stage_p1only;
    int value = isc.value;
    if (is_stage) {
      if (alt == 0)
        o << "stage = " << value;
      else {
        if (p1only)
          o << "stage = " << value + 1;
        else
          o << "stage != " << value;
      }
    } else {
      if (alt == 0)
        o << "ii = " << value;
      else
        o << "ii != " << value;
    }
  }
  virtual Brancher *copy(Space &home) {
    return new (home) Xtensa_II_Stage_Brancher(home, *this);
  }
  static void post(Space &home, Int::IntView ii, Int::IntView stage,
                   const XtensaScheduleEngine *ScheduleSpecification) {
    (void)new (home)
        Xtensa_II_Stage_Brancher(home, ii, stage, ScheduleSpecification);
  }
  virtual size_t dispose(Space &home) { return sizeof(*this); }
};

// branch on operations
// with all predecessors scheduled,
// with min-min value
// a better fit in the bundle
// assign the cycle to it
//
// for cyclic schedule, try to assign a bigger omega or a smaller issue
class Xtensa_Schedule_Brancher : public Brancher {
protected:
  const XtensaScheduleEngine *ScheduleSpecification;
  const SchedulePrecedenceGraph &PrecedenceGraph;
  ViewArray<Int::IntView> issue;
  Int::IntView length;
  int size;
  mutable int first_candidate;
  mutable int round_robin;

  vector_readyness ready_;

  class Schedule_Choice : public Choice {
  public:
    int insn_choice, issue_value;
    Schedule_Choice(const Xtensa_Schedule_Brancher &b, int p, int v)
        : Choice(b, 2 /*number of alternative choices*/), insn_choice(p),
          issue_value(v) {}
    virtual size_t size(void) const { return sizeof(*this); }
    virtual void archive(Archive &a) const {
      Choice::archive(a);
      a << insn_choice << issue_value;
      return;
    }
  };

public:
  Xtensa_Schedule_Brancher(Space &home, ViewArray<Int::IntView> &issue0,
                           Int::IntView length,
                           const XtensaScheduleEngine *ScheduleSpecification)
      : Brancher(home), ScheduleSpecification(ScheduleSpecification),
        PrecedenceGraph(ScheduleSpecification->PrecedenceGraph), issue(issue0),
        length(length), size(issue0.size()), round_robin(0),
        ready_(size, Readyness(), vector_readyness::allocator_type(home)) {
    // FIXME, if x and y cannot issue together, issue[x] != issue[y]
    //       check if this is subsumed first.
    //       also, if x is single issue, issue[x] != issue[i!=x]
    for (int i = 0; i < size; i++) {
      ready_[i].pred_count = PrecedenceGraph.predecessors[i].size();
      ready_[i].succ_count = PrecedenceGraph.successors[i].size();
      ready_[i].assigned = false;
    }
    home.notice(*this, AP_DISPOSE);
  }
  Xtensa_Schedule_Brancher(Space &home, Xtensa_Schedule_Brancher &c)
      : Brancher(home, c), ScheduleSpecification(c.ScheduleSpecification),
        PrecedenceGraph(c.PrecedenceGraph), size(c.size),
        round_robin(c.round_robin),
        ready_(vector_readyness::allocator_type(home)) {
    ready_ = c.ready_;
    issue.update(home, c.issue);
    length.update(home, c.length);
  }
  virtual bool status(const Space &home) const {
    for (int i = 0; i < size; i++)
      if (!issue[i].assigned()) {
        first_candidate = i;
        return true;
      }
    return false;
  }
  virtual Choice *choice(Space &home) {
    int insn_choice = -1, issue_value;
    // branch based on precedence relations

    // acyclic branch
    // 1. the latest and earliest start for each instruction can be
    //  retrieved from the search engine as min/max of the variable domain
    // 2. calculate a ready list-A with all predecessors scheduled and a
    //  list-B of all successors scheduled

    // 3. with status/branch call, go through the ready list-A, for each
    //   newly scheduled instructions, remove it from the list and put
    //   predecessors ready to list-A; And, do  the similar thing to list-B
    for (int i = 0; i < size; i++) {
      if (issue[i].assigned() && !ready_[i].assigned) {
        ready_[i].assigned = true;
        for (auto Pred : PrecedenceGraph.predecessors[i])
          ready_[Pred].succ_count--;
        for (auto Succ : PrecedenceGraph.successors[i])
          ready_[Succ].pred_count--;
      }
      assert(ready_[i].succ_count >= 0);
      assert(ready_[i].pred_count >= 0);
    }
    //  5.1 select a candidate from list-A with smallest latest start,
    //   have tie breaking by domain-size, and count the number of
    //   candidates with the same min, similarly select a candidate form list-B,

    // there should always be some candidates, so only set the value to min-1
    // and max+1
    struct list_candidate {
      int index;
      int size;
      int value;
      int count;
      bool is_top;
      void update(const list_candidate &c) {
        bool is_better = ((is_top ? value > c.value : value < c.value) ||
                          (value == c.value && size > c.size));
        if (is_better) {
          *this = c;
          return;
        }
        if (value == c.value && size == c.size) {
          if (!is_top)
            index = c.index;
          count++;
        }
      }
      list_candidate(int i, int s, int v, int c, int is_top)
          : index(i), size(s), value(v), count(c), is_top(is_top) {}
    } top_candidate(-1, -1, length.max() + 1, 0, true),
        bottom_candidate(-1, -1, -1, 0, false);
    for (int i = 0; i < size; i++) {
      auto ready = ready_[i];
      if (issue[i].assigned())
        continue;
      int isize = issue[i].size();
      if (ready.pred_count == 0) {
        list_candidate c(i, isize, issue[i].min(), 1, true);
        top_candidate.update(c);
      }
      if (ready.succ_count == 0) {
        list_candidate c(i, isize, issue[i].max(), 1, false);
        bottom_candidate.update(c);
      }
    }
    int e_value_b = length.max() - 1 - bottom_candidate.value;
    if (e_value_b < top_candidate.value ||
        (e_value_b == top_candidate.value &&
         (bottom_candidate.size < top_candidate.size ||
          (bottom_candidate.size == top_candidate.size &&
           bottom_candidate.count < top_candidate.count)))) {
      insn_choice = bottom_candidate.index;
      issue_value = bottom_candidate.value;
    } else if (e_value_b == top_candidate.value &&
               bottom_candidate.size == top_candidate.size &&
               bottom_candidate.count == top_candidate.count) {
      if (round_robin == 0) {
        insn_choice = bottom_candidate.index;
        issue_value = bottom_candidate.value;
        round_robin = 1;
      } else {
        insn_choice = top_candidate.index;
        issue_value = top_candidate.value;
        round_robin = 0;
      }
    } else {
      insn_choice = top_candidate.index;
      issue_value = top_candidate.value;
    }

    //  5.2 select A if numbers candidate from B is more than A, otherwise
    //   B, have tie breaking as B.
    //  5.3 set the min value, if it is from list-A, max value if it is from
    //   list-B, median if it is on both list-A and B.

    // Cyclic branch
    // to deal with loop carried dependence, and "omega" variables
    // 1. first implementation, use the same brancher for omega and issue, say
    //  xtensa_schedule_branch(omega);  // let omegas go first
    //  xtensa_schedule_branch(issue);
    if (insn_choice != -1)
      return new Schedule_Choice(*this, insn_choice, issue_value);
    llvm_unreachable("should have found a candidate");
    for (int i = first_candidate; true; i++) {
      if (!issue[i].assigned()) {
        return new Schedule_Choice(*this, i, issue[i].min());
      }
    }
    return nullptr;
  }
  virtual Choice *choice(const Space &, Archive &e) {
    int insn_choice, issue_value;
    e >> insn_choice >> issue_value;
    return new Schedule_Choice(*this, insn_choice, issue_value);
  }
  virtual ExecStatus commit(Space &home, const Choice &c, unsigned int alt) {
    const Schedule_Choice &sc = static_cast<const Schedule_Choice &>(c);
    int insn_choice = sc.insn_choice, issue_value = sc.issue_value;
    if (alt == 0)
      return me_failed(issue[insn_choice].eq(home, issue_value)) ? ES_FAILED
                                                                 : ES_OK;
    return me_failed(issue[insn_choice].nq(home, issue_value)) ? ES_FAILED
                                                               : ES_OK;
  }
  virtual void print(const Space &home, const Choice &c, unsigned int alt,
                     std::ostream &o) const {
    const Schedule_Choice &sc = static_cast<const Schedule_Choice &>(c);
    int insn_choice = sc.insn_choice, issue_value = sc.issue_value;
    if (alt == 0)
      o << "x[" << insn_choice << "] =" << issue_value;
    else
      o << "x[" << insn_choice << "] !=" << issue_value;
  }
  virtual Brancher *copy(Space &home) {
    return new (home) Xtensa_Schedule_Brancher(home, *this);
  }
  static void post(Space &home, ViewArray<Int::IntView> &issue,
                   Int::IntView length,
                   const XtensaScheduleEngine *ScheduleSpecification) {
    (void)new (home)
        Xtensa_Schedule_Brancher(home, issue, length, ScheduleSpecification);
  }
  virtual size_t dispose(Space &home) {
    home.ignore(*this, AP_DISPOSE);
    return sizeof(*this);
  }
};

// dedicated acyclic brancher
// choose a ready instruction with minimal issue
class Xtensa_Acyclic_Brancher : public Brancher {
protected:
  const XtensaScheduleEngine *ScheduleSpecification;
  const SchedulePrecedenceGraph &PrecedenceGraph;
  ViewArray<Int::IntView> issue;
  Int::IntView length;
  int size;
  Branch_Direction branch_direction;

  // choose instruction not ready at a top/bottom cycle can force
  // other dependent instructions to the same cycle
  vector_readyness ready_;

  class Acyclic_Choice : public Choice {
  public:
    int insn_choice;
    int issue_value;
    bool is_last; // if true, it must be assigned
    Acyclic_Choice(const Xtensa_Acyclic_Brancher &b, int p, int v, bool last)
        : Choice(b, last ? 1 : 2 /*number of alternative choices*/),
          insn_choice(p), issue_value(v), is_last(last) {}
    virtual size_t size(void) const { return sizeof(*this); }
    virtual void archive(Archive &a) const {
      Choice::archive(a);
      a << insn_choice << issue_value << is_last;
      return;
    }
  };

public:
  Xtensa_Acyclic_Brancher(Space &home, ViewArray<Int::IntView> &issue0,
                          Int::IntView length, Branch_Direction dir,
                          const XtensaScheduleEngine *ScheduleSpecification)
      : Brancher(home), ScheduleSpecification(ScheduleSpecification),
        PrecedenceGraph(ScheduleSpecification->PrecedenceGraph), issue(issue0),
        length(length), size(issue0.size()), branch_direction(dir),
        ready_(size, Readyness(), vector_readyness::allocator_type(home)) {
    for (int i = 0; i < size; i++) {
      ready_[i].pred_count = PrecedenceGraph.predecessors[i].size();
      ready_[i].succ_count = PrecedenceGraph.successors[i].size();
      ready_[i].assigned = false;
    }
    home.notice(*this, AP_DISPOSE);
  }
  Xtensa_Acyclic_Brancher(Space &home, Xtensa_Acyclic_Brancher &c)
      : Brancher(home, c), ScheduleSpecification(c.ScheduleSpecification),
        PrecedenceGraph(c.PrecedenceGraph), size(c.size),
        branch_direction(c.branch_direction),
        ready_(vector_readyness::allocator_type(home)) {
    ready_ = c.ready_;
    issue.update(home, c.issue);
    length.update(home, c.length);
  }
  bool topdown() { return (branch_direction & BD_Topdown); }
  bool bottomup() { return (branch_direction & BD_Bottomup); }
  virtual bool status(const Space &home) const {
    for (int i = 0; i < size; i++)
      if (!issue[i].assigned()) {
        return true;
      }
    return false;
  }
  virtual Choice *choice(Space &home) {
    for (int i = 0; i < size; i++) {
      if (issue[i].assigned() && !ready_[i].assigned) {
        ready_[i].assigned = true;
        if (bottomup())
          for (auto Pred : PrecedenceGraph.predecessors[i])
            ready_[Pred].succ_count--;
        if (topdown())
          for (auto Succ : PrecedenceGraph.successors[i])
            ready_[Succ].pred_count--;
      }
      assert(ready_[i].succ_count >= 0);
      assert(ready_[i].pred_count >= 0);
    }

    struct frontier_candidate {
      int insn;
      int value;
      int degree;
      int size;
      int count;
    } top_candidate = {-1, -1, -1, -1, -1},
      bottom_candidate = {-1, -1, -1, -1, -1}, candidate = {-1, -1, -1, -1, -1};

    top_candidate.insn = -1;
    bottom_candidate.insn = -1;

    auto update_candidate = [](bool is_top, frontier_candidate &best, int cycle,
                               int size, int degree, int i) {
      bool better = best.insn == -1;
      if (!better) {
        if (is_top)
          better = cycle < best.value;
        else
          better = cycle > best.value;
      }
      if (better) {
        best.value = cycle;
        best.count = 1;
        best.size = size;
        best.degree = degree;
        best.insn = i;
      } else if (cycle == best.value) {
        best.count++;
        if (size < best.size || (size == best.size && degree > best.degree)) {
          best.insn = i;
          best.size = size;
          best.degree = degree;
        }
      }
    };

    for (int i = 0; i < size; i++) {
      if (!issue[i].assigned()) {
        int size = issue[i].size();
        int degree = issue[i].degree();
        if (topdown() && ready_[i].pred_count == 0) {
          int cycle = issue[i].min();
          update_candidate(true, top_candidate, cycle, size, degree, i);
        }
        if (bottomup() && ready_[i].succ_count == 0) {
          int cycle = issue[i].max();
          update_candidate(false, bottom_candidate, cycle, size, degree, i);
        }
      }
    }
    bool use_top;
    if (bottomup() && topdown()) {
      int rtop_degree = -top_candidate.degree;
      int rbottom_degree = -top_candidate.degree;
      use_top = std::tie(top_candidate.count, top_candidate.size, rtop_degree) <
                std::tie(bottom_candidate.count, bottom_candidate.size,
                         rbottom_degree);
    } else
      use_top = topdown();
    if (use_top)
      candidate = top_candidate;
    else
      candidate = bottom_candidate;
    assert(candidate.insn != -1);
    return new Acyclic_Choice(*this, candidate.insn, candidate.value,
                              candidate.count == 1);
  }
  virtual Choice *choice(const Space &, Archive &e) {
    int insn_choice, issue_value;
    bool is_last;
    e >> insn_choice >> issue_value >> is_last;
    return new Acyclic_Choice(*this, insn_choice, issue_value, is_last);
  }
  virtual ExecStatus commit(Space &home, const Choice &c, unsigned int alt) {
    const Acyclic_Choice &ac = static_cast<const Acyclic_Choice &>(c);
    int insn_choice = ac.insn_choice, issue_value = ac.issue_value;
    if (alt == 0)
      return me_failed(issue[insn_choice].eq(home, issue_value)) ? ES_FAILED
                                                                 : ES_OK;
    return me_failed(issue[insn_choice].nq(home, issue_value)) ? ES_FAILED
                                                               : ES_OK;
  }
  virtual void print(const Space &home, const Choice &c, unsigned int alt,
                     std::ostream &o) const {
    const Acyclic_Choice &ac = static_cast<const Acyclic_Choice &>(c);
    int insn_choice = ac.insn_choice, issue_value = ac.issue_value;
    if (alt == 0)
      o << "x[" << insn_choice << "] =" << issue_value;
    else
      o << "x[" << insn_choice << "] !=" << issue_value;
    if (ac.is_last)
      o << "is_last";
  }
  virtual Brancher *copy(Space &home) {
    return new (home) Xtensa_Acyclic_Brancher(home, *this);
  }
  static void post(Space &home, ViewArray<Int::IntView> &issue,
                   Int::IntView length, Branch_Direction dir,
                   const XtensaScheduleEngine *ScheduleSpecification) {
    (void)new (home) Xtensa_Acyclic_Brancher(home, issue, length, dir,
                                             ScheduleSpecification);
  }
  virtual size_t dispose(Space &home) {
    home.ignore(*this, AP_DISPOSE);
    return sizeof(*this);
  }
};

// branch on vissue with preset heuristics for each instruction
class XtensaSemiDynamicCyclicBrancher : public Brancher {
protected:
  const XtensaScheduleEngine *ScheduleSpecification;
  const SchedulePrecedenceGraph &PrecedenceGraph;
  ViewArray<Int::IntView> issue;
  Int::IntView length;
  int size;
  bool ShrinkLiveRange;
  vector_readyness ready_;

  class CyclicChoice : public Choice {
  public:
    // We may want to trim some values off either end of the initial region
    int candidate;
    int Alternate;
    int Ranges[6];

    CyclicChoice(const XtensaSemiDynamicCyclicBrancher &b, int ic, int v,
                 bool Direction)
        : Choice(b, 2), candidate(ic) {
      Alternate = 2;
      if (Direction) {
        Ranges[0] = b.issue[ic].min();
        Ranges[1] = v;
        Ranges[2] = v + 1;
        Ranges[3] = b.issue[ic].max();
      } else {
        Ranges[0] = v + 1;
        Ranges[1] = b.issue[ic].max();
        Ranges[2] = b.issue[ic].min();
        Ranges[3] = v;
      }
    }
    CyclicChoice(const XtensaSemiDynamicCyclicBrancher &b, int ic, int a,
                 int v0, int v1, int v2, int v3, int v4, int v5)
        : Choice(b, a), candidate(ic) {
      Ranges[0] = v0;
      Ranges[1] = v1;
      Ranges[2] = v2;
      Ranges[3] = v3;
      Ranges[4] = v4;
      Ranges[5] = v5;
      Alternate = a;
      assert(v0 <= v1 && v2 <= v3);
      if (a == 3)
        assert(v4 <= v5);
    }
    virtual size_t size(void) const { return sizeof(*this); }
    virtual void archive(Archive &a) const {
      Choice::archive(a);
      a << candidate << Alternate << Ranges[0] << Ranges[1] << Ranges[2]
        << Ranges[3] << Ranges[4] << Ranges[5];
      return;
    }
  };

public:
  XtensaSemiDynamicCyclicBrancher(
      Space &home, ViewArray<Int::IntView> &issue0, Int::IntView length,
      bool reduce, const XtensaScheduleEngine *ScheduleSpecification)
      : Brancher(home), ScheduleSpecification(ScheduleSpecification),
        PrecedenceGraph(ScheduleSpecification->PrecedenceGraph), issue(issue0),
        length(length), size(issue0.size()), ShrinkLiveRange(reduce),
        ready_(size, Readyness(), vector_readyness::allocator_type(home)) {
    for (int i = 0; i < size; i++) {
      ready_[i].pred_count = PrecedenceGraph.predecessors[i].size();
      ready_[i].succ_count = PrecedenceGraph.successors[i].size();
      ready_[i].assigned = false;
    }
    home.notice(*this, AP_DISPOSE);
  }

  XtensaSemiDynamicCyclicBrancher(Space &home,
                                  XtensaSemiDynamicCyclicBrancher &c)
      : Brancher(home, c), ScheduleSpecification(c.ScheduleSpecification),
        PrecedenceGraph(c.PrecedenceGraph), size(c.size),
        ShrinkLiveRange(c.ShrinkLiveRange),
        ready_(vector_readyness::allocator_type(home)) {
    ready_ = c.ready_;
    issue.update(home, c.issue);
    length.update(home, c.length);
  }

  virtual bool status(const Space &home) const {
    for (int i = 0; i < size; i++) {
      if (!issue[i].assigned())
        return true;
    }
    return false;
  }

  virtual Choice *choice(Space &home) {
    int Cand = size;
    InstructionBranchPolicy Policy;
    for (int i = 0; i < size; i++) {
      if (issue[i].assigned() && !ready_[i].assigned) {
        ready_[i].assigned = true;
        for (auto Pred : PrecedenceGraph.predecessors[i])
          ready_[Pred].succ_count--;
        for (auto Succ : PrecedenceGraph.successors[i])
          ready_[Succ].pred_count--;
      }
      assert(ready_[i].succ_count >= 0);
      assert(ready_[i].pred_count >= 0);
    }
    for (int I = 0; I < size; I++) {
      if (issue[I].assigned())
        continue;
      if (Cand == size) {
        Cand = I;
        Policy = ScheduleSpecification->Base.BranchPolicy[I];
        continue;
      }
      // TODO if an instruction is "neutral directed", schedule it when either
      // the predecessors are all scheduled or successors are scheduled
      auto &PolicyI = ScheduleSpecification->Base.BranchPolicy[I];
      if (Policy < PolicyI)
        continue;
      if (PolicyI < Policy) {
        Cand = I;
        Policy = PolicyI;
        continue;
      }
      if (!Policy.Projected && !PolicyI.Projected && Policy.Directed &&
          PolicyI.Directed) {
        int ReadyC = 0, Ready = 0;
        if (ready_[Cand].succ_count == 0)
          ReadyC++;
        if (ready_[Cand].pred_count == 0)
          ReadyC++;
        if (ready_[I].succ_count == 0)
          Ready++;
        if (ready_[I].pred_count == 0)
          Ready++;
        if (ReadyC != Ready) {
          if (ReadyC > Ready)
            continue;
          Cand = I;
          Policy = PolicyI;
          continue;
        }
      }
      if (issue[I].size() * Policy.Priority <
          issue[Cand].size() * PolicyI.Priority) {
        Cand = I;
        Policy = PolicyI;
        continue;
      }
    }
    assert(Cand != size);
    const auto &VI = issue[Cand];
    int Min = VI.min();
    int Max = VI.max();
    if (Policy.Projected) {
      int Length = length.max();

      // TODO
      // 1. Create three regions, say, the first branch region is
      //   [ProjectedValue - 0.16 * Size, ProjectedValue + 0.16 * Size],
      //   and there are two other regions around it.
      // 2. If either of the other two regions is empty, trim the main region
      //    as well to make the projected value at the center of it.
      // E.G. if the region is [0-8]
      // If the projected value is 4, then the regions are [0-2][3-5][6-8]
      // If it is 1, the regions are [0-2][3-8]
      // If it is 0, the regions are [0][1-8]

      // enlarge the side if in range
      double ProjectedAverage = ScheduleSpecification->ProjectedCycleP[Cand] *
                                Length *
                                ScheduleSpecification->ProjectedLengthInverseP;
      int Average = VI.med();
#if 0
      // biased biparty branch for the projected value
      bool SelectLow = ProjectedAverage <= Average;
      if (Min <= ProjectedAverage && ProjectedAverage <= Max) {
        if (SelectLow)
          Average = std::round(ProjectedAverage + ((Max - Min) * 0.13));
        else
          Average = std::round(ProjectedAverage - ((Max - Min) * 0.13));
        if (Average < Min)
          Average = Min;
        if (Average > Max - 1)
          Average = Max - 1;
      }
      SelectLow = ProjectedAverage <= Average;
      return new CyclicChoice(*this, Cand, Average, SelectLow);
#endif
      int C = ProjectedAverage;
      if (C <= Min)
        return new CyclicChoice(*this, Cand, Average, true);
      if (C >= Max)
        return new CyclicChoice(*this, Cand, Average, false);
      int Cut1 = std::round(ProjectedAverage - (Max - Min) * 0.19);
      int Cut2 = ProjectedAverage + (Max - Min) * 0.19;
      if (Cut1 <= Min) {
        if (Cut2 >= Max) // possible?
          return new CyclicChoice(*this, Cand, Average,
                                  ProjectedAverage <= Average);
        return new CyclicChoice(*this, Cand, Cut2, true);
      }
      if (Cut2 >= Max)
        return new CyclicChoice(*this, Cand, Cut1 - 1, false);
      return new CyclicChoice(*this, Cand, 3, Cut1, Cut2, Min, Cut1 - 1,
                              Cut2 + 1, Max);
    }
    if (Policy.Directed) {
      if (!ScheduleSpecification->ReduceMovement) {
        if (Policy.Direction != 0)
          return new CyclicChoice(*this, Cand, VI.med(), Policy.Direction < 0);
        return new CyclicChoice(*this, Cand, VI.med(),
                                ready_[Cand].pred_count == 0);
      }
      if (Policy.Direction < 0) {
        int MinR = Min + VI.regret_min();
        return new CyclicChoice(*this, Cand, 2, Min, Min, MinR, MinR, 1, 0);
      }
      if (Policy.Direction > 0) {
        int MaxR = Max - VI.regret_max();
        return new CyclicChoice(*this, Cand, 2, Max, Max, MaxR, MaxR, 1, 0);
      }
      return new CyclicChoice(*this, Cand, 2, Min, Min, Max, Max, 1, 0);
    }
    if (Policy.RightAfter) {
      int AnchorValue = issue[Policy.Anchor].val();

      // select lowest value for iv
      if (AnchorValue > Max)
        return new CyclicChoice(*this, Cand, Max - 1, false);
      if (AnchorValue <= Min)
        return new CyclicChoice(*this, Cand, Min, true);
      return new CyclicChoice(*this, Cand, AnchorValue - 1, false);
    }
    return nullptr;
  }

  virtual Choice *choice(const Space &, Archive &e) {
    int insn_choice;
    int Alternate;
    int V0, V1, V2, V3, V4, V5;
    e >> insn_choice >> Alternate >> V0 >> V1 >> V2 >> V3 >> V4 >> V5;
    return new CyclicChoice(*this, insn_choice, Alternate, V0, V1, V2, V3, V4,
                            V5);
  }

  virtual ExecStatus commit(Space &home, const Choice &c, unsigned int alt) {
    const CyclicChoice &cc = static_cast<const CyclicChoice &>(c);
    int cand = cc.candidate;
    int LB = cc.Ranges[alt * 2];
    int UB = cc.Ranges[alt * 2 + 1];
    if (LB == UB)
      return me_failed(issue[cand].eq(home, LB)) ? ES_FAILED : ES_OK;
    if (me_failed(issue[cand].lq(home, UB)))
      return ES_FAILED;
    return me_failed(issue[cand].gq(home, LB)) ? ES_FAILED : ES_OK;
  }

  virtual void print(const Space &home, const Choice &c, unsigned int alt,
                     std::ostream &o) const {
    const CyclicChoice &cc = static_cast<const CyclicChoice &>(c);
    int cand = cc.candidate;
    int LB = cc.Ranges[alt * 2];
    int UB = cc.Ranges[alt * 2 + 1];
    o << LB << " <= vissue[" << cand << "] <=" << UB;
  }

  virtual Brancher *copy(Space &home) {
    return new (home) XtensaSemiDynamicCyclicBrancher(home, *this);
  }

  static void post(Space &home, ViewArray<Int::IntView> &issue,
                   Int::IntView length, bool reduce,
                   const XtensaScheduleEngine *ScheduleSpecification) {
    (void)new (home) XtensaSemiDynamicCyclicBrancher(
        home, issue, length, reduce, ScheduleSpecification);
  }

  virtual size_t dispose(Space &home) {
    home.ignore(*this, AP_DISPOSE);
    return sizeof(*this);
  }
};

// branch on vissue with heuristics
class Xtensa_Cyclic_Brancher : public Brancher {
protected:
  const XtensaScheduleEngine *ScheduleSpecification;
  const SchedulePrecedenceGraph &PrecedenceGraph;
  ViewArray<Int::IntView> issue;
  Int::IntView length;
  int size;
  Branch_Direction branch_direction;
  int strategy;
  // 5. branch one direction openly
  // 4. branch to a number of ranges, and branch to the middle range first, like
  // 2-3, 0-1, 4-5
  // 3. try to narrow all variables to branch_tile size, and then branch to
  // individual value in the domain
  // 2. select a smallest range, discard value more than min()+II, and then
  // split
  // 1. branch on ready insn with smallest value, try only N possibilities
  unsigned branch_tile;
  vector_readyness ready_;

  class Cyclic_Choice5 : public Choice {
  public:
    int candidate;
    int value, bound;
    Cyclic_Choice5(const Xtensa_Cyclic_Brancher &b, int ic, int v, int ub)
        : Choice(b, value != bound ? 2 : 1 /*number of alternative choices*/),
          candidate(ic), value(v), bound(ub) {}
    virtual size_t size(void) const { return sizeof(*this); }
    virtual void archive(Archive &a) const {
      Choice::archive(a);
      a << candidate << value << bound;
      return;
    }
  };
  class Cyclic_Choice2 : public Choice {
  public:
    int candidate;
    int lb, split, ub;
    Cyclic_Choice2(const Xtensa_Cyclic_Brancher &b, int ic, int lb, int split,
                   int ub)
        : Choice(b, lb != ub ? 2 : 1 /*number of alternative choices*/),
          candidate(ic), lb(lb), split(split), ub(ub) {}
    virtual size_t size(void) const { return sizeof(*this); }
    virtual void archive(Archive &a) const {
      Choice::archive(a);
      a << candidate << lb << split << ub;
      return;
    }
  };

  class Cyclic_Choice1 : public Choice {
  public:
    int candidate;
    int tile;
    int value[MAX_BRANCH_TILE];
    Cyclic_Choice1(const Xtensa_Cyclic_Brancher &b, int ic, int tile, int iv[])
        : Choice(b, tile /*number of alternative choices*/), candidate(ic),
          tile(tile) {
      for (int i = 0; i < tile; i++)
        value[i] = iv[i];
    }
    virtual size_t size(void) const { return sizeof(*this); }
    virtual void archive(Archive &a) const {
      Choice::archive(a);
      a << candidate << tile;
      for (int i = 0; i < tile; i++)
        a << value[i];
      return;
    }
  };

  class Cyclic_Choice4 : public Choice {
  public:
    int candidate;
    int tile;
    int value[MAX_BRANCH_TILE * 2];
    Cyclic_Choice4(const Xtensa_Cyclic_Brancher &b, int ic, int tile, int iv[])
        : Choice(b, tile /*number of alternative choices*/), candidate(ic),
          tile(tile) {
      for (int i = 0; i < tile * 2; i++)
        value[i] = iv[i];
    }
    virtual size_t size(void) const { return sizeof(*this); }
    virtual void archive(Archive &a) const {
      Choice::archive(a);
      a << candidate << tile;
      for (int i = 0; i < tile * 2; i++)
        a << value[i];
      return;
    }
  };
  bool topdown() { return (branch_direction & BD_Topdown); }
  bool bottomup() { return (branch_direction & BD_Bottomup); }

public:
  Xtensa_Cyclic_Brancher(Space &home, ViewArray<Int::IntView> &issue0,
                         Int::IntView length, Branch_Direction dir,
                         int strategy,
                         const XtensaScheduleEngine *ScheduleSpecification)
      : Brancher(home), ScheduleSpecification(ScheduleSpecification),
        PrecedenceGraph(ScheduleSpecification->PrecedenceGraph), issue(issue0),
        length(length), size(issue0.size()), branch_direction(dir),
        strategy(strategy), branch_tile(5),
        ready_(size, Readyness(), vector_readyness::allocator_type(home)) {
    assert(branch_tile <= MAX_BRANCH_TILE);
    assert(strategy == 5 || strategy == 1 || strategy == 2 || strategy == 3 ||
           strategy == 4);
    assert(!(strategy == 5 && branch_direction != BD_Topdown));
    for (int i = 0; i < size; i++) {
      ready_[i].pred_count = PrecedenceGraph.predecessors[i].size();
      ready_[i].succ_count = PrecedenceGraph.successors[i].size();
      ready_[i].assigned = false;
    }
    home.notice(*this, AP_DISPOSE);
  }
  Xtensa_Cyclic_Brancher(Space &home, Xtensa_Cyclic_Brancher &c)
      : Brancher(home, c), ScheduleSpecification(c.ScheduleSpecification),
        PrecedenceGraph(c.PrecedenceGraph), size(c.size),
        branch_direction(c.branch_direction), strategy(c.strategy),
        branch_tile(c.branch_tile),
        ready_(vector_readyness::allocator_type(home)) {
    ready_ = c.ready_;
    issue.update(home, c.issue);
    length.update(home, c.length);
  }
  virtual bool status(const Space &home) const {
    for (int i = 0; i < size; i++) {
      if (!issue[i].assigned())
        return true;
    }
    return false;
  }
  virtual Choice *choice(Space &home) {
    for (int i = 0; i < size; i++) {
      if (issue[i].assigned() && !ready_[i].assigned) {
        ready_[i].assigned = true;
        for (auto Pred : PrecedenceGraph.predecessors[i])
          ready_[Pred].succ_count--;
        for (auto Succ : PrecedenceGraph.successors[i])
          ready_[Succ].pred_count--;
      }
      assert(ready_[i].succ_count >= 0);
      assert(ready_[i].pred_count >= 0);
    }

    struct cyclic_candidate {
      int insn;
      int value;
      double merit;
      int degree;
      int count;
      bool fine_grain;
      bool is_top;
      void update(const cyclic_candidate &C) {
        const double epsilon = 1.0e-10;
        if (insn == -1 || C.merit < merit) {
          *this = C;
        } else if (std::fabs(C.merit - merit) < epsilon) {
          if (C.value < value || (C.value == value && C.degree > degree)) {
            insn = C.insn;
            merit = C.merit;
            degree = C.degree;
          }
          count++;
        }
      }
      cyclic_candidate(int insn, int value, double merit, int degree, int count,
                       bool fine_grain, bool is_top)
          : insn(insn), value(value), merit(merit), degree(degree),
            count(count), fine_grain(fine_grain), is_top(is_top) {}
      cyclic_candidate() {}
    } top_candidate(-1, -1, -1, -1, -1, false, true),
        bottom_candidate(-1, -1, -1, -1, -1, false, false),
        fine_top_candidate(-1, -1, -1, -1, -1, true, true),
        fine_bottom_candidate(-1, -1, -1, -1, -1, true, false);

    for (int i = 0; i < size; i++) {
      if (!issue[i].assigned()) {
        int degree = issue[i].degree();
        double scale = ScheduleSpecification->Base.ResourceMerit[i];
        if (topdown()) {
          struct cyclic_candidate *candidate;
          if (strategy == 3 && issue[i].size() < branch_tile)
            candidate = &fine_top_candidate;
          else
            candidate = &top_candidate;
          double merit;
          // mostly only choose ready instructions, also bias min over the
          // domain size
          if (strategy == 5)
            merit = issue[i].size() * 10 + issue[i].min() +
                    100 * ready_[i].pred_count;
          else
            merit = issue[i].size() - ready_[i].pred_count;

          if (merit > 0)
            merit /= (scale * scale);
          else
            merit *= (scale * scale);
          cyclic_candidate temp_cand(i, issue[i].min(), merit, degree, 1,
                                     candidate->fine_grain, true);
          candidate->update(temp_cand);
        }
        if (bottomup()) {
          struct cyclic_candidate *candidate;
          if (strategy == 3 && issue[i].size() < branch_tile)
            candidate = &fine_bottom_candidate;
          else
            candidate = &bottom_candidate;

          double merit = issue[i].max() - ready_[i].succ_count;
          if (merit > 0)
            merit /= (scale * scale);
          else
            merit *= (scale * scale);
          cyclic_candidate temp_cand(i, issue[i].max(), merit, degree, 1,
                                     candidate->fine_grain, true);
          candidate->update(temp_cand);
        }
      }
    }
    if (top_candidate.insn == -1)
      top_candidate = fine_top_candidate;
    if (bottom_candidate.insn == -1)
      bottom_candidate = fine_bottom_candidate;
    bool use_top;
    cyclic_candidate candidate;
    if (topdown()) {
      if (bottomup()) {
        if ((!top_candidate.fine_grain && bottom_candidate.fine_grain) ||
            ((top_candidate.fine_grain == bottom_candidate.fine_grain) &&
             ((top_candidate.merit <= bottom_candidate.merit) ||
              (top_candidate.insn == bottom_candidate.insn)))) {
          use_top = true;
          candidate = top_candidate;
        } else {
          use_top = false;
          candidate = bottom_candidate;
        }
      } else {
        use_top = true;
        candidate = top_candidate;
      }
    } else {
      use_top = false;
      candidate = bottom_candidate;
    }
    assert(candidate.insn != -1);
    int minval = issue[candidate.insn].min();
    int maxval = issue[candidate.insn].max();
    int extent = length.min() - 1;
    if (strategy == 5) {
      assert(use_top);
      return new Cyclic_Choice5(*this, candidate.insn, minval,
                                maxval - minval > extent ? minval + extent
                                                         : maxval);
    } else if (strategy == 1) {
      int value[MAX_BRANCH_TILE];
      int n = 0;
      int tile = branch_tile;
      // FIXME dynamically adjust tile size
      //   e.g. enlarge tile size if count is bigger
      // if (tile > candidate.count) tile = candidate.count;
      // if (tile < 2) tile = 2;
      if (use_top) {
        for (IntVarValues ival(issue[candidate.insn]); ival(); ++ival) {
          if (ival.val() - minval > extent)
            break;
          value[n++] = ival.val();
          if (n == tile)
            break;
        }
      } else {
        int start = issue[candidate.insn].size() - tile;
        IntVarValues ival(issue[candidate.insn]);
        // FIXME how to reversely iterate?
        while (start-- > 0)
          ++ival;
        while (maxval - ival.val() > extent)
          ++ival;
        for (; ival(); ++ival) {
          value[n++] = ival.val();
        }
        for (int i = 0; i <= n / 2; i++)
          std::swap(value[i], value[n - i - 1]);
      }
      return new Cyclic_Choice1(*this, candidate.insn, n, &candidate.value);
    } else if (strategy == 2 || strategy == 4) {
      if (maxval - minval > extent) {
        if (use_top && ready_[candidate.insn].pred_count == 0) {
          maxval = minval + extent;
          while (!issue[candidate.insn].in(maxval))
            maxval--;
        } else if (!use_top && ready_[candidate.insn].succ_count == 0) {
          minval = maxval - extent;
          while (!issue[candidate.insn].in(minval))
            minval++;
        }
      }
      if (strategy == 2) {
        int split = minval + (maxval - minval) / 2;
        if (!use_top)
          return new Cyclic_Choice2(*this, candidate.insn, maxval, split,
                                    minval);
        return new Cyclic_Choice2(*this, candidate.insn, minval, split, maxval);
      } else if (strategy == 4) {
        int splits[MAX_BRANCH_TILE * 2];
        int num_split = branch_tile;
        int span = maxval - minval + 1;
        if (num_split > span)
          num_split = span;
        bool is_odd = (num_split & 1);
        int center = num_split / 2;
        int k = 0;
        for (int j = 0; j < (num_split + 1) / 2; j++) {
          int i1, i2;
          if (is_odd) {
            i1 = center - j;
            i2 = center + j;
          } else {
            i1 = center - j - 1;
            i2 = center + j;
          }
          splits[k++] = minval + span * i1 / num_split;
          splits[k++] = minval + span * (i1 + 1) / num_split - 1;
          if (i1 != i2) {
            splits[k++] = minval + span * i2 / num_split;
            splits[k++] = minval + span * (i2 + 1) / num_split - 1;
          }
        }
        splits[2 * num_split - 1] = maxval;
        return new Cyclic_Choice4(*this, candidate.insn, num_split, splits);
      }
      llvm_unreachable("internal error");
    } else if (strategy == 3) {
      if (candidate.fine_grain) {
        int split = minval + (maxval - minval) / 2;
        if (!use_top)
          return new Cyclic_Choice2(*this, candidate.insn, maxval, split,
                                    minval);
        return new Cyclic_Choice2(*this, candidate.insn, minval, split, maxval);
      }
      if (maxval - minval > extent) {
        if (use_top && ready_[candidate.insn].pred_count == 0) {
          maxval = minval + extent;
          while (!issue[candidate.insn].in(maxval))
            maxval--;
        } else if (!use_top && ready_[candidate.insn].succ_count == 0) {
          minval = maxval - extent;
          while (!issue[candidate.insn].in(minval))
            minval++;
        }
      }
      int split = minval + (maxval - minval) / 2;
      if (!use_top)
        return new Cyclic_Choice2(*this, candidate.insn, maxval, split, minval);
      return new Cyclic_Choice2(*this, candidate.insn, minval, split, maxval);
    }
    llvm_unreachable("internal error");
    return nullptr;
  }
  virtual Choice *choice(const Space &, Archive &e) {
    if (strategy == 5) {
      int insn_choice, value, bound;
      e >> insn_choice >> value >> bound;
      return new Cyclic_Choice5(*this, insn_choice, value, bound);
    }
    if (strategy == 1) {
      int insn_choice, tile;
      e >> insn_choice >> tile;
      int values[MAX_BRANCH_TILE];
      for (int i = 0; i < tile; i++)
        e >> values[i];
      return new Cyclic_Choice1(*this, insn_choice, tile, values);
    }
    if (strategy == 4) {
      int insn_choice, tile;
      e >> insn_choice >> tile;
      int values[MAX_BRANCH_TILE * 2];
      for (int i = 0; i < tile * 2; i++)
        e >> values[i];
      return new Cyclic_Choice4(*this, insn_choice, tile, values);
    }
    if (strategy == 2 || strategy == 3) {
      int insn_choice, lb, ub, split;
      e >> insn_choice >> lb >> split >> ub;
      return new Cyclic_Choice2(*this, insn_choice, lb, split, ub);
    }
    llvm_unreachable("unknown strategy");
    return nullptr;
  }
  virtual ExecStatus commit(Space &home, const Choice &c, unsigned int alt) {
    if (strategy == 5) {
      const Cyclic_Choice5 &cc = static_cast<const Cyclic_Choice5 &>(c);
      int cand = cc.candidate;
      if (alt == 0)
        return me_failed(issue[cand].eq(home, cc.value)) ? ES_FAILED : ES_OK;
      if (me_failed(issue[cand].nq(home, cc.value)))
        return ES_FAILED;
      return me_failed(issue[cand].lq(home, cc.bound)) ? ES_FAILED : ES_OK;
    }
    if (strategy == 1) {
      const Cyclic_Choice1 &cc = static_cast<const Cyclic_Choice1 &>(c);
      int cand = cc.candidate;
      assert(alt < (unsigned)cc.tile);
      return me_failed(issue[cand].eq(home, cc.value[alt])) ? ES_FAILED : ES_OK;
    }
    if (strategy == 4) {
      const Cyclic_Choice4 &cc = static_cast<const Cyclic_Choice4 &>(c);
      int cand = cc.candidate;
      assert(alt < (unsigned)cc.tile);
      if (cc.value[2 * alt] == cc.value[2 * alt + 1]) {
        return me_failed(issue[cand].eq(home, cc.value[2 * alt])) ? ES_FAILED
                                                                  : ES_OK;
      }
      if (me_failed(issue[cand].gq(home, cc.value[2 * alt])))
        return ES_FAILED;
      return me_failed(issue[cand].lq(home, cc.value[2 * alt + 1])) ? ES_FAILED
                                                                    : ES_OK;
    }
    assert(strategy == 2 || strategy == 3);
    const Cyclic_Choice2 &cc = static_cast<const Cyclic_Choice2 &>(c);
    int cand = cc.candidate;
    if (cc.lb == cc.ub)
      return me_failed(issue[cand].eq(home, cc.lb)) ? ES_FAILED : ES_OK;
    if (alt == 0) {
      if (cc.lb < cc.ub)
        // [lb, split]
        return me_failed(issue[cand].lq(home, cc.split)) ? ES_FAILED : ES_OK;
      // [split, lb]
      return me_failed(issue[cand].gr(home, cc.split)) ? ES_FAILED : ES_OK;
    }
    if (cc.lb < cc.ub) {
      // (split, ub]
      if (me_failed(issue[cand].gr(home, cc.split)))
        return ES_FAILED;
      return me_failed(issue[cand].lq(home, cc.ub)) ? ES_FAILED : ES_OK;
    }
    // [ub, split)
    if (me_failed(issue[cand].lq(home, cc.split)))
      return ES_FAILED;
    return me_failed(issue[cand].gq(home, cc.ub)) ? ES_FAILED : ES_OK;
  }
  virtual void print(const Space &home, const Choice &c, unsigned int alt,
                     std::ostream &o) const {
    if (strategy == 5) {
      const Cyclic_Choice5 &cc = static_cast<const Cyclic_Choice5 &>(c);
      int cand = cc.candidate;
      if (alt == 0)
        o << "o[" << cand << "] =" << cc.value;
      o << "o[" << cand << "] = (" << cc.value << ", " << cc.bound << "]";
      return;
    }
    if (strategy == 1) {
      const Cyclic_Choice1 &cc = static_cast<const Cyclic_Choice1 &>(c);
      int cand = cc.candidate;
      o << "o[" << cand << "] =" << cc.value[alt];
      return;
    }
    if (strategy == 4) {
      const Cyclic_Choice4 &cc = static_cast<const Cyclic_Choice4 &>(c);
      int cand = cc.candidate;
      o << "o[" << cand << "] =" << cc.value[2 * alt] << ','
        << cc.value[2 * alt + 1];
      return;
    }
    if (strategy == 2 || strategy == 3) {
      const Cyclic_Choice2 &cc = static_cast<const Cyclic_Choice2 &>(c);
      int cand = cc.candidate;
      if (alt == 0)
        o << "o[" << cand << "] =" << cc.lb << cc.split;
      o << "o[" << cand << "] =" << cc.split << cc.ub;
    }
  }
  virtual Brancher *copy(Space &home) {
    return new (home) Xtensa_Cyclic_Brancher(home, *this);
  }
  static void post(Space &home, ViewArray<Int::IntView> &issue,
                   Int::IntView length, Branch_Direction dir, int strategy,
                   const XtensaScheduleEngine *ScheduleSpecification) {
    (void)new (home) Xtensa_Cyclic_Brancher(home, issue, length, dir, strategy,
                                            ScheduleSpecification);
  }
  virtual size_t dispose(Space &home) {
    home.ignore(*this, AP_DISPOSE);
    return sizeof(*this);
  }
};

// branch on vissue with cluster heuristic
class XtensaClusterBrancher : public Brancher {
protected:
  const XtensaScheduleEngine *ScheduleSpecification;
  ViewArray<Int::IntView> Issue;
  Int::IntView II;
  Int::IntView Stage;
  int Size;

  class ClusterChoice : public Choice {
  public:
    int ID;
    enum Tag_T { LowerFirst = 0, HigherFirst = 1, OneValue = 2 } Tag;
    int Split;
    ClusterChoice(const XtensaClusterBrancher &B, int id, bool low, int split)
        : Choice(B, 2), ID(id), Tag(low ? LowerFirst : HigherFirst),
          Split(split) {}
    ClusterChoice(const XtensaClusterBrancher &B, int id, int split)
        : Choice(B, 1), ID(id), Tag(OneValue), Split(split) {}
    virtual size_t size(void) const { return sizeof(*this); }
    virtual void archive(Archive &a) const {
      Choice::archive(a);
      a << ID << Tag << Split;
      return;
    }
  };

public:
  XtensaClusterBrancher(Space &home, ViewArray<Int::IntView> &Issue,
                        Int::IntView II, Int::IntView Stage,
                        const XtensaScheduleEngine *ScheduleSpecification)
      : Brancher(home), ScheduleSpecification(ScheduleSpecification),
        Issue(Issue), II(II), Stage(Stage), Size(Issue.size()) {
    home.notice(*this, AP_DISPOSE);
  }
  XtensaClusterBrancher(Space &home, XtensaClusterBrancher &c)
      : Brancher(home, c), ScheduleSpecification(c.ScheduleSpecification),
        Size(c.Size) {
    Issue.update(home, c.Issue);
    II.update(home, c.II);
    Stage.update(home, c.Stage);
  }
  virtual bool status(const Space &home) const {
    for (int i = 0; i < Size; i++) {
      if (!Issue[i].assigned())
        return true;
    }
    return false;
  }
  virtual Choice *choice(Space &home) {
    struct Candidate_T {
      int ID;
      double Merit;
      Candidate_T(int ID, double Merit)
          : ID(ID), Merit(Merit) {}
      bool IsBetter(const Candidate_T &C) {
        return Merit < C.Merit;
      }
    } Candidate(-1, DBL_MAX);
    double Scale =
        II.max() * Stage.max() * ScheduleSpecification->ProjectedLengthInverse;
    int NumClusters = ScheduleSpecification->Base.Clusters.size();
    std::vector<int> ClusterPlaced(NumClusters, 0);
    for (int i = 0; i < Size; i++) {
      if (!Issue[i].assigned())
        continue;
      int C = ScheduleSpecification->Base.ClusterID[i];
      if (C == -1)
        continue;
      ClusterPlaced[C]++;
    }
    bool IsBottomUp = ScheduleSpecification->CPSDirection;
    std::vector<double> ClusterMeritScale(NumClusters, 1.0);
    for (int i = 0; i < NumClusters; i++)
      ClusterMeritScale[i] =
          1.0 / (1.0 + ClusterPlaced[i] * 1.0 /
                           ScheduleSpecification->Base.Clusters[i].size());
    for (int pi = 0; pi < Size; pi++) {
      int i;
      if (IsBottomUp)
        i = Size - 1 - pi;
      else
        i = pi;
      if (Issue[i].assigned())
        continue;
      double Merit =
          ScheduleSpecification->Base.ResourceMerit[i] * Issue[i].size();
      int ClusterID = ScheduleSpecification->Base.ClusterID[i];
      if (ClusterID != -1)
        Merit *= ClusterMeritScale[ClusterID];
      Candidate_T Cur(i, Merit);
      if (Cur.IsBetter(Candidate)) {
        Candidate.ID = i;
        Candidate.Merit = Merit;
      }
    }

    assert(Candidate.ID != -1 && "There should be a candidate.");

    // TODO A, basically sequentially place all instructions in the cluster
    // 0. adjust the merit for the cluster
    // 1. re-select "head" instruction of the cluster
    // 2. heuristic branching on remaining instructions in the cluster
    int i = Candidate.ID;
    int Min = Issue[i].min();
    int Max = Issue[i].max();
    int Split = Issue[i].med();
    double projected_d = ScheduleSpecification->ProjectedCycle[i];
    if (IsBottomUp)
      projected_d += ScheduleSpecification->ProjectedCycleGap;
    projected_d *= Scale;
    int ID = ScheduleSpecification->Base.ClusterID[i];
    if (ID != -1) {
      int NumP = 0, NumS = 0, NumPeer = 0;
      int PVal = -1;
      int AllP = ScheduleSpecification->Base.ClusterPredecessors[i].size();
      int AllS = ScheduleSpecification->Base.ClusterSuccessors[i].size();
      for (int P : ScheduleSpecification->Base.ClusterPredecessors[i])
        if (Issue[P].assigned())
          NumP++;
      for (int S : ScheduleSpecification->Base.ClusterSuccessors[i])
        if (Issue[S].assigned())
          NumS++;
      for (int P : ScheduleSpecification->Base.ClusterPeers[i])
        if (Issue[P].assigned()) {
          NumPeer++;
          if (PVal == -1) {
            PVal = Issue[P].val();
            if (P < i)
              PVal++;
            else
              PVal--;
          }
        }
      if (AllP != 0 && AllP == NumP)
        return new ClusterChoice(*this, Candidate.ID, true, Split);
      if (AllS != 0 && AllS == NumS)
        return new ClusterChoice(*this, Candidate.ID, false, Split);
      if (NumPeer != 0)
        projected_d = PVal;
    }
    bool Low = projected_d <= Split;
    int projected_n = projected_d;

    // enlarge the side if in range
    if (Min <= projected_n && projected_n <= Max) {
      if (Low)
        Split = (Min + Max * 2) / 3;
      else
        Split = (Min * 2 + Max) / 3;
    }
    return new ClusterChoice(*this, Candidate.ID, Low, Split);
  }
  virtual Choice *choice(const Space &, Archive &e) {
    int ID, Split;
    int Low;
    e >> ID >> Low >> Split;
    return new ClusterChoice(*this, ID, Low, Split);
  }
  virtual ExecStatus commit(Space &home, const Choice &c, unsigned int alt) {
    const ClusterChoice &C = static_cast<const ClusterChoice &>(c);
    if (C.Tag == ClusterChoice::OneValue)
      return me_failed(Issue[C.ID].eq(home, C.Split)) ? ES_FAILED : ES_OK;
    if ((C.Tag == ClusterChoice::LowerFirst) == (alt == 0))
      return me_failed(Issue[C.ID].lq(home, C.Split)) ? ES_FAILED : ES_OK;
    return me_failed(Issue[C.ID].gr(home, C.Split)) ? ES_FAILED : ES_OK;
  }
  virtual void print(const Space &home, const Choice &c, unsigned int alt,
                     std::ostream &o) const {
    const ClusterChoice &C = static_cast<const ClusterChoice &>(c);
    if ((C.Tag == ClusterChoice::LowerFirst) == (alt == 0))
      o << "vissue[" << C.ID << "] <= " << C.Split;
    else
      o << "vissue[" << C.ID << "] > " << C.Split;
  }
  virtual Brancher *copy(Space &home) {
    return new (home) XtensaClusterBrancher(home, *this);
  }
  static void post(Space &home, ViewArray<Int::IntView> &Issue, Int::IntView II,
                   Int::IntView Stage,
                   const XtensaScheduleEngine *ScheduleSpecification) {
    (void)new (home)
        XtensaClusterBrancher(home, Issue, II, Stage, ScheduleSpecification);
  }
  virtual size_t dispose(Space &home) {
    home.ignore(*this, AP_DISPOSE);
    return sizeof(*this);
  }
};

// dynamic breaking of semi symmetry of resources
class XtensaResourceBrancher : public Brancher {
protected:
  const XtensaScheduleEngine *ScheduleSpecification;
  ViewArray<Int::IntView> Issue;
  Int::IntView II;
  Int::IntView Stage;
  int Size;
  const Tensilica::DFAPacketizer *DFA0;

  class ResourceChoice : public Choice {
  public:
    int Cycle;
    int NumCandidate;

    // Number of alternatives is Max - Min + 1, and we start at Med,
    // The sequence is like Med, Med+1, Med-1, Med+2, Med-2, ... Min/Max
    int Max;
    int Min;
    int Med;
    // space alloc
    // std::vector<int> Candidate;
    std::vector<int> Candidate;
    ResourceChoice(const XtensaResourceBrancher &B, int Cycle, int N, int Max,
                   int Min, int Med, const std::vector<int> &Candidate)
        : Choice(B, Max + 1 - Min), Cycle(Cycle), NumCandidate(N), Max(Max),
          Min(Min), Med(Med), Candidate(Candidate) {}
    virtual size_t size(void) const { return sizeof(*this) + NumCandidate * 8; }
    virtual void archive(Archive &a) const {
      Choice::archive(a);
      a << Cycle << NumCandidate << Max << Min << Med;
      for (auto C : Candidate)
        a << C;
      return;
    }
  };

public:
  XtensaResourceBrancher(Space &home, ViewArray<Int::IntView> &Issue,
                         Int::IntView II, Int::IntView Stage,
                         const XtensaScheduleEngine *ScheduleSpecification)
      : Brancher(home), ScheduleSpecification(ScheduleSpecification),
        Issue(Issue), II(II), Stage(Stage), Size(Issue.size()),
        DFA0(ScheduleSpecification->ResourceModel) {
    home.notice(*this, AP_DISPOSE);
  }
  XtensaResourceBrancher(Space &home, XtensaResourceBrancher &c)
      : Brancher(home, c), ScheduleSpecification(c.ScheduleSpecification),
        Size(c.Size) {
    Issue.update(home, c.Issue);
    II.update(home, c.II);
    Stage.update(home, c.Stage);
  }
  virtual bool status(const Space &home) const {
    for (int i = 0; i < Size; i++) {
      if (!Issue[i].assigned())
        return true;
    }
    return false;
  }
  virtual Choice *choice(Space &home) {
    int CurII = II.max();
    int VCycle = INT_MAX;
    std::vector<int> Candidate0;
    // get the minimum virtual cycle we can assign something
    for (int i = 0; i < Size; i++) {
      if (Issue[i].assigned())
        continue;
      int Min = Issue[i].min();
      if (Min > VCycle)
        continue;
      if (Min < VCycle) {
        VCycle = Min;
        Candidate0.clear();
      }
      Candidate0.push_back(i);
    }
    // get the classes of the candidate ordered by criticalness
    double BestMerit = DBL_MAX;
    int BestClass = 0;
    std::vector<int> Candidate;
    for (auto C : Candidate0) {
      double Merit = ScheduleSpecification->Base.ResourceMerit[C];
      if (Merit > BestMerit)
        continue;
      if (Merit < BestMerit) {
        BestClass = ScheduleSpecification->Base.Classes[C];
        BestMerit = Merit;
        Candidate.clear();
        Candidate.push_back(C);
      } else {
        if (BestClass != ScheduleSpecification->Base.Classes[C])
          continue;
        Candidate.push_back(C);
      }
    }
    int NumCandidate = Candidate.size();
    // sort all candidate by size?
    struct SortCand {
      int Index;
      int Size;
      bool operator<(const SortCand &a) const { return Size < a.Size; }
    };
    std::vector<SortCand> SC(NumCandidate);
    for (int I = 0; I < NumCandidate; I++) {
      SC[I].Index = Candidate[I];
      SC[I].Size = Issue[Candidate[I]].size();
    }
    std::stable_sort(SC.begin(), SC.end());
    for (int I = 0; I < NumCandidate; I++)
      Candidate[I] = SC[I].Index;
    int State = 0;
    int PCycle = VCycle % CurII;
    for (int i = 0; i < Size; i++) {
      if (!Issue[i].assigned())
        continue;
      if (Issue[i].val() % CurII == PCycle)
        State = DFA0->reserveResources(State,
                                       ScheduleSpecification->Base.Classes[i]);
    }
    assert(DFA0->isLegal(State));
    int RState = State;
    int Max = NumCandidate;
    for (int i = 0; i < NumCandidate; i++) {
      RState = DFA0->reserveResources(RState, BestClass);
      if (!DFA0->isLegal(RState)) {
        Max = i;
        break;
      }
    }
    // assert(Max > 0);
    // TODO, more proper min and med
    int Min = 0;
    int Med = (Max + Min + 1) / 2;
    return new ResourceChoice(*this, VCycle, NumCandidate, Max, Min, Med,
                              Candidate);
  }

  virtual Choice *choice(const Space &home, Archive &e) {
    int VCycle, NumCandidate, Max, Min, Med;
    e >> VCycle >> NumCandidate >> Max >> Min >> Med;
    std::vector<int> Candidate(NumCandidate);
    for (int I = 0; I < NumCandidate; I++)
      e >> Candidate[I];
    return new ResourceChoice(*this, VCycle, NumCandidate, Max, Min, Med,
                              Candidate);
  }

  int numAssigned(int Max, int Min, int Med, unsigned alt) const {
    int Alt = alt;
    int NumA;
    int Even = std::min(Max - Med, Med - Min);
    if (Alt < Even * 2 + 1) {
      if ((Alt & 1) == 0)
        NumA = Med - (Alt >> 1);
      else
        NumA = Med + ((Alt + 1) >> 1);
    } else {
      int P = Alt - Even * 2;
      if (Max - Med > Med - Min) {
        NumA = Med + P;
      } else {
        NumA = Med - P;
      }
    }
    assert(NumA <= Max && NumA >= Min);
    return NumA;
  }

  virtual ExecStatus commit(Space &home, const Choice &c, unsigned int alt) {
    const ResourceChoice &C = static_cast<const ResourceChoice &>(c);
    int NumA = numAssigned(C.Max, C.Min, C.Med, alt);
    int I;
    for (I = 0; I < NumA; I++)
      if (me_failed(Issue[C.Candidate[I]].eq(home, C.Cycle)))
        return ES_FAILED;
    for (; I < C.NumCandidate; I++)
      if (me_failed(Issue[C.Candidate[I]].nq(home, C.Cycle)))
        return ES_FAILED;
    return ES_OK;
  }
  virtual void print(const Space &home, const Choice &c, unsigned int alt,
                     std::ostream &o) const {
    const ResourceChoice &C = static_cast<const ResourceChoice &>(c);
    int NumA = numAssigned(C.Max, C.Min, C.Med, alt);
    int I;
    for (I = 0; I < NumA; I++)
      o << "Issue[" << C.Candidate[I] << "]=" << C.Cycle;
    for (; I < C.NumCandidate; I++)
      o << "Issue[" << C.Candidate[I] << "]!=" << C.Cycle;
  }
  virtual Brancher *copy(Space &home) {
    return new (home) XtensaResourceBrancher(home, *this);
  }
  static void post(Space &home, ViewArray<Int::IntView> &Issue, Int::IntView II,
                   Int::IntView Stage,
                   const XtensaScheduleEngine *ScheduleSpecification) {
    (void)new (home)
        XtensaResourceBrancher(home, Issue, II, Stage, ScheduleSpecification);
  }
  virtual size_t dispose(Space &home) {
    home.ignore(*this, AP_DISPOSE);
    return sizeof(*this);
  }
};

void xtensa_ii_stage_branch(Home home, const IntVar &length, IntVar &stage,
                            const XtensaScheduleEngine *ScheduleSpecification);
void xtensa_schedule_branch(Home home, const IntVarArgs &issue,
                            const IntVar &length,
                            const XtensaScheduleEngine *ScheduleSpecification);
void xtensa_cyclic_branch(Home home, const IntVarArgs &vissue,
                          const IntVar &vlength, Branch_Direction dir,
                          int strategy,
                          const XtensaScheduleEngine *ScheduleSpecification);
void xtensa_semi_dynamic_cyclic_branch(
    Home home, const IntVarArgs &vissue, const IntVar &vlength, bool reduce,
    const XtensaScheduleEngine *ScheduleSpecification);
void xtensa_acyclic_branch(Home home, const IntVarArgs &issue,
                           const IntVar &length, Branch_Direction dir,
                           const XtensaScheduleEngine *ScheduleSpecification);
void xtensa_bundle_constraint(Home home, const IntVarArgs &issue,
                              IntVar &length,
                              const Tensilica::DFAPacketizer *DFA0,
                              const XtensaScheduleEngine *ScheduleSpecification,
                              IntPropLevel icl = IPL_VAL);
} // namespace Gecode

#endif
