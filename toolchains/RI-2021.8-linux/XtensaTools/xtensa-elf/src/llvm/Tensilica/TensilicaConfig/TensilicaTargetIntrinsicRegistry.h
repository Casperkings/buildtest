//===-- TensilicaTargetIntrinsicRegistry.h   - Target Info ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains target-specific intrinsic information functions that are
// shared between different Tensilica configurable targets. Xtensa and RISCV
// both use this file to prevent code duplication.
//
//===----------------------------------------------------------------------===//

#include "Tensilica/TensilicaIntrnProperty.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/TargetIntrinsicRegistry.h"
#include "llvm/IR/Intrinsics.h"

using namespace llvm;
using namespace llvm::Intrinsic;

static const char *const IntrinsicNameTable[] = {
#define GET_INTRINSIC_NAME_TABLE
#include "TensilicaGenIntrinsicImpl.inc"
#undef GET_INTRINSIC_NAME_TABLE
};

static const Tensilica::IntrnProperty IntrnProps[] = {
    {Intrinsic::not_intrinsic, false, {}}
#define GET_INTRINSIC_PROPERTY
#include "TensilicaGenIntrnProperty.inc"
};

static unsigned TensilicaLookupIntrinsicID(StringRef Name) {
  ArrayRef<const char *> NameTable(IntrinsicNameTable);
  int Idx = Intrinsic::lookupLLVMIntrinsicByName(NameTable, Name);
  if (Idx == -1)
    return Intrinsic::not_intrinsic;
  return static_cast<Intrinsic::ID>(Idx + Intrinsic::num_intrinsics);
}

static const char *TensilicaGetIntrinsicName(unsigned Id) {
  if (Id < Intrinsic::num_intrinsics)
    return nullptr;
  assert(Id < TensilicaIntrinsic::num_tensilica_intrinsics && 
         "Invalid intrinsic ID");
  return IntrinsicNameTable[Id - Intrinsic::num_intrinsics];
}

static bool TensilicaIsIncrIntrn(unsigned ID) {
  // Return the Tensilica::IntrnProperty from the above generated table given
  // the intrinsic id. Expects the above table to be sorted in increasing
  // order of the ids (or equivalently the sorted order of intrinsic names)
  if (ID < Intrinsic::num_intrinsics)
    return false;

  Tensilica::IntrnProperty IntrinsicToFind = {ID, false, {}};
  const Tensilica::IntrnProperty *Prop = std::lower_bound(
      std::begin(IntrnProps), std::end(IntrnProps), IntrinsicToFind);
  if (Prop != std::end(IntrnProps) && *Prop == IntrinsicToFind &&
      Prop->isMemIntrn())
    return Prop->isPostInc() || Prop->isPreInc();

  return false;
}

static bool TensilicaIsIncrRegIntrn(unsigned ID) {
  // Return the Tensilica::IntrnProperty from the above generated table given
  // the intrinsic id. Expects the above table to be sorted in increasing
  // order of the ids (or equivalently the sorted order of intrinsic names)
  if (ID < Intrinsic::num_intrinsics)
    return false;

  Tensilica::IntrnProperty IntrinsicToFind = {ID, false, {}};
  const Tensilica::IntrnProperty *Prop = std::lower_bound(
      std::begin(IntrnProps), std::end(IntrnProps), IntrinsicToFind);
  if (Prop != std::end(IntrnProps) && *Prop == IntrinsicToFind &&
      Prop->isMemIntrn())
    return Prop->isPostIncReg() || Prop->isPreIncReg();

  return false;
}

static bool TensilicaIsAligningIntrn(unsigned ID) {
  // Return the Tensilica::IntrnProperty from the above generated table given
  // the intrinsic id. Expects the above table to be sorted in increasing
  // order of the ids (or equivalently the sorted order of intrinsic names)
  if (ID < Intrinsic::num_intrinsics)
    return false;

  Tensilica::IntrnProperty IntrinsicToFind = {ID, false, {}};
  const Tensilica::IntrnProperty *Prop = std::lower_bound(
      std::begin(IntrnProps), std::end(IntrnProps), IntrinsicToFind);
  if (Prop != std::end(IntrnProps) && *Prop == IntrinsicToFind &&
      Prop->isMemIntrn())
    return Prop->isAligning();

  return false;
}

static int TensilicaGetIntrnOffsetOpnd(unsigned ID) {
  // Return the Tensilica::IntrnProperty from the above generated table given
  // the intrinsic id. Expects the above table to be sorted in increasing
  // order of the ids (or equivalently the sorted order of intrinsic names)
  if (ID < Intrinsic::num_intrinsics)
    return -1;

  Tensilica::IntrnProperty IntrinsicToFind = {ID, false, {}};
  const Tensilica::IntrnProperty *Prop = std::lower_bound(
      std::begin(IntrnProps), std::end(IntrnProps), IntrinsicToFind);
  if (Prop != std::end(IntrnProps) && *Prop == IntrinsicToFind &&
      Prop->hasOffsetOpnd())
    return Prop->getOffsetOpnd();

  return -1;
}

static int TensilicaGetIntrnBaseOpnd(unsigned ID) {
  // Return the Tensilica::IntrnProperty from the above generated table given
  // the intrinsic id. Expects the above table to be sorted in increasing
  // order of the ids (or equivalently the sorted order of intrinsic names)
  if (ID < Intrinsic::num_intrinsics)
    return -1;

  Tensilica::IntrnProperty IntrinsicToFind = {ID, false, {}};
  const Tensilica::IntrnProperty *Prop = std::lower_bound(
      std::begin(IntrnProps), std::end(IntrnProps), IntrinsicToFind);
  if (Prop != std::end(IntrnProps) && *Prop == IntrinsicToFind &&
      Prop->isMemIntrn())
    return Prop->getBaseOpnd();

  return -1;
}

#define GET_INTRINSIC_ATTRIBUTES
#include "TensilicaGenIntrinsicImpl.inc"
#undef GET_INTRINSIC_ATTRIBUTES

static AttributeList TensilicaGetIntrinsicAttributes(LLVMContext &C, 
                                                     unsigned Id) {
  return getAttributes(C, TensilicaIntrinsic::ID(Id));
}

#define GET_INTRINSIC_GENERATOR_GLOBAL
#include "TensilicaGenIntrinsicImpl.inc"
#undef GET_INTRINSIC_GENERATOR_GLOBAL

static void
TensilicaGetIntrinsicInfoTableEntries(const unsigned **IITable,
                                   const unsigned char **IILongEncTable,
                                   unsigned *IILongEncTableLen) {
  *IITable = IIT_Table;
  *IILongEncTable = IIT_LongEncodingTable;
  *IILongEncTableLen = sizeof(IIT_LongEncodingTable) / sizeof(char);
}

static APInt TensilicaGetIntrnInputInfo(unsigned ID, unsigned Operand) {
  if (ID < Intrinsic::num_intrinsics)
    return APInt(32, -1);

  static const std::map<unsigned, std::vector<unsigned>> DemandedBits = {
#define GET_INTRINSIC_INPUT_PROPERTY
#include "TensilicaGenIntrnProperty.inc"
  };
  auto &&Iter = DemandedBits.find(ID);
  if (Iter == DemandedBits.end())
    return APInt(32, -1);
  assert(Operand < Iter->second.size());
  return APInt(32, Iter->second[Operand]);
}

static unsigned TensilicaGetIntrnInputSignedness(unsigned ID, unsigned Operand) {
  if (ID < Intrinsic::num_intrinsics)
    return Intrinsic::Unknown;

  static const std::map<unsigned, std::vector<unsigned>> InputInfo = {
#define GET_INTRINSIC_INPUT_SIGN
#include "TensilicaGenIntrnProperty.inc"
  };
  auto &&Iter = InputInfo.find(ID);
  if (Iter == InputInfo.end())
    return Intrinsic::Unknown;

  assert(Operand < Iter->second.size());
  return Iter->second[Operand];
}

static std::pair<unsigned, APInt> TensilicaGetIntrnOutputInfo(unsigned ID,
                                                           unsigned Operand) {
  typedef std::pair<unsigned, APInt> RType;

  if (ID < Intrinsic::num_intrinsics)
    return RType(0, APInt(32, -1));

  static const std::map<unsigned, std::vector<std::pair<unsigned, unsigned>>>
      OutputInfo = {
#define GET_INTRINSIC_OUTPUT_PROPERTY
#include "TensilicaGenIntrnProperty.inc"
      };

  auto &&Iter = OutputInfo.find(ID);
  if (Iter == OutputInfo.end())
    return RType(0, APInt(32, -1));
  assert(Operand < Iter->second.size());
  auto &&Raw = Iter->second[Operand];
  return RType(Raw.first, APInt(32, Raw.second));
}
static unsigned TensilicaGetIntrnOutputSignedness(unsigned ID, unsigned Operand) {
  if (ID < Intrinsic::num_intrinsics)
    return Intrinsic::Unknown;

  static const std::map<unsigned, std::vector<unsigned>> OutputInfo = {
#define GET_INTRINSIC_OUTPUT_SIGN
#include "TensilicaGenIntrnProperty.inc"
  };
  auto &&Iter = OutputInfo.find(ID);
  if (Iter == OutputInfo.end())
    return Intrinsic::Unknown;

  assert(Operand < Iter->second.size());
  return Iter->second[Operand];
}

static void initTargetIntrinsicRegistry() {
  // Initialize the protos/intrinsics
  TargetIntrinsicRegistry::RegisterTargetLookupIntrinsicID(
      TensilicaLookupIntrinsicID);
  TargetIntrinsicRegistry::RegisterTargetGetIntrinsicName(
      TensilicaGetIntrinsicName);
  TargetIntrinsicRegistry::RegisterTargetGetIntrinsicAttributes(
      TensilicaGetIntrinsicAttributes);
  TargetIntrinsicRegistry::RegisterTargetGetIntrinsicInfoTableEntries(
      TensilicaGetIntrinsicInfoTableEntries);
  TargetIntrinsicRegistry::RegisterTargetIsIncrIntrn(
      TensilicaIsIncrIntrn);
  TargetIntrinsicRegistry::RegisterTargetIsIncrRegIntrn(
      TensilicaIsIncrRegIntrn);
  TargetIntrinsicRegistry::RegisterTargetIsAligningIntrn(
      TensilicaIsAligningIntrn);
  TargetIntrinsicRegistry::RegisterTargetGetIntrnOffsetOpnd(
      TensilicaGetIntrnOffsetOpnd);
  TargetIntrinsicRegistry::RegisterTargetGetIntrnBaseOpnd(
      TensilicaGetIntrnBaseOpnd);
  TargetIntrinsicRegistry::RegisterTargetGetIntrnInputInfo(
      TensilicaGetIntrnInputInfo);
  TargetIntrinsicRegistry::RegisterTargetGetIntrnInputSign(
      TensilicaGetIntrnInputSignedness);
  TargetIntrinsicRegistry::RegisterTargetGetIntrnOutputInfo(
      TensilicaGetIntrnOutputInfo);
  TargetIntrinsicRegistry::RegisterTargetGetIntrnOutputSign(
      TensilicaGetIntrnOutputSignedness);
}

