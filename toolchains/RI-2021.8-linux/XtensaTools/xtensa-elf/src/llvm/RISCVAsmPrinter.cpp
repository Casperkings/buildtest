//===-- RISCVAsmPrinter.cpp - RISCV LLVM assembly writer ------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains a printer that converts from our internal representation
// of machine-dependent LLVM code to the RISCV assembly language.
//
//===----------------------------------------------------------------------===//

#if defined(TENSILICA) || 1
#include "Tensilica/TensilicaDFAPacketizer.h"
#endif
#include "RISCV.h"
#include "MCTargetDesc/RISCVInstPrinter.h"
#include "MCTargetDesc/RISCVMCExpr.h"
#include "RISCVTargetMachine.h"
#include "TargetInfo/RISCVTargetInfo.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

#define DEBUG_TYPE "asm-printer"

STATISTIC(RISCVNumInstrsCompressed,
          "Number of RISC-V Compressed instructions emitted");

namespace {
class RISCVAsmPrinter : public AsmPrinter {
public:
  explicit RISCVAsmPrinter(TargetMachine &TM,
                           std::unique_ptr<MCStreamer> Streamer)
      : AsmPrinter(TM, std::move(Streamer)) {}

  StringRef getPassName() const override { return "RISCV Assembly Printer"; }

  void EmitInstruction(const MachineInstr *MI) override;

  bool PrintAsmOperand(const MachineInstr *MI, unsigned OpNo,
                       const char *ExtraCode, raw_ostream &OS) override;
  bool PrintAsmMemoryOperand(const MachineInstr *MI, unsigned OpNo,
                             const char *ExtraCode, raw_ostream &OS) override;

  void EmitToStreamer(MCStreamer &S, const MCInst &Inst);
  bool emitPseudoExpansionLowering(MCStreamer &OutStreamer,
                                   const MachineInstr *MI);

  // Wrapper needed for tblgenned pseudo lowering.
  bool lowerOperand(const MachineOperand &MO, MCOperand &MCOp) const {
    return LowerRISCVMachineOperandToMCOperand(MO, MCOp, *this);
  }
};
}

#define GEN_COMPRESS_INSTR
#include "RISCVGenCompressInstEmitter.inc"
void RISCVAsmPrinter::EmitToStreamer(MCStreamer &S, const MCInst &Inst) {
  MCInst CInst;
  bool Res = compressInst(CInst, Inst, *TM.getMCSubtargetInfo(),
                          OutStreamer->getContext());
  if (Res)
    ++RISCVNumInstrsCompressed;
#if defined(TENSILICA) || 1
  if (Res) {
    // preserve comment if available
    int CommentID = Inst.getNumOperands() - 1;
    if (CommentID >= 0) {
      const auto &MO = Inst.getOperand(CommentID);
      if (MO.isExpr() && MO.getExpr()->getKind() == MCExpr::Target) {
        const auto E = cast<RISCVMCExpr>(MO.getExpr());
        if (E->getKind() == RISCVMCExpr::VK_RISCV_Comment)
          CInst.addOperand(MO);
      }
    }
  }
#endif
  AsmPrinter::EmitToStreamer(*OutStreamer, Res ? CInst : Inst);
}

// Simple pseudo-instructions have their lowering (with expansion to real
// instructions) auto-generated.
#include "RISCVGenMCPseudoLowering.inc"

#if defined(TENSILICA) || 1

void RISCVAsmPrinter::EmitInstruction(const MachineInstr *MI) {
  SmallString<128> Str;
  raw_svector_ostream O(Str);

  std::vector<MCInst> MIV;
  // Comment in assembly code the instructions accessing ymemory.
  unsigned YmemoryInstructions = 0;
  const auto TII = reinterpret_cast<const Tensilica::InstrInfo *>(
      MI->getParent()->getParent()->getSubtarget().getInstrInfo());
  switch (MI->getOpcode()) {
  case TargetOpcode::BUNDLE: {
    // insert necessary "nop" required by the assembler, say, find out a
    // smallest format for the bundle and position the instructions to
    // proper slot and insert nops to empty slots
    const MachineBasicBlock *MBB = MI->getParent();
    MachineBasicBlock::const_instr_iterator I = ++MI->getIterator();
    while (I != MBB->instr_end() && I->isInsideBundle()) {
      // MOVE_AR is OK here
      // FIXME
      // Do we need to handle generic opcode?
      assert((I->getOpcode() == RISCV::MOVE_AR || !I->isPseudo()) && 
             "Pseudo instruction in bundle");
      if (!I->isDebugValue()) {
        MCInst TmpInst;
        LowerRISCVMachineInstrToMCInst(&*I, TmpInst, *this);
        MIV.push_back(TmpInst);

        if (TII->isYmemoryLoad(*I))
          ++YmemoryInstructions;
      }
      assert(!emitPseudoExpansionLowering(*OutStreamer, &*I) &&
             "Unexpected Pseudo in Bundle");
      ++I;
    }
  } break;
  case RISCV::ENDLOOP:
    // Skip the ENDLOOP pseudo instruction.
    return;
  case TargetOpcode::DBG_VALUE:
    llvm_unreachable("Should be handled target independently");
    break;
  default: {
    // FIXME: Need to lower Pseudo instructions before packetizer, to allow
    // bunding them.
    if (emitPseudoExpansionLowering(*OutStreamer, MI))
      return;
    MCInst TmpInst;
    LowerRISCVMachineInstrToMCInst(MI, TmpInst, *this);
    MIV.push_back(TmpInst);

    if (TII->isYmemoryLoad(*MI))
      ++YmemoryInstructions;
    break;
  }
  }
  // assert(YmemoryInstructions <= 1 &&
  //       "More than one ymemory instruction in bundle");
  if (YmemoryInstructions > 0) {
    if (YmemoryInstructions > 1) {
      OutStreamer->AddComment("ymemory (multiple in bundle!)");
    } else {
      OutStreamer->AddComment("ymemory");
    }
  }
  llvm::Tensilica::Bundler Bundler;
  auto Bundle = Bundler.bundle(*TII, MIV);
  assert(Bundle.first != -1 && "Internal Compiler Error, illegal bundle");
  if (Bundle.second.size() > 1) {
    std::string FormatString(" {\t# format ");
    FormatString += Bundler.getFormatName(Bundle.first);
    FormatString += "\n";
    OutStreamer->EmitRawText(FormatString);
    for (auto &MII : Bundle.second)
      AsmPrinter::EmitToStreamer(*OutStreamer, MII);
    OutStreamer->EmitRawText(" }\n");
  }
  else {
    // The following routine tries to use compressed opcode, currently we avoid
    // it in bundles (need to consider doing it in packetizer?).
    EmitToStreamer(*OutStreamer, Bundle.second.back());
  }
}

#else
void RISCVAsmPrinter::EmitInstruction(const MachineInstr *MI) {
  // Do any auto-generated pseudo lowerings.
  if (emitPseudoExpansionLowering(*OutStreamer, MI))
    return;

  MCInst TmpInst;
  LowerRISCVMachineInstrToMCInst(MI, TmpInst, *this);
  EmitToStreamer(*OutStreamer, TmpInst);
}
#endif

bool RISCVAsmPrinter::PrintAsmOperand(const MachineInstr *MI, unsigned OpNo,
                                      const char *ExtraCode, raw_ostream &OS) {
  // First try the generic code, which knows about modifiers like 'c' and 'n'.
  if (!AsmPrinter::PrintAsmOperand(MI, OpNo, ExtraCode, OS))
    return false;

  const MachineOperand &MO = MI->getOperand(OpNo);
  if (ExtraCode && ExtraCode[0]) {
    if (ExtraCode[1] != 0)
      return true; // Unknown modifier.

    switch (ExtraCode[0]) {
    default:
      return true; // Unknown modifier.
    case 'z':      // Print zero register if zero, regular printing otherwise.
      if (MO.isImm() && MO.getImm() == 0) {
        OS << RISCVInstPrinter::getRegisterName(RISCV::X0);
        return false;
      }
      break;
    case 'i': // Literal 'i' if operand is not a register.
      if (!MO.isReg())
        OS << 'i';
      return false;
    }
  }

  switch (MO.getType()) {
  case MachineOperand::MO_Immediate:
    OS << MO.getImm();
    return false;
  case MachineOperand::MO_Register:
    OS << RISCVInstPrinter::getRegisterName(MO.getReg());
    return false;
  case MachineOperand::MO_GlobalAddress:
    PrintSymbolOperand(MO, OS);
    return false;
  case MachineOperand::MO_BlockAddress: {
    MCSymbol *Sym = GetBlockAddressSymbol(MO.getBlockAddress());
    Sym->print(OS, MAI);
    return false;
  }
  default:
    break;
  }

  return true;
}

bool RISCVAsmPrinter::PrintAsmMemoryOperand(const MachineInstr *MI,
                                            unsigned OpNo,
                                            const char *ExtraCode,
                                            raw_ostream &OS) {
  if (!ExtraCode) {
    const MachineOperand &MO = MI->getOperand(OpNo);
    // For now, we only support register memory operands in registers and
    // assume there is no addend
    if (!MO.isReg())
      return true;

    OS << "0(" << RISCVInstPrinter::getRegisterName(MO.getReg()) << ")";
    return false;
  }

  return AsmPrinter::PrintAsmMemoryOperand(MI, OpNo, ExtraCode, OS);
}

// Force static initialization.
#if defined(TENSILICA) || 1
extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeRISCVConfigAsmPrinter() {
#else
extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeRISCVAsmPrinter() {
#endif
  RegisterAsmPrinter<RISCVAsmPrinter> X(getTheRISCV32Target());
  RegisterAsmPrinter<RISCVAsmPrinter> Y(getTheRISCV64Target());
}
