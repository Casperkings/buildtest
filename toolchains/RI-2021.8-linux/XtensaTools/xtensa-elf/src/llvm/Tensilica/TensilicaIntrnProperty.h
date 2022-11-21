//===-- TensilicaIntrnProperty.h - Xtensa Intrinsic Property Interface ----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the interfaces that Xtensa uses to return IntrinsicInfo
// for a target intrinsic
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_CODEGEN_TENSILICA_TENSILICAINTRNPROPERTY_H
#define LLVM_LIB_CODEGEN_TENSILICA_TENSILICAINTRNPROPERTY_H

#include "llvm/CodeGen/ValueTypes.h"

#define XTENSA_INTRN_LOAD true
#define XTENSA_INTRN_STORE true

namespace llvm {

enum XtensaMemIntrnKind {
  XTENSA_INTRN_BASE_DISP_IMM, // eg: IVP_LVNX16_I
  XTENSA_INTRN_BASE_DISP_REG, // eg: IVP_LVNX16_X
  XTENSA_INTRN_POST_INC_IMM,  // eg: IVP_LVNX16_IP
  XTENSA_INTRN_POST_INC_REG,  // eg: IVP_LVNX16_XP
  XTENSA_INTRN_PRE_INC_IMM,   // eg: AE_L16M.IU
  XTENSA_INTRN_PRE_INC_REG,   // eg: AE_L16M.XU
  XTENSA_INTRN_SCATTER,
  XTENSA_INTRN_GATHER_A,
  XTENSA_INTRN_ALIGNING,           // All aligning load/stores incl. prime/flush
  XTENSA_INTRN_ALIGNING_POS_VAR,   // eg: IVP_LAVN_2X16S_XP
  XTENSA_INTRN_ALIGNING_POS_FIXED, // eg: IVP_LAN_2X16S_IP
  XTENSA_INTRN_OTHER,
};

// Struct to represent operands of the memory intrinsic
struct XtensaMemIntrnOpnd {
  int BaseDefOpnd;
  int BaseUseOpnd;
  int OffsetOpnd;
};

// Structure to represent properties of a Xtensa memory intrinsic
struct XtensaMemIntrnProperty {
  bool IsLoad;
  bool IsStore;
  EVT MemVT;
  unsigned Size;  // the size of the memory location
  unsigned Align; // alignment
  XtensaMemIntrnKind Kind;
  struct XtensaMemIntrnOpnd Opnd;
};

namespace Tensilica {
struct IntrnProperty {
  unsigned Id; // Intrinsic ID
  bool IsMemIntrn;
  union {
    struct XtensaMemIntrnProperty Mem;
  } Prop;

  bool isMemIntrn() const { return IsMemIntrn; }

  EVT MemVT() const {
    assert(IsMemIntrn);
    return Prop.Mem.MemVT;
  }

  unsigned accessSize() const {
    assert(IsMemIntrn);
    return Prop.Mem.Size;
  }

  unsigned alignment() const {
    assert(IsMemIntrn);
    return Prop.Mem.Align;
  }

  bool isLoad() const {
    assert(IsMemIntrn);
    return Prop.Mem.IsLoad;
  }

  bool isStore() const {
    assert(IsMemIntrn);
    return Prop.Mem.IsStore;
  }

  bool isBaseDisp() const {
    assert(IsMemIntrn);
    return Prop.Mem.Kind == XtensaMemIntrnKind::XTENSA_INTRN_BASE_DISP_IMM ||
           Prop.Mem.Kind == XtensaMemIntrnKind::XTENSA_INTRN_BASE_DISP_REG;
  }

  bool isBaseDispImm() const {
    assert(IsMemIntrn);
    return Prop.Mem.Kind == XtensaMemIntrnKind::XTENSA_INTRN_BASE_DISP_IMM;
  }

  bool isBaseDispReg() const {
    assert(IsMemIntrn);
    return Prop.Mem.Kind == XtensaMemIntrnKind::XTENSA_INTRN_BASE_DISP_REG;
  }

  bool isPostInc() const {
    assert(IsMemIntrn);
    return Prop.Mem.Kind == XtensaMemIntrnKind::XTENSA_INTRN_POST_INC_IMM ||
           Prop.Mem.Kind == XtensaMemIntrnKind::XTENSA_INTRN_POST_INC_REG ||
           Prop.Mem.Kind == XtensaMemIntrnKind::XTENSA_INTRN_ALIGNING ||
           Prop.Mem.Kind ==
               XtensaMemIntrnKind::XTENSA_INTRN_ALIGNING_POS_FIXED ||
           Prop.Mem.Kind == XtensaMemIntrnKind::XTENSA_INTRN_ALIGNING_POS_VAR;
  }

  bool isPostIncImm() const {
    assert(IsMemIntrn);
    return Prop.Mem.Kind == XtensaMemIntrnKind::XTENSA_INTRN_POST_INC_IMM;
  }

  bool isPostIncReg() const {
    assert(IsMemIntrn);
    return Prop.Mem.Kind == XtensaMemIntrnKind::XTENSA_INTRN_POST_INC_REG;
  }

  bool isPreInc() const {
    assert(IsMemIntrn);
    return Prop.Mem.Kind == XtensaMemIntrnKind::XTENSA_INTRN_PRE_INC_IMM ||
           Prop.Mem.Kind == XtensaMemIntrnKind::XTENSA_INTRN_PRE_INC_REG;
  }

  bool isPreIncImm() const {
    assert(IsMemIntrn);
    return Prop.Mem.Kind == XtensaMemIntrnKind::XTENSA_INTRN_PRE_INC_IMM;
  }

  bool isPreIncReg() const {
    assert(IsMemIntrn);
    return Prop.Mem.Kind == XtensaMemIntrnKind::XTENSA_INTRN_PRE_INC_REG;
  }

  bool isScatter() const {
    assert(IsMemIntrn);
    return Prop.Mem.Kind == XtensaMemIntrnKind::XTENSA_INTRN_SCATTER;
  }

  bool isGatherA() const {
    assert(IsMemIntrn);
    return Prop.Mem.Kind == XtensaMemIntrnKind::XTENSA_INTRN_GATHER_A;
  }

  bool isAligning() const {
    assert(IsMemIntrn);
    return Prop.Mem.Kind == XtensaMemIntrnKind::XTENSA_INTRN_ALIGNING ||
           Prop.Mem.Kind ==
               XtensaMemIntrnKind::XTENSA_INTRN_ALIGNING_POS_FIXED ||
           Prop.Mem.Kind == XtensaMemIntrnKind::XTENSA_INTRN_ALIGNING_POS_VAR;
  }

  int getBaseOpnd() const {
    assert(IsMemIntrn);
    return Prop.Mem.Opnd.BaseUseOpnd;
  }

  int getBaseDefOpnd() const {
    assert(IsMemIntrn);
    assert(Prop.Mem.Kind == XtensaMemIntrnKind::XTENSA_INTRN_PRE_INC_IMM ||
           Prop.Mem.Kind == XtensaMemIntrnKind::XTENSA_INTRN_PRE_INC_REG ||
           Prop.Mem.Kind == XtensaMemIntrnKind::XTENSA_INTRN_POST_INC_IMM ||
           Prop.Mem.Kind == XtensaMemIntrnKind::XTENSA_INTRN_POST_INC_REG);
    return Prop.Mem.Opnd.BaseDefOpnd;
  }

  bool hasOffsetOpnd() const {
    return IsMemIntrn &&
      (Prop.Mem.Kind == XtensaMemIntrnKind::XTENSA_INTRN_BASE_DISP_IMM ||
       Prop.Mem.Kind == XtensaMemIntrnKind::XTENSA_INTRN_BASE_DISP_REG ||
       Prop.Mem.Kind == XtensaMemIntrnKind::XTENSA_INTRN_PRE_INC_IMM ||
       Prop.Mem.Kind == XtensaMemIntrnKind::XTENSA_INTRN_PRE_INC_REG ||
       Prop.Mem.Kind == XtensaMemIntrnKind::XTENSA_INTRN_POST_INC_IMM ||
       Prop.Mem.Kind == XtensaMemIntrnKind::XTENSA_INTRN_POST_INC_REG);
  }

  int getOffsetOpnd() const {
    assert(IsMemIntrn);
    assert(Prop.Mem.Kind == XtensaMemIntrnKind::XTENSA_INTRN_BASE_DISP_IMM ||
           Prop.Mem.Kind == XtensaMemIntrnKind::XTENSA_INTRN_BASE_DISP_REG ||
           Prop.Mem.Kind == XtensaMemIntrnKind::XTENSA_INTRN_PRE_INC_IMM ||
           Prop.Mem.Kind == XtensaMemIntrnKind::XTENSA_INTRN_PRE_INC_REG ||
           Prop.Mem.Kind == XtensaMemIntrnKind::XTENSA_INTRN_POST_INC_IMM ||
           Prop.Mem.Kind == XtensaMemIntrnKind::XTENSA_INTRN_POST_INC_REG);
    return Prop.Mem.Opnd.OffsetOpnd;
  }

  bool operator<(const IntrnProperty &RHS) const { return Id < RHS.Id; }

  bool operator==(const IntrnProperty &RHS) const { return RHS.Id == Id; }
};
} // namespace Tensilica
} // namespace llvm

#endif // LLVM_LIB_CODEGEN_TENSILICA_TENSILICAINTRNPROPERTY_H
