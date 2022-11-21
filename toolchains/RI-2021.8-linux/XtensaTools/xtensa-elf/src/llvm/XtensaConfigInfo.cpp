//===-- XtensaConfigInfo.cpp - Xtensa TDK flow Implementation -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "XtensaConfig/XtensaConfigInfo.h"
#include "Tensilica/TensilicaConfig/TensilicaConfigInstrInfo.h"
#include "Tensilica/TensilicaConfig/TensilicaConfigPacketizer.h"
#include "Tensilica/TensilicaConfig/TensilicaConfigTargetLowering.h"
#include "Tensilica/TensilicaConfig/TensilicaConfigSIMDInfo.h"
#include "Tensilica/TensilicaConfig/TensilicaConfigImap.h"
#include "Tensilica/TensilicaDFAPacketizer.h"
#include "Tensilica/TensilicaInstrInfo.h"
#include "Tensilica/TensilicaIMAP.h"
#include "Tensilica/TensilicaConfig/TensilicaConfigRewrite.h"
#include "XtensaConfig/XtensaConfigTargetLowering.h"
#include "XtensaIntrinsicInfo.h"
#include "XtensaRegisterInfo.h"
#include "Tensilica/TensilicaRewrite.h"
#include "XtensaSubtarget.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/Mutex.h"

using namespace llvm;
using namespace llvm::Tensilica;

static bool isLittleEndian() {
  bool BigEndian = false;
#define GET_ENDIAN
#include "TensilicaGenFeatures.inc"
#undef GET_ENDIAN
  return !BigEndian;
}

//----------------------------------------------------------------------------//
// Register info
//----------------------------------------------------------------------------//
#define GET_REGINFO_HEADER
#define GET_REGCLASSES
#define GET_REGINFO_TARGET_DESC
#include "XtensaGenRegisterInfo.inc"

//----------------------------------------------------------------------------//
// Instruction info
//----------------------------------------------------------------------------//
namespace llvm {
extern void InitXtensaMCInstrInfo(MCInstrInfo *II);
}

llvm::Tensilica::ConfigInstrInfo::~ConfigInstrInfo() {}

#define GET_INSTR_IMM_RANGE_FUNCS
#include "TensilicaGenInstrProperty.inc"

static int TensilicaInstrPropMap[] = {
#define GET_INSTR_TARGET_PROP_MAP
#include "XtensaGenInstrInfo.inc"
    0};

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
#define XTENSA_ENUM_TO_OPCODE
#include "Tensilica/OpcodeMap.h"
#define ENUM_TO_OPCODE
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
      OpcodeToEnum{
#define XTENSA_OPCODE_TO_ENUM 
#include "Tensilica/OpcodeMap.h"
#define OPCODE_TO_ENUM
#include "TensilicaGenRewrite.inc"
      },
      initTensilicaMCInstrInfo(&llvm::InitXtensaMCInstrInfo) {
}

//----------------------------------------------------------------------------//
// Calling conventions
//----------------------------------------------------------------------------//
namespace llvm {
static ArrayRef<MCPhysReg> getRegList(MVT::SimpleValueType SVT) {
  struct XtensaMVT2RegList {
    MVT VT;
    std::vector<MCPhysReg> RegList;
  };

  static const XtensaMVT2RegList MVT2RL[] = {
      {MVT::i32,
       {Xtensa::a2, Xtensa::a3, Xtensa::a4, Xtensa::a5, Xtensa::a6,
        Xtensa::a7}},
#define GET_TYPE_PARAM_REG_LIST
#include "TensilicaGenTypeCallRegs.inc"
      {MVT::Other, {}}};

  int i;
  for (i = 0; MVT2RL[i].VT != MVT::Other; i++)
    if (MVT2RL[i].VT == SVT)
      return MVT2RL[i].RegList;

  llvm_unreachable("unexpected type");
}

// Allocate consecutive registers or stack slots for an aggregate
// parameter.  We assume that each member of the HA has
// InConsecutiveRegs set, and that the last member also has
// InConsecutiveRegsLast set.  We must process all members of the HA
// before we can allocate it, as we need to know the total number of
// registers that will be needed in order to (attempt to) allocate a
// contiguous block.
static bool CC_Xtensa_Custom_Aggregate(unsigned &ValNo, MVT &ValVT, MVT &LocVT,
                                       CCValAssign::LocInfo &LocInfo,
                                       ISD::ArgFlagsTy &ArgFlags,
                                       CCState &State) {
  SmallVectorImpl<CCValAssign> &PendingMembers = State.getPendingLocs();

   // All the elements must be of the same type.
  assert((!PendingMembers.size() || PendingMembers[0].getLocVT() == LocVT) &&
         "All the elements must be of the same type");

  // Add the argument to the list to be allocated once we know the
  // size of the aggregate.  Store the type's required alignment as
  // extra info for later.  In the [N x i64] case, all trace has been
  // removed by the time we actually get to do allocation.
  PendingMembers.push_back(CCValAssign::getPending(ValNo, ValVT, LocVT, LocInfo,
                                                   ArgFlags.getOrigAlign()));

  if (!ArgFlags.isInConsecutiveRegsLast())
    return true;

  // Try to allocate a contiguous block of registers, each of the correct
  // size to hold one member.
  auto &DL = State.getMachineFunction().getDataLayout();
  unsigned StackAlign = DL.getStackAlignment().value();
  unsigned Align = std::min(PendingMembers[0].getExtraInfo(), StackAlign);

  ArrayRef<MCPhysReg> RegList = getRegList(LocVT.SimpleTy);

  if (LocVT.SimpleTy == MVT::i32) {
    unsigned RegIdx = State.getFirstUnallocated(RegList);

    // First consume all registers that would give an unaligned
    // object.  Whether we go on stack or in regs, no one will be
    // using them in future.
    unsigned RegAlign = alignTo(Align, 4) / 4;
    while (RegIdx % RegAlign != 0 && RegIdx < RegList.size())
      State.AllocateReg(RegList[RegIdx++]);
  }

  unsigned RegResult = State.AllocateRegBlock(RegList, PendingMembers.size());
  if (RegResult) {
    for (auto el : PendingMembers) {
      el.convertToReg(RegResult);
      State.addLoc(el);
      ++RegResult;
    }
    PendingMembers.clear();
    return true;
  }

  // Register allocation failed.  We'll be needing the stack.
  unsigned Size = LocVT.getSizeInBits() / 8;

  // Mark all regs as unavailable.
  for (auto Reg : RegList)
    State.AllocateReg(Reg);

  for (auto &It : PendingMembers) {
    It.convertToMem(State.AllocateStack(Size, Align));
    State.addLoc(It);

    // After the first item has been allocated, the rest are packed as
    // tightly as possible. (E.g. an incoming i64 would have starting
    // Align of 8, but we'll be allocating a bunch of i32 slots).
    Align = Size;
  }

  // All pending members have now been allocated
  PendingMembers.clear();

  // This will be allocated by the last member of the aggregate
  return true;
}

static bool CC_Xtensa_ARF64(unsigned ValNo, MVT ValVT, MVT LocVT,
                            CCValAssign::LocInfo LocInfo,
                            ISD::ArgFlagsTy ArgFlags, CCState &State) {
  assert(LocVT == MVT::f64 && "Expected f64");

  // Attempt to allocate a pair of AR registers for MVT::f64.  The
  // following code is identical to CCAssignToRegWithShadow except
  // that we allocate one more AR register.
  static const MCPhysReg FirstRegs[] = {Xtensa::a2, Xtensa::a4, Xtensa::a6};
  static const MCPhysReg SecondRegs[] = {
      Xtensa::a3,
      Xtensa::a5,
      Xtensa::a7,
  };
  static const MCPhysReg ShadowRegs[] = {Xtensa::a2, Xtensa::a3, Xtensa::a5};
  if (unsigned Reg = State.AllocateReg(FirstRegs, ShadowRegs)) {
    // Allocate one more AR register, but do not record its location.
    unsigned SecondReg = State.AllocateReg(SecondRegs);
    (void)SecondReg;
    assert(SecondReg != 0 && "Couldn't allocate the second reg");

    State.addLoc(CCValAssign::getReg(ValNo, ValVT, Reg, LocVT, LocInfo));
    return true;
  }

  return false;
}

static bool RetCC_Xtensa_ARF64(unsigned ValNo, MVT ValVT, MVT LocVT,
                               CCValAssign::LocInfo LocInfo,
                               ISD::ArgFlagsTy ArgFlags, CCState &State) {
  assert(LocVT == MVT::f64 && "Expected f64");

  // Attempt to allocate a pair of AR registers for MVT::f64.  The
  // following code is identical to CCAssignToRegWithShadow except
  // that we allocate one more AR register.
  static const MCPhysReg FirstRegs[] = {Xtensa::a2, Xtensa::a4};
  static const MCPhysReg SecondRegs[] = {Xtensa::a3, Xtensa::a5};
  static const MCPhysReg ShadowRegs[] = {Xtensa::a2, Xtensa::a3};
  if (unsigned Reg = State.AllocateReg(FirstRegs, ShadowRegs)) {
    // Allocate one more AR register, but do not record its location.
    unsigned SecondReg = State.AllocateReg(SecondRegs);
    (void)SecondReg;
    assert(SecondReg != 0 && "Couldn't allocate the second reg");

    State.addLoc(CCValAssign::getReg(ValNo, ValVT, Reg, LocVT, LocInfo));
    return true;
  }

  return false;
}

} // namespace llvm

#include "XtensaGenCallingConv.inc"

//----------------------------------------------------------------------------//
// Target Lowering info
//----------------------------------------------------------------------------//
#define GET_OPERATION_ISEL_LOWER_INFO_FUNC
#include "TensilicaGenOperationISelLower.inc"

#define GET_INTRN_ISEL_LOWER_IMM_PRED
#include "TensilicaGenIntrinsicISelLower.inc"

static unsigned XtensaIntrinsicISelInfoIdxs[] = {
#define GET_INTRN_ISEL_LOWER_INFO_IDX
#include "TensilicaGenIntrinsicISelLower.inc"
};

static Tensilica::RegTransferInfo XtensaRegTransferInfos[] = {
#define GET_REG_TRANSFER_INFO
#include "TensilicaGenRegTransfer.inc"
    {0xffffffff, 0, 0, 0, 0, 0, 0, 0, false}};

static Tensilica::PseudoExpandInfo XtensaPseudoExpandInfos[] = {
    {0, Intrinsic::not_intrinsic}
#define GET_PSEUDO_EXPAND_INFO
#include "TensilicaGenPseudoExpand.inc"
};

static Tensilica::IntrinsicISelInfo XtensaIntrinsicISelInfos[] = {
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

static Tensilica::OpISelInfo XtensaOperationISelInfos[] = {
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
static Tensilica::OpAction XtensaOpActions[] = {
    {ISD::ADD, MVT::i32, Legal, Tensilica::OpAction::NoPred}
#define GET_OP_ACTION
#include "TensilicaGenOpAction.inc"
};

static Tensilica::LoadStoreAction XtensaLoadStoreActions[] = {
    {ISD::LOAD, ISD::NON_EXTLOAD, MVT::i32, MVT::i32, Legal}
#define GET_LDST_OP_ACTION
#include "TensilicaGenLoadStoreAction.inc"
};

static Tensilica::PromotedOpAction XtensaPromotedOpActions[] = {
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
    : PseudoExpandInfos(XtensaPseudoExpandInfos,
                        sizeof(XtensaPseudoExpandInfos) /
                            sizeof(Tensilica::PseudoExpandInfo)),
      RegTransferInfos(XtensaRegTransferInfos,
                       sizeof(XtensaRegTransferInfos) /
                           sizeof(Tensilica::RegTransferInfo)),
      IntrinsicISelInfos(XtensaIntrinsicISelInfos,
                         sizeof(XtensaIntrinsicISelInfos) /
                             sizeof(Tensilica::IntrinsicISelInfo)),
      OperationISelInfos(XtensaOperationISelInfos,
                         sizeof(XtensaOperationISelInfos) /
                             sizeof(Tensilica::OpISelInfo)),
      IntrinsicISelInfoIdxs(XtensaIntrinsicISelInfoIdxs,
                            sizeof(XtensaIntrinsicISelInfoIdxs) /
                                sizeof(unsigned)),
      OpActions(XtensaOpActions,
                sizeof(XtensaOpActions) / sizeof(Tensilica::OpAction)),
      LoadStoreActions(XtensaLoadStoreActions,
                       sizeof(XtensaLoadStoreActions) /
                           sizeof(Tensilica::LoadStoreAction)),
      PromotedOpActions(XtensaPromotedOpActions,
                        sizeof(XtensaPromotedOpActions) /
                            sizeof(Tensilica::PromotedOpAction)),
      VSelImmMatchFuncMap{
#define GET_VSEL_IMM_MATCH_FUNCTIONS
#include "TensilicaGenOperationISelLower.inc"
          {0, nullptr}} {
  assert(std::is_sorted(OperationISelInfos.begin(), OperationISelInfos.end()) &&
         "Unsorted OperationISelInfos");
}

static Tensilica::OpDesc XtensaOpDescs[] = {{ISD::ADD, MVT::i32}
#define GET_XTENSA_SUPPORTED_OPS
#include "TensilicaGenOpAction.inc"
};

static llvm::MVT XtensaVshuffleMvts[] = {MVT::Other
#define GET_GENERIC_VSHUFFLE_MVT
#include "TensilicaGenOpAction.inc"
};

static llvm::MVT XtensaSelectMvts[] = {MVT::i32
#define GET_SELECT_MVT
#include "TensilicaGenOpAction.inc"
};

static llvm::MVT XtensaUnalignedMvts[] = {MVT::INVALID_SIMPLE_VALUE_TYPE
#define GET_UNALIGNED_MVT
#include "TensilicaGenOpAction.inc"
};

static TypeDesc XtensaConsecutiveTypes[] = {
#define GET_TYPE_PARAM_CONSECUTIVE_REG
#include "TensilicaGenConsecutiveCallRegs.inc"
    {Type::VoidTyID, 0, 0}};

llvm::XtensaConfigTargetLowering::XtensaConfigTargetLowering(bool UnsafeFPMath,
                                                             bool CoProc,
                                                             bool UseMul16)

    : UnsafeFPMath(UnsafeFPMath),
      XtensaOps(XtensaOpDescs, 
                sizeof(XtensaOpDescs) / sizeof(Tensilica::OpDesc)),
      VshuffleMvts(XtensaVshuffleMvts,
                   sizeof(XtensaVshuffleMvts) / sizeof(llvm::MVT)),
      SelectMvts(XtensaSelectMvts,
                 sizeof(XtensaSelectMvts) / sizeof(llvm::MVT)),
      UnalignedMvts(XtensaUnalignedMvts,
                    sizeof(XtensaUnalignedMvts) / sizeof(llvm::MVT)),
      ConsecutiveTypes(XtensaConsecutiveTypes,
                       sizeof(XtensaConsecutiveTypes) / sizeof(TypeDesc)),
      Names2Regs{
          {"a0", Xtensa::LR},   {"a1", Xtensa::SP},   {"a2", Xtensa::a2},
          {"a3", Xtensa::a3},   {"a4", Xtensa::a4},   {"a5", Xtensa::a5},
          {"a6", Xtensa::a6},   {"a7", Xtensa::a7},   {"a8", Xtensa::a8},
          {"a9", Xtensa::a9},   {"a10", Xtensa::a10}, {"a11", Xtensa::a11},
          {"a12", Xtensa::a12}, {"a13", Xtensa::a13}, {"a14", Xtensa::a14},
          {"a15", Xtensa::a15},
#define GET_REGNAME_TO_REGNUM
#include "TensilicaGenRegNameToRegNum.inc"
#undef GET_REGNAME_TO_REGNUM
      },
      CC_Xtensa(::CC_Xtensa), CC_Xtensa_OldFPABI(::CC_Xtensa_OldFPABI),
      CC_Xtensa_VarArg(::CC_Xtensa_VarArg),
      CC_Xtensa_VecVarArg(::CC_Xtensa_VecVarArg), RetCC_Xtensa(::RetCC_Xtensa),
      RetCC_Xtensa_OldFPABI(::RetCC_Xtensa_OldFPABI) {}

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
      InstructionListEnd(Xtensa::INSTRUCTION_LIST_END), MoveAr(Xtensa::MOVE_AR),
      Nop(Xtensa::NOP), XsrSar(Xtensa::XSR_SAR) {
  using namespace Xtensa;
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
class XtensaSIMDInfo : public ConfigSIMDInfo {
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

ArrayRef<VFPriorityInfo> XtensaSIMDInfo::getVFPriorityInfo() const {
  static VFPriorityInfo Info[] = {
#define GetVFPriority
#include "TensilicaGenSIMDInfo.inc"
#undef GetVFPriority
      {UnkownBaseType, 0, {}}};
  return ArrayRef<VFPriorityInfo>(Info,
                                  sizeof(Info) / sizeof(VFPriorityInfo) - 1);
}

ArrayRef<SIMDTypeInfo> XtensaSIMDInfo::getSIMDTypeInfo() const {
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
XtensaSIMDInfo::getVselIntrinInfo() const {
  static TargetSIMDInfo::VselIntrinInfo Info[] = {
#define GetVselIntrinsicInfo
#include "TensilicaGenSIMDInfo.inc"
#undef GetVselIntrinsicInfo
      {0, 0, 0, false}};
  return ArrayRef<TargetSIMDInfo::VselIntrinInfo>(
      Info, sizeof(Info) / sizeof(TargetSIMDInfo::VselIntrinInfo) - 1);
}

int XtensaSIMDInfo::getScalarBoolTy() const {
  int ScalarBoolTy = -1;
#define GetScalarBoolTy
#include "TensilicaGenSIMDInfo.inc"
#undef GetScalarBoolTy
  return ScalarBoolTy;
}

ArrayRef<CtypeCvtInfo> XtensaSIMDInfo::getCtypeRTORCVTInfo() const {
  static CtypeCvtInfo Info[] = {
#define GetRTORCVT
#define GEN_RTOR(SRC_CTYPE_ID, TARGET_CTYPE_ID, INTRINSIC_ID)                  \
  {SRC_CTYPE_ID, TARGET_CTYPE_ID, INTRINSIC_ID},
#include "TensilicaGenSIMDInfo.inc"
#undef GetRTORCVT
      {0, 0, Intrinsic::not_intrinsic}};
  return ArrayRef<CtypeCvtInfo>(Info, sizeof(Info) / sizeof(CtypeCvtInfo) - 1);
}

ArrayRef<CtypeCvtInfo> XtensaSIMDInfo::getCtypeRTOMCVTInfo() const {
  static CtypeCvtInfo Info[] = {
#define GetRTOMCVT
#define GEN_RTOM(SRC_CTYPE_ID, TARGET_CTYPE_ID, INTRINSIC_ID)                  \
  {SRC_CTYPE_ID, TARGET_CTYPE_ID, INTRINSIC_ID},
#include "TensilicaGenSIMDInfo.inc"
#undef GetRTOMCVT
      {0, 0, Intrinsic::not_intrinsic}};

  return ArrayRef<CtypeCvtInfo>(Info, sizeof(Info) / sizeof(CtypeCvtInfo) - 1);
}
ArrayRef<CtypeCvtInfo> XtensaSIMDInfo::getCtypeMTORCVTInfo() const {
  static CtypeCvtInfo Info[] = {
#define GetMTORCVT
#define GEN_MTOR(SRC_CTYPE_ID, TARGET_CTYPE_ID, INTRINSIC_ID)                  \
  {SRC_CTYPE_ID, TARGET_CTYPE_ID, INTRINSIC_ID},
#include "TensilicaGenSIMDInfo.inc"
#undef GetMTORCVT
      {0, 0, Intrinsic::not_intrinsic}};
  return ArrayRef<CtypeCvtInfo>(Info, sizeof(Info) / sizeof(CtypeCvtInfo) - 1);
}

ArrayRef<SIMDOpInfo> XtensaSIMDInfo::getSIMDOpInfo() const {
  static SIMDOpInfo Info[] = {
#define GetSIMDOps
#define SIMDOp(TI_OP, DESC_CTYPE_ID, VF, RES_CTYPE_ID, INTRINSIC_ID)           \
  {TI_OP, DESC_CTYPE_ID, VF, RES_CTYPE_ID, INTRINSIC_ID},
#include "TensilicaGenSIMDInfo.inc"
#undef GetSIMDOps
      {xt_unknown, -1, 0, -1, Intrinsic::not_intrinsic}};
  return ArrayRef<SIMDOpInfo>(Info, sizeof(Info) / sizeof(SIMDOpInfo) - 1);
}

ArrayRef<VecIntrinsicDesc> XtensaSIMDInfo::getVectIntrinsicInfo() const {
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
XtensaSIMDInfo::getVectElemTypeCvtIntrinsicInfo() const {
  static VectElemTypeCvtDesc Info[] = {
#define GetVectElemTypeCvt
#define GEN_VECT_ELEM_TYPE_CVT(INTRIN, VF) {INTRIN, VF},
#include "TensilicaGenSIMDInfo.inc"
#undef GetVectElemTypeCvt
      {0, 0}};
  return ArrayRef<VectElemTypeCvtDesc>(
      Info, sizeof(Info) / sizeof(VectElemTypeCvtDesc) - 1);
}

ArrayRef<VecLibFunDesc> XtensaSIMDInfo::getVecLibFuncInfo() const {
  static VecLibFunDesc Info[] = {
#define GetVecLibInfo
#define VecLib(SCALAR_FN_NAME, VECTOR_FN_NAME) {SCALAR_FN_NAME, VECTOR_FN_NAME},
#include "TensilicaGenSIMDInfo.inc"
#undef GetVecLibInfo
      {nullptr, nullptr}};
  return ArrayRef<VecLibFunDesc>(Info,
                                 sizeof(Info) / sizeof(VecLibFunDesc) - 1);
}

bool XtensaSIMDInfo::supportVectorizableFunction() const {
  bool SupportVecFunc = false;
#define GetSupportVecFunc
#include "TensilicaGenSIMDInfo.inc"
#undef GetSupportVecFunc
  return SupportVecFunc;
}

bool XtensaSIMDInfo::supportVectFuncWithNonMultiplesOfVectLength() const {
  bool SupportNonMultiplesOfVectLength = false;
#define GetSupportNonMultiplesOfVectLength
#include "TensilicaGenSIMDInfo.inc"
#undef GetSupportNonMultiplesOfVectLength
  return SupportNonMultiplesOfVectLength;
}

bool XtensaSIMDInfo::isVectorizedFunctionResFirst() const {
  bool ResFirst = false;
#define GetVecFuncResFirst
#include "TensilicaGenSIMDInfo.inc"
#undef GetVecFuncResFirst
  return ResFirst;
}

ArrayRef<unsigned> XtensaSIMDInfo::getAccPredicatedIntrinsicInfo() const {
  static unsigned Info[] = {
#define GetAccPredIntrinsicInfo
#include "TensilicaGenSIMDInfo.inc"
#undef GetAccPredIntrinsicInfo
      0};
  return ArrayRef<unsigned>(Info, sizeof(Info) / sizeof(unsigned) - 1);
}

ArrayRef<IntrinsicImmOpnd> XtensaSIMDInfo::getVectIntrinsicImmOpndInfo() const {
  static IntrinsicImmOpnd Info[] = {
#define GetVecIntrinsicImmOpnd
#define VecIntrinImmOpnd(INTRIN_ID, IMM_OPND_IDX) {INTRIN_ID, IMM_OPND_IDX},
#include "TensilicaGenSIMDInfo.inc"
#undef GetVecIntrinsicImmOpnd
      {0, 0}};

  return ArrayRef<IntrinsicImmOpnd>(
      Info, sizeof(Info) / sizeof(IntrinsicImmOpnd) - 1);
}

ArrayRef<TargetIntrinRedInfo>
XtensaSIMDInfo::getIntrinsicReductionInfo() const {
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

ArrayRef<EpiLoopPredicateInfo> XtensaSIMDInfo::getEpiLoopPredicateInfo() const {
  static EpiLoopPredicateInfo Info[] = {
#define GetEpiLoopPredInfo
#define EpiLoopPred(INTRN_ID, VF, IMM, IS_NZ) {INTRN_ID, VF, IMM, IS_NZ},
#include "TensilicaGenSIMDInfo.inc"
#undef GetEpiLoopPredInfo
      {0, 0, false, false}};
  return ArrayRef<EpiLoopPredicateInfo>(
      Info, sizeof(Info) / sizeof(EpiLoopPredicateInfo) - 1);
}

ArrayRef<PredIntrinInfo> XtensaSIMDInfo::getPredicateIntrinsicInfo() const {
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
XtensaSIMDInfo::getRedPartialSumInfo() const {
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
// XtensaSubtarget
//----------------------------------------------------------------------------//
#define GET_SUBTARGETINFO_CTOR
#include "XtensaGenSubtargetInfo.inc"
void XtensaSubtarget::initSubtargetFeatures(StringRef CPU, StringRef FS) {
  std::string CPUName = CPU;
  if (CPUName.empty())
    CPUName = "flix";
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
  ParseSubtargetFeatures(CPUName, ArchFS);
}

XtensaSubtarget &
XtensaSubtarget::initializeSubtargetDependencies(StringRef CPU, StringRef FS) {
  initSubtargetFeatures(CPU, FS);
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
extern "C" XtensaConfigInfo *
LLVMInitializeXtensaConfigInfo(llvm::StringRef FS,
                               const llvm::TargetOptions &Options) {
  static ManagedStatic<sys::SmartMutex<true> > InitMutex;
  sys::SmartScopedLock<true> Lock(*InitMutex);
  static XtensaConfigInfo Info;
  Info.IsLittleEndian = isLittleEndian();
  // Register info: Use the constructor generated by tablegen.
  static llvm::XtensaGenRegisterInfo RI(Xtensa::LR);
  RI.CALL0_CSRs_SaveList = CALL0_CSRs_SaveList;
  RI.CALL0_CSRs_RegMask = CALL0_CSRs_RegMask;
  RI.CALL8_CSRs_SaveList = CALL8_CSRs_SaveList;
  RI.CALL8_CSRs_RegMask = CALL8_CSRs_RegMask;
  RI.PtrRegClass = &Xtensa::ARRegClass;
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
  bool UnsafeFPMath = Options.UnsafeFPMath;
  bool CoProc = FS.count("+coproc");
  bool UseMul16 = FS.count("+mul16");

  static llvm::Tensilica::ConfigTargetLowering CTL(UnsafeFPMath, CoProc,
                                                   UseMul16);
  Info.LoweringInfo = &CTL;

  static std::map<unsigned, std::unique_ptr<llvm::XtensaConfigTargetLowering>>
      ConfigTLIs;
  auto &TLI = ConfigTLIs[1 * CoProc + 2 * UseMul16];
  if (!TLI) {
    // This needs to be done before we create a new subtarget since any
    // creation will depend on the TM and the code generation flags on the
    // function that reside in TargetOptions.
    // resetTargetOptions(F);
    TLI = std::make_unique<llvm::XtensaConfigTargetLowering>(UnsafeFPMath,
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

  static llvm::Tensilica::ConfigRewrite RewriteInfo;
  Info.RewriteInfo = &RewriteInfo;

  // SIMDInfo
  static XtensaSIMDInfo SIMDInfo;
  Info.SIMDInfo = &SIMDInfo;

  return &Info;
}
