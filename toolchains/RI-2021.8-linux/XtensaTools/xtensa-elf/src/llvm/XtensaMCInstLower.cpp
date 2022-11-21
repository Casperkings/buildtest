//===-- XtensaMCInstLower.cpp - Convert Xtensa MachineInstr to MCInst -----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// This file contains code to lower Xtensa MachineInstrs to their corresponding
/// MCInst records.
///
//===----------------------------------------------------------------------===//

#include "XtensaMCInstLower.h"
#include "Xtensa.h"
#include "XtensaBaseInfo.h"
#include "XtensaMCExpr.h"
#include "XtensaSubtarget.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/IR/Mangler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"

using namespace llvm;

XtensaMCInstLower::XtensaMCInstLower(class AsmPrinter &asmprinter)
    : Printer(asmprinter) {}

void XtensaMCInstLower::Initialize(Mangler *M, MCContext *C) {
  Mang = M;
  Ctx = C;
}

MCOperand XtensaMCInstLower::LowerSymbolOperand(const MachineOperand &MO,
                                                MachineOperandType MOTy,
                                                unsigned Offset) const {
  MCSymbolRefExpr::VariantKind Kind = MCSymbolRefExpr::VK_None;
  const MCSymbol *Symbol;

  switch (MOTy) {
  case MachineOperand::MO_MachineBasicBlock:
    Symbol = MO.getMBB()->getSymbol();
    break;
  case MachineOperand::MO_GlobalAddress:
    Symbol = Printer.getSymbol(MO.getGlobal());
    Offset += MO.getOffset();
    break;
  case MachineOperand::MO_BlockAddress:
    Symbol = Printer.GetBlockAddressSymbol(MO.getBlockAddress());
    Offset += MO.getOffset();
    break;
  case MachineOperand::MO_ExternalSymbol:
    Symbol = Printer.GetExternalSymbolSymbol(MO.getSymbolName());
    Offset += MO.getOffset();
    break;
  case MachineOperand::MO_JumpTableIndex:
    Symbol = Printer.GetJTISymbol(MO.getIndex());
    break;
  case MachineOperand::MO_ConstantPoolIndex:
    Symbol = Printer.GetCPISymbol(MO.getIndex());
    Offset += MO.getOffset();
    break;
  default:
    llvm_unreachable("<unknown operand type>");
  }

  const MCExpr *Expr = MCSymbolRefExpr::create(Symbol, Kind, *Ctx);

  if (Offset) {
    // Assume offset is never negative.
    assert(Offset > 0);

    const MCConstantExpr *OffsetExpr = MCConstantExpr::create(Offset, *Ctx);
    Expr = MCBinaryExpr::createAdd(Expr, OffsetExpr, *Ctx);
  }

  unsigned access = MO.getTargetFlags();
  switch (access) {
  case XtensaII::MO_LO16:
    Expr = XtensaMCExpr::createLo(Expr, *Ctx);
    break;
  case XtensaII::MO_HI16:
    Expr = XtensaMCExpr::createHi(Expr, *Ctx);
    break;
  }

  return MCOperand::createExpr(Expr);
}

MCOperand XtensaMCInstLower::LowerOperand(const MachineOperand &MO,
                                          unsigned Offset) const {
  const MachineInstr *MI = MO.getParent();
  const XtensaSubtarget &STI =
      MI->getParent()->getParent()->getSubtarget<XtensaSubtarget>();

  bool InvertImm = !STI.isLittleEndian() &&
                   STI.getInstrInfo()->isBBCIOrBBSI(MI->getOpcode());

  MachineOperandType MOTy = MO.getType();
  switch (MOTy) {
  default:
    llvm_unreachable("unknown operand type");
  case MachineOperand::MO_Metadata: {
    const MDNode *Node = MO.getMetadata();
    const MDString *MDS = cast<MDString>(Node->getOperand(0));
    StringRef Comment = MDS->getString();
    auto Expr = XtensaMCExpr::create(XtensaMCExpr::VK_Xtensa_Comment, nullptr,
                                     Comment, *Ctx);
    return MCOperand::createExpr(Expr);
  }
  case MachineOperand::MO_Register:
    // Ignore all implicit register operands.
    if (MO.isImplicit())
      break;
    return MCOperand::createReg(MO.getReg());
  case MachineOperand::MO_Immediate: {
    if (InvertImm) {
      assert(Offset == 0 && "InvertImm BBCIOrBBSI with Offset");
      return MCOperand::createImm(31 - MO.getImm());
    } else
      return MCOperand::createImm(MO.getImm() + Offset);
  }
  case MachineOperand::MO_MachineBasicBlock:
  case MachineOperand::MO_GlobalAddress:
  case MachineOperand::MO_ExternalSymbol:
  case MachineOperand::MO_JumpTableIndex:
  case MachineOperand::MO_ConstantPoolIndex:
  case MachineOperand::MO_BlockAddress:
    return LowerSymbolOperand(MO, MOTy, Offset);
  case MachineOperand::MO_RegisterMask:
    break;
  }

  return MCOperand();
}

void XtensaMCInstLower::Lower(const MachineInstr *MI, MCInst &OutMI) const {
  OutMI.setOpcode(MI->getOpcode());

  for (unsigned i = 0, e = MI->getNumOperands(); i != e; ++i) {
    const MachineOperand &MO = MI->getOperand(i);
    MCOperand MCOp = LowerOperand(MO);

    if (MCOp.isValid())
      OutMI.addOperand(MCOp);
  }
}
