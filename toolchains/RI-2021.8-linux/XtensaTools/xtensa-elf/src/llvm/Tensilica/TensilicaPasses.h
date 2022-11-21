//===-- TensilicaPasses.h - Interface for Tensilica passes ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the entry points for Tensilica passes.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_CODEGEN_TENSILICA_TENSILICAPASSES_H
#define LLVM_LIB_CODEGEN_TENSILICA_TENSILICAPASSES_H
namespace llvm {
FunctionPass *createTensilicaSWP();
extern void initializeTensilicaSWPPass(PassRegistry &);
extern void initializeTensilicaRWMPass(PassRegistry &);
extern void initializeTensilicaRenameDisconnectedComponentPass(PassRegistry &);
extern void initializeTensilicaAddressFoldPass(PassRegistry &);
extern void initializeTensilicaIVReschedulePass(PassRegistry &);
extern void initializeTensilicaIVSimplifyPass(PassRegistry &);
extern void initializeTensilicaMemOffsetReusePass(PassRegistry &);
extern void initializeTensilicaPostRASchedulerPass(PassRegistry &);
extern void initializeRDFOptPass(PassRegistry &);
extern void initializeTensilicaPacketizerPass(PassRegistry &);
extern void initializeTensilicaBitSimplifyPass(PassRegistry &);
extern void initializeTensilicaRewritePass(PassRegistry &);
extern void initializeTensilicaIMAPPass(PassRegistry &);
extern void initializeTensilicaCopyPropagationPass(PassRegistry &);
extern char &TensilicaAddressFoldID;
extern char &TensilicaIVRescheduleID;
extern char &TensilicaIVSimplifyID;
extern char &TensilicaMemOffsetReuseID;
extern char &TensilicaRenameDisconnectedComponentID;
extern char &TensilicaSWPID;
extern char &TensilicaPostRASchedulerID;
extern char &RDFOptID;
extern char &TensilicaPacketizerID;
extern char &TensilicaRewriteID;
extern char &TensilicaIMAPID;
extern char &TensilicaCopyPropagationID;
FunctionPass *createTensilicaHardwareLoops();
FunctionPass *createTensilicaBitSimplify();
FunctionPass *createTensilicaRWMPass();
FunctionPass *createTensilicaHWLoopRepairPass(bool repairEndloop);
FunctionPass *createTensilicaMisalignedLdStOpt();
FunctionPass *createTensilicaVecPromotion();
}

static void initializeTensilicaPasses(llvm::PassRegistry &PR) {
  initializeTensilicaRWMPass(PR);
  initializeTensilicaBitSimplifyPass(PR);
  initializeTensilicaMemOffsetReusePass(PR);
  initializeTensilicaIVReschedulePass(PR);
  initializeTensilicaIVSimplifyPass(PR);
  initializeTensilicaAddressFoldPass(PR);
  initializeTensilicaPostRASchedulerPass(PR);
  initializeRDFOptPass(PR);
  initializeTensilicaPacketizerPass(PR);
  initializeTensilicaRewritePass(PR);
  initializeTensilicaIMAPPass(PR);
  initializeTensilicaCopyPropagationPass(PR);
  initializeTensilicaRenameDisconnectedComponentPass(PR);
}

#endif // LLVM_LIB_CODEGEN_TENSILICA_TENSILICAPASSES_H
