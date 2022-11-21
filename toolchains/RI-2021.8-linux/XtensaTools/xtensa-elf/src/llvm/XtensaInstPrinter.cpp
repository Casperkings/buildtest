//===-- XtensaInstPrinter.cpp - Convert Xtensa MCInst to assembly syntax---===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class prints an Xtensa MCInst to a .s file.
//
//===----------------------------------------------------------------------===//

#include "InstPrinter/XtensaInstPrinter.h"
#include "XtensaMCExpr.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

#define DEBUG_TYPE "asm-printer"

#include "XtensaGenAsmWriter.inc"

void XtensaInstPrinter::printRegName(raw_ostream &OS, unsigned RegNo) const {
  OS << StringRef(getRegisterName(RegNo));
}

void XtensaInstPrinter::printInst(const MCInst *MI, uint64_t Address,
                                  StringRef Annot, const MCSubtargetInfo &STI,
                                  raw_ostream &OS) {
  printInstruction(MI, Address, OS);
  // emit pipeliner comment if available
  int CommentID = MI->getNumOperands() - 1;
  if (CommentID >= 0) {
    const auto &MO = MI->getOperand(CommentID);
    if (MO.isExpr() && MO.getExpr()->getKind() == MCExpr::Target) {
      const auto E = cast<XtensaMCExpr>(MO.getExpr());
      if (E->getKind() == XtensaMCExpr::VK_Xtensa_Comment)
        printOperand(MI, CommentID, OS);
    }
  }
  printAnnotation(OS, Annot);
}

void XtensaInstPrinter::printOperand(const MCInst *MI, unsigned OpNo,
                                     raw_ostream &OS) {
  const MCOperand &Op = MI->getOperand(OpNo);
  if (Op.isReg()) {
    printRegName(OS, Op.getReg());
    return;
  }

  if (Op.isImm()) {
    OS << Op.getImm();
    return;
  }

  assert(Op.isExpr() && "unknown operand kind in printOperand");
  Op.getExpr()->print(OS, &MAI);
}
