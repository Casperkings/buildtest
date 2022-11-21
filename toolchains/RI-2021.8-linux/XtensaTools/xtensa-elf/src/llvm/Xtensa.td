//===-- Xtensa.td - Describe the Xtensa Target Machine --------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This is the top level entry point for the Xtensa target.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Target-independent interfaces which we are implementing
//===----------------------------------------------------------------------===//

include "llvm/Target/Target.td"


def FeatureMul16 : SubtargetFeature<"mul16", "HasMul16", "true",
    "Enable 16-bit Mul instructions", []>;

def FeatureMul32 : SubtargetFeature<"mul32", "HasMul32", "true",
    "Enable 32-bit Mul instructions", []>;

def FeatureMul32h : SubtargetFeature<"mul32h", "HasMul32h", "true",
    "Enable 32-bit Mul High instructions", []>;

def FeatureHWF32 : SubtargetFeature<"hwf32", "HasHWF32", "true",
    "Enable HW single precision floating point instructions", []>;

def FeatureHWF64 : SubtargetFeature<"hwf64", "HasHWF64", "true",
    "Enable HW double precision floating point instructions", []>;

def FeatureClamps : SubtargetFeature<"clamps", "HasClamps", "true",
    "Enable CLAMPS instruction", []>;

def FeatureCall4 : SubtargetFeature<"call4", "HasCall4", "true",
    "Enable CALL4 instruction", []>;

def FeatureConst16 : SubtargetFeature<"const16", "HasConst16", "true",
    "Enable CONST16 instruction", []>;

def FeatureDensity : SubtargetFeature<"density", "HasDensity", "true",
    "Enable the density instructions", []>;

def FeatureDepbits : SubtargetFeature<"depbits", "HasDepbits", "true",
    "Enable DEPBITS instruction", []>;

def FeatureDiv32 : SubtargetFeature<"div32", "HasDiv32", "true",
    "Enable the 32-bit division instructions", []>;

def FeatureL32r : SubtargetFeature<"l32r", "HasL32r", "true",
    "Enable L32R instruction", []>;

def FeatureMac16 : SubtargetFeature<"mac16", "HasMac16", "true",
    "Enable the MAC16 instructions", []>;

def FeatureMinMax : SubtargetFeature<"minmax", "HasMinMax", "true",
    "Enable MIN, MAX, MINU, and MAXU instructions", []>;

def FeatureNsa : SubtargetFeature<"nsa", "HasNsa", "true",
    "Enable NSA and NSAU instructions", []>;

def FeatureSalt : SubtargetFeature<"salt", "HasSalt", "true",
    "Enable SALT and SALTU instructions", []>;

def FeatureSext : SubtargetFeature<"sext", "HasSext", "true",
    "Enable SEXT instruction", []>;

def FeatureLoop : SubtargetFeature<"loop", "HasLoop", "true",
    "Enable LOOP, LOOPGTZ, and LOOPNEZ instructions", []>;

def FeatureSync : SubtargetFeature<"sync", "HasSync", "true",
    "Enable L32AI and S32RI instructions", []>;

def FeatureS32C1I : SubtargetFeature<"s32c1i", "HasS32C1I", "true",
    "Enable S32C1I instruction", []>;

def FeatureExcl : SubtargetFeature<"excl", "HasExcl", "true",
    "Enable L32EX, S32EX, and GETEX instructions", []>;

def FeatureBooleans : SubtargetFeature<"booleans", "HasBooleans", "true",
    "Enable Boolean registers, along with branches and other "
    "operations that refer to them", []>;

def FeatureWindowedABI : SubtargetFeature<"windowed", "IsWindowedABI", "true",
    "Use the Windowed ABI", []>;

def FeatureOldFPABI : SubtargetFeature<"fpabi", "IsFPABI", "true",
    "Use the new FP ABI using FP registers for calls", []>;

def FeatureCoproc : SubtargetFeature<"coproc", "Coproc", "true",
    "Allow use of coprocessor register files. Not safe when "
    "compiling exception or interrupt handlers.", []>;

def FeatureDefaultL32rFlix : SubtargetFeature<"l32r-flix", "DefaultL32rFlix",
    "true", "Enable flixing of L32R with other load instructions", []>;

def FeatureBranchPredict : SubtargetFeature<"bpredict", "IsBPredict", "true",
    "Enable HW branch prediction", []>;

def FeatureBrdec : SubtargetFeature<"brdec", "HasBrdec", "true",
    "Enable BRDEC instruction", []>;

def FeatureXEA3 : SubtargetFeature<"xea3", "IsXEA3", "true",
    "Use Xtensa Exception Architecture 3", []>;

def FeatureHiFi2: SubtargetFeature<"hifi2", "HasHiFi2Ops", "true",
    "Use HiFi2 ops", []>;

def FeatureVecMemcpy: SubtargetFeature<"vecmemcpy", "HasVecMemcpy", "true",
    "Use vector memcpy", []>;

def FeatureVecMemset: SubtargetFeature<"vecmemset", "HasVecMemset", "true",
    "Use vector memset", []>;

//===----------------------------------------------------------------------===//
// Target-dependent custom type definition
//===----------------------------------------------------------------------===//
class CustomType<string name, int alignment, int id, ValueType vt, 
                 ValueType regvt, int nregs> {
  string Namespace = "CustomType";
  string Name = name;
  int Alignment = alignment;
  int Id = id;
  ValueType VT = vt;
  ValueType RegisterVT = regvt;
  int NumRegs = nregs;
}

//===----------------------------------------------------------------------===//
// Map from an instruction to its index in instruction property table.
//===----------------------------------------------------------------------===//
class InstTargetPropertyMapping<string instname, int idx> {
  string InstName = instname;
  int PropIdx = idx;
}

//===----------------------------------------------------------------------===//
// Descriptions
//===----------------------------------------------------------------------===//

include "TensilicaCustomTypes.td"
include "XtensaRegisterInfo.td"
include "TensilicaCustomRegisterInfo.td"
include "XtensaIntrinsics.td"
include "XtensaInstrInfo.td"
include "TensilicaCustomInstrInfo.td"
include "TensilicaCustomInstrTargetPropMap.td"
include "TensilicaCustomInstrPatterns.td"
include "XtensaCallingConv.td"
include "TensilicaCustomCalleeSavedRegs.td"
include "TensilicaGenSchedule.td"

def XtensaInstrInfo : InstrInfo;

//===----------------------------------------------------------------------===//
// Xtensa processors supported.
//===----------------------------------------------------------------------===//

class Proc<string Name, list<SubtargetFeature> Features>
 : Processor<Name, NoItineraries, Features>;

def : Proc<"generic",      [FeatureMul16, FeatureMul32, FeatureHWF32]>;

//===----------------------------------------------------------------------===//
// Declare the target which we are implementing
//===----------------------------------------------------------------------===//

def Xtensa : Target {
  // Pull in Instruction Info:
  let InstructionSet = XtensaInstrInfo;
  let AllowRegisterRenaming = 1;
}
