#include "clang/AST/OperationKinds.h"
#include "clang/AST/Type.h"
#include "clang/Basic/Builtins.h"
#include "clang/Basic/TargetInfo.h"

namespace clang {

#define FIRST_CUSTOM_VALUETYPE 139

//===----------------------------------------------------------------------===//
// Lookup Table for Ctype operators
//===----------------------------------------------------------------------===//

/// Statically load information on ctype operators

/// For binary operators and data conversions, a ctype may have several
/// operators with different RHS operand; For each operator, it has two
/// data structures, a flat sorted look-up table and an index table for look-up

/// We consider the following overloaded opereators
/// {"+", {BO_Add, "ADD", 2}},
/// {"-", {BO_Sub, "SUB", 2}},
/// {"*", {BO_Mul, "MUL", 2}},
/// {"/", {BO_Div, "DIV", 2}},
/// {"%", {BO_Rem, "REM", 2}},
/// {"<<", {BO_Shl, "SHL", 2}},
/// {">>", {BO_Shr, "SHR", 2}},
/// {"<", {BO_LT, "LT", 2}},
/// {">", {BO_GT, "GT", 2}},
/// {"<=", {BO_LE, "LE", 2}},
/// {">=", {BO_GE, "GE", 2}},
/// {"==", {BO_EQ, "EQ", 2}},
/// {"!=", {BO_NE, "NE", 2}},
/// {"&", {BO_And, "AND", 2}},
/// {"^", {BO_Xor, "XOR", 2}},
/// {"|", {BO_Or, "OR", 2}},
/// {"||", {BO_LOr, "LOR", 2}},
/// {"&&", {BO_LAnd, "LAND", 2}},
/// {"--", {UO_Minus, "NEGATE", 1}}
/// {"!", {UO_LNot, "LNOT", 1}},
/// {"~", {UO_Not, "NOT",1}}
/// 21 arithmetic operators + 3 data conversion operators

// Each Ctype has an array of LoopupTableIdx for each kind of operator
const unsigned TensilicaNumOverloadedOps = 21;
const unsigned TensilicaNumCvtOps = 3;

#define TENSILICA_UNDEFINED -1

/// Internal operator ID for look-up table
enum TensilicaOpKind {
  CtypeADD = 0,
  CtypeSUB,
  CtypeMUL,
  CtypeDIV,
  CtypeREM,
  CtypeSHL,
  CtypeSHR,
  CtypeLT,
  CtypeGT,
  CtypeLE,
  CtypeGE,
  CtypeEQ,
  CtypeNE,
  CtypeAND,
  CtypeXOR,
  CtypeOR,
  CtypeLAND,
  CtypeLOR,
  CtypeMinus,
  CtypeLNOT,
  CtypeNOT,
  CtypeRTOR,
  CtypeRTOM,
  CtypeMTOR,
  CtypeInvalidOP
};

struct TensilicaLookupTblIdx {
  // 32 bit, do we need larger?
  unsigned StartPos;
  // Use NumEntries to see if needs go through table
  unsigned NumEntries;
};

/// Table entry for binary operators
struct TensilicaLookupTblBinOPEntry {
  const unsigned RHSOpndCtypeID;
  const unsigned ResultCtypeID;
  const unsigned BuiltinID;
  friend bool operator<(const TensilicaLookupTblBinOPEntry &LHS,
                        const TensilicaLookupTblBinOPEntry &RHS) {
    return LHS.RHSOpndCtypeID < RHS.RHSOpndCtypeID;
  }
};

/// Table entry for conversion operators
struct TensilicaLookupTblCvtOPEntry {
  const unsigned CtypeID;
  const unsigned BuiltinID;
  friend bool operator<(const TensilicaLookupTblCvtOPEntry &LHS,
                        const TensilicaLookupTblCvtOPEntry &RHS) {
    return LHS.CtypeID < RHS.CtypeID;
  }
};

/// Represents starting entries to op tables
struct TensilicaCtypeOpInfo {
  TensilicaLookupTblIdx
      EntryIdx[TensilicaNumOverloadedOps + TensilicaNumCvtOps];
};

/// Basic information of Ctype
enum TensilicaCtypeProperty {
  CTYPE_UNKNOWN = 0,
  CTYPE_BOOLEAN = (1 << 0),
  CTYPE_COND_BOOLEAN = (1 << 1),
  CTYPE_FP = (1 << 2),
  CTYPE_DP = (1 << 3),
  CTYPE_HP = (1 << 4)
};

struct TensilicaCtypeInfo {
  const char *Name;
  unsigned Bitwidth;
  unsigned Alignment;
  int PromoteType;
  int EqCtypeTabStart;
  int NumEqCtypes;
  // Vector ctype info
  int ElemType;
  int NumElems;
  unsigned RegClassId;
  unsigned NumRegs;
  unsigned Prop;
};

/// Reg file info
struct TensilicaRegFileInfo {
  const char *ShortName;
  unsigned NumRegs;
};

/// Represent a target supported vector type
struct TensilicaVectorType {
  llvm::Type::TypeID ElemTypeId;
  unsigned ElemBitWidth;
  unsigned NumElems;
  unsigned Alignment;
};

struct TensilicaPromotedVectorType {
  llvm::Type::TypeID ElemTypeId;
  unsigned ElemBitWidth;
  unsigned NumElems;
  llvm::Type::TypeID PromotedElemTypeId;
  unsigned PromotedElemBitWidth;
  unsigned PromotedNumElems;
};

class TensilicaTargetExtTypeInfo;
typedef std::unique_ptr<TensilicaTargetExtTypeInfo> TensilicaExtTypePtr;

typedef unsigned TensilicaEqCtypeEntry;

struct TensilicaTables {
  TensilicaTables(
      const TensilicaCtypeOpInfo *CTypeOpInfosPtr, unsigned NumCTypeOpInfos,
      const TensilicaCtypeInfo *CtypeInfoPtr, unsigned NumCtypeInfos,
      const TensilicaLookupTblCvtOPEntry *CtypeRTORPtr, unsigned NumRTOR,
      const TensilicaLookupTblCvtOPEntry *CtypeMTORPtr, unsigned NumMTOR,
      const TensilicaLookupTblCvtOPEntry *CtypeRTOMPtr, unsigned NumRTOM,
      const TensilicaEqCtypeEntry *EqCtypePtr, unsigned NumEqCtypes,
      const TensilicaLookupTblBinOPEntry *CtypeBinaryOpPtr,
      unsigned NumCtypeBinaryOps)
      : CTypeOpInfos(CTypeOpInfosPtr, NumCTypeOpInfos),
        CtypeInfoTable(CtypeInfoPtr, NumCtypeInfos),
        CtypeMTORCVTTable(CtypeMTORPtr, NumMTOR),
        CtypeRTORCVTTable(CtypeRTORPtr, NumRTOR),
        CtypeRTOMCVTTable(CtypeRTOMPtr, NumRTOM),
        EqCtypeTable(EqCtypePtr, NumEqCtypes),
        CtypeBinOpTable(CtypeBinaryOpPtr, NumCtypeBinaryOps) {}
  ArrayRef<TensilicaCtypeOpInfo> CTypeOpInfos;
  ArrayRef<TensilicaCtypeInfo> CtypeInfoTable;
  ArrayRef<TensilicaLookupTblCvtOPEntry> CtypeMTORCVTTable;
  ArrayRef<TensilicaLookupTblCvtOPEntry> CtypeRTORCVTTable;
  ArrayRef<TensilicaLookupTblCvtOPEntry> CtypeRTOMCVTTable;
  ArrayRef<TensilicaEqCtypeEntry> EqCtypeTable;
  ArrayRef<TensilicaLookupTblBinOPEntry> CtypeBinOpTable;
};

class TensilicaCtypeInfoConfig {
private:
  TensilicaCtypeInfoConfig(TensilicaTables &TenTbls);

  TensilicaTables &TenTbls;
  /// CtypesInfoIDMap with Tensilica Ctype ID as the key holds all the
  /// TargetExtendedTypeInfo created for the TIE Ctypes supported by
  /// a specified config. It also manages the allocated memory.
  mutable std::vector<TensilicaExtTypePtr> CtypesInfoIDMap;

  /// CtypeInfoNameMap with Tensilica Ctype Name as the key holds the mapping
  /// between Tensilica Ctype Name and TargetExtendedTypeInfo. With this data
  /// structure, we can find the corresponding Tensilica Ctype if it exists for
  /// a given builtin type in Clang
  mutable llvm::StringMap<TargetExtendedTypeInfo *> CtypeInfoNameMap;

public:
  static const TensilicaCtypeInfoConfig *getInstance();

  const TargetExtendedTypeInfo *getExtTIWithTypeID(unsigned TID) const;

  const TargetExtendedTypeInfo *getExtTIWithTypeName(StringRef TypeName) const;

  const TensilicaCtypeOpInfo &getCtypeOperatorInfo(unsigned TID) const {
    assert(TID < TenTbls.CTypeOpInfos.size() && "CTypeOpInfos out of bound");
    return TenTbls.CTypeOpInfos[TID];
  }

  unsigned getNumCtypes() const { return TenTbls.CtypeInfoTable.size(); }

  const TensilicaCtypeInfo &getCtypeInfo(unsigned TID) const {
    assert(TID < TenTbls.CtypeInfoTable.size() &&
           "CtypeInfoTable out of bound");
    return TenTbls.CtypeInfoTable[TID];
  }

  const ArrayRef<TensilicaLookupTblCvtOPEntry> getCtypeRTORCVTTable() const {
    return TenTbls.CtypeRTORCVTTable;
  }

  const ArrayRef<TensilicaLookupTblCvtOPEntry> getCtypeRTOMCVTTable() const {
    return TenTbls.CtypeRTOMCVTTable;
  }

  const ArrayRef<TensilicaLookupTblCvtOPEntry> getCtypeMTORCVTTable() const {
    return TenTbls.CtypeMTORCVTTable;
  }

  const ArrayRef<TensilicaEqCtypeEntry> getEqCtypeTable() const {
    return TenTbls.EqCtypeTable;
  }

  const ArrayRef<TensilicaLookupTblBinOPEntry>
  getCtypeBinaryOperatorTable() const {
    return TenTbls.CtypeBinOpTable;
  }

  TargetComplexTypeOpInfo *getFloatComplextTypeOpInfo() const;

  TargetComplexTypeOpInfo *getDoubleComplextTypeOpInfo() const;
};

class TensilicaTargetExtTypeInfo : public TargetExtendedTypeInfo {
  typedef ArrayRef<TensilicaLookupTblCvtOPEntry> LUTCVTAryRef;

public:
  TensilicaTargetExtTypeInfo() = delete;
  TensilicaTargetExtTypeInfo(const TensilicaTargetExtTypeInfo &) = delete;
  TensilicaTargetExtTypeInfo &
  operator=(const TensilicaTargetExtTypeInfo &) = delete;

  TensilicaTargetExtTypeInfo(unsigned CtypeID,
                             const TensilicaCtypeInfoConfig &TCIM)
      : CtypeID(CtypeID), TCIM(TCIM),
        OpInfo(TCIM.getCtypeOperatorInfo(CtypeID)),
        CtypeInfo(TCIM.getCtypeInfo(CtypeID)) {
    // FIXME: should replace CtypeID with TargetExtTypeId
    TargetExtTypeId = CtypeID;
  }

  /// Return the name of a TIE ctype
  const char *getTypeName() const override { return CtypeInfo.Name; }

  bool isVectorType() const override { return CtypeInfo.ElemType != -1; }

  const TargetExtendedTypeInfo *getElemType() const override {
    if (isVectorType())
      return TCIM.getExtTIWithTypeID(CtypeInfo.ElemType);

    return nullptr;
  }

  int getNumElems() const override { return CtypeInfo.NumElems; }

  /// Return the bit width of a TIE ctype
  unsigned getBitWidth() const override {
    unsigned Bitsize = CtypeInfo.Bitwidth;
    Bitsize = (Bitsize + 7) / 8 * 8;
    unsigned Alignment = getAlignment();
    Bitsize = ((Bitsize + Alignment - 1) / Alignment) * Alignment;
    return Bitsize;
  }

  /// Return the type info of promote type
  const TargetExtendedTypeInfo *getPromoteTypeInfo() const override {
    int PromotedType = CtypeInfo.PromoteType;
    if (PromotedType != TENSILICA_UNDEFINED) {
      return TCIM.getExtTIWithTypeID(PromotedType);
    }
    return nullptr;
  }

  /// Return the alignment of a TIE ctype
  unsigned getAlignment() const override {
    unsigned Alignment = CtypeInfo.Alignment;
    Alignment = (Alignment + 7) / 8 * 8;
    return Alignment;
  }

  /// Return if this type is a boolean type
  virtual bool isBooleanType() const override {
    return CtypeInfo.Prop & CTYPE_BOOLEAN;
  }

  /// Return if this type is a boolean type and can be used for cond exprs
  virtual bool isCondBooleanType() const override {
    return CtypeInfo.Prop & CTYPE_COND_BOOLEAN;
  }

  /// Return if this type is a floating point type that may have a type
  //  conversion from 'float'
  virtual bool isFloatingPoint() const override {
    return CtypeInfo.Prop & CTYPE_FP;
  }

  /// Return if this type is equivalent to other type \p OtherType
  virtual bool
  isTypeEqualTo(const TargetExtendedTypeInfo *OtherType) const override;

  /// Interfaces for operators associated with target extended types

  /// Data conversions, R for register and M for memory
  virtual Builtin::ID
  getConvertRTOR(const TargetExtendedTypeInfo *ToType) const override {
    return getConvertOperator(CtypeRTOR, TCIM.getCtypeRTORCVTTable(), ToType);
  }

  virtual Builtin::ID
  getConvertMTOR(const TargetExtendedTypeInfo *ToType) const override {
    return getConvertOperator(CtypeMTOR, TCIM.getCtypeMTORCVTTable(), ToType);
  }

  virtual Builtin::ID
  getConvertRTOM(const TargetExtendedTypeInfo *ToType) const override {
    return getConvertOperator(CtypeRTOM, TCIM.getCtypeRTOMCVTTable(), ToType);
  }

  /// Return the Tensilica Ctype ID, only for internal use
  unsigned getCtypeID() const { return CtypeID; }

  /// Look-up functions for Tensilica Ctype operators

  /// Return the Builtin ID for a given binary operator and the second operand
  bool
  getBinaryOperator(BinaryOperatorKind Kind,
                    const TargetExtendedTypeInfo *Other,
                    SmallVector<Builtin::ID, 3> &BuiltinSet) const override;

  /// Return the builtin ID for a given unary operator
  Builtin::ID getUnaryOperator(UnaryOperatorKind Kind) const override;

  virtual Kind getKind() const override { return TensilicaTargetExtInfo; }

  static bool classof(const TargetExtendedTypeInfo *D) {
    return D->getKind() == TensilicaTargetExtInfo;
  }

private:
  unsigned CtypeID; // for internal use
  const TensilicaCtypeInfoConfig &TCIM;
  const TensilicaCtypeOpInfo &OpInfo;
  const TensilicaCtypeInfo &CtypeInfo;
  Builtin::ID getConvertOperator(TensilicaOpKind Kind, const LUTCVTAryRef &LUT,
                                 const TargetExtendedTypeInfo *ToType) const;
};

class TensilicaTargetInfo : public TargetInfo {
protected:
  enum XtensaABIType { XT_WINDOWED_ABI, XT_CALL0_ABI };

  // The alignment for adjusting array alignment.
  unsigned AdjArrayAlign;
  enum XtensaABIType XtensaABI;

  bool IsHardFloat;
  int HardHalfCtype;
  int HardFloatCtype;
  int HardDoubleCtype;
  int HardFloatComplexCtype;
  int HardDoubleComplexCtype;
  unsigned LdStWidth;
  bool HasHardSync;

  bool HasDiv32;
  bool HasMul32;
  bool HasMul32h;
  bool HasClamps;
  bool HasCall4;
  bool HasConst16;
  bool HasDensity;
  bool HasDepbits;
  bool HasLoop;
  bool HasMac16;
  bool HasMinmax;
  bool HasMul16;
  bool HasNsa;
  bool HasSext;
  bool HasS32c1i;
  bool HasBooleans;
  bool HasSync;
  bool HasExcl;
  bool HasFpabi;
  bool HasBpredict;
  bool HasBrdec;
  bool HasXea3;
  bool HasHifi2;
  bool HasVecmemcpy;
  bool HasVecmemset;
  bool HasCoproc;
  bool HasHwf32;
  bool HasHwf64;
  bool HasSalt;
  bool HasL32r;
  bool HasL32rFlix;

  bool HasCmov;
  bool HasZbb;

  const TensilicaCtypeInfoConfig *TCI;
  std::unique_ptr<llvm::TargetIntrinsicInfo> TII;

public:
  TensilicaTargetInfo(const llvm::Triple &Triple);

  void getArchDefines(const LangOptions &Opts, MacroBuilder &Builder) const;

  ArrayRef<int> getDwarfRegSizeTable() const override;

  bool validateRegisterVariableType(QualType Ty,
                                    StringRef RegName) const override;

  ArrayRef<Builtin::Info> getTargetBuiltins() const override;

  const llvm::TargetIntrinsicInfo *getTargetIntrinsicInfo() const override {
    return TII.get();
  }

  const TargetExtendedTypeInfo *
  getTargetExtendedTypeInfo(unsigned TID) const override;

  const TargetExtendedTypeInfo *
  getTargetExtendedTypeInfo(StringRef TypeName) const;

  unsigned adjustArrayAlignment(unsigned Align, uint64_t Size) const override;

  const TargetExtendedTypeInfo *
  getTgtExtTyInfoForBuiltinTy(BuiltinType::Kind K,
                              LangOptions &LangOpts) const override;

  unsigned getNumTargetExtendedTypes() const override;
  const char *getTargetExtendTypeName(int TID) const override;
  unsigned getFirstTargetExtNonStandardType() const override;

  int getTgtExtTyIdForHalfFloatTy() const override;
  int getTgtExtTyIdForFloatTy() const override;
  int getTgtExtTyIdForDoubleTy() const override;
  int getTgtExtTyIdForFloatComplexTy() const override;
  int getTgtExtTyIdForDoubleComplexTy() const override;
  TargetComplexTypeOpInfo *getFloatComplextTypeOpInfo() const override;
  TargetComplexTypeOpInfo *getDoubleComplextTypeOpInfo() const override;
  unsigned getLdStWidth() const override;

  std::vector<std::pair<llvm::VectorType *, unsigned>>
  getSupportedVectorTypes(llvm::LLVMContext &Context) const override;
  std::pair<llvm::VectorType *, unsigned>
  getPromotedVectorType(llvm::LLVMContext &Context, llvm::VectorType *VecTy,
                        unsigned OrigNumElems) const override;
  void setSupportedOpenCLOpts() override;
};
} // namespace clang
