//===- RDFBasePtr.h -------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// RDF-graph based detecting pointer difference.
//

#ifndef RDF_BASEPTR_H
#define RDF_BASEPTR_H

#include "llvm/CodeGen/RDFGraph.h"
#include "llvm/CodeGen/RDFLiveness.h"
#include "llvm/CodeGen/MachineDominators.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/ADT/SetVector.h"

namespace llvm {

namespace rdf {

struct EqNode {
  NodeId Id = 0;
  int64_t Offset = 0;
};

struct BaseEquivalence {
  BaseEquivalence(DataFlowGraph &dfg)
      : MDT(dfg.getDT()), DFG(dfg), L(dfg.getMF().getRegInfo(), dfg) {}

  void trace(bool On) { Trace = On; }
  bool trace() const { return Trace; }
  DataFlowGraph &getDFG() { return DFG; }

  using EqMapping = std::unordered_map<NodeId, EqNode>;

  // Main interface, it is lazy and calculates root if necessary on the fly.
  EqNode getRoot(NodeId Id);

  // Helper routines
  EqNode getPred(NodeId Id);
  NodeId getReachingDefForUse(NodeId UseId);
  bool evalConstForUse(NodeId UseId, int64_t &Val);
  bool isKnownPhysReg(RegisterRef R) const;

protected:
  const MachineDominatorTree &MDT;
  DataFlowGraph &DFG;
  Liveness L;
  bool Trace = false;

private:
  EqMapping EqMap;
};

} // namespace rdf
} // namespace llvm

#endif
