//===-- TensilicaInstrImmRange.h - Tensilica instruction immediate range --===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the interfaces that Xtensa uses to query about the
// custom/TIE instructions immediate range
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_CODEGEN_TENSILICA_TENSILICAINSTRIMMRANGE_H
#define LLVM_LIB_CODEGEN_TENSILICA_TENSILICAINSTRIMMRANGE_H

#include <cstdint>
#include <llvm/ADT/ArrayRef.h>
#include <utility>
#include <vector>

namespace llvm {

// An immediate range is represented as a range [LowerBound, UpperBound] and
// a step.
struct XtensaImmRange {
  int32_t LowerBound;
  int32_t UpperBound;
  int32_t Step;

  int32_t getLowerBound() const { return LowerBound; }

  int32_t getUpperBound() const { return UpperBound; }

  int32_t getStep() const { return Step; }

  // Is Val a member of this ImmRange
  bool contains(int32_t Val) const {
    if (LowerBound > Val || UpperBound < Val)
      return false;
    if ((Val - LowerBound) % Step)
      return false;
    return true;
  }

  // Is this ImmRange a subset of Other
  bool subset(const XtensaImmRange &Other) const {
    if (LowerBound < Other.LowerBound || UpperBound > Other.UpperBound)
      return false;
    if (!(Step % Other.Step) && !((LowerBound - Other.LowerBound) % Other.Step))
      return true;
    // Iterate through all elements in the range
    for (int32_t Elem = LowerBound; Elem <= UpperBound; Elem += Step) {
      if (!Other.contains(Elem))
        return false;
    }
    return true;
  }
};

// The valid immediates for an operand are maintained as a set of
// immediate ranges.
struct XtensaImmRangeSet {
  const ArrayRef<XtensaImmRange> ImmRanges;

  bool isEmpty() const { return ImmRanges.size() == 0; }

  // Is Val in this XtensaImmRangeSet
  bool contains(int32_t Val) const {
    if (isEmpty())
      return false;

    for (const XtensaImmRange &IR : ImmRanges) {
      if (IR.contains(Val))
        return true;
    }
    return false;
  }

  // Is this XtensaImmRangeSet a subset of Other
  bool subset(const XtensaImmRangeSet &Other) const {
    if (isEmpty())
      return false;

    // Iterate through every range in this set
    for (const XtensaImmRange &IR : ImmRanges) {
      if (!Other.subset(IR))
        return false;
    }
    return true;
  }

  // return the lower and upper bound if there is only one range
  std::pair<int32_t, int32_t> getBounds() const {
    return (ImmRanges.size() == 1)
               ? std::make_pair(ImmRanges.front().LowerBound,
                                ImmRanges.front().UpperBound)
               : std::make_pair(-1, -2);
  }

private:
  // Is ImmRange IR a subset of this ImmRangeSet
  bool subset(const XtensaImmRange &IR) const {
    if (isEmpty())
      return false;

    // If this ImmRangeSet has only 1 range do a direct subset check
    if (ImmRanges.size() == 1)
      return IR.subset(ImmRanges[0]);
    // Iterate through all elements in the range
    for (int32_t Elem = IR.LowerBound; Elem <= IR.UpperBound; Elem += IR.Step) {
      if (!contains(Elem))
        return false;
    }
    return true;
  }
};
} // namespace llvm

#endif // LLVM_LIB_CODEGEN_TENSILICA_TENSILICAINSTRIMMRANGE_H
