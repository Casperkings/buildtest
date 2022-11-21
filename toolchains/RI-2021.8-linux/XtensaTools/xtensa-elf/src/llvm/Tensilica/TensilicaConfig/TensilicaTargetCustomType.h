//===-- TensilicaTargetCustomType.h   - Target Custom Type ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains target-specific custom type information that are
// shared between different Tensilica configurable targets. Xtensa and RISCV
// both use this file to prevent code duplication.
//
//===----------------------------------------------------------------------===//

#include "llvm/IR/DerivedTypes.h"
using namespace llvm;

typedef enum {
  UnsignedIntBaseType,
  SignedIntBaseType,
  FloatingPointBaseType,
  CustomBaseType,
  UnkownBaseType
} SIMDBaseTypeKind;

typedef struct {
  unsigned VectorLength;
  SIMDBaseTypeKind BaseKind;
  unsigned Base;
  unsigned RegType;
  unsigned MemType;
  unsigned AlignType;
} SIMDTypeInfo;

void initCustomTypeInfo() {
  // Initialize the Custom/Tie types
  CustomType::CustomTypeInfo CTITable[] = {
#define GET_CUSTOM_TYPE_INFO_RECORDS
#include "TensilicaGenCustomTypeInfo.inc"
#undef GET_CUSTOM_TYPE_INFO_RECORDS
      {0, "", 0, 0, 0, 0, 0}};
  CustomType::CustomTypeInfoTable.clear();
  CustomType::CustomTypeNameToIDMap.clear();
  unsigned NumCtypes =
      sizeof(CTITable) / sizeof(CustomType::CustomTypeInfo) - 1;
  for (unsigned i = 0; i < NumCtypes; ++i) {
    unsigned ID = CTITable[i].ID;
    const char *Name = CTITable[i].Name;
    unsigned BitWidth = CTITable[i].BitWidth;
    unsigned Alignment = CTITable[i].Alignment;
    unsigned MVT = CTITable[i].MVT;
    unsigned RegisterMVT = CTITable[i].RegisterMVT;
    unsigned NumRegs = CTITable[i].NumRegs;
    CustomType::InitializeCustomType(ID, Name, BitWidth, Alignment, MVT,
                                     RegisterMVT, NumRegs);
  }
}

static SIMDTypeInfo XtensaSIMDTypeInfo[] = {
#define GetSIMDTypeInfo
#define SIMDTypeInfo(VL, KIND, BASE, REG_TYPE, MEM_TYPE, ALIGN_TYPE)           \
  {VL, KIND, BASE, REG_TYPE, MEM_TYPE, ALIGN_TYPE},
#include "TensilicaGenSIMDInfo.inc"
#undef GetSIMDTypeInfo
    {0, UnkownBaseType, 0, 0, 0, 0}};

static void initCustomTypeToVecTypeInfo() {
  CustomType::CustomTyToVecTyInfoMap.clear();
  int numEntries = sizeof(XtensaSIMDTypeInfo) / sizeof(SIMDTypeInfo) - 1;
  for (int i = 0; i < numEntries; ++i) {
    SIMDTypeInfo &info = XtensaSIMDTypeInfo[i];
    unsigned CtypeId = info.MemType;
    unsigned ElemTy = 0;
    unsigned NumBits = info.Base;
    unsigned NumElems = info.VectorLength;
    if (info.BaseKind == SignedIntBaseType ||
        info.BaseKind == UnsignedIntBaseType)
      ElemTy = Type::IntegerTyID;
    else if (info.BaseKind == FloatingPointBaseType) {
      switch (NumBits) {
      default:
        continue;
      case 16:
        ElemTy = Type::HalfTyID;
        break;
      case 32:
        ElemTy = Type::FloatTyID;
        break;
      case 64:
        ElemTy = Type::DoubleTyID;
        break;
      }
    } else
      continue;

    CustomType::InitalizeCustomTypeToVecTypeInfo(CtypeId, ElemTy, NumElems,
                                                 NumBits);
  }
}
