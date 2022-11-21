//===-- RISCVConfigInfo.cpp - RISCV TDK flow Implementation ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Tensilica/TensilicaConfig/TensilicaConfigInstrInfo.h"
#include "Tensilica/TensilicaConfig/TensilicaConfigPacketizer.h"
#include "Tensilica/TensilicaConfig/TensilicaConfigTargetLowering.h"
#include "Tensilica/TensilicaConfig/TensilicaConfigSIMDInfo.h"
#include "Tensilica/TensilicaConfig/TensilicaConfigImap.h"
#include "Tensilica/TensilicaOperationISelLower.h"
#include "Tensilica/TensilicaDFAPacketizer.h"
#include "Tensilica/TensilicaInstrInfo.h"
#include "Tensilica/TensilicaIMAP.h"
#include "RISCVConfig/RISCVConfigInfo.h"
#include "RISCVConfig/RISCVConfigTargetLowering.h"
#include "RISCVIntrinsicInfo.h"
#include "RISCVSubtarget.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/Mutex.h"

using namespace llvm;
using namespace llvm::Tensilica;

//----------------------------------------------------------------------------//
// Register info
//----------------------------------------------------------------------------//
#define GET_REGINFO_HEADER
#define GET_REGCLASSES
#define GET_REGINFO_TARGET_DESC
#include "RISCVGenRegisterInfo.inc"

//----------------------------------------------------------------------------//
// Instruction info
//----------------------------------------------------------------------------//
namespace llvm {
extern void InitRISCVMCInstrInfo(MCInstrInfo *II);
}

llvm::Tensilica::ConfigInstrInfo::~ConfigInstrInfo() {}

#define GET_INSTR_IMM_RANGE_FUNCS
#include "TensilicaGenInstrProperty.inc"

static int TensilicaInstrPropMap[] = {
#define GET_INSTR_TARGET_PROP_MAP
#include "RISCVGenInstrInfo.inc"
  0
};

static llvm::Tensilica::InstrProperty TensilicaInstrProperties[] = {{0,
                                                                     false,
                                                                     false,
                                                                     false,
                                                                     false,
                                                                     false,
                                                                     0,
                                                                     0,
                                                                     0,
                                                                     0,
                                                                     0,
                                                                     false,
                                                                     0,
                                                                     0,
                                                                     {},
                                                                     nullptr}
#define GET_INSTR_PROPERTY
#include "TensilicaGenInstrProperty.inc"
};

static unsigned TensilicaEnumToOpcode[] = {
#define OPC_NULL static_cast<unsigned>(Tensilica::LLVMXCC_OPC_NULL)
    OPC_NULL,          // MapOpcode::NONE
#define RISCV_ENUM_TO_OPCODE
#include "Tensilica/OpcodeMap.h"
#define OPCODE_TO_ENUM
#include "TensilicaGenRewrite.inc"
#undef OPC_NULL
};

llvm::Tensilica::ConfigInstrInfo::ConfigInstrInfo()
    : AStage(
#define GET_A_STAGE
#include "TensilicaGenPipeAlter.inc"
#undef GET_A_STAGE
          ),
      EStage(
#define GET_E_STAGE
#include "TensilicaGenPipeAlter.inc"
#undef GET_E_STAGE
          ),
      PipeAlterMap{
#define GET_PIPEALTER_MAP
#include "TensilicaGenPipeAlter.inc"
#undef GET_PIPEALTER_MAP
      },
      PipeAlterReverseMap{
#define GET_PIPEALTER_REVERSE_MAP
#include "TensilicaGenPipeAlter.inc"
#undef GET_PIPEALTER_REVERSE_MAP
      },
      InstrProperties(TensilicaInstrProperties),
      InstrPropMap(TensilicaInstrPropMap),
      InsnBitsInputMap{
#define GET_INSTR_INPUT_PROPERTY
#include "TensilicaGenInstrProperty.inc"
      },
      IgnoreStateOutputMap{
#define GET_OPC_IGNORED_STATES
#include "TensilicaGenOpcIgnoreStateOutput.inc"
      },
      I_X_Map{
#define GET_INSTR_I_TO_X
#include "TensilicaGenInstrAddrMode.inc"
      },
      I_IP_Map{
#define GET_INSTR_I_TO_IP
#include "TensilicaGenInstrAddrMode.inc"
      },
      IP_I_Map{
#define GET_INSTR_IP_TO_I
#include "TensilicaGenInstrAddrMode.inc"
      },
      X_XP_Map{
#define GET_INSTR_X_TO_XP
#include "TensilicaGenInstrAddrMode.inc"
      },
      I_IU_Map{
#define GET_INSTR_I_TO_IU
#include "TensilicaGenInstrAddrMode.inc"
      },
      X_XU_Map{
#define GET_INSTR_X_TO_XU
#include "TensilicaGenInstrAddrMode.inc"
      },
      XP_I_Map{
#define GET_INSTR_XP_TO_I
#include "TensilicaGenInstrAddrMode.inc"
      },
      EnumToOpcode(TensilicaEnumToOpcode),
      EnumToVariants{
#define ENUM_TO_VARIANTS
#include "TensilicaGenRewrite.inc"
      },
      OpcodeToEnum{
#define RISCV_OPCODE_TO_ENUM 
#include "Tensilica/OpcodeMap.h"
#define OPCODE_TO_ENUM
#include "TensilicaGenRewrite.inc"
      },
      initTensilicaMCInstrInfo(&llvm::InitRISCVMCInstrInfo) {
}

//----------------------------------------------------------------------------//
// Calling conventions
//----------------------------------------------------------------------------//
#include "RISCVGenCallingConv.inc"

//----------------------------------------------------------------------------//
// Target Lowering info
//----------------------------------------------------------------------------//
#define GET_OPERATION_ISEL_LOWER_INFO_FUNC
#include "TensilicaGenOperationISelLower.inc"

#define GET_INTRN_ISEL_LOWER_IMM_PRED
#include "TensilicaGenIntrinsicISelLower.inc"

static unsigned RISCVIntrinsicISelInfoIdxs[] = {
#define GET_INTRN_ISEL_LOWER_INFO_IDX
#include "TensilicaGenIntrinsicISelLower.inc"
};

static Tensilica::RegTransferInfo RISCVRegTransferInfos[] = {
#define GET_REG_TRANSFER_INFO
#include "TensilicaGenRegTransfer.inc"
    {0xffffffff, 0, 0, 0, 0, 0, 0, 0, false}};

static Tensilica::PseudoExpandInfo RISCVPseudoExpandInfos[] = {
    {0, Intrinsic::not_intrinsic}
#define GET_PSEUDO_EXPAND_INFO
#include "TensilicaGenPseudoExpand.inc"
};

static Tensilica::IntrinsicISelInfo RISCVIntrinsicISelInfos[] = {
    {Intrinsic::not_intrinsic,
     0,
     0,
     0,
     {},
     0,
     {},
     0,
     {},
     false,
     0,
     0,
     false,
     false,
     false}
#define GET_INTRN_ISEL_LOWER_INFO
#include "TensilicaGenIntrinsicISelLower.inc"
};

static Tensilica::OpISelInfo RISCVOperationISelInfos[] = {
    {0,
     Tensilica::OpISelInfo::never_match,
     MVT::INVALID_SIMPLE_VALUE_TYPE,
     nullptr,
     Intrinsic::not_intrinsic,
     0,
     {},
     MVT::INVALID_SIMPLE_VALUE_TYPE,
     false,
     1,
     false,
     true,
     false,
     0,
     0}
#define GET_OPERATION_ISEL_LOWER_INFO
#include "TensilicaGenOperationISelLower.inc"
};

#define Legal llvm::TargetLoweringBase::Legal
#define Custom llvm::TargetLoweringBase::Custom
#define Expand llvm::TargetLoweringBase::Expand
#define Promote llvm::TargetLoweringBase::Promote
static Tensilica::OpAction RISCVOpActions[] = {
    {ISD::ADD, MVT::i32, Legal, Tensilica::OpAction::NoPred}
#define GET_OP_ACTION
#include "TensilicaGenOpAction.inc"
};

static Tensilica::LoadStoreAction RISCVLoadStoreActions[] = {
    {ISD::LOAD, ISD::NON_EXTLOAD, MVT::i32, MVT::i32, Legal}
#define GET_LDST_OP_ACTION
#include "TensilicaGenLoadStoreAction.inc"
};

static Tensilica::PromotedOpAction RISCVPromotedOpActions[] = {
    {ISD::ADD, MVT::i32, MVT::i32}
#define GET_PROMOTED_OP_ACTION
#include "TensilicaGenOpAction.inc"
};
#undef Legal
#undef Custom
#undef Expand
#undef Promote

llvm::Tensilica::ConfigTargetLowering::ConfigTargetLowering(
    bool /*UnsafeFPMath*/, bool /*CoProc*/, bool /*UseMul16*/)
    : PseudoExpandInfos(RISCVPseudoExpandInfos,
                        sizeof(RISCVPseudoExpandInfos) /
                            sizeof(Tensilica::PseudoExpandInfo)),
      RegTransferInfos(RISCVRegTransferInfos,
                       sizeof(RISCVRegTransferInfos) /
                           sizeof(Tensilica::RegTransferInfo)),
      IntrinsicISelInfos(RISCVIntrinsicISelInfos,
                         sizeof(RISCVIntrinsicISelInfos) /
                             sizeof(Tensilica::IntrinsicISelInfo)),
      OperationISelInfos(RISCVOperationISelInfos,
                         sizeof(RISCVOperationISelInfos) /
                             sizeof(Tensilica::OpISelInfo)),
      IntrinsicISelInfoIdxs(RISCVIntrinsicISelInfoIdxs,
                            sizeof(RISCVIntrinsicISelInfoIdxs) /
                                sizeof(unsigned)),
      OpActions(RISCVOpActions,
                sizeof(RISCVOpActions) / sizeof(Tensilica::OpAction)),
      LoadStoreActions(RISCVLoadStoreActions,
                       sizeof(RISCVLoadStoreActions) /
                           sizeof(Tensilica::LoadStoreAction)),
      PromotedOpActions(RISCVPromotedOpActions,
                        sizeof(RISCVPromotedOpActions) /
                            sizeof(Tensilica::PromotedOpAction)),
      VSelImmMatchFuncMap{
#define GET_VSEL_IMM_MATCH_FUNCTIONS
#include "TensilicaGenOperationISelLower.inc"
          {0, nullptr}} {
  assert(std::is_sorted(OperationISelInfos.begin(), OperationISelInfos.end()) &&
         "Unsorted OperationISelInfos");
}

static Tensilica::OpDesc RISCVOpDescs[] = {{ISD::ADD, MVT::i32}
#define GET_XTENSA_SUPPORTED_OPS
#include "TensilicaGenOpAction.inc"
};

static llvm::MVT RISCVVshuffleMvts[] = {MVT::Other
#define GET_GENERIC_VSHUFFLE_MVT
#include "TensilicaGenOpAction.inc"
};

static llvm::MVT RISCVSelectMvts[] = {MVT::i32
#define GET_SELECT_MVT
#include "TensilicaGenOpAction.inc"
};

static llvm::MVT RISCVUnalignedMvts[] = {MVT::INVALID_SIMPLE_VALUE_TYPE
#define GET_UNALIGNED_MVT
#include "TensilicaGenOpAction.inc"
};

static TypeDesc RISCVConsecutiveTypes[] = {
#define GET_TYPE_PARAM_CONSECUTIVE_REG
#include "TensilicaGenConsecutiveCallRegs.inc"
    {Type::VoidTyID, 0, 0}};

llvm::RISCVConfigTargetLowering::RISCVConfigTargetLowering(bool UnsafeFPMath,
                                                           bool CoProc,
                                                           bool UseMul16)

    : UnsafeFPMath(UnsafeFPMath),
      RISCVOps(RISCVOpDescs, sizeof(RISCVOpDescs) / sizeof(Tensilica::OpDesc)),
      VshuffleMvts(RISCVVshuffleMvts,
                   sizeof(RISCVVshuffleMvts) / sizeof(llvm::MVT)),
      SelectMvts(RISCVSelectMvts, sizeof(RISCVSelectMvts) / sizeof(llvm::MVT)),
      UnalignedMvts(RISCVUnalignedMvts,
                    sizeof(RISCVUnalignedMvts) / sizeof(llvm::MVT)),
      ConsecutiveTypes(RISCVConsecutiveTypes,
                       sizeof(RISCVConsecutiveTypes) / sizeof(TypeDesc)),
      Names2Regs {
#if 0
// FIXME
                {"a0", Xtensa::LR},   {"a1", Xtensa::SP},   {"a2", Xtensa::a2},
                {"a3", Xtensa::a3},   {"a4", Xtensa::a4},   {"a5", Xtensa::a5},
                {"a6", Xtensa::a6},   {"a7", Xtensa::a7},   {"a8", Xtensa::a8},
                {"a9", Xtensa::a9},   {"a10", Xtensa::a10}, {"a11", Xtensa::a11},
                {"a12", Xtensa::a12}, {"a13", Xtensa::a13}, {"a14", Xtensa::a14},
                {"a15", Xtensa::a15},
#endif
#define GET_REGNAME_TO_REGNUM
#include "TensilicaGenRegNameToRegNum.inc"
#undef GET_REGNAME_TO_REGNUM
      },
      CC_RISCV(::CC_RISCV),
      RetCC_RISCV(::RetCC_RISCV) {
}

extern "C" {
extern const int TEN_DFA_TRANSITION_TABLE[];
}
//----------------------------------------------------------------------------//
// Packetizer
//----------------------------------------------------------------------------//
llvm::Tensilica::ConfigPacketizer::ConfigPacketizer()
    : OpcodeClasses(nullptr), GenericOpcodeTable(nullptr), Formats(nullptr),
      FunctionalUnits(nullptr),
      BundlingDFATransitionTable(nullptr), /* these fields initialized below */
      NumState(
#define GET_NUMSTATE
#include "TensilicaGenPacketizer.inc"
          ),
      NumClass(
#define GET_NUMCLASS
#include "TensilicaGenPacketizer.inc"
          ),
      NumFormat(
#define GET_NUMFORMAT
#include "TensilicaGenPacketizer.inc"
          ),
      InstructionListEnd(RISCV::INSTRUCTION_LIST_END),
      MoveAr(RISCV::MOVE_AR), Nop(RISCV::NOP), XsrSar(RISCV::XORI) {
  using namespace RISCV;

  static int OpcodeClassesData[] = {
#define GET_OPCODECLASSES
#include "TensilicaGenPacketizer.inc"
  };
  OpcodeClasses = OpcodeClassesData;
  static int GenericOpcodeTableData[] = {
#define GET_GENERIC_OPCODE
#include "TensilicaGenPacketizer.inc"
  };
  GenericOpcodeTable = GenericOpcodeTableData;
  static const struct Tensilica::Format FormatsData[] = {
#define GET_FORMATS
#include "TensilicaGenPacketizer.inc"
  };
  Formats = FormatsData;
  static FunctionalUnitInfo FunctionalUnitsData[] = {
      {"dummy", 0, {}},
#define GET_FUNCTIONAL_UNITS
#include "TensilicaGenPacketizer.inc"
  };
  NumFunctionalUnits =
      sizeof(FunctionalUnitsData) / sizeof(FunctionalUnitInfo) - 1;
  FunctionalUnits = NumFunctionalUnits ? &FunctionalUnitsData[1] : nullptr;
  BundlingDFATransitionTable = TEN_DFA_TRANSITION_TABLE;
}

//----------------------------------------------------------------------------//
// SIMDInfo
//----------------------------------------------------------------------------//
namespace {
class RISCVSIMDInfo : public ConfigSIMDInfo {
public:
  ArrayRef<VFPriorityInfo> getVFPriorityInfo() const override;
  ArrayRef<SIMDTypeInfo> getSIMDTypeInfo() const override;
  ArrayRef<TargetSIMDInfo::VselIntrinInfo> getVselIntrinInfo() const override;
  int getScalarBoolTy() const override;
  ArrayRef<CtypeCvtInfo> getCtypeRTORCVTInfo() const override;
  ArrayRef<CtypeCvtInfo> getCtypeRTOMCVTInfo() const override;
  ArrayRef<CtypeCvtInfo> getCtypeMTORCVTInfo() const override;
  ArrayRef<SIMDOpInfo> getSIMDOpInfo() const override;
  ArrayRef<VecIntrinsicDesc> getVectIntrinsicInfo() const override;
  ArrayRef<VectElemTypeCvtDesc>
  getVectElemTypeCvtIntrinsicInfo() const override;
  ArrayRef<VecLibFunDesc> getVecLibFuncInfo() const override;
  bool supportVectorizableFunction() const override;
  bool supportVectFuncWithNonMultiplesOfVectLength() const override;
  bool isVectorizedFunctionResFirst() const override;
  ArrayRef<unsigned> getAccPredicatedIntrinsicInfo() const override;
  ArrayRef<IntrinsicImmOpnd> getVectIntrinsicImmOpndInfo() const override;
  ArrayRef<TargetIntrinRedInfo> getIntrinsicReductionInfo() const override;
  ArrayRef<EpiLoopPredicateInfo> getEpiLoopPredicateInfo() const override;
  ArrayRef<PredIntrinInfo> getPredicateIntrinsicInfo() const override;
  ArrayRef<TargetIntrinRedPartialSumInfo> getRedPartialSumInfo() const override;
};
} // namespace

ArrayRef<VFPriorityInfo> RISCVSIMDInfo::getVFPriorityInfo() const {
  static VFPriorityInfo Info[] = {
#define GetVFPriority
#include "TensilicaGenSIMDInfo.inc"
#undef GetVFPriority
      {UnkownBaseType, 0, {}}};
  return ArrayRef<VFPriorityInfo>(Info,
                                  sizeof(Info) / sizeof(VFPriorityInfo) - 1);
}

ArrayRef<SIMDTypeInfo> RISCVSIMDInfo::getSIMDTypeInfo() const {
  static SIMDTypeInfo Info[] = {
#define GetSIMDTypeInfo
#define SIMDTypeInfo(VL, KIND, BASE, REG_TYPE, MEM_TYPE, ALIGN_TYPE)           \
  {VL, KIND, BASE, REG_TYPE, MEM_TYPE, ALIGN_TYPE},
#include "TensilicaGenSIMDInfo.inc"
#undef GetSIMDTypeInfo
      {0, UnkownBaseType, 0, 0, 0, 0}};
  return ArrayRef<SIMDTypeInfo>(Info, sizeof(Info) / sizeof(SIMDTypeInfo) - 1);
}

ArrayRef<TargetSIMDInfo::VselIntrinInfo>
RISCVSIMDInfo::getVselIntrinInfo() const {
  static TargetSIMDInfo::VselIntrinInfo Info[] = {
#define GetVselIntrinsicInfo
#include "TensilicaGenSIMDInfo.inc"
#undef GetVselIntrinsicInfo
      {0, 0, 0, false}};
  return ArrayRef<TargetSIMDInfo::VselIntrinInfo>(
      Info, sizeof(Info) / sizeof(TargetSIMDInfo::VselIntrinInfo) - 1);
}

int RISCVSIMDInfo::getScalarBoolTy() const {
  int ScalarBoolTy = -1;
#define GetScalarBoolTy
#include "TensilicaGenSIMDInfo.inc"
#undef GetScalarBoolTy
  return ScalarBoolTy;
}

ArrayRef<CtypeCvtInfo> RISCVSIMDInfo::getCtypeRTORCVTInfo() const {
  static CtypeCvtInfo Info[] = {
#define GetRTORCVT
#define GEN_RTOR(SRC_CTYPE_ID, TARGET_CTYPE_ID, INTRINSIC_ID)                  \
  {SRC_CTYPE_ID, TARGET_CTYPE_ID, INTRINSIC_ID},
#include "TensilicaGenSIMDInfo.inc"
#undef GetRTORCVT
      {0, 0, Intrinsic::not_intrinsic}};
  return ArrayRef<CtypeCvtInfo>(Info, sizeof(Info) / sizeof(CtypeCvtInfo) - 1);
}

ArrayRef<CtypeCvtInfo> RISCVSIMDInfo::getCtypeRTOMCVTInfo() const {
  static CtypeCvtInfo Info[] = {
#define GetRTOMCVT
#define GEN_RTOM(SRC_CTYPE_ID, TARGET_CTYPE_ID, INTRINSIC_ID)                  \
  {SRC_CTYPE_ID, TARGET_CTYPE_ID, INTRINSIC_ID},
#include "TensilicaGenSIMDInfo.inc"
#undef GetRTOMCVT
      {0, 0, Intrinsic::not_intrinsic}};

  return ArrayRef<CtypeCvtInfo>(Info, sizeof(Info) / sizeof(CtypeCvtInfo) - 1);
}
ArrayRef<CtypeCvtInfo> RISCVSIMDInfo::getCtypeMTORCVTInfo() const {
  static CtypeCvtInfo Info[] = {
#define GetMTORCVT
#define GEN_MTOR(SRC_CTYPE_ID, TARGET_CTYPE_ID, INTRINSIC_ID)                  \
  {SRC_CTYPE_ID, TARGET_CTYPE_ID, INTRINSIC_ID},
#include "TensilicaGenSIMDInfo.inc"
#undef GetMTORCVT
      {0, 0, Intrinsic::not_intrinsic}};
  return ArrayRef<CtypeCvtInfo>(Info, sizeof(Info) / sizeof(CtypeCvtInfo) - 1);
}

ArrayRef<SIMDOpInfo> RISCVSIMDInfo::getSIMDOpInfo() const {
  static SIMDOpInfo Info[] = {
#define GetSIMDOps
#define SIMDOp(TI_OP, DESC_CTYPE_ID, VF, RES_CTYPE_ID, INTRINSIC_ID)           \
  {TI_OP, DESC_CTYPE_ID, VF, RES_CTYPE_ID, INTRINSIC_ID},
#include "TensilicaGenSIMDInfo.inc"
#undef GetSIMDOps
      {xt_unknown, -1, 0, -1, Intrinsic::not_intrinsic}};
  return ArrayRef<SIMDOpInfo>(Info, sizeof(Info) / sizeof(SIMDOpInfo) - 1);
}

ArrayRef<VecIntrinsicDesc> RISCVSIMDInfo::getVectIntrinsicInfo() const {
  static VecIntrinsicDesc Info[] = {
#define GetVecIntrinsics
#define VecDesc(SCALAR_INTRIN, VF, VECT_INTRIN)                                \
  {SCALAR_INTRIN, VF, VECT_INTRIN},
#include "TensilicaGenSIMDInfo.inc"
#undef GetVecIntrinsics
      {0, 0, 0}};
  return ArrayRef<VecIntrinsicDesc>(
      Info, sizeof(Info) / sizeof(VecIntrinsicDesc) - 1);
}

ArrayRef<VectElemTypeCvtDesc>
RISCVSIMDInfo::getVectElemTypeCvtIntrinsicInfo() const {
  static VectElemTypeCvtDesc Info[] = {
#define GetVectElemTypeCvt
#define GEN_VECT_ELEM_TYPE_CVT(INTRIN, VF) {INTRIN, VF},
#include "TensilicaGenSIMDInfo.inc"
#undef GetVectElemTypeCvt
      {0, 0}};
  return ArrayRef<VectElemTypeCvtDesc>(
      Info, sizeof(Info) / sizeof(VectElemTypeCvtDesc) - 1);
}

ArrayRef<VecLibFunDesc> RISCVSIMDInfo::getVecLibFuncInfo() const {
  static VecLibFunDesc Info[] = {
#define GetVecLibInfo
#define VecLib(SCALAR_FN_NAME, VECTOR_FN_NAME) {SCALAR_FN_NAME, VECTOR_FN_NAME},
#include "TensilicaGenSIMDInfo.inc"
#undef GetVecLibInfo
      {nullptr, nullptr}};
  return ArrayRef<VecLibFunDesc>(Info,
                                 sizeof(Info) / sizeof(VecLibFunDesc) - 1);
}

bool RISCVSIMDInfo::supportVectorizableFunction() const {
  bool SupportVecFunc = false;
#define GetSupportVecFunc
#include "TensilicaGenSIMDInfo.inc"
#undef GetSupportVecFunc
  return SupportVecFunc;
}

bool RISCVSIMDInfo::supportVectFuncWithNonMultiplesOfVectLength() const {
  bool SupportNonMultiplesOfVectLength = false;
#define GetSupportNonMultiplesOfVectLength
#include "TensilicaGenSIMDInfo.inc"
#undef GetSupportNonMultiplesOfVectLength
  return SupportNonMultiplesOfVectLength;
}

bool RISCVSIMDInfo::isVectorizedFunctionResFirst() const {
  bool ResFirst = false;
#define GetVecFuncResFirst
#include "TensilicaGenSIMDInfo.inc"
#undef GetVecFuncResFirst
  return ResFirst;
}

ArrayRef<unsigned> RISCVSIMDInfo::getAccPredicatedIntrinsicInfo() const {
  static unsigned Info[] = {
#define GetAccPredIntrinsicInfo
#include "TensilicaGenSIMDInfo.inc"
#undef GetAccPredIntrinsicInfo
      0};
  return ArrayRef<unsigned>(Info, sizeof(Info) / sizeof(unsigned) - 1);
}

ArrayRef<IntrinsicImmOpnd> RISCVSIMDInfo::getVectIntrinsicImmOpndInfo() const {
  static IntrinsicImmOpnd Info[] = {
#define GetVecIntrinsicImmOpnd
#define VecIntrinImmOpnd(INTRIN_ID, IMM_OPND_IDX) {INTRIN_ID, IMM_OPND_IDX},
#include "TensilicaGenSIMDInfo.inc"
#undef GetVecIntrinsicImmOpnd
      {0, 0}};

  return ArrayRef<IntrinsicImmOpnd>(
      Info, sizeof(Info) / sizeof(IntrinsicImmOpnd) - 1);
}

ArrayRef<TargetIntrinRedInfo> RISCVSIMDInfo::getIntrinsicReductionInfo() const {
  static TargetIntrinRedInfo Info[] = {
#define GetIntrinsicRed
#define IntrinRed(SCALAR_INTRIN_ID, VF, RED_CTYPE_ID, INIT_INTRIN_ID,          \
                  VEC_INTRIN_ID, FINAL_INTRIN_ID, INIT_TO_ZERO, IS_ORDERED)    \
  {SCALAR_INTRIN_ID,                                                           \
   VF,                                                                         \
   RED_CTYPE_ID,                                                               \
   INIT_INTRIN_ID + llvm::Intrinsic::num_intrinsics,                           \
   VEC_INTRIN_ID + llvm::Intrinsic::num_intrinsics,                            \
   FINAL_INTRIN_ID + llvm::Intrinsic::num_intrinsics,                          \
   INIT_TO_ZERO,                                                               \
   IS_ORDERED},
#include "TensilicaGenSIMDInfo.inc"
#undef GetIntrinsicRed
      {0, 0, 0, 0, 0, 0, 0, 0}};
  return ArrayRef<TargetIntrinRedInfo>(
      Info, sizeof(Info) / sizeof(TargetIntrinRedInfo) - 1);
}

ArrayRef<EpiLoopPredicateInfo> RISCVSIMDInfo::getEpiLoopPredicateInfo() const {
  static EpiLoopPredicateInfo Info[] = {
#define GetEpiLoopPredInfo
#define EpiLoopPred(INTRN_ID, VF, IMM, IS_NZ) {INTRN_ID, VF, IMM, IS_NZ},
#include "TensilicaGenSIMDInfo.inc"
#undef GetEpiLoopPredInfo
      {0, 0, false, false}};
  return ArrayRef<EpiLoopPredicateInfo>(
      Info, sizeof(Info) / sizeof(EpiLoopPredicateInfo) - 1);
}

ArrayRef<PredIntrinInfo> RISCVSIMDInfo::getPredicateIntrinsicInfo() const {
  static PredIntrinInfo Info[] = {
#define GetPredIntrinsicInfo
#define PredIntrinInfo(NORMAL_INTRN_ID, VF, TRUTH, PRED_INTRN_ID)              \
  {NORMAL_INTRN_ID, VF, TRUTH, PRED_INTRN_ID},
#include "TensilicaGenSIMDInfo.inc"
#undef GetPredIntrinsicInfo
      {0, 0, 0, 0}};
  return ArrayRef<PredIntrinInfo>(Info,
                                  sizeof(Info) / sizeof(PredIntrinInfo) - 1);
}

ArrayRef<TargetIntrinRedPartialSumInfo>
RISCVSIMDInfo::getRedPartialSumInfo() const {
  static TargetIntrinRedPartialSumInfo Info[] = {
#define GetRedPartialSumInfo
#define RedPartialSumInfo(INTRN_ID, RED_KIND, ZERO_INTRN_ID, OP_INTRN_ID,      \
                          LATENCY)                                             \
  {INTRN_ID, RED_KIND, ZERO_INTRN_ID, OP_INTRN_ID, LATENCY},
#include "TensilicaGenSIMDInfo.inc"
#undef GetRedPartialSumInfo
      {0, 0, 0, 0, 0}};
  return ArrayRef<TargetIntrinRedPartialSumInfo>(
      Info, sizeof(Info) / sizeof(TargetIntrinRedPartialSumInfo) - 1);
}

//----------------------------------------------------------------------------//
// RISCVSubtarget
//----------------------------------------------------------------------------//
#define GET_SUBTARGETINFO_CTOR
#include "RISCVGenSubtargetInfo.inc"
void RISCVSubtarget::initSubtargetFeatures(StringRef CPU, StringRef FS) {
  std::string ArchFS =
#define GET_FEATURES
#include "TensilicaGenFeatures.inc"
      ;
#undef GET_FEATURES

#define GET_NUMERIC_PARAMETERS
#include "TensilicaGenFeatures.inc"
#undef GET_NUMERIC_PARAMETERS

  // convert from bit to byte
  LoadStoreWidth /= 8;

  NumMemoryAccessBanks =
      (DataCacheBanks == 1 ||
       (ISSDataRAMBanks > 1 && DataCacheBanks > ISSDataRAMBanks))
          ? ISSDataRAMBanks
          : DataCacheBanks;

  if (!FS.empty())
    ArchFS = ArchFS + "," + FS.str();
  ParseSubtargetFeatures(CPU, ArchFS);
}

RISCVSubtarget &RISCVSubtarget::initializeSubtargetDependencies(
    const Triple &TT, StringRef CPU, StringRef FS, StringRef ABIName) {
#if defined(TENSILICA) || 1
  UserReservedRegister = BitVector(RISCV::NUM_TARGET_REGS);
#endif
  // Determine default and user-specified characteristics
  bool Is64Bit = TT.isArch64Bit();
  std::string CPUName = CPU;
#if defined(TENSILICA) || 1
  if (CPUName.empty())
    CPUName = "flix";
  initSubtargetFeatures(CPUName, FS);
#else
  if (CPUName.empty())
    CPUName = Is64Bit ? "generic-rv64" : "generic-rv32";
  ParseSubtargetFeatures(CPUName, FS);
#endif
  if (Is64Bit) {
    XLenVT = MVT::i64;
    XLen = 64;
  }

  TargetABI = RISCVABI::computeTargetABI(TT, getFeatureBits(), ABIName);
  RISCVFeatures::validate(TT, getFeatureBits());
  return *this;
}

static std::map<unsigned, SubregTransferInfo> OpcodeSubregTransfer = {
#define GET_SUBREG_TRANSFER
#include "TensilicaGenRewrite.inc"
#undef GET_SUBREG_TRANSFER
};

static std::vector<TensilicaConfigEQTable> ConfigEqTable = {
#define GET_VECTOR_EQ_INSTRS
#include "TensilicaGenRewrite.inc"
  {0, 0, 0, nullptr, nullptr, 0, 0}
  };
  
static std::vector<IntDTypes> DTypesRegClasses = {
#define GET_DTYPES
#include "TensilicaGenRewrite.inc"
  };

static std::map<unsigned, TargetOps> BaseTargetEquivalentTable = { 
#define BASE_OP_TO_TIE_EQUIVALENT
#include "TensilicaGenRewrite.inc"
  };

llvm::Tensilica::ConfigRewrite::ConfigRewrite():
  OpcodeSubregTransfer(::OpcodeSubregTransfer), ConfigEQTable(::ConfigEqTable),
  DTypes(::DTypesRegClasses),
  BaseTargetEquivalentTable(::BaseTargetEquivalentTable) {
}

//----------------------------------------------------------------------------//
// IMAP
//----------------------------------------------------------------------------//
#include "TensilicaGenIMAPGenerics.inc"
#include "TensilicaGenIMAPInstrAliases.inc"
#include "TensilicaGenIMAPPatterns.inc"

llvm::Tensilica::ConfigImap::ConfigImap()
    : ImapArraySize(::ImapArraySize), ImapArray(::ImapArray),
      GenericsOpcodeTable(::GenericsOpcodeTable),
      AliasesOpcodeTable(::AliasesOpcodeTable) {}

//----------------------------------------------------------------------------//
// Entry
//----------------------------------------------------------------------------//
extern "C" RISCVConfigInfo *
LLVMInitializeRISCVConfigInfo(const llvm::Triple &TargetTriple,
                              llvm::StringRef CPU, llvm::StringRef FS,
                              llvm::StringRef ABIName, unsigned HwMode) {
  static ManagedStatic<sys::SmartMutex<true> > InitMutex;
  sys::SmartScopedLock<true> Lock(*InitMutex);
  static RISCVConfigInfo Info;
  // Register info: Use the constructor generated by tablegen.
  static llvm::RISCVGenRegisterInfo RI(RISCV::X1, 0 /*DwarfFlavour*/,
                                       0 /*EHFlavour*/, 0 /*PC*/, HwMode);
  RI.CSR_ILP32D_LP64D_SaveList = CSR_ILP32D_LP64D_SaveList;
  RI.CSR_ILP32D_LP64D_RegMask = CSR_ILP32D_LP64D_RegMask;
  RI.CSR_ILP32F_LP64F_SaveList = CSR_ILP32F_LP64F_SaveList;
  RI.CSR_ILP32F_LP64F_RegMask = CSR_ILP32F_LP64F_RegMask;
  RI.CSR_ILP32_LP64_SaveList = CSR_ILP32_LP64_SaveList;
  RI.CSR_ILP32_LP64_RegMask = CSR_ILP32_LP64_RegMask;
  RI.CSR_Interrupt_SaveList = CSR_Interrupt_SaveList;
  RI.CSR_Interrupt_RegMask = CSR_Interrupt_RegMask;
  RI.CSR_NoRegs_SaveList = CSR_NoRegs_SaveList;
  RI.CSR_NoRegs_RegMask = CSR_NoRegs_RegMask;
  RI.CSR_XLEN_F32_Interrupt_SaveList = CSR_XLEN_F32_Interrupt_SaveList;
  RI.CSR_XLEN_F32_Interrupt_RegMask = CSR_XLEN_F32_Interrupt_RegMask;
  RI.CSR_XLEN_F64_Interrupt_SaveList = CSR_XLEN_F64_Interrupt_SaveList;
  RI.CSR_XLEN_F64_Interrupt_RegMask = CSR_XLEN_F64_Interrupt_RegMask;
  // TODO: Supporting the FP regclasses through config-gen as usual.
  RI.FPR32RegClass = &RISCV::FPR32RegClass;
  RI.FPR64RegClass = &RISCV::FPR64RegClass;

  RI.PtrRegClass = &RISCV::GPRRegClass;
#define GET_SHARED_OR_REGCLASS
  static const std::set<int> SharedOrSet{
#include "TensilicaGenSharedOrStates.inc"
  };
#undef GET_SHARED_OR_REGCLASS
  RI.SharedOrSet = &SharedOrSet;
  Info.RegisterInfo = &RI;
  // Instruction info.
  static llvm::Tensilica::ConfigInstrInfo InstrInfo;
  Info.InstrInfo = &InstrInfo;
  // Target Lowering info.
  bool UnsafeFPMath = false;
  bool CoProc = true;
  bool UseMul16 = true;

  static llvm::Tensilica::ConfigTargetLowering CTL(UnsafeFPMath,
                                                   CoProc, UseMul16);
  Info.LoweringInfo = &CTL;

  static std::map<unsigned, std::unique_ptr<llvm::RISCVConfigTargetLowering>>
      ConfigTLIs;
  auto &TLI = ConfigTLIs[1 * CoProc + 2 * UseMul16];
  if (!TLI) {
    // This needs to be done before we create a new subtarget since any
    // creation will depend on the TM and the code generation flags on the
    // function that reside in TargetOptions.
    // resetTargetOptions(F);
    TLI = std::make_unique<llvm::RISCVConfigTargetLowering>(UnsafeFPMath,
                                                            CoProc, UseMul16);
  }
  Info.TargetLoweringInfo = TLI.get();

  // Packetizer.
  static llvm::Tensilica::ConfigPacketizer PacketizerInfo;
  Info.PacketizerInfo = &PacketizerInfo;
  llvm::Tensilica::DFAPacketizer::preInitialize(&PacketizerInfo);

  // Imap.
  static llvm::Tensilica::ConfigImap ImapInfo;
  Info.ImapInfo = &ImapInfo;

  // SIMDInfo
  static RISCVSIMDInfo SIMDInfo;
  Info.SIMDInfo = &SIMDInfo;
  return &Info;
}
