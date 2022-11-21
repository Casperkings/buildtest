//===-- TensilicaConfigTargetLowering.h - Target Lowering Config Info -----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares a helper class for Tensilica::TargetLowering
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_CODEGEN_TENSILICA_TENSILICACONFIG_TENSILICACONFIGTARGETLOWERING_H
#define LLVM_LIB_CODEGEN_TENSILICA_TENSILICACONFIG_TENSILICACONFIGTARGETLOWERING_H
#include "Tensilica/TensilicaIntrinsicISelLower.h"
#include "Tensilica/TensilicaOperationISelLower.h"
#include "Tensilica/TensilicaPseudoExpand.h"
#include "Tensilica/TensilicaRegTransfer.h"
#include <llvm/ADT/ArrayRef.h>
#include <vector>

namespace llvm {
namespace Tensilica {

// This structure contains target config-specific information that is not
// passed through tablegen, instead it is generated directly by
// llvm-xtensa-config-gen.
struct ConfigTargetLowering {
  ArrayRef<Tensilica::PseudoExpandInfo> PseudoExpandInfos;
  ArrayRef<Tensilica::RegTransferInfo> RegTransferInfos;
  ArrayRef<Tensilica::IntrinsicISelInfo> IntrinsicISelInfos;
  ArrayRef<Tensilica::OpISelInfo> OperationISelInfos;
  ArrayRef<unsigned> IntrinsicISelInfoIdxs;
  ArrayRef<Tensilica::OpAction> OpActions;
  ArrayRef<Tensilica::LoadStoreAction> LoadStoreActions;
  ArrayRef<Tensilica::PromotedOpAction> PromotedOpActions;
  std::map<unsigned, Tensilica::VSelImmMatchFunc> VSelImmMatchFuncMap;

  // Constructors
  ConfigTargetLowering(bool UnsafeFPMath, bool CoProc, bool UseMul16);
  ConfigTargetLowering(const ConfigTargetLowering &) = delete;
  ConfigTargetLowering(ConfigTargetLowering &&that) = default;
};

} // namespace Tensilica
} // namespace llvm
#endif // LLVM_LIB_CODEGEN_TENSILICA_TENSILICACONFIG_TENSILICACONFIGTARGETLOWERING_H
