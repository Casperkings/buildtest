//===-- XtensaMCTargetDesc.cpp - Xtensa Target Descriptions----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file provides Xtensa specific target descriptions.
//
//===----------------------------------------------------------------------===//

#include "InstPrinter/XtensaInstPrinter.h"
#include "MCTargetDesc/XtensaMCTargetDesc.h"
#include "MCTargetDesc/XtensaMCAsmInfo.h"
#include "XtensaTargetStreamer.h"
#include "llvm/MC/MCDwarf.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

#define GET_INSTRINFO_MC_DESC
#include "XtensaGenInstrInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "XtensaGenSubtargetInfo.inc"

#define GET_REGINFO_MC_DESC
#include "XtensaGenRegisterInfo.inc"

static MCInstrInfo *createXtensaMCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitXtensaMCInstrInfo(X);
  return X;
}

static MCRegisterInfo *createXtensaMCRegisterInfo(const Triple &TT) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitXtensaMCRegisterInfo(X, Xtensa::LR);
  return X;
}

static MCSubtargetInfo *
createXtensaMCSubtargetInfo(const Triple &TT, StringRef CPU, StringRef FS) {
  return createXtensaMCSubtargetInfoImpl(TT, CPU, FS);
}

static MCAsmInfo *createXtensaMCAsmInfo(const MCRegisterInfo &MRI,
                                        const Triple &TT,
                                        const MCTargetOptions &) {
  MCAsmInfo *MAI = new XtensaMCAsmInfo(TT);

  // Initial state of the frame pointer is SP.
  MCCFIInstruction Inst =
      MCCFIInstruction::createDefCfa(nullptr, Xtensa::SP, 0);
  MAI->addInitialFrameState(Inst);

  return MAI;
}

static MCInstPrinter *createXtensaMCInstPrinter(const Triple &T,
                                                unsigned SyntaxVariant,
                                                const MCAsmInfo &MAI,
                                                const MCInstrInfo &MII,
                                                const MCRegisterInfo &MRI) {
  return new XtensaInstPrinter(MAI, MII, MRI);
}

XtensaTargetStreamer::XtensaTargetStreamer(MCStreamer &S)
    : MCTargetStreamer(S) {}
XtensaTargetStreamer::~XtensaTargetStreamer() {}

namespace {

class XtensaTargetAsmStreamer : public XtensaTargetStreamer {
  formatted_raw_ostream &OS;

public:
  XtensaTargetAsmStreamer(MCStreamer &S, formatted_raw_ostream &OS);
  // virtual void emitCCTopData(StringRef Name) override;
  // virtual void emitCCTopFunction(StringRef Name) override;
  // virtual void emitCCBottomData(StringRef Name) override;
  // virtual void emitCCBottomFunction(StringRef Name) override;
};

XtensaTargetAsmStreamer::XtensaTargetAsmStreamer(MCStreamer &S,
                                                 formatted_raw_ostream &OS)
    : XtensaTargetStreamer(S), OS(OS) {}

// void XtensaTargetAsmStreamer::emitCCTopData(StringRef Name) {
//   OS << "\t.cc_top " << Name << ".data," << Name << '\n';
// }

// void XtensaTargetAsmStreamer::emitCCTopFunction(StringRef Name) {
//   OS << "\t.cc_top " << Name << ".function," << Name << '\n';
// }

// void XtensaTargetAsmStreamer::emitCCBottomData(StringRef Name) {
//   OS << "\t.cc_bottom " << Name << ".data\n";
// }

// void XtensaTargetAsmStreamer::emitCCBottomFunction(StringRef Name) {
//   OS << "\t.cc_bottom " << Name << ".function\n";
// }
}
#if 0
static MCStreamer *
createXtensaMCAsmStreamer(MCContext &Ctx, formatted_raw_ostream &OS,
                         bool isVerboseAsm, bool useDwarfDirectory,
                         MCInstPrinter *InstPrint, MCCodeEmitter *CE,
                         MCAsmBackend *TAB, bool ShowInst) {
  MCStreamer *S = llvm::createAsmStreamer(
      Ctx, OS, isVerboseAsm, useDwarfDirectory, InstPrint, CE, TAB, ShowInst);
  new XtensaTargetAsmStreamer(*S, OS);
  return S;
}
#endif

static MCTargetStreamer *createTargetAsmStreamer(MCStreamer &S,
                                                 formatted_raw_ostream &OS,
                                                 MCInstPrinter *InstPrint,
                                                 bool isVerboseAsm) {
  return new XtensaTargetAsmStreamer(S, OS);
}

// Force static initialization.
extern "C" void LLVMInitializeXtensaConfigTargetMC() {
  // Register the MC asm info.
  RegisterMCAsmInfoFn X(TheXtensaTarget, createXtensaMCAsmInfo);

  // Register the MC instruction info.
  TargetRegistry::RegisterMCInstrInfo(TheXtensaTarget, createXtensaMCInstrInfo);

  // Register the MC register info.
  TargetRegistry::RegisterMCRegInfo(TheXtensaTarget,
                                    createXtensaMCRegisterInfo);

  // Register the MC subtarget info.
  TargetRegistry::RegisterMCSubtargetInfo(TheXtensaTarget,
                                          createXtensaMCSubtargetInfo);

  // Register the MCInstPrinter
  TargetRegistry::RegisterMCInstPrinter(TheXtensaTarget,
                                        createXtensaMCInstPrinter);

  TargetRegistry::RegisterAsmTargetStreamer(TheXtensaTarget,
                                            createTargetAsmStreamer);
}
