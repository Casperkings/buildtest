//===-- XtensaTargetMachine.h - Define TargetMachine for Xtensa -----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares the Xtensa specific subclass of TargetMachine.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_XTENSA_XTENSATARGETMACHINE_H
#define LLVM_LIB_TARGET_XTENSA_XTENSATARGETMACHINE_H

#include "XtensaSubtarget.h"
#include "llvm/Target/TargetIntrinsicInfo.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {

class XtensaTargetMachine : public LLVMTargetMachine {
  std::unique_ptr<TargetLoweringObjectFile> TLOF;
  mutable StringMap<std::unique_ptr<XtensaGenSubtargetInfo>> SubtargetMap;
  std::unique_ptr<TargetIntrinsicInfo> IntrinsicInfo;

public:
  XtensaTargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                      StringRef FS, const TargetOptions &Options,
                      Optional<Reloc::Model> RM, Optional<CodeModel::Model> CM,
                      CodeGenOpt::Level OL, bool JIT);
  ~XtensaTargetMachine() override;

  const XtensaSubtarget *getSubtargetImpl(const Function &) const override;

  const TargetIntrinsicInfo *getIntrinsicInfo() const override {
    return IntrinsicInfo.get();
  }

  TargetTransformInfo getTargetTransformInfo(const Function &F) override;

  // Pass Pipeline Configuration
  TargetPassConfig *createPassConfig(PassManagerBase &PM) override;

  TargetLoweringObjectFile *getObjFileLowering() const override {
    return TLOF.get();
  }
};

} // end namespace llvm

#endif
