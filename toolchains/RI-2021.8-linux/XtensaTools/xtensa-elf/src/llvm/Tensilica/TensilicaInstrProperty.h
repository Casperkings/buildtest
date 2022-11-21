//===-- TensilicaInstrProperty.h - Tensilica instruction properties -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the interfaces that Xtensa uses to query about the
// custom/TIE instructions
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_CODEGEN_TENSILICA_TENSILICAINSTRPROPERTY_H
#define LLVM_LIB_CODEGEN_TENSILICA_TENSILICAINSTRPROPERTY_H

#define XTENSA_INSTR_IS_LOAD_STORE true
#define XTENSA_INSTR_COMMUTATIVE true
#define XTENSA_INSTR_EXCEPTION true
#define XTENSA_INSTR_HAS_TIE_PORT true
#define XTENSA_INSTR_IS_SCATTER_GATHER true
#define XTENSA_SCATTER_GATHER_HAS_LATENCY true
#define XTENSA_INSTR_HAS_LATENCY_HINT true

#include "Tensilica/TensilicaInstrImmRange.h"
#include "llvm/Support/ErrorHandling.h"
#include <cassert>
#include <map>

namespace llvm {

enum XtensaLdStKind {
  XTENSA_INSTR_UNKNOWN_ADDR_MODE,
  XTENSA_INSTR_BASE_DISP_IMM,
  XTENSA_INSTR_BASE_DISP_REG,
  XTENSA_INSTR_POST_INC_IMM,
  XTENSA_INSTR_POST_INC_REG,
  XTENSA_INSTR_PRE_INC_IMM,
  XTENSA_INSTR_PRE_INC_REG,
  XTENSA_INSTR_ALIGNING,
  XTENSA_INSTR_ALIGNING_POS,
  XTENSA_INSTR_ALIGNING_POS_VAR
};

enum XtensaScatterGatherKind {
  XTENSA_INSTR_SCATTER,
  XTENSA_INSTR_SCATTER_W,
  XTENSA_INSTR_GATHER_A,
  XTENSA_INSTR_GATHER_D
};

enum XtensaAssociativeFlags {
  XTENSA_REASSOC_NONE = 0,
  // reassociation is allowed only with unsafe fp math enabled
  XTENSA_REASSOC_FASTMATH = (1 << 0),
  // default reassociation kind (op0 is def, op1 and op2 are uses,
  // all instructions have the same opcode)
  XTENSA_REASSOC_DEFAULT = (1 << 1),
  // In special cases we may encode reassociated operands and require
  // other defs to be dead
  XTENSA_REASSOC_SPECIAL = (1 << 2),
};

enum {
  XTENSA_REASSOC_OP0_OFFSET = 4,
  XTENSA_REASSOC_OP1_OFFSET = 8,
  XTENSA_REASSOC_OP2_OFFSET = 12,
  XTENSA_REASSOC_OPS_MASK = 0xf,
};

enum XtensaPartRedType {
  XTENSA_PART_RED_NONE = 0,
  XTENSA_PART_RED_ADD = (1 << 0),
  XTENSA_PART_RED_SUB = (1 << 1),
  XTENSA_PART_RED_MINMAX = (1 << 2),
  XTENSA_PART_RED_MADD = (1 << 3),
  XTENSA_PART_RED_FASTMATH = (1 << 4),
};

// Struct to represent ld/st kinds and its operands
struct XtensaLoadStoreProperty {
  int Kind;
  int ValueOpnd;
  int BaseDefOpnd;
  int BaseUseOpnd;
  int OffsetOpnd;
  int FixedIncAmount;
};

namespace Tensilica {
// Structure to represent properties of an Xtensa Custom TIE instruction
struct InstrProperty {
  unsigned Opcode;
  bool IsLoadStore;
  bool IsScatterGather;
  bool CanRaiseException;
  bool HasTIEPort;
  bool IsCommutative;
  unsigned CommutedOpcode;
  // Note: associative instruction cannot have a commuted opcode.
  // So, for non-default associative instruction CommutedOpcode may be opcode
  // which may be reassociated with current opcode as root (e.g., for madd
  // instruction, CommutedOpcode will be mul).
  unsigned AssociativeFlags;

  unsigned PartRedType;
  unsigned ZeroOpcode;
  unsigned AddOpcode;

  bool HasLatencyHint;
  int DelayOpnd;
  int SGKind;

  // Opcode specific properties
  union {
    struct XtensaLoadStoreProperty LdSt;
  } Prop;

  typedef const XtensaImmRangeSet &(*XtensaGetImmRangeSetFuncPtr)(unsigned);
  XtensaGetImmRangeSetFuncPtr GetImmRangeFunc;

  bool isUnknownAddrMode() const {
    return Prop.LdSt.Kind == XtensaLdStKind::XTENSA_INSTR_UNKNOWN_ADDR_MODE;
  }

  bool isCommutative() const { return IsCommutative; }

  bool isAssociative(bool FastMath) const {
    if (!(AssociativeFlags & XtensaAssociativeFlags::XTENSA_REASSOC_DEFAULT))
      return false;
    assert(
        !(AssociativeFlags & XtensaAssociativeFlags::XTENSA_REASSOC_SPECIAL) &&
        "Both reassociation kinds set");
    return (FastMath || !(AssociativeFlags &
                          XtensaAssociativeFlags::XTENSA_REASSOC_FASTMATH));
  }

  bool isSpecialAssociative(bool FastMath, unsigned &AssocOpcode) const {
    if (!(AssociativeFlags & XtensaAssociativeFlags::XTENSA_REASSOC_SPECIAL))
      return false;
    assert(
        !(AssociativeFlags & XtensaAssociativeFlags::XTENSA_REASSOC_DEFAULT) &&
        "Both reassociation kinds set");
    AssocOpcode = CommutedOpcode;
    return (FastMath || !(AssociativeFlags &
                          XtensaAssociativeFlags::XTENSA_REASSOC_FASTMATH));
  }

  void getSpecialReassociableOperands(unsigned &IdxOp0, unsigned &IdxOp1,
                                      unsigned &IdxOp2) const {
    if (AssociativeFlags & XtensaAssociativeFlags::XTENSA_REASSOC_DEFAULT) {
      IdxOp0 = 0;
      IdxOp1 = 1;
      IdxOp2 = 2;
    } else if (AssociativeFlags &
               XtensaAssociativeFlags::XTENSA_REASSOC_SPECIAL) {
      IdxOp0 = (AssociativeFlags >> XTENSA_REASSOC_OP0_OFFSET) &
               XTENSA_REASSOC_OPS_MASK;
      IdxOp1 = (AssociativeFlags >> XTENSA_REASSOC_OP1_OFFSET) &
               XTENSA_REASSOC_OPS_MASK;
      IdxOp2 = (AssociativeFlags >> XTENSA_REASSOC_OP2_OFFSET) &
               XTENSA_REASSOC_OPS_MASK;
    } else
      llvm_unreachable("Unknown reassociable ops");
  }

  bool isPartRed(bool FastMath) const {
    if (!(PartRedType & (XtensaPartRedType::XTENSA_PART_RED_ADD |
                         XtensaPartRedType::XTENSA_PART_RED_SUB |
                         XtensaPartRedType::XTENSA_PART_RED_MADD |
                         XtensaPartRedType::XTENSA_PART_RED_MINMAX)))
      return false;
    return (FastMath ||
            !(PartRedType & XtensaPartRedType::XTENSA_PART_RED_FASTMATH));
  }

  bool isPartRedMinMax(bool FastMath) const {
    if (!(PartRedType & XtensaPartRedType::XTENSA_PART_RED_MINMAX))
      return false;
    assert(!(PartRedType & (XtensaPartRedType::XTENSA_PART_RED_ADD |
                            XtensaPartRedType::XTENSA_PART_RED_SUB |
                            XtensaPartRedType::XTENSA_PART_RED_MADD)) &&
           "Incompatible partial sum types");
    return (FastMath ||
            !(PartRedType & XtensaPartRedType::XTENSA_PART_RED_FASTMATH));
  }

  bool isPartRedAdd(bool FastMath) const {
    if (!(PartRedType & XtensaPartRedType::XTENSA_PART_RED_ADD))
      return false;
    assert(!(PartRedType & (XtensaPartRedType::XTENSA_PART_RED_MINMAX |
                            XtensaPartRedType::XTENSA_PART_RED_SUB |
                            XtensaPartRedType::XTENSA_PART_RED_MADD)) &&
           "Incompatible partial sum types");
    return (FastMath ||
            !(PartRedType & XtensaPartRedType::XTENSA_PART_RED_FASTMATH));
  }

  bool isPartRedSub(bool FastMath) const {
    if (!(PartRedType & XtensaPartRedType::XTENSA_PART_RED_SUB))
      return false;
    assert(!(PartRedType & (XtensaPartRedType::XTENSA_PART_RED_MINMAX |
                            XtensaPartRedType::XTENSA_PART_RED_ADD |
                            XtensaPartRedType::XTENSA_PART_RED_MADD)) &&
           "Incompatible partial sum types");
    return (FastMath ||
            !(PartRedType & XtensaPartRedType::XTENSA_PART_RED_FASTMATH));
  }

  bool isPartRedMadd(bool FastMath) const {
    if (!(PartRedType & XtensaPartRedType::XTENSA_PART_RED_MADD))
      return false;
    assert(!(PartRedType & (XtensaPartRedType::XTENSA_PART_RED_ADD |
                            XtensaPartRedType::XTENSA_PART_RED_SUB |
                            XtensaPartRedType::XTENSA_PART_RED_MINMAX)) &&
           "Incompatible partial sum types");
    return (FastMath ||
            !(PartRedType & XtensaPartRedType::XTENSA_PART_RED_FASTMATH));
  }

  unsigned getPartRedZeroOpcode() const { return ZeroOpcode; }

  unsigned getPartRedAddOpcode() const { return AddOpcode; }

  bool canRaiseException() const { return CanRaiseException; }

  bool hasTIEPort() const { return HasTIEPort; }

  unsigned getCommutedOpcode() const { return CommutedOpcode; }

  bool isLoadStore() const { return IsLoadStore; }

  bool isScatterGather() const { return IsScatterGather; }

  bool isNonSGLoadStore() const { return isLoadStore() && !isScatterGather(); }

  bool hasLatencyHint() const { return HasLatencyHint; }

  int getDelayOpnd() const {
    assert(HasLatencyHint);
    return DelayOpnd;
  }

  bool isBaseDisp() const {
    return (isNonSGLoadStore() &&
            (Prop.LdSt.Kind == XtensaLdStKind::XTENSA_INSTR_BASE_DISP_IMM ||
             Prop.LdSt.Kind == XtensaLdStKind::XTENSA_INSTR_BASE_DISP_REG));
  }

  bool isBaseDispImm() const {
    return (isNonSGLoadStore() &&
            Prop.LdSt.Kind == XtensaLdStKind::XTENSA_INSTR_BASE_DISP_IMM);
  }

  bool isBaseDispReg() const {
    return (isNonSGLoadStore() &&
            Prop.LdSt.Kind == XtensaLdStKind::XTENSA_INSTR_BASE_DISP_REG);
  }

  bool isPostInc() const {
    return (isNonSGLoadStore() &&
            (Prop.LdSt.Kind == XtensaLdStKind::XTENSA_INSTR_POST_INC_IMM ||
             Prop.LdSt.Kind == XtensaLdStKind::XTENSA_INSTR_POST_INC_REG));
  }

  bool isPostIncImm() const {
    return (isNonSGLoadStore() &&
            Prop.LdSt.Kind == XtensaLdStKind::XTENSA_INSTR_POST_INC_IMM);
  }

  bool isPostIncReg() const {
    return (isNonSGLoadStore() &&
            Prop.LdSt.Kind == XtensaLdStKind::XTENSA_INSTR_POST_INC_REG);
  }

  bool isPreInc() const {
    return (isNonSGLoadStore() &&
            (Prop.LdSt.Kind == XtensaLdStKind::XTENSA_INSTR_PRE_INC_IMM ||
             Prop.LdSt.Kind == XtensaLdStKind::XTENSA_INSTR_PRE_INC_REG));
  }

  bool isPreIncImm() const {
    return (isNonSGLoadStore() &&
            Prop.LdSt.Kind == XtensaLdStKind::XTENSA_INSTR_PRE_INC_IMM);
  }

  bool isPreIncReg() const {
    return (isNonSGLoadStore() &&
            Prop.LdSt.Kind == XtensaLdStKind::XTENSA_INSTR_PRE_INC_REG);
  }

  bool isAligning() const {
    return (isNonSGLoadStore() &&
            Prop.LdSt.Kind == XtensaLdStKind::XTENSA_INSTR_ALIGNING);
  }

  bool isAligningPos() const {
    return (isNonSGLoadStore() &&
            Prop.LdSt.Kind == XtensaLdStKind::XTENSA_INSTR_ALIGNING_POS);
  }

  bool isAligningPosVar() const {
    return (isNonSGLoadStore() &&
            Prop.LdSt.Kind == XtensaLdStKind::XTENSA_INSTR_ALIGNING_POS_VAR);
  }

  bool isAnyAligning() const {
    return isAligning() || isAligningPos() || isAligningPosVar();
  }

  unsigned getValueOpnd() const {
    assert(isBaseDisp() || isPostInc() || isPreInc());
    return Prop.LdSt.ValueOpnd;
  }

  unsigned getBaseOpnd() const {
    assert(isBaseDisp() || isPostInc() || isPreInc() || isAnyAligning());
    return Prop.LdSt.BaseUseOpnd;
  }

  unsigned getBaseDefOpnd() const {
    assert(isPostInc() || isPreInc());
    return Prop.LdSt.BaseDefOpnd;
  }

  unsigned getOffsetOpnd() const {
    assert(isBaseDisp() || isPostInc() || isPreInc());
    return Prop.LdSt.OffsetOpnd;
  }

  bool isScatter() const {
    return (isScatterGather() &&
            SGKind == XtensaScatterGatherKind::XTENSA_INSTR_SCATTER);
  }

  bool isScatterW() const {
    return (isScatterGather() &&
            SGKind == XtensaScatterGatherKind::XTENSA_INSTR_SCATTER_W);
  }

  bool isGatherA() const {
    return (isScatterGather() &&
            SGKind == XtensaScatterGatherKind::XTENSA_INSTR_GATHER_A);
  }

  bool isGatherD() const {
    return (isScatterGather() &&
            SGKind == XtensaScatterGatherKind::XTENSA_INSTR_GATHER_D);
  }

  bool hasImmRange(unsigned Opnd) const {
    if (!GetImmRangeFunc)
      return false;

    const XtensaImmRangeSet &Set = getImmRange(Opnd);
    return !Set.isEmpty();
  }

  const XtensaImmRangeSet &getImmRange(unsigned Opnd) const {
    if (GetImmRangeFunc)
      return (*GetImmRangeFunc)(Opnd);

    static XtensaImmRangeSet EmptySet = {{}};
    return EmptySet;
  }

  bool operator<(const Tensilica::InstrProperty &RHS) const {
    return Opcode < RHS.Opcode;
  }

  bool operator==(const Tensilica::InstrProperty &RHS) const {
    return RHS.Opcode == Opcode;
  }
};

// Enum to get some special opcodes, common for Xtensa and RISCV.
enum class MapOpcode {
  NONE,
#define TENSILICA_OPCODE
#include "Tensilica/OpcodeMap.h"
  // Add instruction before this line
  MOVDA32X2,
  MOVAD32_L,
  MOVAD32_H,
};
} // namespace Tensilica
} // namespace llvm

#endif // LLVM_LIB_CODEGEN_TENSILICA_TENSILICAINSTRPROPERTY_H
