//===-- XtensaAsmPrinter.cpp - Xtensa LLVM assembly writer----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains a printer that converts from our internal representation
// of machine-dependent LLVM code to the XAS-format Xtensa assembly language.
//
//===----------------------------------------------------------------------===//

#include "InstPrinter/XtensaInstPrinter.h"
#include "Xtensa.h"
#include "XtensaMachineConstantPoolValue.h"
#include "XtensaInstrInfo.h"
#include "XtensaMCExpr.h"
#include "XtensaMCInstLower.h"
#include "XtensaSubtarget.h"
#include "XtensaTargetMachine.h"
#include "XtensaTargetStreamer.h"
#include "Tensilica/TensilicaDFAPacketizer.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineDominators.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineJumpTableInfo.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/MachinePostDominators.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Mangler.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSymbolELF.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/XtensaXtparams.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetLoweringObjectFile.h"
#include <algorithm>
#include <cctype>
#include <string.h>
using namespace llvm;

#define DEBUG_TYPE "asm-printer"

static cl::opt<bool>
    ForceGenZcl1iter("xtensa-gen-zcl1iter",
                     cl::desc("Always generate .zcl1iter directive"),
                     cl::init(false), cl::Hidden);

namespace {
class XtensaAsmPrinter : public AsmPrinter {
  XtensaMCInstLower MCInstLowering;
  llvm::Tensilica::Bundler Bundler;
  const XtensaSubtarget *Subtarget;
  MachineDominatorTree *DT;
  MachinePostDominatorTree *PDT;
  bool PrevLoopWasValidRemainder;
  bool NextLoopIsControlEquivalent;
  bool DidGenZcl1iterCheck = false;
  bool GenZcl1iter;
  MachineLoopInfo *TLI;

public:
  explicit XtensaAsmPrinter(TargetMachine &TM,
                            std::unique_ptr<MCStreamer> Streamer)
      : AsmPrinter(TM, std::move(Streamer)), MCInstLowering(*this) {}

  StringRef getPassName() const override { return "Xtensa Assembly Printer"; }
#if 0
   void printInlineJT(const MachineInstr *MI, int opNum, raw_ostream &O,
                      const std::string &directive = ".jmptable");
   void printInlineJT32(const MachineInstr *MI, int opNum, raw_ostream &O) {
     printInlineJT(MI, opNum, O, ".jmptable32");
   }
#endif
  void getAnalysisUsage(AnalysisUsage &AU) const;
  void loopBufferCheck(const MachineBasicBlock *MBB, const XtensaInstrInfo *TII,
                       const StringRef &LoopInstComment);
  void printOperand(const MachineInstr *MI, int opNum, raw_ostream &O);
  bool PrintAsmOperand(const MachineInstr *MI, unsigned OpNo,
                       const char *ExtraCode, raw_ostream &O) override;
  bool PrintAsmMemoryOperand(const MachineInstr *MI, unsigned OpNum,
                             const char *ExtraCode, raw_ostream &O) override;

  void emitArrayBound(MCSymbol *Sym, const GlobalVariable *GV);

  void EmitConstantPool() override;
  void EmitFunctionEntryLabel() override;
  void EmitInstruction(const MachineInstr *MI) override;
  void EmitFunctionBodyStart() override;
  void EmitFunctionBodyEnd() override;
  void EmitOverlayStub(MachineFunction &MF);
  bool isBlockOnlyReachableByFallthrough(
      const MachineBasicBlock *MBB) const override;
  void EmitFunctionBuiltInConstants() override;
  bool needNopForEndloop(const MachineInstr *MI) const;

  bool runOnMachineFunction(MachineFunction &MF) override {
    Subtarget = &MF.getSubtarget<XtensaSubtarget>();
    DT = &getAnalysis<MachineDominatorTree>();
    PDT = &getAnalysis<MachinePostDominatorTree>();
    TLI = &getAnalysis<MachineLoopInfo>();
    PrevLoopWasValidRemainder = false;
    NextLoopIsControlEquivalent = false;
    if (!DidGenZcl1iterCheck) {
      DidGenZcl1iterCheck = true;
      GenZcl1iter = ForceGenZcl1iter;
      std::string HWVersion = getDefaultParamsString("TargetHWVersion");
      if (HWVersion.size() > 2 && strncmp(HWVersion.c_str(), "LX", 2) == 0) {
        int MAEarliest = getDefaultParamsValue("HWMicroArchEarliest");
        int MALatest = getDefaultParamsValue("HWMicroArchLatest");
        if (MALatest >= 250000 && MAEarliest < 270011) {
          int LoopBufferSize = getDefaultParamsValue("LoopBufferSize");
          if (LoopBufferSize > 0)
            GenZcl1iter = true;
        }
      }
      if (::getenv("XTENSA_NO_ZCL1ITER_DIRECTIVE"))
        GenZcl1iter = false;
    }

    // Drop dead jump tables before function emitting.
    // We do it here instead of BranchFolder::OptimizeFunction(),
    // because it requires access to target specific JTI constant pool.
    MachineJumpTableInfo *JTI = MF.getJumpTableInfo();
    if (JTI) {
      // Walk the function to find jump tables that are live.
      BitVector JTIsLive(JTI->getJumpTables().size());
      for (const MachineBasicBlock &BB : MF) {
        // Iterate over instructions (not bundles).
        for (const MachineInstr &I : make_range(BB.instr_begin(),
                                                BB.instr_end()))
          for (const MachineOperand &Op : I.operands()) {
            // Avoid removing of jump tables,
            // which addresses are loaded indirectly (e.g. L32R).
            if (Op.isCPI()) {
              unsigned idx = Op.getIndex();
              MachineConstantPool *MCP = MF.getConstantPool();
              auto &Constants = MCP->getConstants();
              assert(Constants.size() > idx && "Internal algo error");
              // On Xtensa, if instruction refers to a custom constant
              // pool entry, then it must be load of jump table address.
              if (Constants[idx].isMachineConstantPoolEntry() &&
                  Constants[idx].getType()->isIntegerTy()) {
                auto *XCPV = static_cast<XtensaJTIConstantPoolValue*>(
                    Constants[idx].Val.MachineCPVal);
                  // Remember that this JT is live.
                JTIsLive.set(XCPV->getJTI());
              }
            }
            // Normal (CONST16) case.
            else if (Op.isJTI()) {
              // Remember that this JT is live.
              JTIsLive.set(Op.getIndex());
            }
          }
      }

      // Finally, remove dead jump tables.  This happens when the
      // indirect jump was unreachable (and thus deleted).
      for (unsigned i = 0, e = JTIsLive.size(); i != e; ++i)
        if (!JTIsLive.test(i)) {
          JTI->RemoveJumpTable(i);
        }
    }

    // Process overlay function.
    std::string OrigName;
    std::string OrigSect;
    bool isOverlay = false;
    Function *F = const_cast<Function*>(&(MF.getFunction()));
    if (F->hasFnAttribute("xtensa-overlay")) {
      isOverlay = true;
      // For overlay functions we change their names and sections,
      // and then emit them as usual.
      // This is a mild hack to avoid
      // substantial modifications in generic ASMPrinter.cpp code.
      OrigName = F->getName();
      OrigSect = F->getSection();
      StringRef OverlayAttrStr =
          F->getFnAttribute("xtensa-overlay").getValueAsString();
      F->setSection(std::string(".overlay.") +
                    std::string(OverlayAttrStr) +
                    std::string(".text"));

      F->setName(std::string("__overlay__")+ F->getName().str());
    }

    AsmPrinter::runOnMachineFunction(MF);

    if (isOverlay) {
      // Restore the original name and section.
      F->setName(OrigName);
      F->setSection(OrigSect);
      // Emit overlay stub.
      EmitOverlayStub(MF);
    }

    return false;
  }
};
} // end of anonymous namespace



// Emit overlay stub for function.
void XtensaAsmPrinter::EmitOverlayStub(MachineFunction &MF) {
  const Function &F = MF.getFunction();
  assert(F.hasFnAttribute("xtensa-overlay") && "Internal algo error");

  std::string FName = F.getName().str();

  StringRef OverlayNumberStr;
  OverlayNumberStr =
      F.getFnAttribute("xtensa-overlay").getValueAsString();

  std::string SectStr =
      std::string("\t.section ") +
      std::string(".overlay.call.") +
      std::string(OverlayNumberStr) +
      std::string(".text");
  OutStreamer->EmitRawText(SectStr);

  std::string GlobSStr = "\t.globl _overlay_call_in_" +
      OverlayNumberStr.str() + "_";
  OutStreamer->EmitRawText(GlobSStr);

  if (!Subtarget->hasConst16()) {
    std::string LitStr = std::string("\t.literal .L_OSC_") +
        FName + ", __overlay__" + FName;
    OutStreamer->EmitRawText(LitStr);
  }

  MCSymbol *Sym = getSymbol(&F);
  EmitVisibility(Sym, F.getVisibility());
  EmitLinkage(&F, Sym);

  if (MAI->hasFunctionAlignment()) {
    auto A =
        getGVAlignment(&F, F.getParent()->getDataLayout(), MF.getAlignment());
    const TargetLowering *TLI = MF.getSubtarget().getTargetLowering();
    A = std::max(A, TLI->getMinFunctionAlignment());
    EmitAlignment(A);
  }
  if (MAI->hasDotTypeDotSizeDirective())
    OutStreamer->EmitSymbolAttribute(Sym, MCSA_ELF_TypeFunction);

  OutStreamer->EmitLabel(Sym);

  if (Subtarget->hasConst16()) {
    std::string FStr ="__overlay__" + FName;
    std::string InstrStr;
    InstrStr = std::string("\tconst16 a9, ") + std::string("(") +
        FStr + std::string(")@h");
    OutStreamer->EmitRawText(InstrStr);
    InstrStr = std::string("\tconst16 a9, ") + std::string("(") +
        FStr + std::string(")@l");
    OutStreamer->EmitRawText(InstrStr);
    InstrStr = std::string("\tj _overlay_call_in_") +
        OverlayNumberStr.str()  + "_";
    OutStreamer->EmitRawText(InstrStr);
  }
  else {
    std::string InstrStr = "\tl32r a9, ";
    InstrStr += std::string(".L_OSC_") + FName;
    OutStreamer->EmitRawText(InstrStr);
    InstrStr = std::string("\tj _overlay_call_in_") +
        OverlayNumberStr.str()  + "_";
    OutStreamer->EmitRawText(InstrStr);
  }

  OutStreamer->AddBlankLine();
}


namespace {
// Keep track the alignment, constpool entries per Section.
struct SectionCPs {
  MCSection *S;
  unsigned Alignment;
  SmallVector<unsigned, 4> CPEs;
  SectionCPs(MCSection *s, unsigned a) : S(s), Alignment(a) {}
};
} // namespace

void XtensaAsmPrinter::EmitConstantPool() {
  // The following code is mostly copied from AsmPrinter::EmitConstantPool in
  // AsmPrinter.cpp.

  const MachineConstantPool *MCP = MF->getConstantPool();
  const std::vector<MachineConstantPoolEntry> &CP = MCP->getConstants();

  if (CP.empty()) {
    OutStreamer->SwitchSection(
        getObjFileLowering().SectionForGlobal(&MF->getFunction(), TM));
    OutStreamer->EmitRawText("\t.literal_position");
    return;
  }

  // Calculate sections for constant pool entries. We collect entries to go into
  // the same section together to reduce amount of section switch statements.
  SmallVector<SectionCPs, 4> LiteralSections;
  SmallVector<SectionCPs, 4> CPSections;
  for (unsigned i = 0, e = CP.size(); i != e; ++i) {
    const MachineConstantPoolEntry &CPE = CP[i];
    unsigned Align = CPE.getAlignment();

    SectionKind Kind = CPE.getSectionKind(&getDataLayout());
    MCSection *S = getObjFileLowering().getSectionForConstant(getDataLayout(),
                                                              Kind, nullptr, Align);

    // Put a constant pool entry in CPSections or LiteralSections
    // depending on whether it is emitted with .literal or not.
    bool IsLiteral = CPE.isMachineConstantPoolEntry() ||
                     !(dyn_cast<ConstantDataSequential>(CPE.Val.ConstVal) ||
                       dyn_cast<ConstantVector>(CPE.Val.ConstVal) ||
                       dyn_cast<ConstantAggregateZero>(CPE.Val.ConstVal));
    SmallVector<SectionCPs, 4> &Buckets =
        *(IsLiteral ? &LiteralSections : &CPSections);

    // The number of sections are small, just do a linear search from the
    // last section to the first.
    bool Found = false;
    unsigned SecIdx = Buckets.size();
    while (SecIdx != 0) {
      if (Buckets[--SecIdx].S == S) {
        Found = true;
        break;
      }
    }
    if (!Found) {
      SecIdx = Buckets.size();
      Buckets.push_back(SectionCPs(S, Align));
    }

    if (Align > Buckets[SecIdx].Alignment)
      Buckets[SecIdx].Alignment = Align;
    Buckets[SecIdx].CPEs.push_back(i);
  }

  // Now print stuff into the calculated sections.
  const MCSection *CurSection = nullptr;
  unsigned Offset = 0;
  for (unsigned i = 0, e = CPSections.size(); i != e; ++i) {
    for (unsigned j = 0, ee = CPSections[i].CPEs.size(); j != ee; ++j) {
      unsigned CPI = CPSections[i].CPEs[j];
      MCSymbol *Sym = GetCPISymbol(CPI);
      if (!Sym->isUndefined())
        continue;

      if (CurSection != CPSections[i].S) {
        OutStreamer->SwitchSection(CPSections[i].S);
        EmitAlignment(Align(CPSections[i].Alignment));
        CurSection = CPSections[i].S;
        Offset = 0;
      }

      MachineConstantPoolEntry CPE = CP[CPI];

      // JTI constant pool entries are emitted in
      // EmitFunctionBuiltInConstants() and
      // OverlayOut constants always go to literals.
      assert(!CPE.isMachineConstantPoolEntry() && "Internal algo error.");

      // Emit inter-object padding for alignment.
      unsigned AlignMask = CPE.getAlignment() - 1;
      unsigned NewOffset = (Offset + AlignMask) & ~AlignMask;
      OutStreamer->EmitZeros(NewOffset - Offset);

      Type *Ty = CPE.getType();
      Offset = NewOffset + getDataLayout().getTypeAllocSize(Ty);

      OutStreamer->EmitLabel(Sym);
      EmitGlobalConstant(getDataLayout(), CPE.Val.ConstVal);
    }
  }

  OutStreamer->SwitchSection(
      getObjFileLowering().SectionForGlobal(&MF->getFunction(), TM));

  OutStreamer->EmitRawText("\t.literal_position");

  // Emit .literal now.
  for (unsigned i = 0, e = LiteralSections.size(); i != e; ++i) {
    for (unsigned j = 0, ee = LiteralSections[i].CPEs.size(); j != ee; ++j) {
      unsigned CPI = LiteralSections[i].CPEs[j];
      MCSymbol *Sym = GetCPISymbol(CPI);
      if (!Sym->isUndefined())
        continue;

      MachineConstantPoolEntry CPE = CP[CPI];
      if (!CPE.isMachineConstantPoolEntry()) {
        const MCExpr *ME = lowerConstant(CPE.Val.ConstVal);
        OutStreamer->EmitLiteral(Sym->getName(), ME, 4);
      }
      // Check of it is an overlay out function name.
      else if (CPE.getType()->isLabelTy()) {
        auto *XCPV = static_cast<XtensaExternalSymConstantPoolValue*>(
                CPE.Val.MachineCPVal);
        // Emit literal constant for overlay out external symbols
        // in order to load them with L32R.
        const MCExpr *ME =
            MCSymbolRefExpr::create(
                OutContext.getOrCreateSymbol(XCPV->getExternamSym()),
                OutContext);
        OutStreamer->EmitLiteral(Sym->getName(), ME, 4);
      }
      // Xtensa JTI constant pool entries are emitted in
      // EmitFunctionBuiltInConstants(). No need to deal with them here.
    }
  }
}

void XtensaAsmPrinter::EmitFunctionBodyStart() {
  MCInstLowering.Initialize(&(getObjFileLowering().getMangler()),
                            &MF->getContext());
}

/// EmitFunctionBodyEnd - Targets can override this to emit stuff after
/// the last basic block in the function.
void XtensaAsmPrinter::EmitFunctionBodyEnd() {
  // Emit function end directives
}

void XtensaAsmPrinter::EmitFunctionEntryLabel() {
  // Mark the start of the function
  OutStreamer->EmitLabel(CurrentFnSym);
}

void XtensaAsmPrinter::EmitFunctionBuiltInConstants() {
  const MachineConstantPool *MCP = MF->getConstantPool();
  const std::vector<MachineConstantPoolEntry> &CP = MCP->getConstants();
  for (unsigned i = 0, e = CP.size(); i != e; ++i) {
    auto Entry = CP[i];
    // Always put JT addresses directly into the function
    // to minimize out of range relocations,
    // regardless -mtext-section-literals option.
    if (Entry.isMachineConstantPoolEntry() &&
        Entry.getType()->isIntegerTy()) {
      MCSymbol *Sym = GetCPISymbol(i);
      auto *MCPV = Entry.Val.MachineCPVal;
      assert(MCPV->getType()->isIntegerTy() && "Only JTIs allowed.");
      if (auto *XCPV = static_cast<XtensaJTIConstantPoolValue*>(MCPV)) {
        // Convert JTI index to MCSymbol and then into expression that we emit.
        const MCExpr *ME = MCSymbolRefExpr::create(GetJTISymbol(XCPV->getJTI()),
                                                   OutContext);
        OutStreamer->EmitLiteral(Sym->getName(), ME, 4);
      }
    }
  }
}

#if 0
 void XtensaAsmPrinter::
 printInlineJT(const MachineInstr *MI, int opNum, raw_ostream &O,
               const std::string &directive) {
   unsigned JTI = MI->getOperand(opNum).getIndex();
   const MachineFunction *MF = MI->getParent()->getParent();
   const MachineJumpTableInfo *MJTI = MF->getJumpTableInfo();
   const std::vector<MachineJumpTableEntry> &JT = MJTI->getJumpTables();
   const std::vector<MachineBasicBlock*> &JTBBs = JT[JTI].MBBs;
   O << "\t" << directive << " ";
   for (unsigned i = 0, e = JTBBs.size(); i != e; ++i) {
     MachineBasicBlock *MBB = JTBBs[i];
     if (i > 0)
       O << ",";
     O << *MBB->getSymbol();
   }
 }
#endif

void XtensaAsmPrinter::getAnalysisUsage(AnalysisUsage &AU) const {
  AsmPrinter::getAnalysisUsage(AU);
  AU.addRequired<MachineDominatorTree>();
  AU.addRequired<MachinePostDominatorTree>();
  AU.addRequired<MachineLoopInfo>();
}

static void ExtractLoopInstComment(MCInst *MI, StringRef &LoopInstComment) {
  int CommentID = MI->getNumOperands() - 1;
  if (CommentID >= 0) {
    const auto &MO = MI->getOperand(CommentID);
    if (MO.isExpr() && MO.getExpr()->getKind() == MCExpr::Target) {
      const auto E = cast<XtensaMCExpr>(MO.getExpr());
      if (E->getKind() == XtensaMCExpr::VK_Xtensa_Comment)
        LoopInstComment = E->getComment();
    }
  }
}

void XtensaAsmPrinter::loopBufferCheck(const MachineBasicBlock *MBB,
                                       const XtensaInstrInfo *TII,
                                       const StringRef &LoopInstComment) {
  // If ZOLNoWA is true, then no work around is needed.
  bool ZOLNoWA = false;
  PrevLoopWasValidRemainder = false;
  bool LoopIsControlEquivalent = NextLoopIsControlEquivalent;
  NextLoopIsControlEquivalent = false;
  const MachineBasicBlock *BodyHeadMBB = MBB->getNextNode();
  if (BodyHeadMBB) {

    // Find next ZOL loop
    const MachineBasicBlock *NextLoopHeadMBB =
        nullptr; // Next Loop Head (ZOL) MBB
    for (const MachineBasicBlock *lt_bb = BodyHeadMBB;;
         lt_bb = NextLoopHeadMBB->getNextNode()) {
      NextLoopHeadMBB = lt_bb->getNextNode();
      if (NextLoopHeadMBB == nullptr)
        break;

      for (;;) {
        const MachineBasicBlock *NextMBB = NextLoopHeadMBB->getNextNode();
        if (NextMBB == nullptr) {
          NextLoopHeadMBB = nullptr;
          break;
        }
        // Is there a to detect that loop is also innermost?
        if (TLI->isLoopHeader(NextMBB))
          break;
        NextLoopHeadMBB = NextMBB;
      }
      if (NextLoopHeadMBB == nullptr)
        break;

      bool HasLoopInst = false;
      for (auto &MI : *NextLoopHeadMBB) {
        if (MI.isBundle()) {
          MachineBasicBlock::const_instr_iterator I = ++MI.getIterator();
          for (; I != NextLoopHeadMBB->instr_end() && I->isInsideBundle();
               ++I) {
            if (TII->isLoop((*I).getOpcode())) {
              HasLoopInst = true;
              break;
            }
          }
        } else {
          if (TII->isLoop(MI.getOpcode())) {
            HasLoopInst = true;
            break;
          }
        }
      }
      if (HasLoopInst)
        break;
    }
    if (NextLoopHeadMBB != nullptr) {
      if (PDT->dominates(NextLoopHeadMBB, MBB) &&
          DT->dominates(MBB, NextLoopHeadMBB)) {
        std::string FormatString(
            "\t# loop head is control equivalent with BB:");
        FormatString += std::to_string(NextLoopHeadMBB->getNumber());
        FormatString += "\n";
        OutStreamer->EmitRawText(FormatString);
        ZOLNoWA = true;
        NextLoopIsControlEquivalent = true;
      }
    }
    if (LoopIsControlEquivalent) {
      OutStreamer->EmitRawText(
          "\t# loop head is control equivalent with previous ZOL\n");
      ZOLNoWA = true;
    }

    int TripCount = -1;
    if (!ZOLNoWA && !LoopInstComment.empty()) {
      const char *SubStr = strstr(LoopInstComment.data(), "trip count: ");
      if (SubStr != NULL) {
        TripCount = atoi(SubStr + 12);
        std::string FormatString("\t# loop-count fixed at ");
        FormatString += std::to_string(TripCount);
        FormatString += "\n";
        OutStreamer->EmitRawText(FormatString);
        ZOLNoWA = true;
      } else if (strstr(LoopInstComment.data(), "pragma: no_zcl1iter")) {
        OutStreamer->EmitRawText("\t# no_zcl1iter pragma used\n");
        ZOLNoWA = true;
      }
    }
  }

  if (ZOLNoWA) {
    OutStreamer->EmitRawText("\t.zcl1iter\t0\n");
  } else {
    // Check for early exits and wsr.lcount OPs.
    int workaround_version = 1;

    MachineLoop *BodyHeadLoop = TLI->getLoopFor(BodyHeadMBB);
    for (const MachineBasicBlock *LoopMBB = BodyHeadMBB;;
         LoopMBB = LoopMBB->getNextNode()) {
      if (TLI->getLoopFor(LoopMBB) != BodyHeadLoop)
        break;

      for (auto &MI : *LoopMBB) {
        if (MI.getOpcode() == Xtensa::WSR_LCOUNT) {
          workaround_version = 2;
          break;
        }
      }
      for (auto succ_iter = LoopMBB->succ_begin();
           succ_iter != LoopMBB->succ_end(); succ_iter++) {
        if (TLI->getLoopFor(*succ_iter) != BodyHeadLoop &&
            *succ_iter != LoopMBB->getNextNode()) {
          workaround_version = 2;
          break;
        }
      }
      if (workaround_version == 2)
        break;
    }
    std::string FormatString("\t.zcl1iter\t");
    FormatString += std::to_string(workaround_version);
    FormatString += "\n";
    OutStreamer->EmitRawText(FormatString);
  }
}

// Determine whether we need to insert a NOP immediately before
// ENDLOOP.  We need a NOP if the loop is empty.  Also, we need a NOP
// if the loop ends with a branch instruction jumping to a location
// within the loop.
bool XtensaAsmPrinter::needNopForEndloop(const MachineInstr *MI) const {
  MachineBasicBlock::const_instr_iterator I = MI->getIterator();
  const MachineBasicBlock *MBB = MI->getParent();
  const XtensaInstrInfo *TII =
      MF->getSubtarget<XtensaSubtarget>().getInstrInfo();

  auto canBeLastInstr = [](const MachineInstr *I) {
    unsigned Opc = I->getOpcode();
    return !I->isCall() && Opc != Xtensa::WAITI && Opc != Xtensa::RSR_LCOUNT &&
           Opc != Xtensa::ISYNC;
  };

  auto isPseudoInstr = [](const MachineInstr *I) {
    return I->isDebugValue() || I->getOpcode() == Xtensa::EXTW_NOREORDER ||
           I->getOpcode() == Xtensa::MEMW_NOREORDER ||
           I->getOpcode() == TargetOpcode::IMPLICIT_DEF;
  };

  MachineBasicBlock *Header = MI->getOperand(0).getMBB();

  // First, find the preheader for this loop.
  MachineBasicBlock *Preheader = nullptr;
  for (auto *Pred : Header->predecessors()) {
    if (Pred->isLayoutSuccessor(Header))
      Preheader = Pred;
  }
  assert(Preheader && "preheader not found");

  // Find the LOOP instruction.
  MachineInstr *LoopInst = nullptr;
  for (auto &LI : Preheader->instrs()) {
    if (TII->isLoop(LI.getOpcode())) {
      assert(!LoopInst && "Multiple LOOP instructions found in a block");
      LoopInst = &LI;
    }

    // Even after finding a LOOP instruction, keep going to make sure
    // that we have exactly one LOOP instruction.
  }

  assert(LoopInst && "loop instruction not found");

  bool InfLoop = LoopInst->getOperand(0).getReg() == Xtensa::LR;

  // Look for suitable instruction in ENDLOOP's BB
  while (I != MBB->begin()) {
    --I;
    if (isPseudoInstr(&(*I)))
      continue;

    if (!canBeLastInstr(&(*I)) || (!InfLoop && I->isBranch()))
      return true;

    return false;
  }

  // Look for instruction in single fallthrough predecessor
  if (MBB->pred_size() != 1)
    return true;
  MachineBasicBlock *PredMBB = *MBB->pred_begin();
  if (!PredMBB->isLayoutSuccessor(MBB))
    return true;

  // Avoid branches with target inside loop since LCOUNT:
  // If the last instruction of the loop is a taken branch,
  // then the value of LCOUNT is undefined in some implementations.
  MachineLoop *L = TLI->getLoopFor(MBB);
  if (!InfLoop)
    for (auto *SuccMBB : PredMBB->successors())
      if (SuccMBB != MBB && L->contains(TLI->getLoopFor(SuccMBB)))
        return true;

  // Look for suitable instruction in predecessor
  for (auto &PredInsn : PredMBB->instrs()) {
    if (!canBeLastInstr(&PredInsn))
      return true;
    if (!isPseudoInstr(&PredInsn))
      return false;
  }
  return true;
}

void XtensaAsmPrinter::printOperand(const MachineInstr *MI, int opNum,
                                    raw_ostream &O) {
  const DataLayout &DL = getDataLayout();
  const MachineOperand &MO = MI->getOperand(opNum);
  switch (MO.getType()) {
  case MachineOperand::MO_Register:
    O << XtensaInstPrinter::getRegisterName(MO.getReg());
    break;
  case MachineOperand::MO_Immediate:
    O << MO.getImm();
    break;
  case MachineOperand::MO_MachineBasicBlock:
    MO.getMBB()->getSymbol()->print(O, MAI);
    break;
  case MachineOperand::MO_GlobalAddress:
    getSymbol(MO.getGlobal())->print(O, MAI);
    break;
  case MachineOperand::MO_ConstantPoolIndex:
    O << DL.getPrivateGlobalPrefix() << "CPI" << getFunctionNumber() << '_'
      << MO.getIndex();
    break;
  case MachineOperand::MO_BlockAddress:
    GetBlockAddressSymbol(MO.getBlockAddress())->print(O, MAI);
    break;
  default:
    llvm_unreachable("not implemented");
  }
}

/// PrintAsmOperand - Print out an operand for an inline asm expression.
///
bool XtensaAsmPrinter::PrintAsmOperand(const MachineInstr *MI, unsigned OpNo,
                                       const char *ExtraCode, raw_ostream &O) {
  // Print the operand if there is no operand modifier.
  if (!ExtraCode || !ExtraCode[0]) {
    printOperand(MI, OpNo, O);
    return false;
  }

  // Otherwise fallback on the default implementation.
  return AsmPrinter::PrintAsmOperand(MI, OpNo, ExtraCode, O);
}

bool XtensaAsmPrinter::PrintAsmMemoryOperand(const MachineInstr *MI,
                                             unsigned OpNum,
                                             const char *ExtraCode,
                                             raw_ostream &O) {
  if (ExtraCode && ExtraCode[0]) {
    return true; // Unknown modifier.
  }
  printOperand(MI, OpNum, O);
  return false;
}

void XtensaAsmPrinter::EmitInstruction(const MachineInstr *MI) {
  SmallString<128> Str;
  raw_svector_ostream O(Str);

  std::vector<MCInst> MIV;
  // Comment in assembly code the instructions accessing ymemory.
  unsigned YmemoryInstructions = 0;
  bool ProcessLoopInst = false;
  StringRef LoopInstComment;
  const XtensaInstrInfo *TII = MI->getParent()
                                   ->getParent()
                                   ->getSubtarget<XtensaSubtarget>()
                                   .getInstrInfo();
  switch (MI->getOpcode()) {
  case Xtensa::BUNDLE: {
    // insert necessary "nop" required by the assembler, say, find out a
    // smallest format for the bundle and position the instructions to
    // proper slot and insert nops to empty slots
    const MachineBasicBlock *MBB = MI->getParent();
    MachineBasicBlock::const_instr_iterator I = ++MI->getIterator();
    while (I != MBB->instr_end() && I->isInsideBundle()) {
      // FIXME, this is hack, the pseudo instruction should
      // not be packetized at all or, the emit driver should take care
      // of them, even inside bundles
      if (!I->isDebugValue()) {
        MCInst TmpInst;
        MCInstLowering.Lower(&*I, TmpInst);
        MIV.push_back(TmpInst);

        if (TII->isYmemoryLoad(*I))
          ++YmemoryInstructions;
        if (GenZcl1iter && TII->isLoop((*I).getOpcode())) {
          ProcessLoopInst = true;
          ExtractLoopInstComment(&TmpInst, LoopInstComment);
        }
      }
      ++I;
    }
  } break;
  case Xtensa::ENDLOOP:
    // Skip the ENDLOOP pseudo instruction.
    if (needNopForEndloop(MI))
      OutStreamer->EmitRawText("\tnop\n");
    OutStreamer->EmitRawText("\t# endloop\n");
    return;
  case Xtensa::DBG_VALUE:
    llvm_unreachable("Should be handled target independently");
    break;
#if 0
   case Xtensa::BR_JT:
   case Xtensa::BR_JT32:
     O << "\tbru "
       << XtensaInstPrinter::getRegisterName(MI->getOperand(1).getReg()) <<
       '\n';
     if (MI->getOpcode() == Xtensa::BR_JT)
       printInlineJT(MI, 0, O);
     else
       printInlineJT32(MI, 0, O);
     O << '\n';
     OutStreamer.EmitRawText(O.str());
     return;
#endif
  default: {
    MCInst TmpInst;
    MCInstLowering.Lower(MI, TmpInst);
    MIV.push_back(TmpInst);

    if (TII->isYmemoryLoad(*MI))
      ++YmemoryInstructions;
    if (GenZcl1iter && TII->isLoop(MI->getOpcode())) {
      ProcessLoopInst = true;
      ExtractLoopInstComment(&TmpInst, LoopInstComment);
    }
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
  if (ProcessLoopInst) {
    const MachineBasicBlock *MBB = MI->getParent();
    loopBufferCheck(MBB, TII, LoopInstComment);
  }
  auto Bundle = Bundler.bundle(*TII, MIV);
  assert(Bundle.first != -1 && "Internal Compiler Error, illegal bundle");
  if (Bundle.second.size() > 1) {
    std::string FormatString(" {\t# format ");
    FormatString += Bundler.getFormatName(Bundle.first);
    FormatString += "\n";
    OutStreamer->EmitRawText(FormatString);
  }
  for (auto &MII : Bundle.second) {
    EmitToStreamer(*OutStreamer, MII);
  }
  if (Bundle.second.size() > 1)
    OutStreamer->EmitRawText(" }\n");
}

bool XtensaAsmPrinter::isBlockOnlyReachableByFallthrough(
    const MachineBasicBlock *MBB) const {
  if (MBB->hasAddressTaken())
    return false;
  return AsmPrinter::isBlockOnlyReachableByFallthrough(MBB);
}

// Force static initialization.
extern "C" void LLVMInitializeXtensaConfigAsmPrinter() {
  RegisterAsmPrinter<XtensaAsmPrinter> X(TheXtensaTarget);
}
