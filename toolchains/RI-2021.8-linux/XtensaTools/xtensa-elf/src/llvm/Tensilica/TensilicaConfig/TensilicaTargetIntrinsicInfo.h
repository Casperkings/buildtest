//===-- TensilicaTargetIntrinsicInfo.h   - Target IntrinsicInfo -*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains target-specific intrinsic information implementation,
// shared between different Tensilica configurable targets. Xtensa and RISCV
// both use this file to prevent code duplication.
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;
using namespace llvm::TargetIntrinsic;

static const char *const IntrinsicNameTable[] = {
#define GET_INTRINSIC_NAME_TABLE
#include "TensilicaGenIntrinsicImpl.inc"
#undef GET_INTRINSIC_NAME_TABLE
};

static FrontendBuiltinInfo FrontendBuiltinInfos[] = {
#define BUILTIN(ID, TYPE, ATTRS, HEADERFILE)                                   \
  {#ID, TYPE, ATTRS, HEADERFILE, ALL_LANGUAGES, nullptr},
#define GET_BUILTIN_INFO
#include "BuiltinsTensilica.def"
#undef GET_BUILTIN_INFO
};

typedef struct {
  unsigned ImmBID;
  unsigned RegBID;
  unsigned ArgDiffIdx;
} BuiltinImmToRegInfoT;

static BuiltinImmToRegInfoT BuiltinImmToRegInfo[] = {
#define GET_BUILTIN_IMM_TO_REG
#define BUILTIN_IMM_TO_REG(IMM_FORM, REG_FORM, ARG_IDX)                        \
  {IMM_FORM, REG_FORM, ARG_IDX},
#include "BuiltinsTensilica.def"
#undef GET_BUILTIN_IMM_TO_REG
    {0, 0, 0}};

static unsigned NumBuiltinImmToRegInfo =
    sizeof(BuiltinImmToRegInfo) / sizeof(BuiltinImmToRegInfoT) - 1;

Tensilica::IntrinsicInfo::IntrinsicInfo() : TargetIntrinsicInfo() {}

unsigned Tensilica::IntrinsicInfo::getNumOfFrontendBuiltins() const {
  return sizeof(FrontendBuiltinInfos) / sizeof(FrontendBuiltinInfo);
}

const FrontendBuiltinInfo *
Tensilica::IntrinsicInfo::getFrontendBuiltinInfo() const {
  return FrontendBuiltinInfos;
}

const TargetBuiltinImmArgInfoVT *
Tensilica::IntrinsicInfo::getFrontendBuiltinImmArgInfo(
    unsigned BuiltinID) const {
  static TargetBuiltinCustomCheckInfo BuiltinCustomCheckInfos = {
#define GET_BUILTIN_IMM_RANGE
#include "BuiltinsTensilica.def"
#undef GET_BUILTIN_IMM_RANGE
  };

  const auto &it = BuiltinCustomCheckInfos.find(BuiltinID);
  if (it != BuiltinCustomCheckInfos.end()) {
    return &(it->second);
  } else
    return nullptr;
}

unsigned Tensilica::IntrinsicInfo::getRegFormBuiltinID(unsigned BID,
                                                       int &ArgDiffIdx) const {
  ArgDiffIdx = -1;
  if (!NumBuiltinImmToRegInfo)
    return 0;

  ArrayRef<BuiltinImmToRegInfoT> ImmToRegInfo(BuiltinImmToRegInfo,
                                              NumBuiltinImmToRegInfo);
  auto *Iter =
      std::lower_bound(ImmToRegInfo.begin(), ImmToRegInfo.end(), BID,
                       [](const BuiltinImmToRegInfoT &Info,
                          const unsigned BID) { return Info.ImmBID < BID; });
  if (Iter != ImmToRegInfo.end() && Iter->ImmBID == BID) {
    ArgDiffIdx = Iter->ArgDiffIdx;
    return Iter->RegBID;
  }
  return 0;
}

std::string Tensilica::IntrinsicInfo::getName(unsigned IntrID, Type **Tys,
                                              unsigned numTys) const {
  if (IntrID < Intrinsic::num_intrinsics)
    return nullptr;
  assert(IntrID < TensilicaIntrinsic::num_tensilica_intrinsics &&
         "Invalid intrinsic ID");
  return IntrinsicNameTable[IntrID - Intrinsic::num_intrinsics];
}

unsigned Tensilica::IntrinsicInfo::lookupName(const char *Name,
                                              unsigned Len) const {
  StringRef NameRef(Name, Len);
  if (!NameRef.startswith("llvm."))
    return 0; // All intrinsics start with 'llvm.'
  ArrayRef<const char *> NameTable(IntrinsicNameTable);
  int Idx = Intrinsic::lookupLLVMIntrinsicByName(NameTable, NameRef);
  if (Idx == -1)
    return Intrinsic::not_intrinsic;
  return static_cast<Intrinsic::ID>(Idx + Intrinsic::num_intrinsics);
}

bool Tensilica::IntrinsicInfo::isOverloaded(unsigned id) const {
// Overload Table
#define GET_INTRINSIC_OVERLOAD_TABLE
#include "TensilicaGenIntrinsicImpl.inc"
#undef GET_INTRINSIC_OVERLOAD_TABLE
}

Function *Tensilica::IntrinsicInfo::getDeclaration(Module *M, unsigned IntrID,
                                                   Type **Tys,
                                                   unsigned numTys) const {
  llvm_unreachable("Not implemented");
}

const Tensilica::IntrnProperty *
Tensilica::IntrinsicInfo::getIntrnProperty(unsigned ID) const {
  static const Tensilica::IntrnProperty IntrnProps[] = {
      {Intrinsic::not_intrinsic, false, {}}
#define GET_INTRINSIC_PROPERTY
#include "TensilicaGenIntrnProperty.inc"
  };

  // Return the Tensilica::IntrnProperty from the above generated table given
  // the intrinsic id. Expects the above table to be sorted in increasing
  // order of the ids (or equivalently the sorted order of intrinsic names)
  Tensilica::IntrnProperty IntrinsicToFind = {ID, false, {}};
  const Tensilica::IntrnProperty *Prop = std::lower_bound(
      std::begin(IntrnProps), std::end(IntrnProps), IntrinsicToFind);
  if (Prop != std::end(IntrnProps) && *Prop == IntrinsicToFind)
    return Prop;

  return &IntrnProps[0];
}

