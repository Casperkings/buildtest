//===-- RISCVTargetMachine.cpp - Define TargetMachine for RISCV -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Implements the info about RISCV target spec.
//
//===----------------------------------------------------------------------===//

#include "RISCVTargetMachine.h"
#include "RISCV.h"
#if defined(TENSILICA) || 1
#include "Tensilica/TensilicaPasses.h"
#include "RISCVConfig/RISCVConfigInfo.h"
#include "RISCVIntrinsicInfo.h"
#endif
#include "RISCVTargetObjectFile.h"
#include "RISCVTargetTransformInfo.h"
#include "TargetInfo/RISCVTargetInfo.h"
#include "Utils/RISCVBaseInfo.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/CodeGen/GlobalISel/IRTranslator.h"
#include "llvm/CodeGen/GlobalISel/InstructionSelect.h"
#include "llvm/CodeGen/GlobalISel/Legalizer.h"
#include "llvm/CodeGen/GlobalISel/RegBankSelect.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/InitializePasses.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Target/TargetOptions.h"
#if defined(TENSILICA) || 1
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Utils.h"
#endif

using namespace llvm;

#if defined(TENSILICA) || 1
static cl::opt<bool> EnablePacketizer("ten-packetizer",
                                      cl::desc("Enable Packetizer"),
                                      cl::init(true), cl::Hidden);

static cl::opt<bool> EnableTensilicaBitSimplify("ten-bit-tracker",
                                                cl::init(true), cl::Hidden,
                                                cl::desc("Bit simplification"));

static cl::opt<bool>
    EnableTensilicaRewritePass("ten-rewrite",
                            cl::desc("Enable Tensilica rewrite pass"),
                            cl::init(true), cl::Hidden);

static cl::opt<bool> EnableSWP("ten-swp", cl::desc("Enable SWP"),
                               cl::init(true), cl::Hidden);

static cl::opt<bool>
    EnableTensilicaHWLoops("tensilica-hwloops",
                           cl::desc("Enable Tensilica Hardware Loops pass"),
                           cl::init(true), cl::Hidden);
static cl::opt<bool>
    EnableEarlyIfConversion("ten-early-ifcvt",
                            cl::desc("Enable early if-conversion"),
                            cl::init(true), cl::Hidden);

static cl::opt<bool> IVResched("ten-iv-resched",
                               cl::desc("Reschedule IV updates"),
                               cl::init(false), cl::Hidden);

static cl::opt<bool> IVSimp("ten-iv-simp",
                            cl::desc("Enable Tensilica IV simplify"),
                            cl::init(false), cl::Hidden);

static cl::opt<bool> EnableIMAP("ten-imap",
                                cl::desc("Enable IMAP"),
                                cl::init(true), cl::Hidden);

static cl::opt<bool>
    PostScheduleHeuristic("ten-post-misched",
                          cl::desc("Post RA scheduler heuristic"), cl::init(0),
                          cl::ZeroOrMore, cl::Hidden);

static TargetIntrinsicInfo *createRISCVTargetIntrinsicInfo() {
  TargetIntrinsicInfo *TTI = new RISCVIntrinsicInfo();
  return TTI;
}
#endif

#if defined(TENSILICA) || 1
extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeRISCVConfigTarget() {
#else
extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeRISCVTarget() {
#endif
  RegisterTargetMachine<RISCVTargetMachine> X(getTheRISCV32Target());
  RegisterTargetMachine<RISCVTargetMachine> Y(getTheRISCV64Target());

#if defined(TENSILICA) || 1
  // Register instrinsic info
  RegisterTargetIntrinsicInfoFn RegIntrinsicInfoX(
      getTheRISCV32Target(), createRISCVTargetIntrinsicInfo);
  RegisterTargetIntrinsicInfoFn RegIntrinsicInfoY(
      getTheRISCV64Target(), createRISCVTargetIntrinsicInfo);
#endif

  auto PR = PassRegistry::getPassRegistry();
  initializeGlobalISel(*PR);
  initializeRISCVExpandPseudoPass(*PR);
#if defined(TENSILICA) || 1
  initializeTensilicaPasses(*PR);
#endif
}

static StringRef computeDataLayout(const Triple &TT) {
  if (TT.isArch64Bit()) {
    return "e-m:e-p:64:64-i64:64-i128:128-n64-S128";
  } else {
    assert(TT.isArch32Bit() && "only RV32 and RV64 are currently supported");
    return "e-m:e-p:32:32-i64:64-n32-S128";
  }
}

static Reloc::Model getEffectiveRelocModel(const Triple &TT,
                                           Optional<Reloc::Model> RM) {
  if (!RM.hasValue())
    return Reloc::Static;
  return *RM;
}

RISCVTargetMachine::RISCVTargetMachine(const Target &T, const Triple &TT,
                                       StringRef CPU, StringRef FS,
                                       const TargetOptions &Options,
                                       Optional<Reloc::Model> RM,
                                       Optional<CodeModel::Model> CM,
                                       CodeGenOpt::Level OL, bool JIT)
    : LLVMTargetMachine(T, computeDataLayout(TT), TT, CPU, FS, Options,
                        getEffectiveRelocModel(TT, RM),
                        getEffectiveCodeModel(CM, CodeModel::Small), OL),
#if defined(TENSILICA) || 1
      IntrinsicInfo(createRISCVTargetIntrinsicInfo()),
#endif
      TLOF(std::make_unique<RISCVELFTargetObjectFile>()) {
  initAsmInfo();

  // RISC-V supports the MachineOutliner.
  setMachineOutliner(true);
}

const RISCVSubtarget *
RISCVTargetMachine::getSubtargetImpl(const Function &F) const {
  Attribute CPUAttr = F.getFnAttribute("target-cpu");
  Attribute FSAttr = F.getFnAttribute("target-features");

  std::string CPU = !CPUAttr.hasAttribute(Attribute::None)
                        ? CPUAttr.getValueAsString().str()
                        : TargetCPU;
  std::string FS = !FSAttr.hasAttribute(Attribute::None)
                       ? FSAttr.getValueAsString().str()
                       : TargetFS;
  std::string Key = CPU + FS;
  auto &I = SubtargetMap[Key];
  if (!I) {
    // This needs to be done before we create a new subtarget since any
    // creation will depend on the TM and the code generation flags on the
    // function that reside in TargetOptions.
    resetTargetOptions(F);
    auto ABIName = Options.MCOptions.getABIName();
    if (const MDString *ModuleTargetABI = dyn_cast_or_null<MDString>(
            F.getParent()->getModuleFlag("target-abi"))) {
      auto TargetABI = RISCVABI::getTargetABI(ABIName);
      if (TargetABI != RISCVABI::ABI_Unknown &&
          ModuleTargetABI->getString() != ABIName) {
        report_fatal_error("-target-abi option != target-abi module flag");
      }
      ABIName = ModuleTargetABI->getString();
    }
#if defined(TENSILICA) || 1
    bool Is64Bit = TargetTriple.isArch64Bit();
    I = std::make_unique<RISCVGenSubtargetInfo>(
          TargetTriple, CPU, FS, ABIName,
          *this, LLVMInitializeRISCVConfigInfo(TargetTriple, CPU, FS, ABIName,
                                               Is64Bit ? 2 : 1));
#else
    I = std::make_unique<RISCVSubtarget>(TargetTriple, CPU, FS, ABIName, *this);
#endif
  }
  return I.get();
}

TargetTransformInfo
RISCVTargetMachine::getTargetTransformInfo(const Function &F) {
  return TargetTransformInfo(RISCVTTIImpl(this, F));
}

namespace {
class RISCVPassConfig : public TargetPassConfig {
public:
#if defined(TENSILICA) || 1
  RISCVPassConfig(RISCVTargetMachine &TM, PassManagerBase &PM)
      : TargetPassConfig(TM, PM) {
    if (PostScheduleHeuristic == 0)
      substitutePass(&PostRASchedulerID, &TensilicaPostRASchedulerID);
  }
#else
  RISCVPassConfig(RISCVTargetMachine &TM, PassManagerBase &PM)
      : TargetPassConfig(TM, PM) {}
#endif

  RISCVTargetMachine &getRISCVTargetMachine() const {
    return getTM<RISCVTargetMachine>();
  }

  void addIRPasses() override;
#if defined(TENSILICA) || 1
  bool addPreISel() override;
  bool addILPOpts() override;
  void addBlockPlacement() override;
  void addMachineSSAOptimization() override;
#endif
  bool addInstSelector() override;
  bool addIRTranslator() override;
  bool addLegalizeMachineIR() override;
  bool addRegBankSelect() override;
  bool addGlobalInstructionSelect() override;
  void addPreEmitPass() override;
  void addPreEmitPass2() override;
  void addPreRegAlloc() override;
};
}


TargetPassConfig *RISCVTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new RISCVPassConfig(*this, PM);
}

void RISCVPassConfig::addIRPasses() {
  addPass(createAtomicExpandPass());
#if defined(TENSILICA) || 1
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
    addPass(
        createTensilicaInstCanonicalizationPass(/*ForVectorization*/ false));
    addPass(createTensilicaMisalignedLdStOpt());
    addPass(createCFGSimplificationPass());
    // Apply dead code elimination
    addPass(createAggressiveDCEPass());

    addPass(createTensilicaVecPromotion());
    addPass(createCFGSimplificationPass());
  }
#endif
  TargetPassConfig::addIRPasses();
}

#if defined(TENSILICA) || 1
bool RISCVPassConfig::addPreISel() {
  if (EnableTensilicaHWLoops && getOptLevel() != CodeGenOpt::None)
    addPass(createHardwareLoopsPass());
  return false;
}

void RISCVPassConfig::addMachineSSAOptimization() {
  TargetPassConfig::addMachineSSAOptimization();
  addPass(&llvm::LocalStackSlotAllocationID, true, true);
}
#endif

bool RISCVPassConfig::addInstSelector() {
#if defined(TENSILICA) || 1
  addPass(createRISCVISelDag(getRISCVTargetMachine(), getOptLevel()));
#else
  addPass(createRISCVISelDag(getRISCVTargetMachine()));
#endif

  return false;
}

bool RISCVPassConfig::addIRTranslator() {
  addPass(new IRTranslator());
  return false;
}

bool RISCVPassConfig::addLegalizeMachineIR() {
  addPass(new Legalizer());
  return false;
}

#if defined(TENSILICA) || 1
bool RISCVPassConfig::addILPOpts() {
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
  return true;

  if (EnableTensilicaRewritePass) {
    using namespace llvm;
    addPass(&TensilicaRewriteID, /* verify after */ true, true);
    addPass(&llvm::DeadMachineInstructionElimID, false, true);
  }
}

void RISCVPassConfig::addBlockPlacement() {
  assert((getOptLevel() != CodeGenOpt::None) && "Unexpected opt level");
  if (EnableTensilicaHWLoops)
    addPass(createTensilicaHWLoopRepairPass(/*endloop*/ false));
  TargetPassConfig::addBlockPlacement();
}
#endif

bool RISCVPassConfig::addRegBankSelect() {
  addPass(new RegBankSelect());
  return false;
}

bool RISCVPassConfig::addGlobalInstructionSelect() {
  addPass(new InstructionSelect());
  return false;
}

void RISCVPassConfig::addPreEmitPass() { addPass(&BranchRelaxationPassID); }

void RISCVPassConfig::addPreEmitPass2() {
#if defined(TENSILICA) || 1
  if (getOptLevel() != CodeGenOpt::None) {
    if (EnableTensilicaHWLoops)
      addPass(createTensilicaHWLoopRepairPass(/*endloop*/ true));
    if (EnablePacketizer) {
      addPass(&llvm::TensilicaCopyPropagationID, false);
      addPass(&llvm::RDFOptID);
      addPass(&llvm::TensilicaPacketizerID, false);
    }
  }
#endif
  // Schedule the expansion of AMOs at the last possible moment, avoiding the
  // possibility for other passes to break the requirements for forward
  // progress in the LR/SC block.
  addPass(createRISCVExpandPseudoPass());
}

void RISCVPassConfig::addPreRegAlloc() {
  addPass(createRISCVMergeBaseOffsetOptPass());
#if defined(TENSILICA) || 1
  auto AddIVPasses = [&]() {
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
  addPass(&llvm::TensilicaMemOffsetReuseID, false, true);

  if (EnableIMAP && getOptLevel() != CodeGenOpt::None) {
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

  if (EnableSWP && (getOptLevel() == CodeGenOpt::Default ||
                    getOptLevel() == CodeGenOpt::Aggressive)) {
    initializeTensilicaSWPPass(*PassRegistry::getPassRegistry());
    insertPass(&RenameIndependentSubregsID, &llvm::TensilicaSWPID);
    insertPass(&llvm::TensilicaSWPID, &llvm::TensilicaRenameDisconnectedComponentID);
#if 0
    if (EnableTensilicaAADebugPass)
      insertPass(&llvm::TensilicaAADebugID, &llvm::TensilicaSWPID);
    else
      insertPass(&RenameIndependentSubregsID, &llvm::TensilicaSWPID);
    insertPass(&llvm::TensilicaRenameDisconnectedComponentID,
               &llvm::DeadMachineInstructionElimID);
    insertPass(&llvm::DeadMachineInstructionElimID,
               &llvm::TensilicaDeadCodeEliminationID);
#endif
  }
#endif
}
