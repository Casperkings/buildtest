//===-- XtensaInstrFormats.td - Xtensa Instruction Formats ---*- tablegen--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Instruction format superclass
//===----------------------------------------------------------------------===//
class InstXtensa<dag outs, dag ins, string asmstr, list<dag> pattern>
    : Instruction {
  field bits<32> Inst = 0;

  let Namespace = "Xtensa";
  dag OutOperandList = outs;
  dag InOperandList = ins;
  let AsmString = asmstr;
  let hasSideEffects = 0;
  let Pattern = pattern;
  field bits<32> SoftFail = 0;
  field bit isCore = 0;
}

// Xtensa pseudo instructions format
class PseudoInstXtensa<dag outs, dag ins, string asmstr, list<dag> pattern>
    : InstXtensa<outs, ins, asmstr, pattern> {
  let isPseudo = 1;
}

// Xtensa base core instruction.
// These instructions are ordered before configuration-specific ones and
// will be accessible from configuration-independent part.
class CoreInstXtensa<dag outs, dag ins, string asmstr, list<dag> pattern>
    : InstXtensa<outs, ins, asmstr, pattern> {
  let isCore = 1;
}

// Xtensa base core pseudo-instruction.
class CorePseudoInstXtensa<dag outs, dag ins, string asmstr, list<dag> pattern>
    : PseudoInstXtensa<outs, ins, asmstr, pattern> {
  let isCore = 1;
}

//===----------------------------------------------------------------------===//
// Instruction format superclass (used by config-gen)
//===----------------------------------------------------------------------===//
class InstTensilica<dag outs, dag ins, string asmstr, list<dag> pattern>
    : Instruction {
  field bits<32> Inst = 0;

  let Namespace = "Xtensa";
  dag OutOperandList = outs;
  dag InOperandList = ins;
  let AsmString = asmstr;
  let hasSideEffects = 0;
  let mayLoad = 0;
  let mayStore = 0;
  let Pattern = pattern;
  field bits<32> SoftFail = 0;
  field bit isCore = 0;
}

// Xtensa pseudo instructions format
class PseudoInstTensilica<dag outs, dag ins, string asmstr, list<dag> pattern>
    : InstTensilica<outs, ins, asmstr, pattern> {
  let isPseudo = 1;
}

// Xtensa base core instruction.
// These instructions are ordered before configuration-specific ones and
// will be accessible from configuration-independent part.
class CoreInstTensilica<dag outs, dag ins, string asmstr, list<dag> pattern>
    : InstTensilica<outs, ins, asmstr, pattern> {
  let isCore = 1;
}

// Xtensa base core pseudo-instruction.
class CorePseudoInstTensilica<dag outs, dag ins, string asmstr, 
                              list<dag> pattern>
    : PseudoInstTensilica<outs, ins, asmstr, pattern> {
  let isCore = 1;
}

class MemoryAccessMap;
// Accessor: Load, Store
// Aligning: None, Primary(LAV.IP), Remainder(LA.PP SAPOS),
// variable-length(LAV.XP)
// DataMapping(input/output data mapping from_type:to_type [for now, one
// transformation for each pair?] ): GenuineV512, SExt16-32-512
// Predication: none, true, false
// IndexType: Imm, Reg
// UpdateMode: None, Post, Pre

// Exceptions L32R L32E S32E L2AV GATHER SCATTER
// TODO L32I.N

def getRegIndexOpcode : InstrMapping {
  let FilterClass = "MemoryAccessMap";
  let RowFields = [ "Accessor", "DataMap", "Predication", "Aligning" ];
  let ColFields = [ "IndexType" ];
  let KeyCol = [ "Immediate" ];
  let ValueCols = [ [ "Reg" ] ];
}

def getImmediateIndexOpcode : InstrMapping {
  let FilterClass = "MemoryAccessMap";
  // the row, for example, is vec-512-load
  let RowFields = [ "Accessor", "DataMap", "Predication", "Aligning" ];
  let ColFields = [ "IndexType" ];
  let KeyCol = [ "Reg" ];
  let ValueCols = [ [ "Immediate" ] ];
}

def getPostUpdateOpcode : InstrMapping {
  let FilterClass = "MemoryAccessMap";
  // the row, for example, is vec-512-load
  let RowFields = [ "Accessor", "DataMap", "Predication", "Aligning" ];
  let ColFields = [ "UpdateMode" ];
  let KeyCol = [ "None" ];
  let ValueCols = [ [ "Post" ] ];
}

def getNoUpdateOpcode : InstrMapping {
  let FilterClass = "MemoryAccessMap";
  // the row, for example, is vec-512-load
  let RowFields = [ "Accessor", "DataMap", "Predication", "Aligning" ];
  let ColFields = [ "UpdateMode" ];
  let KeyCol = [ "Post" ];
  let ValueCols = [ [ "None" ] ];
}
