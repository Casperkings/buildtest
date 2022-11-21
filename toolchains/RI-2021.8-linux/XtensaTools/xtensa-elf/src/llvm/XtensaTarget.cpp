//===--- XtensaTarget.cpp - Implement target feature support --------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements construction of a TargetInfo object for Xtensa.
//
//===----------------------------------------------------------------------===//

#include "TensilicaCommon.h"
#include "clang/AST/OperationKinds.h"
#include "clang/AST/Type.h"
#include "clang/Basic/Builtins.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/LangOptions.h"
#include "clang/Basic/MacroBuilder.h"
#include "clang/Basic/TargetBuiltins.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Basic/Version.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/IndexedMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/ADT/Triple.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/TargetIntrinsicRegistry.h"
#include "llvm/IR/Type.h"
#include "llvm/MC/MCSectionMachO.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TargetParser.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/XtensaXtparams.h"
#include "llvm/Target/TargetIntrinsicInfo.h"
#include <algorithm>
#include <memory>
#include <xtensa-versions.h>

#include <stdio.h>
using namespace clang;
using namespace llvm;

class XtensaTargetInfo : public TensilicaTargetInfo {
private:
  static const char *const GCCRegNames[];

public:
  XtensaTargetInfo(const llvm::Triple &Triple) : TensilicaTargetInfo(Triple) {
    TLSSupported = false;
    NoAsmVariants = true;
    LongLongAlign = 64;
    SuitableAlign = 32;
    DoubleAlign = LongDoubleAlign = 64;
    SizeType = UnsignedInt;
    PtrDiffType = SignedInt;
    IntPtrType = SignedInt;
    WCharType = UnsignedShort;
    WIntType = SignedInt;
    UseZeroLengthBitfieldAlignment = true;

    std::string LayoutString("e-m:e-p:32:32-i1:8:32-i8:8:32-i16:16:32"
                             "-i64:64-i128:128-i256:256-i512:512-i1024:1024"
                             "-f64:64-a:0:32-n32-S128");
    if (BigEndian)
      LayoutString[0] = 'E';
    resetDataLayout(LayoutString);
    LongAlign = 32;
    LongWidth = 32;
    PointerAlign = 32;
    PointerWidth = 32;
    IntMaxType = TargetInfo::SignedLongLong;
    Int64Type = TargetInfo::SignedLongLong;
    DoubleAlign = 64;
    LongDoubleWidth = 64;
    LongDoubleAlign = 64;
    SizeType = TargetInfo::UnsignedInt;
    PtrDiffType = TargetInfo::SignedInt;
    IntPtrType = TargetInfo::SignedInt;
    RegParmMax = 6;
    FloatFormat = &llvm::APFloat::IEEEsingle();
    DoubleFormat = &llvm::APFloat::IEEEdouble();
    LongDoubleFormat = &llvm::APFloat::IEEEdouble();

    // __STDCPP_DEFAULT_NEW_ALIGNMENT__ -
    // the alignment guaranteed by a call to operator new(std::size_t)
    NewAlign = std::max(LdStWidth, static_cast<unsigned>(LongDoubleAlign));

    if (HasHardSync) {
      MaxAtomicPromoteWidth = 32;
      MaxAtomicInlineWidth = 32;
    }
  }

  ~XtensaTargetInfo() {}

  bool validateTarget(DiagnosticsEngine &Diags) const override { return true; }

  StringRef getABI() const override {
    if (XtensaABI == XT_WINDOWED_ABI)
      return "windowed";
    assert(XtensaABI == XT_CALL0_ABI);
    return "call0";
  }

  bool setABI(const std::string &Name) override {
    if (Name == "call0") {
      XtensaABI = XT_CALL0_ABI;
      return true;
    }
    return false;
  }

  void getDefaultFeatures(llvm::StringMap<bool> &Features) const {}

  void getTargetDefines(const LangOptions &Opts,
                        MacroBuilder &Builder) const override {
    getArchDefines(Opts, Builder);
    switch (XtensaABI) {
    case XT_WINDOWED_ABI:
      Builder.defineMacro("__XTENSA_WINDOWED_ABI__");
      break;
    case XT_CALL0_ABI:
      Builder.defineMacro("__XTENSA_CALL0_ABI__");
      break;
    }
    Builder.defineMacro(BigEndian ? "__XTENSA_EB__" : "__XTENSA_EL__");
    if (!IsHardFloat)
      Builder.defineMacro("__XTENSA_SOFT_FLOAT__");
    if (llvm::isXtensaSafetyMode())
      Builder.defineMacro("__XT_FUSA__");
    Builder.defineMacro("__ELF__");
  }

  bool handleTargetFeatures(std::vector<std::string> &Features,
                            DiagnosticsEngine &Diags) override {
    bool CoprocNotSet = true;
    for (auto it = Features.begin(); it != Features.end();) {
      std::string &Feature = *it;
      assert(!Feature.empty() && "Empty string");

      // Strip off the flag
      std::string StrippedFeature = Feature.substr(1);

      if (CoprocNotSet && StrippedFeature == "coproc")
        CoprocNotSet = false;

      // Check features with an enabled flag '+'
      if (Feature[0] == '+') {
        // If the feature is not supported by the config, ignore it.
        if (!hasFeature(StrippedFeature))
          it = Features.erase(it);
        else
          it++;
      } else {
        // Check features with a disabled flag '-'
        if (StrippedFeature == "coproc" && HasCoproc)
          HasCoproc = false;
        it++;
      }
    }

    // By default, 'coproc' is off.
    if (CoprocNotSet && HasCoproc)
      HasCoproc = false;

    return true;
  }

  bool hasFeature(StringRef Feature) const override {
    return llvm::StringSwitch<bool>(Feature)
        .Case("Xtensa", true)
        .Case("hard-complex-float",
              HardDoubleComplexCtype != -1 || HardFloatComplexCtype != -1)
        .Case("hard-float-complex-ctype", HardFloatComplexCtype != -1)
        .Case("hard-double-complex-ctype", HardDoubleComplexCtype != -1)
        .Case("hard-float-ctype", HardFloatCtype != -1)
        .Case("hard-double-ctype", HardDoubleCtype != -1)
        .Case("hard-half-ctype", HardHalfCtype != -1)
        .Case("hard-sync", HasHardSync != false)
        .Case("div32", HasDiv32)
        .Case("mul32", HasMul32)
        .Case("mul32h", HasMul32h)
        .Case("clamps", HasClamps)
        .Case("call4", HasCall4)
        .Case("const16", HasConst16)
        .Case("density", HasDensity)
        .Case("depbits", HasDepbits)
        .Case("loop", HasLoop)
        .Case("mac16", HasMac16)
        .Case("minmax", HasMinmax)
        .Case("mul16", HasMul16)
        .Case("nsa", HasNsa)
        .Case("sext", HasSext)
        .Case("s32c1i", HasS32c1i)
        .Case("booleans", HasBooleans)
        .Case("sync", HasSync)
        .Case("excl", HasExcl)
        .Case("fpabi", HasFpabi)
        .Case("bpredict", HasBpredict)
        .Case("brdec", HasBrdec)
        .Case("xea3", HasXea3)
        .Case("hifi2", HasHifi2)
        .Case("coproc", HasCoproc)
        .Case("salt", HasSalt)
        .Case("l32r", HasL32r)
        .Case("l32r-flix", HasL32rFlix)
        .Case("vecmemcpy", HasVecmemcpy)
        .Case("vecmemset", HasVecmemset)
        .Default(false);
  }

  BuiltinVaListKind getBuiltinVaListKind() const override {
    return TargetInfo::XtensaBuiltinVaList;
  }

  ArrayRef<const char *> getGCCRegNames() const override {
    static const char *const GCCRegNames[] = {
        // Core regs
        "a0", "a1", "a2",  "a3",  "a4",  "a5",  "a6",  "a7",
        "a8", "a9", "a10", "a11", "a12", "a13", "a14", "a15",
#define GET_CLANG_GCC_REG_NAMES
#include "TensilicaGenClangRegisterInfo.inc"
#undef GET_CLANG_GCC_REG_NAMES
    };
    return llvm::makeArrayRef(GCCRegNames);
  }

  ArrayRef<TargetInfo::GCCRegAlias> getGCCRegAliases() const override {
    return None;
  }

  bool validateAsmOperandType(StringRef Constraint,
                              QualType Type) const override {
    // Strip off constraint modifiers.
    while (Constraint[0] == '=' || Constraint[0] == '+' || Constraint[0] == '&')
      Constraint = Constraint.substr(1);

    switch (Constraint[0]) {
    default:
      break;
    case 'f':
      // Check the type of operand for contraint 'f' same as in XCC
      // Refer to: CGTARG_TN_For_Asm_Operand() in cgtarget.cxx in XCC
      if ((HardFloatCtype != -1 &&
           Type->isSpecificBuiltinType(BuiltinType::Float)) ||
          (HardDoubleCtype != -1 &&
           Type->isSpecificBuiltinType(BuiltinType::Double)))
        return true;
      return false;
    }
    return true;
  }

  bool validateAsmConstraint(const char *&Name,
                             TargetInfo::ConstraintInfo &Info) const override {
    switch (*Name) {
    default:
      return false;
    // Note, 'i'/'n' are simple constraints of GCC inline assembly extensions.
    case 'a': // AR
    case 'b': // BR
    case 't': // State
    case 'v': // Tie register
      Info.setAllowsRegister();
      return true;
    case 'f':
      if (HardFloatCtype != -1 || HardDoubleCtype != -1) {
        Info.setAllowsRegister();
        return true;
      }
    }
    return false;
  }

  bool validateGlobalRegisterVariable(StringRef RegName, unsigned RegSize,
                                      bool &HasSizeMismatch) const override {
    HasSizeMismatch = false;
    return false;
  }

  const char *getClobbers() const override { return ""; }

  int getEHDataRegisterNumber(unsigned RegNo) const override {
    if (XtensaABI == XT_WINDOWED_ABI) {
      if (RegNo == 0)
        return 2;
      if (RegNo == 1)
        return 3;
    } else if (XtensaABI == XT_CALL0_ABI) {
      if (RegNo == 0)
        return 6;
      if (RegNo == 1)
        return 7;
    }
    return -1;
  }
};

extern "C" TargetInfo *createXtensaClangTargetInfo(const llvm::Triple &Triple) {
  return new XtensaTargetInfo(Triple);
}
