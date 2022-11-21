//===-------------------TensilicaTargetSIMDInfo.h--------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef LLVM_LIB_CODEGEN_TENSILICA_TENSILICATARGETSIMDINFO_H
#define LLVM_LIB_CODEGEN_TENSILICA_TENSILICATARGETSIMDINFO_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Analysis/TargetSIMDInfo.h"
#include "llvm/IR/Instruction.h"
#include <map>
#include <vector>

namespace llvm {
class CustomType;
class VectorType;
class LLVMContext;
namespace Tensilica {
class ISelLowering;
class ConfigSIMDInfo;
} // namespace Tensilica

class TensilicaTargetSIMDInfo : public TargetSIMDInfo {
  explicit TensilicaTargetSIMDInfo(LLVMContext &Ctx,
                                   const Tensilica::ISelLowering *TLI,
                                   const Tensilica::ConfigSIMDInfo *CSI);

public:
  // Make TensilicaTargetSIMDInfo a singleton.
  static TensilicaTargetSIMDInfo *
  getInstance(LLVMContext &Ctx, const Tensilica::ISelLowering *TLI,
              const Tensilica::ConfigSIMDInfo *CSI);

  ~TensilicaTargetSIMDInfo();

  bool checkMisalignedPtrInVectorization() const override { return true; }

  Type *getScalarBoolTy() const override;

  unsigned getMaxVF() const override { return MaxVF; }

  BitVector getVFsForScalarTy(Type *Ty) const override;

  bool isSignedCtype(Type *CT) const override;

  bool isLegalVectorType(VectorType *Ty,
                         bool &HasSameRegMemType) const override;

  Type *getVecTypeForCustomType(Type *CustomType) const override;
  Type *getRegTypeForVecType(Type *VecType, bool Signed) const override;
  Type *getMemTypeForVecType(Type *VecType, bool Signed) const override;

  const TargetSIMDType *
  getTargetSIMDTypeForRegType(Type *RegType) const override;

  TargetSIMDType *getTargetSIMDTypeForVecType(Type *VecType,
                                              bool Signed) const override;

  TargetSIMDType *getTargetSIMDTypeForScalarType(Type *ScalarType, int VL,
                                                 bool Signed) const override;

  // Return the alignment type for the given type T
  Type *getAlignType(Type *T) const override;

  // Return the intrinsic for a builtin, of which the operands have the same
  // type
  unsigned getIntrinsicForBinOp(unsigned BO, Type *DescType, Type *ResType,
                                CmpInst::Predicate Cond) const override;

  // Return the instrinsic for a builtin binary op with an immediate argument
  unsigned getIntrinsicForBinOpWithImmArg(unsigned BO, Type *DescType,
                                          Type *ResType) const override;

  unsigned getIntrinsicForVSel(Type *DescType, bool Imm) const override;

  bool isIntABSIntrinsic(unsigned IntrinID) const override;

  unsigned getIntrinsicForABS(Type *DescType) const override;

  unsigned getIntrinsicForABSSub(Type *DescType) const override;

  unsigned getIntrinsicForPredMov(Type *DescType, Type *PredType,
                                  bool Predicate) const override;

  unsigned getIntrinsicForMax(Type *DescType) const override;

  unsigned getIntrinsicForMin(Type *DescType) const override;

  unsigned getIntrinsicForMAdd(Type *DescType,
                               Type *AddOprndType) const override;

  unsigned getIntrinsicForMSub(Type *DescType,
                               Type *SubOprndType) const override;

  unsigned getIntrinsicForRecip(Type *DescType) const override;

  unsigned getIntrinsicForRsqrt(Type *DescType) const override;

  unsigned getIntrinsicForSqrt(Type *DescType) const override;

  unsigned getIntrinsicForZero(Type *ResType) const override;

  unsigned getIntrinsicForRedAdd(Type *DescType, Type *ResType) const override;

  unsigned getIntrinsicForRedMin(Type *DescType, Type *ResType) const override;

  unsigned getIntrinsicForRedMax(Type *DescType, Type *ResType) const override;

  unsigned getIntrinsicForRedBor(Type *DescType, Type *ResType) const override;

  unsigned getIntrinsicForRedBand(Type *DescType, Type *ResType) const override;

  unsigned getIntrinsicForRedBxor(Type *DescType, Type *ResType) const override;

  unsigned getIntrinsicForAvg(Type *DescType) const override;

  unsigned getIntrinsicForBNot(Type *DescType, Type *ResType) const override;

  unsigned getIntrinsicForLoadapPost(Type *MemType,
                                     Type *RegType) const override;

  unsigned getIntrinsicForLoadauPost(Type *MemType, Type *RegType,
                                     bool PositiveStride) const override;

  unsigned getIntrinsicForLoadavaruPost(Type *MemType, Type *RegType,
                                        bool PositiveStride) const override;

  unsigned getIntrinsicForLoadVector(Type *MemType,
                                     Type *RegType) const override;

  unsigned getIntrinsicForStoreauPost(Type *MemType, Type *RegType,
                                      bool PositiveStride) const override;

  unsigned getIntrinsicForStoreavaruPost(Type *MemType, Type *RegType,
                                         bool PositiveStride) const override;

  unsigned getIntrinsicForStoreafPost(Type *MemType, Type *RegType,
                                      bool PositiveStride) const override;

  unsigned getIntrinsicForStoreavarfPost(Type *MemType, Type *RegType,
                                         bool PositiveStride) const override;

  unsigned getIntrinsicForStoreanzfPost(Type *MemType, Type *RegType,
                                        bool PositiveStride) const override;

  unsigned getIntrinsicForStoreanzvarfPost(Type *MemType, Type *RegType,
                                           bool PositiveStride) const override;

  unsigned getIntrinsicForStoreVector(Type *MemType,
                                      Type *RegType) const override;

  unsigned getIntrinsicForClearAlignReg(Type *AlignRegType) const override;

  unsigned getIntrinsicForEpiLoopPredicate(unsigned VF, bool Imm,
                                           bool IsNZ) const override;

  unsigned getPredicatedIntrinsic(unsigned IntrinId, unsigned VF,
                                  bool Predicate) const override;

  unsigned getIntrinsicForTypeConversion(TypeConversionKind K, Type *SrcTy,
                                         Type *DestTy, bool SrcSigned,
                                         bool DestSigned) const override;

  unsigned getIntrinsicForTypeConversion(Type *SrcTy, Type *DestTy,
                                         TypeConversionKind &K, bool SrcSigned,
                                         bool DestSigned) const override;

  // Return the vectorized intrinsic according to the given VF
  unsigned getVectorizedIntrinsic(unsigned IntrinID,
                                  unsigned VF) const override;

  // Return true if the target support vectorizable function.
  bool supportVectorizableFuction() const override;

  // Return true if the target supports vectorized functions with non multiples
  // of the vector length.
  bool supportVectFuncWithNonMultiplesOfVectLength() const override;

  // Return true if result should be placed as the first argument for
  // vectorized function.
  bool isVectorizedFunctionResFirst() const override;

  // Return true if the function F has a vector equivalent with any
  // vectorization factor.
  bool isFunctionVectorizable(StringRef F) const override;

  // Return the name of the equivalent of F, vectorized with any vectorization
  // factor. If no such mapping exists, return the empty string.
  StringRef getVectorizedFunction(StringRef F) const override;

  // Return true if the given intrinsic performs type conversion between the
  // element type of llvm vector types and scalar TIE ctypes.
  bool isVectorElemTypeCvtIntrinsic(unsigned IntrinID,
                                    unsigned VF) const override;

  // Return true if the given intrinsic is a predicated intrinsic that performs
  // accumulation, for example, madd.
  bool isAccPredicatedIntrinsic(unsigned IntrinID) const override;

  // Return if the given intrinsic is vectorizable
  bool isIntrinsicVectorizable(unsigned IntrinID) const override;

  // Return if the given intrinsic is vectorizable with the vectorization
  // factor VF
  bool isIntrinsicVectorizable(unsigned IntrinID, unsigned VF) const override;

  // Return if the idx'th input operand is immediate
  bool hasIntrinsicImmOpnd(unsigned IntrinID, unsigned Idx) const override;

  // Return if the intrinsic has reduction info
  bool hasIntrinsicReductionInfo(unsigned IntrinID) const override;

  // Return IntrinsicReductionInfo for the given intrinsic
  const TargetIntrinRedInfo *
  getIntrinsicReductionInfo(unsigned IntrinID, unsigned VF,
                            bool Ordered) const override;

  // Return the TargetIntrinRedPartialSumInfo for the given intrinsic
  virtual const TargetIntrinRedPartialSumInfo *
  getIntrinsicRedPartialmSumInfo(unsigned IntrinID) const override;

  // Return if the given type T is vectorizable
  bool isTypeVectorizable(Type *T) const override;

  // Return VFs in priority order for \p ScalarTy.
  const SmallVectorImpl<unsigned> *
  getVFPriorityList(Type *ScalarTy) const override;

  // Return true if shuffle values in \p ValTy with \p IntMask is legal.
  bool isShuffleLegal(Type *ValTy, ArrayRef<uint32_t> IntMask) const override;

  // Builds a shuffle vector value.
  Value *createShuffleVector(IRBuilder<> Builder, Value *V0, Value *V1,
                             ArrayRef<uint32_t> IntMask) const override;

  // Return vsel intrinsic info for the given intrinsic.
  const TargetSIMDInfo::VselIntrinInfo *
  getVselIntrinsicInfo(unsigned IntrinID) const override;

private:
  llvm::DenseMap<Type *, CustomType *> VecTyToCTyMap;
  typedef std::pair<unsigned, Type *> SIMDOpIdx;
  typedef std::vector<unsigned> SIMDOpList;
  llvm::DenseMap<SIMDOpIdx, SIMDOpList> SIMDOpMap;
  llvm::DenseMap<Type *, Type *> CtypeToAlignTypeMap;
  llvm::DenseMap<Type *, TargetSIMDType *> VecTypeToSIMDType;
  llvm::DenseMap<Type *, TargetSIMDType *> SIntVecTypeToSIMDType;
  llvm::DenseMap<Type *, TargetSIMDType *> RegTypeToSIMDType;
  llvm::DenseMap<Type *, Type *> CTypeToVecTypeMap;
  llvm::SmallPtrSet<Type *, 32> SignedCtypes;
  llvm::DenseMap<VectorType *, bool> LegalVecTypes;
  llvm::DenseMap<Type *, BitVector> TyToVFsMap;
  unsigned Int32ABSIID;
  const Tensilica::ISelLowering *TLI;
  const Tensilica::ConfigSIMDInfo *CSI;
  unsigned MaxVF;

  // Returns the intrinsic representing the TIE intrinsic operator
  // with input and output types
  unsigned getIntrinsicForTIOp(unsigned TIOpId, Type *DescType,
                               ArrayRef<Type *> InputTypes,
                               ArrayRef<Type *> OutputTypes,
                               ArrayRef<Type *> OverloadTypes = None) const;

  // Return the intrinsic list for the given tie operator
  const SIMDOpList *searchIntrinsicForTIOp(unsigned TIOpId,
                                           Type *DescType) const;

  // Check the types of arguments and return values
  bool checkIntrinsic(unsigned IntrinId, ArrayRef<Type *> InputTypes,
                      ArrayRef<Type *> OutputTypes,
                      bool CheckOutputTypes = true,
                      ArrayRef<Type *> OverloadTypes = None) const;

  // \brief Initialize the mapping between vector types and custom types
  void initSIMDTypes();

  // \brief Initialize the map for looking up intrinsics
  void initSIMDOps();

  // \brief Initialize the map for looking up vectorization factors in
  // priority order.
  void initVFPriority();

  // Builds a shuffle mask value.
  Value *generateShuffleMask(IRBuilder<> Builder, Module *M,
                             unsigned TypeFactor, unsigned ElemMemSize,
                             Type *MaskTy, ArrayRef<uint32_t> IntMask,
                             bool FlipMask) const;
};

} // namespace llvm
#endif
