//===-- XtensaTargetMachine.cpp - Define TargetMachine for Xtensa----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "Tensilica/TensilicaMachineScheduler.h"
#include "XtensaTargetMachine.h"
#include "Xtensa.h"
#include "Tensilica/TensilicaPasses.h"
#include "XtensaConfig/XtensaConfigInfo.h"
#include "XtensaIntrinsicInfo.h"
#include "XtensaTargetObjectFile.h"
#include "XtensaTargetTransformInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Utils.h"

using namespace llvm;

static cl::opt<bool>
    EnableXtensaAADebugPass("xtensa-aa-debug",
                            cl::desc("Enable Xtensa AA debug pass"),
                            cl::init(false), cl::Hidden);

static cl::opt<bool> EnableTensilicaBitSimplify("ten-bit-tracker",
                                                cl::init(true), cl::Hidden,
                                                cl::desc("Bit simplification"));

static cl::opt<bool> EnableTensilicaIMAPPass("ten-imap",
                                          cl::desc("Enable Xtensa IMAP pass"),
                                          cl::init(true), cl::Hidden);

static cl::opt<bool>
    EnableTensilicaRewritePass("ten-rewrite",
                            cl::desc("Enable Tensilica rewrite pass"),
                            cl::init(true), cl::Hidden);

static cl::opt<bool> IVResched("xtensa-iv-resched",
                               cl::desc("Reschedule IV updates"),
                               cl::init(true), cl::Hidden);

static cl::opt<bool> IVSimp("xtensa-iv-simp",
                            cl::desc("Xtensa IV simplify"),
                            cl::init(true), cl::Hidden);

static cl::opt<int>
    XtensaScheduleHeuristic("xtensa-misched", cl::ReallyHidden, cl::init(0),
                            cl::ZeroOrMore,
                            cl::desc("machine scheduler heuristic"));

static cl::opt<int>
    XtensaPostScheduleHeuristic("xtensa-post-misched", cl::ReallyHidden,
                                cl::init(0), cl::ZeroOrMore,
                                cl::desc("post machine scheduler heuristic"));

static cl::opt<bool>
    EnableEarlyIfConversion("xtensa-early-ifcvt",
                            cl::desc("Enable early if-conversion"),
                            cl::init(true), cl::Hidden);

static cl::opt<bool>
    EnableMachineCombinerPass("xtensa-machine-combiner",
                              cl::desc("Enable the machine combiner pass"),
                              cl::init(true), cl::Hidden);

static cl::opt<int>
    GlobalMergeMaxOffset("xtensa-gmerge-max-offset", cl::ReallyHidden,
                         cl::init(1024), cl::ZeroOrMore,
                         cl::desc("Max offset in the merged global"));

static cl::opt<bool>
    EnableTensilicaHWLoops("tensilica-hwloops",
                           cl::desc("Enable Tensilica Hardware Loops pass"),
                           cl::init(false), cl::Hidden);

static cl::opt<bool> EnableRDFOpt("xtensa-rdf-opt", cl::Hidden, cl::ZeroOrMore,
  cl::init(true), cl::desc("Enable RDF-based optimizations"));

static Reloc::Model getEffectiveRelocModel(Optional<Reloc::Model> RM) {
  if (!RM.hasValue())
    return Reloc::Static;
  return *RM;
}

#if 0 // zzz_merge: use one from include/llvm/Target/TargetMachine.h
static CodeModel::Model getEffectiveCodeModel(Optional<CodeModel::Model> CM) {
  if (CM) {
    if (*CM != CodeModel::Small && *CM != CodeModel::Large)
      report_fatal_error("Target only supports CodeModel Small or Large");
    return *CM;
  }
  return CodeModel::Small;
}
#endif

static std::string GetDataLayoutString(bool isLE) {
  std::string R("e-m:e-p:32:32-i1:8:32-i8:8:32-i16:16:32"
                "-i64:64-i128:128-i256:256-i512:512-i1024:1024"
                "-f64:64-a:0:32-n32-S128");
  if (!isLE)
    R[0] = 'E';
  return R;
}

static TargetIntrinsicInfo *createXtensaTargetIntrinsicInfo() {
  TargetIntrinsicInfo *TTI = new XtensaIntrinsicInfo();
  return TTI;
}

/// XtensaTargetMachine ctor - Create an ILP32 architecture model
///
XtensaTargetMachine::XtensaTargetMachine(const Target &T, const Triple &TT,
                                         StringRef CPU, StringRef FS,
                                         const TargetOptions &Options,
                                         Optional<Reloc::Model> RM,
                                         Optional<CodeModel::Model> CM,
                                         CodeGenOpt::Level OL, bool JIT)
    : LLVMTargetMachine(
          T,
          GetDataLayoutString(
              LLVMInitializeXtensaConfigInfo(FS, Options)->IsLittleEndian),
          TT, CPU, FS, Options, getEffectiveRelocModel(RM),
          getEffectiveCodeModel(CM, CodeModel::Small), OL),
      TLOF(std::make_unique<XtensaTargetObjectFile>()),
      IntrinsicInfo(createXtensaTargetIntrinsicInfo()) {
  // By default (and when -ffast-math is on), enable estimate codegen, and use
  // 0 refinement step for all operations. Defaults may be overridden by using
  // command-line options.

  initAsmInfo();
}

XtensaTargetMachine::~XtensaTargetMachine() {}

TargetTransformInfo
XtensaTargetMachine::getTargetTransformInfo(const Function &F) {
  return TargetTransformInfo(XtensaTTIImpl(this, F));
}

const XtensaSubtarget *
XtensaTargetMachine::getSubtargetImpl(const Function &F) const {
  Attribute CPUAttr = F.getFnAttribute("target-cpu");
  Attribute FSAttr = F.getFnAttribute("target-features");

  std::string CPU = !CPUAttr.hasAttribute(Attribute::None)
                        ? CPUAttr.getValueAsString().str()
                        : TargetCPU;
  std::string FS = !FSAttr.hasAttribute(Attribute::None)
                       ? FSAttr.getValueAsString().str()
                       : TargetFS;

  auto &I = SubtargetMap[CPU + FS];
  if (!I) {
    // This needs to be done before we create a new subtarget since any
    // creation will depend on the TM and the code generation flags on the
    // function that reside in TargetOptions.
    resetTargetOptions(F);
    I = std::make_unique<XtensaGenSubtargetInfo>(
        TargetTriple, CPU, FS, "", *this,
        LLVMInitializeXtensaConfigInfo(FS, Options));
  }
  return I.get();
}

static ScheduleDAGInstrs *createFLIXMachineSched(MachineSchedContext *C) {
  if (XtensaScheduleHeuristic == 0)
    return new ScheduleDAGMILive(C, std::make_unique<FLIXScheduler>(C));
  else
    return new FLIXMachineScheduler(C,
                                    std::make_unique<ConvergingFLIXScheduler>(
                                        llvm::ConvergingFLIXScheduler::PreRA));
}

static ScheduleDAGInstrs *createPostMachineSched(MachineSchedContext *C) {
  return new ScheduleDAGMI(C,
                           std::make_unique<ConvergingFLIXScheduler>(
                               llvm::ConvergingFLIXScheduler::PostRA),
                           /*IsPostRA=*/true);
}

static MachineSchedRegistry SchedCustomRegistry("xtensa",
                                                "Run Xtensa's custom scheduler",
                                                createFLIXMachineSched);

namespace {
/// Xtensa Code Generator Pass Configuration Options.
class XtensaPassConfig : public TargetPassConfig {
public:
  XtensaPassConfig(XtensaTargetMachine *TM, PassManagerBase &PM)
      : TargetPassConfig(*TM, PM) {
    if (XtensaPostScheduleHeuristic == 0)
      substitutePass(&PostRASchedulerID, &TensilicaPostRASchedulerID);
    else
      substitutePass(&PostRASchedulerID, &PostMachineSchedulerID);
  }

  XtensaTargetMachine &getXtensaTargetMachine() const {
    return getTM<XtensaTargetMachine>();
  }

  ScheduleDAGInstrs *
  createMachineScheduler(MachineSchedContext *C) const override {
    return createFLIXMachineSched(C);
  }

  ScheduleDAGInstrs *
  createPostMachineScheduler(MachineSchedContext *C) const override {
    return createPostMachineSched(C);
  }

  void addIRPasses() override;
  bool addPreISel() override;
  bool addILPOpts() override;
  bool addInstSelector() override;
  void addPreRegAlloc() override;
  void addPostRegAlloc() override;
  void addPreSched2() override;
  void addBlockPlacement() override;
  void addPreEmitPass() override;
};
} // namespace

namespace llvm {
FunctionPass *createXtensaGenericPromotionPass();
FunctionPass *createXtensaGenericSpecializationPass();
FunctionPass *createXtensaHardwareLoops();
FunctionPass *createXtensaInfHardwareLoops();
FunctionPass *createXtensaDebugInfoFixPass();
FunctionPass *createXtensaAADebug();
FunctionPass *createTensilicaHardwareLoops();
FunctionPass *createXtensaFunctionStageAdjust();
extern char &XtensaAADebugID;
void XtensaAADebugPass(PassRegistry &);
extern char &XtensaDeadCodeEliminationID;
void XtensaDeadCodeEliminationPass(PassRegistry &);
extern char &LateBranchFolderPassID;
} // namespace llvm

TargetPassConfig *XtensaTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new XtensaPassConfig(this, PM);
}

void XtensaPassConfig::addIRPasses() {
  addPass(createAtomicExpandPass());
  if (TM->getOptLevel() != CodeGenOpt::None) {
    addPass(createInterleavedAccessPass());
    addPass(
        createInstructionCombiningPass(false, true /*simplify intrinsics*/));
    addPass(createDeadInstEliminationPass());
  }

  // Improve vector reuse across loop iterations.
  if (getOptLevel() == CodeGenOpt::Aggressive)
    addPass(createGVNPass());

  if (TM->getOptLevel() != CodeGenOpt::None) {
    addPass(createLoopSimplifyPass());
    // Transform ctype loads into llvm vector loads
    addPass(createTensilicaInstCanonicalizationPass(/*ForVectorization*/ false));
    addPass(createTensilicaMisalignedLdStOpt());
    addPass(createCFGSimplificationPass());
    // Apply dead code elimination
    addPass(createAggressiveDCEPass());

    addPass(createTensilicaVecPromotion());
    addPass(createCFGSimplificationPass());
  }

  TargetPassConfig::addIRPasses();
}

bool XtensaPassConfig::addPreISel() {
  if (getOptLevel() != CodeGenOpt::None) {
    addPass(createGlobalMergePass(TM, GlobalMergeMaxOffset,
                                  false, /* Only optimize for size */
                                  true /* Merge external by default */));
  }
  if (EnableTensilicaHWLoops && getOptLevel() != CodeGenOpt::None)
    addPass(createHardwareLoopsPass());

  addPass(createXtensaLowerThreadLocalPass());
  return false;
}

bool XtensaPassConfig::addILPOpts() {
  assert((getOptLevel() != CodeGenOpt::None) && "Unexpected opt level");

  if (EnableTensilicaHWLoops)
    addPass(createTensilicaHardwareLoops());

  addPass(createTensilicaRWMPass());

  if (EnableTensilicaBitSimplify) {
    addPass(createTensilicaBitSimplify());
    addPass(&llvm::UnreachableMachineBlockElimID, true, true);
  }

  if (EnableEarlyIfConversion)
    addPass(&EarlyIfConverterID);

  if (EnableTensilicaRewritePass) {
    addPass(&llvm::TensilicaRewriteID, /* verify after */ true, true);
    addPass(&llvm::DeadMachineInstructionElimID, false, true);
  }

  if (EnableMachineCombinerPass)
    addPass(&MachineCombinerID);
  return true;
}

bool XtensaPassConfig::addInstSelector() {
  addPass(createXtensaISelDag(getXtensaTargetMachine(), getOptLevel()));
  addPass(createXtensaGenericPromotionPass());
  return false;
}

namespace llvm {
extern void initializeXtensaAADebugPass(PassRegistry &);
extern void initializeXtensaHardwareLoopsPass(PassRegistry &);
extern void initializeXtensaInfHardwareLoopsPass(PassRegistry &);
extern void initializeXtensaGenericPass(PassRegistry &);
extern void initializeXtensaDeadCodeEliminationPass(PassRegistry &);
extern void initializeXtensaFTAOElimPass(PassRegistry &);
extern void initializeXtensaFunctionStageAdjustPass(llvm::PassRegistry &);
extern void initializeLateBranchFolderPassPass(PassRegistry &);
} // namespace llvm

void XtensaPassConfig::addPreRegAlloc() {
  if (getOptLevel() != CodeGenOpt::None) {
    auto AddIVPasses = [&] () {
      if (IVResched)
        addPass(&llvm::TensilicaIVRescheduleID, true, true);
      if (IVSimp) {
        addPass(&llvm::TensilicaIVSimplifyID, true, true);

        addPass(&llvm::LiveVariablesID, false, true);
        addPass(&llvm::TensilicaAddressFoldID, true, true);

        // Sink some new insns from addressing mode changes
        addPass(&MachineSinkingID);
      }
    };
    AddIVPasses();
    addPass(createXtensaHardwareLoops(), false);
    addPass(createXtensaInfHardwareLoops(), false);

    addPass(&llvm::TensilicaMemOffsetReuseID, false, true);
    addPass(&llvm::LiveVariablesID, false, true);
    addPass(&llvm::DeadMachineInstructionElimID, false, true);

    if (EnableTensilicaIMAPPass) {
      addPass(&llvm::TensilicaIMAPID, /* verify after */ true, true);
      addPass(&llvm::LiveVariablesID, false, true);
      addPass(&llvm::DeadMachineInstructionElimID, false, true);

      // Some IMAP output instructions can be further simplified.
      if (EnableTensilicaBitSimplify) {
        addPass(createTensilicaBitSimplify());
        addPass(&llvm::UnreachableMachineBlockElimID, true, true);
      }

      AddIVPasses();
      // IMAP patterns may create new invariants. Try to hoist them.
      addPass(&llvm::MachineLICMID, false, true);
    }
  }

  // Insert AA debug pass just before the pipeliner
  if (EnableXtensaAADebugPass) {
    initializeXtensaAADebugPass(*PassRegistry::getPassRegistry());
    insertPass(&RenameIndependentSubregsID, &llvm::XtensaAADebugID);
  }

  addPass(createXtensaFunctionStageAdjust());

  // Enable SWP right before MachineScheduler, at -O2 or more
  if (getOptLevel() == CodeGenOpt::Default ||
      getOptLevel() == CodeGenOpt::Aggressive) {
    initializeTensilicaSWPPass(*PassRegistry::getPassRegistry());
    if (EnableXtensaAADebugPass)
      insertPass(&llvm::XtensaAADebugID, &llvm::TensilicaSWPID);
    else
      insertPass(&RenameIndependentSubregsID, &llvm::TensilicaSWPID);
    insertPass(&llvm::TensilicaSWPID, &llvm::TensilicaRenameDisconnectedComponentID);
#if 0
    insertPass(&llvm::TensilicaRenameDisconnectedComponentID,
               &llvm::DeadMachineInstructionElimID);
    insertPass(&llvm::DeadMachineInstructionElimID,
               &llvm::XtensaDeadCodeEliminationID);
#endif
  }

  // Enable SWP on SSA Form
  // addPass(createTensilicaSWP(), false);
}

void XtensaPassConfig::addPostRegAlloc() {
}

void XtensaPassConfig::addPreSched2() {
  addPass(createXtensaGenericPromotionPass());
}

void XtensaPassConfig::addBlockPlacement() {
  assert((getOptLevel() != CodeGenOpt::None) && "Unexpected opt level");
  addPass(createTensilicaHWLoopRepairPass(/*endloop*/ false));
  TargetPassConfig::addBlockPlacement();
}

void XtensaPassConfig::addPreEmitPass() {
  addPass(createXtensaFrameToArgsOffsetEliminationPass(), false);
  addPass(createXtensaGenericSpecializationPass());
  if (getOptLevel() != CodeGenOpt::None) {
    if (EnableRDFOpt)
      addPass(&llvm::RDFOptID);
    // Copy propagation/forward substitution pass to enable more packetization.
    addPass(&llvm::TensilicaCopyPropagationID, false);
    addPass(createTensilicaHWLoopRepairPass(/*endloop*/ true), false);
    addPass(&llvm::TensilicaPacketizerID, false);
    addPass(&llvm::LateBranchFolderPassID, false);
  }
  addPass(createXtensaDebugInfoFixPass(), false);
}

// Force static initialization.
extern "C" void LLVMInitializeXtensaConfigTarget() {
  RegisterTargetMachine<XtensaTargetMachine> X(TheXtensaTarget);

  // Register the instrinsic info
  RegisterTargetIntrinsicInfoFn RegIntrinsicInfo(
      TheXtensaTarget, createXtensaTargetIntrinsicInfo);

  // Register some of the passes for testing (llc -run-pass=...).
  auto PR = PassRegistry::getPassRegistry();

  initializeTensilicaPasses(*PR);

  initializeXtensaHardwareLoopsPass(*PassRegistry::getPassRegistry());

  initializeXtensaInfHardwareLoopsPass(*PassRegistry::getPassRegistry());

  initializeXtensaGenericPass(*PassRegistry::getPassRegistry());

  initializeXtensaFunctionStageAdjustPass(*PassRegistry::getPassRegistry());

  initializeXtensaFTAOElimPass(*PassRegistry::getPassRegistry());

  initializeXtensaDeadCodeEliminationPass(*PassRegistry::getPassRegistry());

  initializeLateBranchFolderPassPass(*PassRegistry::getPassRegistry());
}
