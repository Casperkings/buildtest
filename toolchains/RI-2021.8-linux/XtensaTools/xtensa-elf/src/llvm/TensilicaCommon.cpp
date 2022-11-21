#include "TensilicaCommon.h"
#include "clang/Basic/MacroBuilder.h"
#include "clang/Basic/TargetInfo.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/TargetIntrinsicRegistry.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/TargetParser.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/XtensaXtparams.h"
#include "llvm/Target/TargetIntrinsicInfo.h"
#include <xtensa-versions.h>

using namespace llvm;
using namespace clang;

/// Ctype operator info tables
static const TensilicaCtypeOpInfo CtypeOpInfos[] = {
#define GET_CTYPE_OP_INFO
#include "TensilicaCtypeOps.def"
#undef GET_CTYPE_OP_INFO
    {{{0, 0}}}};

static const unsigned CtypeOpInfoSize =
    sizeof(CtypeOpInfos) / sizeof(TensilicaCtypeOpInfo) - 1;

/// Binary operator table
static const TensilicaLookupTblBinOPEntry CtypeBinOpTbl[] = {
#define GET_BINARY_OPS
#define GEN_CTYPE_BIN_OPERATOR(ID, OP, LHS, RHS, RET, BUILTIN)                 \
  {RHS, RET, BUILTIN},
#include "TensilicaCtypeOps.def"
#undef GET_BINARY_OPS
    {0, 0, 0},
};

static const unsigned CtypeBinOpTblSize =
    sizeof(CtypeBinOpTbl) / sizeof(TensilicaLookupTblBinOPEntry) - 1;

/// Data conversion RtoR table
static TensilicaLookupTblCvtOPEntry CtypeRTORCvtTbl[] = {
#define GET_OP_RTOR
#define GEN_CTYPE_RTOR(ID, SRC_TY, TGT_TY, BUILTIN) {TGT_TY, BUILTIN},
#include "TensilicaCtypeOps.def"
#undef GET_OP_RTOR
    {0, 0}};
static const unsigned CtypeRTORCvtTblSize =
    sizeof(CtypeRTORCvtTbl) / sizeof(TensilicaLookupTblCvtOPEntry) - 1;

/// Data conversion RtoM table
static const TensilicaLookupTblCvtOPEntry CtypeRTOMCvtTbl[] = {
#define GET_OP_RTOM
#define GEN_CTYPE_RTOM(ID, SRC_TY, TGT_TY, BUILTIN) {TGT_TY, BUILTIN},
#include "TensilicaCtypeOps.def"
#undef GET_OP_RTOM
    {0, 0},
};

static const unsigned CtypeRTOMCvtTblSize =
    sizeof(CtypeRTOMCvtTbl) / sizeof(TensilicaLookupTblCvtOPEntry) - 1;

/// Data conversion MtoR table
static const TensilicaLookupTblCvtOPEntry CtypeMTORCVTTable[] = {
#define GET_OP_MTOR
#define GEN_CTYPE_MTOR(ID, SRC_TY, TGT_TY, BUILTIN) {TGT_TY, BUILTIN},
#include "TensilicaCtypeOps.def"
#undef GET_OP_MTOR
    {0, 0},
};

static const unsigned CtypeMTORCVTTableSize =
    sizeof(CtypeMTORCVTTable) / sizeof(TensilicaLookupTblCvtOPEntry) - 1;

/// Although most ctypes have only one eq ctype, we still use a table structure,
/// in case some ctypes have more than one eq ctypes
static const TensilicaEqCtypeEntry EqCtypeTbl[] = {
#define GET_EQUIVALENT_CTYPES
#define EQUIVALENT_CTYPE(SRC_TYPE, DST_TYPE) DST_TYPE,
#include "TensilicaClangCtypes.def"
#undef GET_EQUIVALENT_CTYPES
    0};
static const unsigned EqCtypeTblSize =
    sizeof(EqCtypeTbl) / sizeof(TensilicaEqCtypeEntry) - 1;

/// Basic information table
static const TensilicaCtypeInfo CtypeInfoTbl[] = {
#define GET_CTYPE_INFO
#define GEN_XTENSA_CTYPE_INFO(ID, NAME, BITWIDTH, ALIGNMENT, PROMOTE_TYPE,     \
                              EQ_CTYPE_TAB_START, NUM_EQ_CTYPES, ELEM_TYPE,    \
                              NUM_ELEMS, REGCLASS_ID, NUM_REGS, PROP)          \
  {#NAME,         BITWIDTH,  ALIGNMENT, PROMOTE_TYPE, EQ_CTYPE_TAB_START,      \
   NUM_EQ_CTYPES, ELEM_TYPE, NUM_ELEMS, REGCLASS_ID,  NUM_REGS,                \
   PROP},
#include "TensilicaClangCtypes.def"
#undef GET_CTYPE_INFO
    {"unknown", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

static const unsigned CtypeInfoTblSize =
    sizeof(CtypeInfoTbl) / sizeof(TensilicaCtypeInfo) - 1;

static const TensilicaRegFileInfo RegFileInfoTbl[] = {
#define GET_REG_FILE_INFO
#define REG_FILE_INFO(ID, SHORT_NAME, NUM_REGS) {#SHORT_NAME, NUM_REGS},
#include "TensilicaClangCtypes.def"
#undef GET_REG_FILE_INFO
    {"unknown", 0}};

static const unsigned RegFileInfoTblSize =
    sizeof(RegFileInfoTbl) / sizeof(TensilicaRegFileInfo) - 1;

//===----------------------------------------------------------------------===//
//                        Complex Floating-point Types                        //
//===----------------------------------------------------------------------===//
static TargetComplexTypeOpInfo XtComplexFloatOpInfo = {
#define GET_COMPLEX_FLOAT_INFO
#define GEN_COMPLEX_FLOAT_INFO(OP_NAME, BUILTIN) BUILTIN,
#include "TensilicaCtypeOps.def"
#undef GET_COMPLEX_FLOAT_INFO
};

static TargetComplexTypeOpInfo XtComplexDoubleOpInfo = {
#define GET_COMPLEX_DOUBLE_INFO
#define GEN_COMPLEX_DOUBLE_INFO(OP_NAME, BUILTIN) BUILTIN,
#include "TensilicaCtypeOps.def"
#undef GET_COMPLEX_DOUBLE_INFO
};

static TensilicaTables
    TenConfigTbls(CtypeOpInfos, CtypeOpInfoSize, CtypeInfoTbl, CtypeInfoTblSize,
                  CtypeRTORCvtTbl, CtypeRTORCvtTblSize, CtypeMTORCVTTable,
                  CtypeMTORCVTTableSize, CtypeRTOMCvtTbl, CtypeRTOMCvtTblSize,
                  EqCtypeTbl, EqCtypeTblSize, CtypeBinOpTbl, CtypeBinOpTblSize);

/// Return the internal operator ID of a given Clang binary operator \p Kind
static unsigned getCtypeOpKind(BinaryOperatorKind Kind) {
  switch (Kind) {
  default:
    break;
  case BO_Add:
    return CtypeADD;
  case BO_Sub:
    return CtypeSUB;
  case BO_Mul:
    return CtypeMUL;
  case BO_Div:
    return CtypeDIV;
  case BO_Rem:
    return CtypeREM;
  case BO_Shl:
    return CtypeSHL;
  case BO_Shr:
    return CtypeSHR;
  case BO_LT:
    return CtypeLT;
  case BO_GT:
    return CtypeGT;
  case BO_LE:
    return CtypeLE;
  case BO_GE:
    return CtypeGE;
  case BO_EQ:
    return CtypeEQ;
  case BO_NE:
    return CtypeNE;
  case BO_And:
    return CtypeAND;
  case BO_Xor:
    return CtypeXOR;
  case BO_Or:
    return CtypeOR;
  case BO_LOr:
    return CtypeLOR;
  case BO_LAnd:
    return CtypeLAND;
  }
  return CtypeInvalidOP;
}

/// Return the internal operator ID of a given Clang unary operator \p Kind
static unsigned getCtypeOpKind(UnaryOperatorKind Kind) {
  switch (Kind) {
  default:
    break;
  case UO_LNot:
    return CtypeLNOT;
  case UO_Not:
    return CtypeNOT;
  case UO_Minus:
    return CtypeMinus;
  }
  return CtypeInvalidOP;
}

TensilicaTargetInfo::TensilicaTargetInfo(const llvm::Triple &Triple)
    : TargetInfo(Triple) {
  BigEndian = false;
  HasTargetExtendedTypes = true;
  // load config from BuiltinsTensilica.def
  IsHardFloat = false;
  HardHalfCtype = -1;
  HardFloatCtype = -1;
  HardDoubleCtype = -1;
  // TBD: check the usage of HardFloatComplexCtype and
  // HardFloatComplexCtype
  HardFloatComplexCtype = -1;
  HardDoubleComplexCtype = -1;
  HasHardSync = false;
  LdStWidth = 0;

  HasDiv32 = false;
  HasMul32 = false;
  HasMul32h = false;
  HasClamps = false;
  HasCall4 = false;
  HasConst16 = false;
  HasDensity = false;
  HasDepbits = false;
  HasLoop = false;
  HasMac16 = false;
  HasMinmax = false;
  HasMul16 = false;
  HasNsa = false;
  HasSext = false;
  HasS32c1i = false;
  HasBooleans = false;
  HasSync = false;
  HasExcl = false;
  HasFpabi = false;
  HasBpredict = false;
  HasBrdec = false;
  HasXea3 = false;
  HasHifi2 = false;
  HasCoproc = false;
  HasSalt = false;
  HasL32r = false;
  HasL32rFlix = false;
  HasCmov = false;
  HasZbb = false;

#define GET_CLANG_TARGET_INFO
#include "BuiltinsTensilica.def"
#undef GET_CLANG_TARGET_INFO

  // It's safer to always align at 16-byte boundaries.
  AdjArrayAlign = std::max(LdStWidth, (unsigned)128);

  // Tensilica has its special check on register variables, this covers both
  // Xtensa and RISCV.
  NeedCheckRegVarType = true;

  // Find the target and create the target intrinsic information.
  std::string Error;
  const std::string &TT = Triple.getTriple();
  const llvm::Target *TheTarget = llvm::TargetRegistry::lookupTarget(TT, Error);

  TII.reset(TheTarget->createTargetIntrinsicInfo());

  TCI = TensilicaCtypeInfoConfig::getInstance();
}

void TensilicaTargetInfo::getArchDefines(const LangOptions &Opts,
                                         MacroBuilder &Builder) const {
  Builder.defineMacro("__xtensa__");
  Builder.defineMacro("__XTENSA__");
  unsigned VerMajor = (XTENSA_SWVERSION / 100000) * 1000;
  unsigned VerMinor = (XTENSA_SWVERSION / 1000) % 100;
  Builder.defineMacro("__XCC__", Twine(VerMajor));
  Builder.defineMacro("__XCC_MINOR__", Twine(VerMinor));
  Builder.defineMacro("__XT_CLANG__", Twine(VerMajor));
  Builder.defineMacro("__XT_CLANG_MINOR__", Twine(VerMinor));
}

void TensilicaTargetInfo::setSupportedOpenCLOpts() {
  auto &Opts = getSupportedOpenCLOpts();
  Opts.support("cl_khr_fp64");
  if (HardHalfCtype != -1)
    Opts.support("cl_khr_fp16");
}

ArrayRef<int> TensilicaTargetInfo::getDwarfRegSizeTable() const {
  // TensilicaDRSizeTable only take care of callee-saved TIE regs.
  static int TensilicaDRSizeTable[] = {
#define GET_TIE_CSR_SIZE_INFO
#define GEN_GET_TIE_CSR_SIZE_INFO(REG_NAME, REG_SIZE) REG_SIZE,
#include "TensilicaGenClangRegisterInfo.inc"
#undef GET_TIE_CSR_SIZE_INFO
      0};
  return llvm::makeArrayRef(TensilicaDRSizeTable);
}

bool TensilicaTargetInfo::validateRegisterVariableType(
    QualType Ty, StringRef RegName) const {
  const auto *VarTy = Ty.getCanonicalType().getTypePtr();

  const TargetExtendedTypeInfo *TETI = nullptr;
  unsigned NumRegs = 0;

  if (const BuiltinType *BT = dyn_cast<BuiltinType>(VarTy))
    // For builtin types, directly get the target extended type info.
    TETI = BT->getTargetExtendedTypeInfo();
  else {
    // Handle complex float/double.
    if (const ComplexType *CTy = dyn_cast<ComplexType>(VarTy)) {
      QualType ElemTy = CTy->getElementType();
      int TID = -1;
      if (ElemTy->isSpecificBuiltinType(BuiltinType::Float)) {
        TID = getTgtExtTyIdForFloatComplexTy();
        if (TID != -1)
          TETI = getTargetExtendedTypeInfo(TID);
        else
          NumRegs = 2;
      } else if (ElemTy->isSpecificBuiltinType(BuiltinType::Double) ||
                 ElemTy->isSpecificBuiltinType(BuiltinType::LongDouble)) {
        TID = getTgtExtTyIdForDoubleComplexTy();
        if (TID != -1)
          TETI = getTargetExtendedTypeInfo(TID);
        else
          NumRegs = 4;
      }
      if (!TETI)
        TETI = getTargetExtendedTypeInfo("_TIE_int32");
    }
  }

  if (!TETI) {
    // Handle float/double.
    if (VarTy->isSpecificBuiltinType(BuiltinType::Float))
      NumRegs = 1;
    else if (VarTy->isSpecificBuiltinType(BuiltinType::Double) ||
             VarTy->isSpecificBuiltinType(BuiltinType::LongDouble))
      NumRegs = 2;
    // Handle pointer types.
    else if (VarTy->isPointerType())
      NumRegs = 1;
    else
      // For other cases, return false.
      return false;

    TETI = getTargetExtendedTypeInfo("_TIE_int32");
  }

  assert(TETI && "expect a valid TETI");

  // Check register requirements for Tensilica TIE ctypes and builtin types
  // that have a corresponding TIE ctype.
  const auto &CI = TCI->getCtypeInfo(TETI->getTargetExtTypeId());
  unsigned RegClassId = CI.RegClassId;
  if (!NumRegs)
    NumRegs = CI.NumRegs;

  assert(RegClassId < RegFileInfoTblSize &&
         "expect RegClassId is less than RegFileInfoTblSize");

  const TensilicaRegFileInfo &RFI = RegFileInfoTbl[RegClassId];

  StringRef CanRegName = getNormalizedGCCRegisterName(RegName);

  // Check if the register name starts with the short name of register.
  StringRef RFShortName(RFI.ShortName);
  if (!CanRegName.startswith(RFShortName))
    return false;

  // Check if the register number is in the range.
  unsigned int RegNo = 0;
  StringRef RegNoString = CanRegName.substr(RFShortName.size());
  bool IsNotNumber = RegNoString.getAsInteger(10, RegNo);

  if (IsNotNumber)
    return false;

  if (RegNo % NumRegs != 0)
    return false;

  return true;
}

static llvm::VectorType *getVectorType(llvm::LLVMContext &Context,
                                       llvm::Type::TypeID ElemTypeId,
                                       unsigned ElemBitWidth,
                                       unsigned NumElems) {
  llvm::Type *ElemType = nullptr;
  switch (ElemTypeId) {
  case llvm::Type::IntegerTyID:
    ElemType = llvm::Type::getIntNTy(Context, ElemBitWidth);
    break;
  case llvm::Type::HalfTyID:
    ElemType = llvm::Type::getHalfTy(Context);
    break;
  case llvm::Type::FloatTyID:
    ElemType = llvm::Type::getFloatTy(Context);
    break;
  case llvm::Type::DoubleTyID:
    ElemType = llvm::Type::getDoubleTy(Context);
    break;
  default:
    llvm_unreachable("Should never get here");
  }

  return llvm::VectorType::get(ElemType, NumElems);
}

// Table of vector types supported by target
static const TensilicaVectorType VectorTypeTbl[] = {
    {llvm::Type::VoidTyID, 0, 0, 0},
#define GET_VEC_TYPES
#include "TensilicaClangVecTypes.def"
#undef GET_VEC_TYPES
};

std::vector<std::pair<llvm::VectorType *, unsigned>>
TensilicaTargetInfo::getSupportedVectorTypes(llvm::LLVMContext &Context) const {

  std::vector<std::pair<llvm::VectorType *, unsigned>> VectorTys;
  for (const TensilicaVectorType VecType : VectorTypeTbl) {
    if (VecType.ElemTypeId == llvm::Type::VoidTyID)
      continue;

    llvm::VectorType *LVT = getVectorType(
        Context, VecType.ElemTypeId, VecType.ElemBitWidth, VecType.NumElems);
    VectorTys.push_back(std::make_pair(LVT, VecType.Alignment));
  }

  return VectorTys;
}

// Table of vector types supported by target
static const TensilicaPromotedVectorType PromotedVectorTypeTbl[] = {
    {llvm::Type::VoidTyID, 0, 0, llvm::Type::VoidTyID, 0, 0},
#define GET_PROMOTED_VEC_TYPES
#include "TensilicaClangVecTypes.def"
#undef GET_PROMOTED_VEC_TYPES
};

std::pair<llvm::VectorType *, unsigned>
TensilicaTargetInfo::getPromotedVectorType(llvm::LLVMContext &Context,
                                           llvm::VectorType *VecTy,
                                           unsigned OrigNumElems) const {

  for (const TensilicaPromotedVectorType &VecType : PromotedVectorTypeTbl) {
    if (VecType.ElemTypeId == llvm::Type::VoidTyID)
      continue;

    llvm::VectorType *LVT = getVectorType(
        Context, VecType.ElemTypeId, VecType.ElemBitWidth, VecType.NumElems);

    if (VecTy == LVT) {
      auto *PromotedTy =
          getVectorType(Context, VecType.PromotedElemTypeId,
                        VecType.PromotedElemBitWidth, OrigNumElems);
      unsigned PromotedWidth =
          VecType.PromotedElemBitWidth * VecType.PromotedNumElems;
      return std::make_pair(PromotedTy, PromotedWidth);
    }
  }

  return std::make_pair((llvm::VectorType *)nullptr, (unsigned)1);
}

ArrayRef<Builtin::Info> TensilicaTargetInfo::getTargetBuiltins() const {
  // Use llvm tensilica riscv target to get the builtins provided by the given
  // target
  assert(TII != nullptr && "TargetIntrinsicInfo is missing");

  unsigned NumBuiltins = TII->getNumOfFrontendBuiltins();
  const Builtin::Info *CFBI =
      reinterpret_cast<const Builtin::Info *>(TII->getFrontendBuiltinInfo());

  return llvm::makeArrayRef(CFBI, NumBuiltins);
}

const TargetExtendedTypeInfo *
TensilicaTargetInfo::getTargetExtendedTypeInfo(unsigned TID) const {
  if (TID >= TCI->getNumCtypes())
    return nullptr;

  const TargetExtendedTypeInfo *ExtTI = TCI->getExtTIWithTypeID(TID);
  assert(ExtTI != nullptr &&
         "The TargetExtendedTypeInfo should not be nullptr");

  return ExtTI;
}

const TargetExtendedTypeInfo *
TensilicaTargetInfo::getTargetExtendedTypeInfo(StringRef TypeName) const {
  return TCI->getExtTIWithTypeName(TypeName);
}

unsigned TensilicaTargetInfo::adjustArrayAlignment(unsigned Align,
                                                   uint64_t Size) const {
  // AdjArrayAlign is std::max(LdStWidth, (unsigned)128)
  uint64_t NewAlign = AdjArrayAlign;

  if (Size < NewAlign) {
    NewAlign = Size;
    // Get power of 2
    if (NewAlign & (NewAlign - 1))
      NewAlign = llvm::NextPowerOf2(NewAlign);
  }

  return std::max(Align, (unsigned)NewAlign);
}

const TargetExtendedTypeInfo *
TensilicaTargetInfo::getTgtExtTyInfoForBuiltinTy(BuiltinType::Kind K,
                                                 LangOptions &LangOpts) const {
  const char *CtypeName = nullptr;

  switch (K) {
  case BuiltinType::Char_S:
  case BuiltinType::SChar:
    CtypeName = "_TIE_int8";
    break;
  case BuiltinType::Char_U:
  case BuiltinType::UChar:
    CtypeName = "_TIE_uint8";
    break;
  case BuiltinType::Short:
    CtypeName = "_TIE_int16";
    break;
  case BuiltinType::UShort:
    CtypeName = "_TIE_uint16";
    break;
  case BuiltinType::Int:
    CtypeName = "_TIE_int32";
    break;
  case BuiltinType::Long:
    // long is 64 bit according to OpenCL spec $6.1.1
    if (LangOpts.OpenCL)
      CtypeName = "_TIE_int64";
    else
      CtypeName = "_TIE_int32";
    break;
  case BuiltinType::UInt:
    CtypeName = "_TIE_uint32";
    break;
  case BuiltinType::ULong:
    // unsigned long is 64 bit according to OpenCL spec $6.1.1
    if (LangOpts.OpenCL)
      CtypeName = "_TIE_uint64";
    else
      CtypeName = "_TIE_uint32";
    break;
  case BuiltinType::LongLong:
    // long long is a reserved 128-bit OpenCL builtin scalar type $6.1.4
    if (!LangOpts.OpenCL)
      CtypeName = "_TIE_int64";
    break;
  case BuiltinType::ULongLong:
    // unsigned long long is a reserved 128-bit OpenCL builtin scalar
    // type $6.1.4
    if (!LangOpts.OpenCL)
      CtypeName = "_TIE_uint64";
    break;
  case BuiltinType::Half: {
    int FID = getTgtExtTyIdForHalfFloatTy();
    if (FID != -1)
      return TCI->getExtTIWithTypeID(FID);
    return nullptr;
  }
  case BuiltinType::Float: {
    int FID = getTgtExtTyIdForFloatTy();
    if (FID != -1)
      return TCI->getExtTIWithTypeID(FID);
    return nullptr;
  }
  case BuiltinType::Double: {
    int DID = getTgtExtTyIdForDoubleTy();
    if (DID != -1)
      return TCI->getExtTIWithTypeID(DID);
    return nullptr;
  }
  default:
    break;
  }

  if (CtypeName != nullptr)
    return TCI->getExtTIWithTypeName(CtypeName);

  return nullptr;
}

unsigned TensilicaTargetInfo::getNumTargetExtendedTypes() const {
  return TCI->getNumCtypes();
}

const char *TensilicaTargetInfo::getTargetExtendTypeName(int TID) const {
  return TCI->getExtTIWithTypeID(TID)->getTypeName();
}

unsigned TensilicaTargetInfo::getFirstTargetExtNonStandardType() const {
  unsigned NumCtypes = TCI->getNumCtypes();
  for (unsigned Idx = 0; Idx < NumCtypes; ++Idx) {
    StringRef CtypeName = TCI->getExtTIWithTypeID(Idx)->getTypeName();
    if (CtypeName.equals("_TIE_void"))
      // The one after _TIE_Void should be the first non-standard Ctypes
      return Idx + 1;
  }
  llvm_unreachable("Should always be able to find _TIE_void");
  return 0;
}

int TensilicaTargetInfo::getTgtExtTyIdForHalfFloatTy() const {
  return HardHalfCtype;
}

int TensilicaTargetInfo::getTgtExtTyIdForFloatTy() const {
  return HardFloatCtype;
}

int TensilicaTargetInfo::getTgtExtTyIdForDoubleTy() const {
  return HardDoubleCtype;
}

int TensilicaTargetInfo::getTgtExtTyIdForFloatComplexTy() const {
  return HardFloatComplexCtype;
}

int TensilicaTargetInfo::getTgtExtTyIdForDoubleComplexTy() const {
  return HardDoubleComplexCtype;
}

TargetComplexTypeOpInfo *
TensilicaTargetInfo::getFloatComplextTypeOpInfo() const {
  return TCI->getFloatComplextTypeOpInfo();
}

TargetComplexTypeOpInfo *
TensilicaTargetInfo::getDoubleComplextTypeOpInfo() const {
  return TCI->getDoubleComplextTypeOpInfo();
}

unsigned TensilicaTargetInfo::getLdStWidth() const { return LdStWidth; }

bool TensilicaTargetExtTypeInfo::isTypeEqualTo(
    const TargetExtendedTypeInfo *OtherETI) const {
  if (OtherETI == nullptr)
    return false;

  const auto *OtherXETI = dyn_cast<TensilicaTargetExtTypeInfo>(OtherETI);
  if (OtherXETI == nullptr)
    return false;

  int NumEqCtypes = CtypeInfo.NumEqCtypes;
  if (NumEqCtypes == 0)
    return false;

  unsigned OtherCtypeId = OtherXETI->getCtypeID();
  int StartPos = CtypeInfo.EqCtypeTabStart;

  auto EqCtypeTable = TCIM.getEqCtypeTable();

  if (NumEqCtypes == 1)
    return OtherCtypeId == EqCtypeTable[StartPos];

  // Binary search
  auto LowerBound = std::begin(EqCtypeTable) + StartPos;
  auto UpperBound = LowerBound + NumEqCtypes;

  auto IT = std::lower_bound(LowerBound, UpperBound, OtherCtypeId);
  if (IT != UpperBound)
    return true;

  return false;
}

bool TensilicaTargetExtTypeInfo::getBinaryOperator(
    BinaryOperatorKind Kind, const TargetExtendedTypeInfo *RhsExprETI,
    SmallVector<Builtin::ID, 3> &BuiltinSet) const {
  if (RhsExprETI == nullptr)
    return false;

  const auto *RhsExprXETI = dyn_cast<TensilicaTargetExtTypeInfo>(RhsExprETI);
  if (RhsExprXETI == nullptr)
    return false;

  /// Get the internal Tensilica Ctype operator ID
  unsigned COK = getCtypeOpKind(Kind);
  if (COK == CtypeInvalidOP)
    return false;

  /// Get the table index
  const auto &EI = OpInfo.EntryIdx[COK];

  if (EI.NumEntries == 0)
    return false;

  auto CtypeBinOpTbl = TCIM.getCtypeBinaryOperatorTable();
  if (EI.NumEntries == 1 &&
      CtypeBinOpTbl[EI.StartPos].RHSOpndCtypeID == RhsExprXETI->getCtypeID()) {
    BuiltinSet.push_back(
        static_cast<Builtin::ID>(CtypeBinOpTbl[EI.StartPos].BuiltinID));
    return true;
  }

  auto LowerBound = std::begin(CtypeBinOpTbl) + EI.StartPos;
  auto UpperBound = LowerBound + EI.NumEntries;

  auto IT = std::equal_range(
      LowerBound, UpperBound,
      TensilicaLookupTblBinOPEntry{RhsExprXETI->getCtypeID(), 0, 0});

  for (auto Entry = IT.first; Entry != IT.second; ++Entry) {
    if (Entry->RHSOpndCtypeID == RhsExprXETI->getCtypeID())
      BuiltinSet.push_back(static_cast<Builtin::ID>(Entry->BuiltinID));
  }

  if (BuiltinSet.size() != 0)
    return true;

  return false;
}

Builtin::ID
TensilicaTargetExtTypeInfo::getUnaryOperator(UnaryOperatorKind Kind) const {
  // Get the internal Tensilica Ctype operator ID
  unsigned COK = getCtypeOpKind(Kind);
  if (COK == CtypeInvalidOP)
    return Builtin::NotBuiltin;

  // Get the table index
  const auto &EI = OpInfo.EntryIdx[COK];

  if (EI.NumEntries == 0 || EI.NumEntries > 1)
    return Builtin::NotBuiltin;

  return static_cast<Builtin::ID>(EI.StartPos);
}

Builtin::ID TensilicaTargetExtTypeInfo::getConvertOperator(
    TensilicaOpKind Kind, const LUTCVTAryRef &LUT,
    const TargetExtendedTypeInfo *ToTypeETI) const {

  assert(ToTypeETI != nullptr &&
         "The TargetExtendedTypeInfo of ToType should not be null");

  const auto *ToTypeXETI = dyn_cast<TensilicaTargetExtTypeInfo>(ToTypeETI);
  if (ToTypeXETI == nullptr)
    return Builtin::NotBuiltin;

  assert((Kind != CtypeRTOM || Kind != CtypeRTOR || Kind != CtypeMTOR) &&
         "Invalid ctype convertion");

  // Get the table index
  const auto &EI = OpInfo.EntryIdx[Kind];

  if (EI.NumEntries == 0)
    return Builtin::NotBuiltin;

  if (EI.NumEntries == 1 &&
      LUT[EI.StartPos].CtypeID == ToTypeXETI->getCtypeID())
    return static_cast<Builtin::ID>(LUT[EI.StartPos].BuiltinID);

  // For binary shift operators, each operator should only have one entry
  auto LowerBound = LUT.begin() + EI.StartPos;
  auto UpperBound = LowerBound + EI.NumEntries;

  auto IT = std::lower_bound(
      LowerBound, UpperBound,
      TensilicaLookupTblCvtOPEntry{ToTypeXETI->getCtypeID(), 0});
  if (IT != UpperBound && IT->CtypeID == ToTypeXETI->getCtypeID())
    return static_cast<Builtin::ID>(IT->BuiltinID);

  return Builtin::NotBuiltin;
}

const TensilicaCtypeInfoConfig *TensilicaCtypeInfoConfig::getInstance() {
  static TensilicaCtypeInfoConfig TCI(TenConfigTbls);
  return &TCI;
}

TensilicaCtypeInfoConfig::TensilicaCtypeInfoConfig(TensilicaTables &TenTbls)
    : TenTbls(TenTbls) {
  for (unsigned i = 0; i < TenTbls.CtypeInfoTable.size(); ++i) {
    TensilicaExtTypePtr NewExtTI(new TensilicaTargetExtTypeInfo(i, *this));
    const char *CtypeTypeName = NewExtTI->getTypeName();
    CtypeInfoNameMap[CtypeTypeName] = NewExtTI.get();
    CtypesInfoIDMap.push_back(std::move(NewExtTI));
  }
}

const TargetExtendedTypeInfo *
TensilicaCtypeInfoConfig::getExtTIWithTypeID(unsigned TID) const {
  assert(TID < CtypesInfoIDMap.size() && "CtypesInfoIDMap out of bound");
  return CtypesInfoIDMap[TID].get();
}

const TargetExtendedTypeInfo *
TensilicaCtypeInfoConfig::getExtTIWithTypeName(StringRef TypeName) const {
  auto IT = CtypeInfoNameMap.find(TypeName);
  if (IT != CtypeInfoNameMap.end())
    return IT->second;
  return nullptr;
}

TargetComplexTypeOpInfo *
TensilicaCtypeInfoConfig::getFloatComplextTypeOpInfo() const {
  return &XtComplexFloatOpInfo;
}

TargetComplexTypeOpInfo *
TensilicaCtypeInfoConfig::getDoubleComplextTypeOpInfo() const {
  return &XtComplexDoubleOpInfo;
}
