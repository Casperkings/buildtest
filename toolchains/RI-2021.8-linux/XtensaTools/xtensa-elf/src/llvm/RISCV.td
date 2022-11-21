//===-- RISCV.td - Describe the RISCV Target Machine -------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

include "llvm/Target/Target.td"

//===----------------------------------------------------------------------===//
// RISC-V subtarget features and instruction predicates.
//===----------------------------------------------------------------------===//

def FeatureStdExtM
    : SubtargetFeature<"m", "HasStdExtM", "true",
                       "'M' (Integer Multiplication and Division)">;
def HasStdExtM : Predicate<"Subtarget->hasStdExtM()">,
                           AssemblerPredicate<"FeatureStdExtM",
                           "'M' (Integer Multiplication and Division)">;

def FeatureStdExtA
    : SubtargetFeature<"a", "HasStdExtA", "true",
                       "'A' (Atomic Instructions)">;
def HasStdExtA : Predicate<"Subtarget->hasStdExtA()">,
                           AssemblerPredicate<"FeatureStdExtA",
                           "'A' (Atomic Instructions)">;

def FeatureStdExtF
    : SubtargetFeature<"f", "HasStdExtF", "true",
                       "'F' (Single-Precision Floating-Point)">;
def HasStdExtF : Predicate<"Subtarget->hasStdExtF()">,
                           AssemblerPredicate<"FeatureStdExtF",
                           "'F' (Single-Precision Floating-Point)">;

def FeatureStdExtD
    : SubtargetFeature<"d", "HasStdExtD", "true",
                       "'D' (Double-Precision Floating-Point)",
                       [FeatureStdExtF]>;
def HasStdExtD : Predicate<"Subtarget->hasStdExtD()">,
                           AssemblerPredicate<"FeatureStdExtD",
                           "'D' (Double-Precision Floating-Point)">;

def FeatureStdExtC
    : SubtargetFeature<"c", "HasStdExtC", "true",
                       "'C' (Compressed Instructions)">;
def HasStdExtC : Predicate<"Subtarget->hasStdExtC()">,
                           AssemblerPredicate<"FeatureStdExtC",
                           "'C' (Compressed Instructions)">;

def FeatureRVCHints
    : SubtargetFeature<"rvc-hints", "EnableRVCHintInstrs", "true",
                       "Enable RVC Hint Instructions.">;
def HasRVCHints : Predicate<"Subtarget->enableRVCHintInstrs()">,
                            AssemblerPredicate<"FeatureRVCHints",
                            "RVC Hint Instructions">;

def Feature64Bit
    : SubtargetFeature<"64bit", "HasRV64", "true", "Implements RV64">;
def IsRV64 : Predicate<"Subtarget->is64Bit()">,
                       AssemblerPredicate<"Feature64Bit",
                       "RV64I Base Instruction Set">;
def IsRV32 : Predicate<"!Subtarget->is64Bit()">,
                       AssemblerPredicate<"!Feature64Bit",
                       "RV32I Base Instruction Set">;

def RV64           : HwMode<"+64bit">;
def RV32           : HwMode<"-64bit">;

def FeatureRV32E
    : SubtargetFeature<"e", "IsRV32E", "true",
                       "Implements RV32E (provides 16 rather than 32 GPRs)">;
def IsRV32E : Predicate<"Subtarget->isRV32E()">,
                        AssemblerPredicate<"FeatureRV32E">;

def FeatureRelax
    : SubtargetFeature<"relax", "EnableLinkerRelax", "true",
                       "Enable Linker relaxation.">;

foreach i = {1-31} in
    def FeatureReserveX#i :
        SubtargetFeature<"reserve-x"#i, "UserReservedRegister[RISCV::X"#i#"]",
                         "true", "Reserve X"#i>;

// #if defined(TENSILICA) || 1
def FeatureHWF32 : SubtargetFeature<"hwf32", "HasHWF32", "true",
    "Enable HW single precision floating point instructions", []>;

def FeatureHWF64 : SubtargetFeature<"hwf64", "HasHWF64", "true",
    "Enable HW double precision floating point instructions", []>;

def FeatureLoop : SubtargetFeature<"loop", "HasLoop", "true",
    "Enable LOOP, LOOPGTZ, and LOOPNEZ instructions", []>;

def FeatureBrdec : SubtargetFeature<"brdec", "HasBrdec", "true",
    "Enable BRDEC instruction", []>;

def FeatureBooleans : SubtargetFeature<"booleans", "HasBooleans", "true",
    "Enable Boolean registers, along with branches and other "
    "operations that refer to them", []>;

def FeatureCoproc : SubtargetFeature<"coproc", "Coproc", "true",
    "Allow use of coprocessor register files. Not safe when "
    "compiling exception or interrupt handlers.", []>;

def FeatureHiFi2: SubtargetFeature<"hifi2", "HasHiFi2Ops", "true",
    "Use HiFi2 ops", []>;

def FeatureCmov: SubtargetFeature<"cmov", "HasCmov", "true",
    "Enable Xicmov conditional move instructions", []>;

def FeatureMinMax : SubtargetFeature<"minmax", "HasMinMax", "true",
    "Enable MIN, MAX, MINU, and MAXU instructions", []>;

def FeatureSext : SubtargetFeature<"sext", "HasSext", "true",
    "Enable SEXT.B, SEXT.H and ZEXT.H instructions", []>;

def FeatureStdExtZbb
    : SubtargetFeature<"zbb", "HasStdExtZbb", "true",
                       "'Zbb' (Supported base 'B' Instructions)">;

def FeatureVecMemcpy: SubtargetFeature<"vecmemcpy", "HasVecMemcpy", "true",
    "Use vector memcpy", []>;

def FeatureVecMemset: SubtargetFeature<"vecmemset", "HasVecMemset", "true",
    "Use vector memset", []>;

def HardwareF32 : Predicate<"Subtarget->hasHWF32()">;
def HasMinMax : Predicate<"Subtarget->hasMinMax()">;
def HasSext : Predicate<"Subtarget->hasSext()">;
def HasStdExtZbbOrZbp : Predicate<"Subtarget->hasStdExtZbb()">;
// #endif

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
// Named operands for CSR instructions.
//===----------------------------------------------------------------------===//

include "RISCVSystemOperands.td"

//===----------------------------------------------------------------------===//
// Registers, calling conventions, instruction descriptions.
//===----------------------------------------------------------------------===//

include "TensilicaCustomTypes.td"
include "RISCVSchedule.td"
include "RISCVRegisterInfo.td"
include "TensilicaCustomRegisterInfo.td"
include "RISCVCallingConv.td"
include "RISCVIntrinsics.td"
include "RISCVInstrInfo.td"
include "TensilicaCustomInstrInfo.td"
include "TensilicaCustomInstrTargetPropMap.td"
include "TensilicaCustomInstrPatterns.td"
include "RISCVRegisterBanks.td"
include "RISCVSchedRocket32.td"
include "RISCVSchedRocket64.td"
include "TensilicaGenSchedule.td"

//===----------------------------------------------------------------------===//
// RISC-V processors supported.
//===----------------------------------------------------------------------===//

def : ProcessorModel<"generic-rv32", NoSchedModel, [FeatureRVCHints]>;

def : ProcessorModel<"generic-rv64", NoSchedModel, [Feature64Bit,
                     FeatureRVCHints]>;

def : ProcessorModel<"rocket-rv32", Rocket32Model, [FeatureRVCHints]>;

def : ProcessorModel<"rocket-rv64", Rocket64Model, [Feature64Bit,
                     FeatureRVCHints]>;


//===----------------------------------------------------------------------===//
// Define the RISC-V target.
//===----------------------------------------------------------------------===//

def RISCVInstrInfo : InstrInfo {
  let guessInstructionProperties = 0;
}

def RISCVAsmParser : AsmParser {
  let ShouldEmitMatchRegisterAltName = 1;
  let AllowDuplicateRegisterNames = 1;
}

def RISCVAsmWriter : AsmWriter {
  int PassSubtarget = 1;
}

def RISCV : Target {
  let InstructionSet = RISCVInstrInfo;
  let AssemblyParsers = [RISCVAsmParser];
  let AssemblyWriters = [RISCVAsmWriter];
  let AllowRegisterRenaming = 1;
}
