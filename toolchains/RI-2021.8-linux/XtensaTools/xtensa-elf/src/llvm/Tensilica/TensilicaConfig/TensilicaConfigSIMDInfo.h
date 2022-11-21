//===-- TensilicaConfigSIMDInfo.h - Config SIMD Information --------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares a helper class for TensilicaTargetSIMDInfo
//
//===----------------------------------------------------------------------===//
#include "llvm/ADT/ArrayRef.h"
#include "llvm/Analysis/TargetSIMDInfo.h"

namespace llvm {
namespace Tensilica {

enum SIMDBaseTypeKind {
  UnsignedIntBaseType,
  SignedIntBaseType,
  FloatingPointBaseType,
  CustomBaseType,
  UnkownBaseType
};

struct SIMDTypeInfo {
  unsigned VectorLength;
  SIMDBaseTypeKind BaseKind;
  unsigned Base;
  unsigned RegType;
  unsigned MemType;
  unsigned AlignType;
};

struct VFPriorityInfo {
  SIMDBaseTypeKind BaseKind;
  unsigned Base;
  SmallVector<unsigned, 4> VFPriority;
};

struct CtypeCvtInfo {
  unsigned SrcCtypeID;
  unsigned DestCtypeID;
  unsigned IID;
  bool operator<(const CtypeCvtInfo &Other) const {
    if (SrcCtypeID < Other.SrcCtypeID)
      return true;

    if (SrcCtypeID == Other.SrcCtypeID && DestCtypeID < Other.DestCtypeID)
      return true;
    return false;
  }
};

// TIE intrinsic operators
enum TIOperator {
  xt_unknown = 0,
  xt_abs,
  xt_add,
  xt_ashr,
  xt_band,
  xt_bor,
  xt_bnand,
  xt_bnor,
  xt_bnot,
  xt_bxor,
  xt_conj,
  xt_eq,
  xt_ge,
  xt_gt,
  xt_land,
  xt_le,
  xt_lor,
  xt_lnot,
  xt_lshr,
  xt_lt,
  xt_madd,
  xt_madd_conj,
  xt_max,
  xt_min,
  xt_mpy,
  xt_mpy_conj,
  xt_msub,
  xt_msub_conj,
  xt_ne,
  xt_neg,
  xt_recip,
  xt_rsqrt,
  xt_sqrt,
  xt_shl,
  xt_sub,
  xt_quo,
  xt_rem,
  xt_extract0,
  xt_extract1,
  xt_pack2,
  xt_vsel,
  xt_vshfl,
  xt_loada,
  xt_loadau,
  xt_loadap,
  xt_storea,
  xt_storeau,
  xt_storeap,
  xt_storeaf,
  xt_loadau_post,
  xt_loadau_pos_post,
  xt_loadau_neg_post,
  xt_loadu_pos_post,
  xt_loadu_neg_post,
  xt_loadavaru_post,
  xt_loadavaru_neg_post,
  xt_loadap_post,
  xt_loadavarp_post,
  xt_storeau_post,
  xt_storeau_pos_post,
  xt_storeau_neg_post,
  xt_storeu_pos_post,
  xt_storeu_neg_post,
  xt_storeavaru_post,
  xt_storeavaru_neg_post,
  xt_storeap_post,
  xt_storeaf_pos_post,
  xt_storeaf_neg_post,
  xt_storeanzf_pos_post,
  xt_storeanzf_neg_post,
  xt_storeavarf_pos_post,
  xt_storeavarf_neg_post,
  xt_storeanzvarf_pos_post,
  xt_storeanzvarf_neg_post,
  xt_lsiu,
  xt_lsiu_post,
  xt_lsx,
  xt_lsxu,
  xt_lsxu_post,
  xt_lviu,
  xt_lviu_post,
  xt_lvx,
  xt_lvxu,
  xt_lvxu_post,
  xt_mpy_even,
  xt_mpy_odd,
  xt_madd_even,
  xt_madd_odd,
  xt_msub_even,
  xt_msub_odd,
  xt_ssiu,
  xt_ssiu_post,
  xt_sviu,
  xt_sviu_post,
  xt_svx,
  xt_svxu,
  xt_mulr,
  xt_mulr_even,
  xt_mulr_odd,
  xt_wvsar,
  xt_wround,
  xt_radd,
  xt_rmin,
  xt_rmax,
  xt_zero,
  xt_vpack,
  xt_movt,
  xt_movf,
  xt_csel,
  xt_abssub,
  xt_const2,
  xt_const4,
  xt_movf2,
  xt_avg,
  xt_imadd,
  xt_impy,
  xt_imsub,
  xt_imulr,
  xt_rmadd,
  xt_rmpy,
  xt_rmsub,
  xt_rmulr,
  // Internal reduction ops
  xt_band_red,
  xt_bor_red,
  xt_bxor_red
};

typedef struct {
  unsigned TIOp;
  signed DescCtypeId;
  unsigned VF;
  signed ResCtypeId;
  unsigned IntrinsicId;
} SIMDOpInfo;

struct VecIntrinsicDesc {
  unsigned ScalarIntrinID;
  unsigned VectFactor;
  unsigned VectIntrinID;
  bool operator<(const VecIntrinsicDesc &Other) const {
    return std::tie(ScalarIntrinID, VectFactor) <
           std::tie(Other.ScalarIntrinID, Other.VectFactor);
  }
};

struct VectElemTypeCvtDesc {
  unsigned IntrinID;
  unsigned VectFactor;
};

struct VecLibFunDesc {
  const char *ScalarFnName;
  const char *VectorFnName;
};

struct IntrinsicImmOpnd {
  unsigned IntrinID;
  unsigned ImmOpndIdx;
  bool operator<(const IntrinsicImmOpnd &Other) const {
    return IntrinID < Other.IntrinID;
  }
};

struct EpiLoopPredicateInfo {
  unsigned IntrnId;
  unsigned VF;
  bool Imm;
  bool IsNZ;
};

struct PredIntrinInfo {
  unsigned NormalIntrinId;
  unsigned Factor;
  bool Truth;
  unsigned PredIntrinId;

  bool operator<(const PredIntrinInfo &Other) const {
    return std::tie(NormalIntrinId, Factor, Truth) <
           std::tie(Other.NormalIntrinId, Other.Factor, Other.Truth);
  }
};

class ConfigSIMDInfo {
public:
  virtual ArrayRef<VFPriorityInfo> getVFPriorityInfo() const = 0;
  virtual ArrayRef<SIMDTypeInfo> getSIMDTypeInfo() const = 0;
  virtual ArrayRef<TargetSIMDInfo::VselIntrinInfo>
  getVselIntrinInfo() const = 0;
  virtual int getScalarBoolTy() const = 0;
  virtual ArrayRef<CtypeCvtInfo> getCtypeRTORCVTInfo() const = 0;
  virtual ArrayRef<CtypeCvtInfo> getCtypeRTOMCVTInfo() const = 0;
  virtual ArrayRef<CtypeCvtInfo> getCtypeMTORCVTInfo() const = 0;
  virtual ArrayRef<SIMDOpInfo> getSIMDOpInfo() const = 0;
  virtual ArrayRef<VecIntrinsicDesc> getVectIntrinsicInfo() const = 0;
  virtual ArrayRef<VectElemTypeCvtDesc>
  getVectElemTypeCvtIntrinsicInfo() const = 0;
  virtual ArrayRef<VecLibFunDesc> getVecLibFuncInfo() const = 0;
  virtual bool supportVectorizableFunction() const = 0;
  virtual bool supportVectFuncWithNonMultiplesOfVectLength() const = 0;
  virtual bool isVectorizedFunctionResFirst() const = 0;
  virtual ArrayRef<unsigned> getAccPredicatedIntrinsicInfo() const = 0;
  virtual ArrayRef<IntrinsicImmOpnd> getVectIntrinsicImmOpndInfo() const = 0;
  virtual ArrayRef<TargetIntrinRedInfo> getIntrinsicReductionInfo() const = 0;
  virtual ArrayRef<EpiLoopPredicateInfo> getEpiLoopPredicateInfo() const = 0;
  virtual ArrayRef<PredIntrinInfo> getPredicateIntrinsicInfo() const = 0;
  virtual ArrayRef<TargetIntrinRedPartialSumInfo>
  getRedPartialSumInfo() const = 0;
};
} // namespace Tensilica
} // namespace llvm