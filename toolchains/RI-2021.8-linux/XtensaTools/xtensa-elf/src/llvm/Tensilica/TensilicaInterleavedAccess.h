//===-- Tensilica/TensilicaInterleavedAccess.h ----------------------------===//
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

#ifndef LLVM_LIB_CODEGEN_TENSILICA_TENSILICAINTERLEAVEDACCESS_H
#define LLVM_LIB_CODEGEN_TENSILICA_TENSILICAINTERLEAVEDACCESS_H

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>

namespace llvm {
class TargetSIMDInfo;
namespace Tensilica {
class SubtargetInfo;
class ISelLowering;

class TensilicaInterleavedAccess {
public:
  bool isInterleavedAccessTypeSupported(unsigned OpCode, VectorType *SubVecTy,
                                        unsigned Factor, bool NeedsSExt,
                                        const ISelLowering *IL,
                                        const TargetSIMDInfo *TSI) const;

  int getInterleavedAccessCost(unsigned OpCode, VectorType *SubVecTy,
                               unsigned Factor, ArrayRef<unsigned> Indices,
                               unsigned Alignment,
                               const TargetSIMDInfo *TSI) const;

  bool lowerInterleavedLoadValues(SmallVectorImpl<Value *> &InputVectors,
                                  SmallVectorImpl<Value *> &OutputVectors,
                                  unsigned Factor, IRBuilder<> &Builder,
                                  const TargetSIMDInfo *TSI) const;

  bool lowerInterleavedStoreValues(SmallVectorImpl<Value *> &InputVectors,
                                   SmallVectorImpl<Value *> &OutputVectors,
                                   unsigned Factor, IRBuilder<> &Builder,
                                   const TargetSIMDInfo *TSI) const;

  bool lowerInterleavedLoad(LoadInst *LI,
                            ArrayRef<ShuffleVectorInst *> Shuffles,
                            ArrayRef<unsigned> Indices, unsigned Factor,
                            const SubtargetInfo *Subtarget) const;

  bool lowerInterleavedStore(StoreInst *SI, ShuffleVectorInst *SVI,
                             unsigned Factor,
                             const SubtargetInfo *Subtarget) const;
};
} // namespace Tensilica
} // namespace llvm
#endif
