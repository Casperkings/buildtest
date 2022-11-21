//===-- XtensaJTIConstantPoolValue.cpp - Xtensa LLVM custom constant pool--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains classes for custom constant pool values, which hold
// jump table indices and target external symbols.
//
//===----------------------------------------------------------------------===//

#include "Xtensa.h"
#include "llvm/ADT/FoldingSet.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/raw_ostream.h"
#include <string>

using namespace llvm;

// JTI constant pool value.
class XtensaJTIConstantPoolValue : public MachineConstantPoolValue {
  // Each jump table index (not to be confused with JT entry)
  // defines a new jump table.
  // Jump table indices are integers.
  // They are translated to actual MCSymbols in XtensaASMPrinter.
  const unsigned JTI;
  LLVMContext &Ctx;

public:
  static XtensaJTIConstantPoolValue *Create(unsigned _JTI, LLVMContext &_Ctx) {
    return new XtensaJTIConstantPoolValue(_JTI, _Ctx);
  }

  // We mark XtensaJTIConstantPoolValues with int* type to distinguish them.
  XtensaJTIConstantPoolValue(unsigned _JTI, LLVMContext &_Ctx)
      : MachineConstantPoolValue(Type::getInt32Ty(_Ctx)),
        JTI(_JTI), Ctx(_Ctx) {}

  int getExistingMachineCPValue(MachineConstantPool *CP,
                                unsigned Alignment) override {
    (void)Alignment;
    auto Constants = CP->getConstants();
    for (unsigned I = 0, E = Constants.size(); I != E; ++I) {
      if (Constants[I].isMachineConstantPoolEntry()) {
        auto *XCPV = static_cast<XtensaJTIConstantPoolValue *>(
            Constants[I].Val.MachineCPVal);
        if (XCPV->JTI == JTI)
          return I;
      }
    }
    return -1;
  }

  void addSelectionDAGCSEId(FoldingSetNodeID &ID) { ID.AddInteger(JTI); }

  void print(raw_ostream &O) const override { O << JTI; }

  unsigned getJTI() { return JTI; }
};

// This class is suitable for external symbol literals
// that need to be loaded using L32R instruction.
// In particular, such symbols appear when CodeGen legalizes unsupported
// operations by replacing them with library calls.
class XtensaExternalSymConstantPoolValue : public MachineConstantPoolValue {
  const std::string ExternamSym;
  LLVMContext &Ctx;

public:
  static XtensaExternalSymConstantPoolValue*
   Create(const std::string &S, LLVMContext &_Ctx) {
    return new XtensaExternalSymConstantPoolValue(S, _Ctx);
  }

  // We mark XtensaExternalSymConstantPoolValues with label type
  // to distinguish them.
  XtensaExternalSymConstantPoolValue(const std::string &S, LLVMContext &_Ctx)
      : MachineConstantPoolValue(Type::getLabelTy(_Ctx)),
        ExternamSym(S), Ctx(_Ctx) {}

  int getExistingMachineCPValue(MachineConstantPool *CP,
                                unsigned Alignment) override {
    (void)Alignment;
    auto Constants = CP->getConstants();
    for (unsigned I = 0, E = Constants.size(); I != E; ++I) {
      if (Constants[I].isMachineConstantPoolEntry()) {
        auto *XCPV = static_cast<XtensaExternalSymConstantPoolValue *>(
            Constants[I].Val.MachineCPVal);
        if (XCPV->ExternamSym == ExternamSym)
          return I;
      }
    }
    return -1;
  }

  void addSelectionDAGCSEId(FoldingSetNodeID &ID) {
    ID.AddString(ExternamSym);
  }

  void print(raw_ostream &O) const override { O << ExternamSym; }

  std::string getExternamSym() { return ExternamSym; }
};
