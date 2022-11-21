//===-- RISCVInstrInfo.td - Target Description for RISCV ---*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file describes the RISC-V instructions in TableGen format.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// RISC-V specific DAG Nodes.
//===----------------------------------------------------------------------===//

// Pattern fragment to specify i32 value type in AR register.
// if defined(TENSILICA) || 1
def I32 : PatLeaf<(i32 GPR:$R)>;
// endif

// Target-independent type requirements, but with target-specific formats.
def SDT_CallSeqStart : SDCallSeqStart<[SDTCisVT<0, i32>,
                                       SDTCisVT<1, i32>]>;
def SDT_CallSeqEnd   : SDCallSeqEnd<[SDTCisVT<0, i32>,
                                     SDTCisVT<1, i32>]>;

// Target-dependent type requirements.
def SDT_RISCVCall     : SDTypeProfile<0, -1, [SDTCisVT<0, XLenVT>]>;
def SDT_RISCVSelectCC : SDTypeProfile<1, 5, [SDTCisSameAs<1, 2>,
                                             SDTCisSameAs<0, 4>,
                                             SDTCisSameAs<4, 5>]>;

// Target-independent nodes, but with target-specific formats.
def callseq_start : SDNode<"ISD::CALLSEQ_START", SDT_CallSeqStart,
                           [SDNPHasChain, SDNPOutGlue]>;
def callseq_end   : SDNode<"ISD::CALLSEQ_END", SDT_CallSeqEnd,
                           [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;

// Target-dependent nodes.
def riscv_call      : SDNode<"RISCVISD::CALL", SDT_RISCVCall,
                             [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue,
                              SDNPVariadic]>;
def riscv_ret_flag  : SDNode<"RISCVISD::RET_FLAG", SDTNone,
                             [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;
def riscv_uret_flag : SDNode<"RISCVISD::URET_FLAG", SDTNone,
                             [SDNPHasChain, SDNPOptInGlue]>;
def riscv_sret_flag : SDNode<"RISCVISD::SRET_FLAG", SDTNone,
                             [SDNPHasChain, SDNPOptInGlue]>;
def riscv_mret_flag : SDNode<"RISCVISD::MRET_FLAG", SDTNone,
                             [SDNPHasChain, SDNPOptInGlue]>;
def riscv_selectcc  : SDNode<"RISCVISD::SELECT_CC", SDT_RISCVSelectCC,
                             [SDNPInGlue]>;
def riscv_tail      : SDNode<"RISCVISD::TAIL", SDT_RISCVCall,
                             [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue,
                              SDNPVariadic]>;
def riscv_sllw      : SDNode<"RISCVISD::SLLW", SDTIntShiftOp>;
def riscv_sraw      : SDNode<"RISCVISD::SRAW", SDTIntShiftOp>;
def riscv_srlw      : SDNode<"RISCVISD::SRLW", SDTIntShiftOp>;

//===----------------------------------------------------------------------===//
// Operand and SDNode transformation definitions.
//===----------------------------------------------------------------------===//

class ImmXLenAsmOperand<string prefix, string suffix = ""> : AsmOperandClass {
  let Name = prefix # "ImmXLen" # suffix;
  let RenderMethod = "addImmOperands";
  let DiagnosticType = !strconcat("Invalid", Name);
}

class ImmAsmOperand<string prefix, int width, string suffix> : AsmOperandClass {
  let Name = prefix # "Imm" # width # suffix;
  let RenderMethod = "addImmOperands";
  let DiagnosticType = !strconcat("Invalid", Name);
}

def ImmZeroAsmOperand : AsmOperandClass {
  let Name = "ImmZero";
  let RenderMethod = "addImmOperands";
  let DiagnosticType = !strconcat("Invalid", Name);
}

class SImmAsmOperand<int width, string suffix = "">
    : ImmAsmOperand<"S", width, suffix> {
}

class UImmAsmOperand<int width, string suffix = "">
    : ImmAsmOperand<"U", width, suffix> {
}

def FenceArg : AsmOperandClass {
  let Name = "FenceArg";
  let RenderMethod = "addFenceArgOperands";
  let DiagnosticType = "InvalidFenceArg";
}

def fencearg : Operand<XLenVT> {
  let ParserMatchClass = FenceArg;
  let PrintMethod = "printFenceArg";
  let DecoderMethod = "decodeUImmOperand<4>";
  let OperandType = "OPERAND_UIMM4";
  let OperandNamespace = "RISCVOp";
}

def UImmLog2XLenAsmOperand : AsmOperandClass {
  let Name = "UImmLog2XLen";
  let RenderMethod = "addImmOperands";
  let DiagnosticType = "InvalidUImmLog2XLen";
}

def uimmlog2xlen : Operand<XLenVT>, ImmLeaf<XLenVT, [{
  if (Subtarget->is64Bit())
    return isUInt<6>(Imm);
  return isUInt<5>(Imm);
}]> {
  let ParserMatchClass = UImmLog2XLenAsmOperand;
  // TODO: should ensure invalid shamt is rejected when decoding.
  let DecoderMethod = "decodeUImmOperand<6>";
  let MCOperandPredicate = [{
    int64_t Imm;
    if (!MCOp.evaluateAsConstantImm(Imm))
      return false;
    if (STI.getTargetTriple().isArch64Bit())
      return  isUInt<6>(Imm);
    return isUInt<5>(Imm);
  }];
  let OperandType = "OPERAND_UIMMLOG2XLEN";
  let OperandNamespace = "RISCVOp";
}

def uimm5 : Operand<XLenVT>, ImmLeaf<XLenVT, [{return isUInt<5>(Imm);}]> {
  let ParserMatchClass = UImmAsmOperand<5>;
  let DecoderMethod = "decodeUImmOperand<5>";
  let OperandType = "OPERAND_UIMM5";
  let OperandNamespace = "RISCVOp";
}

def simm12 : Operand<XLenVT>, ImmLeaf<XLenVT, [{return isInt<12>(Imm);}]> {
  let ParserMatchClass = SImmAsmOperand<12>;
  let EncoderMethod = "getImmOpValue";
  let DecoderMethod = "decodeSImmOperand<12>";
  let MCOperandPredicate = [{
    int64_t Imm;
    if (MCOp.evaluateAsConstantImm(Imm))
      return isInt<12>(Imm);
    return MCOp.isBareSymbolRef();
  }];
  let OperandType = "OPERAND_SIMM12";
  let OperandNamespace = "RISCVOp";
}

// A 13-bit signed immediate where the least significant bit is zero.
def simm13_lsb0 : Operand<OtherVT> {
  let ParserMatchClass = SImmAsmOperand<13, "Lsb0">;
  let EncoderMethod = "getImmOpValueAsr1";
  let DecoderMethod = "decodeSImmOperandAndLsl1<13>";
  let MCOperandPredicate = [{
    int64_t Imm;
    if (MCOp.evaluateAsConstantImm(Imm))
      return isShiftedInt<12, 1>(Imm);
    return MCOp.isBareSymbolRef();
  }];
  let OperandType = "OPERAND_SIMM13_LSB0";
  let OperandNamespace = "RISCVOp";
}

class UImm20Operand : Operand<XLenVT> {
  let EncoderMethod = "getImmOpValue";
  let DecoderMethod = "decodeUImmOperand<20>";
  let MCOperandPredicate = [{
    int64_t Imm;
    if (MCOp.evaluateAsConstantImm(Imm))
      return isUInt<20>(Imm);
    return MCOp.isBareSymbolRef();
  }];
  let OperandType = "OPERAND_UIMM20";
  let OperandNamespace = "RISCVOp";
}

def uimm20_lui : UImm20Operand {
  let ParserMatchClass = UImmAsmOperand<20, "LUI">;
}
def uimm20_auipc : UImm20Operand {
  let ParserMatchClass = UImmAsmOperand<20, "AUIPC">;
}

def Simm21Lsb0JALAsmOperand : SImmAsmOperand<21, "Lsb0JAL"> {
  let ParserMethod = "parseJALOffset";
}

// A 21-bit signed immediate where the least significant bit is zero.
def simm21_lsb0_jal : Operand<OtherVT> {
  let ParserMatchClass = Simm21Lsb0JALAsmOperand;
  let EncoderMethod = "getImmOpValueAsr1";
  let DecoderMethod = "decodeSImmOperandAndLsl1<21>";
  let MCOperandPredicate = [{
    int64_t Imm;
    if (MCOp.evaluateAsConstantImm(Imm))
      return isShiftedInt<20, 1>(Imm);
    return MCOp.isBareSymbolRef();
  }];
  let OperandType = "OPERAND_SIMM21_LSB0";
  let OperandNamespace = "RISCVOp";
}

def BareSymbol : AsmOperandClass {
  let Name = "BareSymbol";
  let RenderMethod = "addImmOperands";
  let DiagnosticType = "InvalidBareSymbol";
  let ParserMethod = "parseBareSymbol";
}

// A bare symbol.
def bare_symbol : Operand<XLenVT> {
  let ParserMatchClass = BareSymbol;
}

def CallSymbol : AsmOperandClass {
  let Name = "CallSymbol";
  let RenderMethod = "addImmOperands";
  let DiagnosticType = "InvalidCallSymbol";
  let ParserMethod = "parseCallSymbol";
}

// A bare symbol used in call/tail only.
def call_symbol : Operand<XLenVT> {
  let ParserMatchClass = CallSymbol;
}

def TPRelAddSymbol : AsmOperandClass {
  let Name = "TPRelAddSymbol";
  let RenderMethod = "addImmOperands";
  let DiagnosticType = "InvalidTPRelAddSymbol";
  let ParserMethod = "parseOperandWithModifier";
}

// A bare symbol with the %tprel_add variant.
def tprel_add_symbol : Operand<XLenVT> {
  let ParserMatchClass = TPRelAddSymbol;
}

def CSRSystemRegister : AsmOperandClass {
  let Name = "CSRSystemRegister";
  let ParserMethod = "parseCSRSystemRegister";
  let DiagnosticType = "InvalidCSRSystemRegister";
}

def csr_sysreg : Operand<XLenVT> {
  let ParserMatchClass = CSRSystemRegister;
  let PrintMethod = "printCSRSystemRegister";
  let DecoderMethod = "decodeUImmOperand<12>";
  let OperandType = "OPERAND_UIMM12";
  let OperandNamespace = "RISCVOp";
}

// A parameterized register class alternative to i32imm/i64imm from Target.td.
def ixlenimm : Operand<XLenVT>;

def ixlenimm_li : Operand<XLenVT> {
  let ParserMatchClass = ImmXLenAsmOperand<"", "LI">;
}

// Standalone (codegen-only) immleaf patterns.
def simm32     : ImmLeaf<XLenVT, [{return isInt<32>(Imm);}]>;
def simm32hi20 : ImmLeaf<XLenVT, [{return isShiftedInt<20, 12>(Imm);}]>;
// A mask value that won't affect significant shift bits.
def immbottomxlenset : ImmLeaf<XLenVT, [{
  if (Subtarget->is64Bit())
    return countTrailingOnes<uint64_t>(Imm) >= 6;
  return countTrailingOnes<uint64_t>(Imm) >= 5;
}]>;

// Addressing modes.
// Necessary because a frameindex can't be matched directly in a pattern.
def AddrFI : ComplexPattern<iPTR, 1, "SelectAddrFI", [frameindex], []>;

// Extract least significant 12 bits from an immediate value and sign extend
// them.
def LO12Sext : SDNodeXForm<imm, [{
  return CurDAG->getTargetConstant(SignExtend64<12>(N->getZExtValue()),
                                   SDLoc(N), N->getValueType(0));
}]>;

// Extract the most significant 20 bits from an immediate value. Add 1 if bit
// 11 is 1, to compensate for the low 12 bits in the matching immediate addi
// or ld/st being negative.
def HI20 : SDNodeXForm<imm, [{
  return CurDAG->getTargetConstant(((N->getZExtValue()+0x800) >> 12) & 0xfffff,
                                   SDLoc(N), N->getValueType(0));
}]>;

//===----------------------------------------------------------------------===//
// Instruction Formats
//===----------------------------------------------------------------------===//

include "RISCVInstrFormats.td"

//===----------------------------------------------------------------------===//
// Instruction Class Templates
//===----------------------------------------------------------------------===//

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
class BranchCC_rri<bits<3> funct3, string opcodestr>
    : RVInstB<funct3, OPC_BRANCH, (outs),
              (ins GPR:$rs1, GPR:$rs2, simm13_lsb0:$imm12),
              opcodestr, "$rs1, $rs2, $imm12">,
      Sched<[WriteJmp]> {
  let isBranch = 1;
  let isTerminator = 1;
}

let hasSideEffects = 0, mayLoad = 1, mayStore = 0 in
class Load_ri<bits<3> funct3, string opcodestr>
    : RVInstI<funct3, OPC_LOAD, (outs GPR:$rd), (ins GPR:$rs1, simm12:$imm12),
              opcodestr, "$rd, ${imm12}(${rs1})">;

// Operands for stores are in the order srcreg, base, offset rather than
// reflecting the order these fields are specified in the instruction
// encoding.
let hasSideEffects = 0, mayLoad = 0, mayStore = 1 in
class Store_rri<bits<3> funct3, string opcodestr>
    : RVInstS<funct3, OPC_STORE, (outs),
              (ins GPR:$rs2, GPR:$rs1, simm12:$imm12),
              opcodestr, "$rs2, ${imm12}(${rs1})">;

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
class ALU_ri<bits<3> funct3, string opcodestr>
    : RVInstI<funct3, OPC_OP_IMM, (outs GPR:$rd), (ins GPR:$rs1, simm12:$imm12),
              opcodestr, "$rd, $rs1, $imm12">,
      Sched<[WriteIALU, ReadIALU]>;

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
class Shift_ri<bits<5> imm11_7, bits<3> funct3, string opcodestr>
    : RVInstIShift<imm11_7, funct3, OPC_OP_IMM, (outs GPR:$rd),
                   (ins GPR:$rs1, uimmlog2xlen:$shamt), opcodestr,
                   "$rd, $rs1, $shamt">,
      Sched<[WriteShift, ReadShift]>;

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
class ALU_rr<bits<7> funct7, bits<3> funct3, string opcodestr>
    : RVInstR<funct7, funct3, OPC_OP, (outs GPR:$rd), (ins GPR:$rs1, GPR:$rs2),
              opcodestr, "$rd, $rs1, $rs2">;

let hasSideEffects = 1, mayLoad = 0, mayStore = 0 in
class CSR_ir<bits<3> funct3, string opcodestr>
    : RVInstI<funct3, OPC_SYSTEM, (outs GPR:$rd), (ins csr_sysreg:$imm12, GPR:$rs1),
              opcodestr, "$rd, $imm12, $rs1">, Sched<[WriteCSR, ReadCSR]>;

let hasSideEffects = 1, mayLoad = 0, mayStore = 0 in
class CSR_ii<bits<3> funct3, string opcodestr>
    : RVInstI<funct3, OPC_SYSTEM, (outs GPR:$rd),
              (ins csr_sysreg:$imm12, uimm5:$rs1),
              opcodestr, "$rd, $imm12, $rs1">, Sched<[WriteCSR]>;

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
class ShiftW_ri<bit arithshift, bits<3> funct3, string opcodestr>
    : RVInstIShiftW<arithshift, funct3, OPC_OP_IMM_32, (outs GPR:$rd),
                    (ins GPR:$rs1, uimm5:$shamt), opcodestr,
                    "$rd, $rs1, $shamt">,
      Sched<[WriteShift32, ReadShift32]>;

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
class ALUW_rr<bits<7> funct7, bits<3> funct3, string opcodestr>
    : RVInstR<funct7, funct3, OPC_OP_32, (outs GPR:$rd),
              (ins GPR:$rs1, GPR:$rs2), opcodestr, "$rd, $rs1, $rs2">;

let hasSideEffects = 1, mayLoad = 0, mayStore = 0 in
class Priv<string opcodestr, bits<7> funct7>
    : RVInstR<funct7, 0b000, OPC_SYSTEM, (outs), (ins GPR:$rs1, GPR:$rs2),
              opcodestr, "">;

//===----------------------------------------------------------------------===//
// Instructions
//===----------------------------------------------------------------------===//

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in {
let isReMaterializable = 1, isAsCheapAsAMove = 1 in
def LUI : RVInstU<OPC_LUI, (outs GPR:$rd), (ins uimm20_lui:$imm20),
                  "lui", "$rd, $imm20">, Sched<[WriteIALU]>;

def AUIPC : RVInstU<OPC_AUIPC, (outs GPR:$rd), (ins uimm20_auipc:$imm20),
                    "auipc", "$rd, $imm20">, Sched<[WriteIALU]>;

let isCall = 1 in
def JAL : RVInstJ<OPC_JAL, (outs GPR:$rd), (ins simm21_lsb0_jal:$imm20),
                  "jal", "$rd, $imm20">, Sched<[WriteJal]>;

let isCall = 1 in
def JALR : RVInstI<0b000, OPC_JALR, (outs GPR:$rd),
                   (ins GPR:$rs1, simm12:$imm12),
                   "jalr", "$rd, ${imm12}(${rs1})">,
           Sched<[WriteJalr, ReadJalr]>;
} // hasSideEffects = 0, mayLoad = 0, mayStore = 0

def BEQ  : BranchCC_rri<0b000, "beq">;
def BNE  : BranchCC_rri<0b001, "bne">;
def BLT  : BranchCC_rri<0b100, "blt">;
def BGE  : BranchCC_rri<0b101, "bge">;
def BLTU : BranchCC_rri<0b110, "bltu">;
def BGEU : BranchCC_rri<0b111, "bgeu">;

def LB  : Load_ri<0b000, "lb">, Sched<[WriteLDB, ReadMemBase]>;
def LH  : Load_ri<0b001, "lh">, Sched<[WriteLDH, ReadMemBase]>;
def LW  : Load_ri<0b010, "lw">, Sched<[WriteLDW, ReadMemBase]>;
def LBU : Load_ri<0b100, "lbu">, Sched<[WriteLDB, ReadMemBase]>;
def LHU : Load_ri<0b101, "lhu">, Sched<[WriteLDH, ReadMemBase]>;

def SB : Store_rri<0b000, "sb">, Sched<[WriteSTB, ReadStoreData, ReadMemBase]>;
def SH : Store_rri<0b001, "sh">, Sched<[WriteSTH, ReadStoreData, ReadMemBase]>;
def SW : Store_rri<0b010, "sw">, Sched<[WriteSTW, ReadStoreData, ReadMemBase]>;

// ADDI isn't always rematerializable, but isReMaterializable will be used as
// a hint which is verified in isReallyTriviallyReMaterializable.
let isReMaterializable = 1, isAsCheapAsAMove = 1 in
def ADDI  : ALU_ri<0b000, "addi">;

def SLTI  : ALU_ri<0b010, "slti">;
def SLTIU : ALU_ri<0b011, "sltiu">;

let isReMaterializable = 1, isAsCheapAsAMove = 1 in {
def XORI  : ALU_ri<0b100, "xori">;
def ORI   : ALU_ri<0b110, "ori">;
}

def ANDI  : ALU_ri<0b111, "andi">;

def SLLI : Shift_ri<0b00000, 0b001, "slli">;
def SRLI : Shift_ri<0b00000, 0b101, "srli">;
def SRAI : Shift_ri<0b01000, 0b101, "srai">;

def ADD  : ALU_rr<0b0000000, 0b000, "add">, Sched<[WriteIALU, ReadIALU, ReadIALU]>;
def SUB  : ALU_rr<0b0100000, 0b000, "sub">, Sched<[WriteIALU, ReadIALU, ReadIALU]>;
def SLL  : ALU_rr<0b0000000, 0b001, "sll">, Sched<[WriteIALU, ReadIALU, ReadIALU]>;
def SLT  : ALU_rr<0b0000000, 0b010, "slt">, Sched<[WriteIALU, ReadIALU, ReadIALU]>;
def SLTU : ALU_rr<0b0000000, 0b011, "sltu">, Sched<[WriteIALU, ReadIALU, ReadIALU]>;
def XOR  : ALU_rr<0b0000000, 0b100, "xor">, Sched<[WriteIALU, ReadIALU, ReadIALU]>;
def SRL  : ALU_rr<0b0000000, 0b101, "srl">, Sched<[WriteIALU, ReadIALU, ReadIALU]>;
def SRA  : ALU_rr<0b0100000, 0b101, "sra">, Sched<[WriteIALU, ReadIALU, ReadIALU]>;
def OR   : ALU_rr<0b0000000, 0b110, "or">, Sched<[WriteIALU, ReadIALU, ReadIALU]>;
def AND  : ALU_rr<0b0000000, 0b111, "and">, Sched<[WriteIALU, ReadIALU, ReadIALU]>;

let hasSideEffects = 1, mayLoad = 0, mayStore = 0 in {
def FENCE : RVInstI<0b000, OPC_MISC_MEM, (outs),
                    (ins fencearg:$pred, fencearg:$succ),
                    "fence", "$pred, $succ">, Sched<[]> {
  bits<4> pred;
  bits<4> succ;

  let rs1 = 0;
  let rd = 0;
  let imm12 = {0b0000,pred,succ};
}

def FENCE_TSO : RVInstI<0b000, OPC_MISC_MEM, (outs), (ins), "fence.tso", "">, Sched<[]> {
  let rs1 = 0;
  let rd = 0;
  let imm12 = {0b1000,0b0011,0b0011};
}

def FENCE_I : RVInstI<0b001, OPC_MISC_MEM, (outs), (ins), "fence.i", "">, Sched<[]> {
  let rs1 = 0;
  let rd = 0;
  let imm12 = 0;
}

def ECALL : RVInstI<0b000, OPC_SYSTEM, (outs), (ins), "ecall", "">, Sched<[WriteJmp]> {
  let rs1 = 0;
  let rd = 0;
  let imm12 = 0;
}

def EBREAK : RVInstI<0b000, OPC_SYSTEM, (outs), (ins), "ebreak", "">,
             Sched<[]> {
  let rs1 = 0;
  let rd = 0;
  let imm12 = 1;
}

// This is a de facto standard (as set by GNU binutils) 32-bit unimplemented
// instruction (i.e., it should always trap, if your implementation has invalid
// instruction traps).
def UNIMP : RVInstI<0b001, OPC_SYSTEM, (outs), (ins), "unimp", "">,
            Sched<[]> {
  let rs1 = 0;
  let rd = 0;
  let imm12 = 0b110000000000;
}
} // hasSideEffects = 1, mayLoad = 0, mayStore = 0

def CSRRW : CSR_ir<0b001, "csrrw">;
def CSRRS : CSR_ir<0b010, "csrrs">;
def CSRRC : CSR_ir<0b011, "csrrc">;

def CSRRWI : CSR_ii<0b101, "csrrwi">;
def CSRRSI : CSR_ii<0b110, "csrrsi">;
def CSRRCI : CSR_ii<0b111, "csrrci">;

/// RV64I instructions

let Predicates = [IsRV64] in {
def LWU   : Load_ri<0b110, "lwu">, Sched<[WriteLDWU, ReadMemBase]>;
def LD    : Load_ri<0b011, "ld">, Sched<[WriteLDD, ReadMemBase]>;
def SD    : Store_rri<0b011, "sd">, Sched<[WriteSTD, ReadStoreData, ReadMemBase]>;

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
def ADDIW : RVInstI<0b000, OPC_OP_IMM_32, (outs GPR:$rd),
                    (ins GPR:$rs1, simm12:$imm12),
                    "addiw", "$rd, $rs1, $imm12">,
            Sched<[WriteIALU32, ReadIALU32]>;

def SLLIW : ShiftW_ri<0, 0b001, "slliw">;
def SRLIW : ShiftW_ri<0, 0b101, "srliw">;
def SRAIW : ShiftW_ri<1, 0b101, "sraiw">;

def ADDW  : ALUW_rr<0b0000000, 0b000, "addw">,
            Sched<[WriteIALU32, ReadIALU32, ReadIALU32]>;
def SUBW  : ALUW_rr<0b0100000, 0b000, "subw">,
            Sched<[WriteIALU32, ReadIALU32, ReadIALU32]>;
def SLLW  : ALUW_rr<0b0000000, 0b001, "sllw">,
            Sched<[WriteIALU32, ReadIALU32, ReadIALU32]>;
def SRLW  : ALUW_rr<0b0000000, 0b101, "srlw">,
            Sched<[WriteIALU32, ReadIALU32, ReadIALU32]>;
def SRAW  : ALUW_rr<0b0100000, 0b101, "sraw">,
            Sched<[WriteIALU32, ReadIALU32, ReadIALU32]>;
} // Predicates = [IsRV64]

//===----------------------------------------------------------------------===//
// Privileged instructions
//===----------------------------------------------------------------------===//

let isBarrier = 1, isReturn = 1, isTerminator = 1 in {
def URET : Priv<"uret", 0b0000000>, Sched<[]> {
  let rd = 0;
  let rs1 = 0;
  let rs2 = 0b00010;
}

def SRET : Priv<"sret", 0b0001000>, Sched<[]> {
  let rd = 0;
  let rs1 = 0;
  let rs2 = 0b00010;
}

def MRET : Priv<"mret", 0b0011000>, Sched<[]> {
  let rd = 0;
  let rs1 = 0;
  let rs2 = 0b00010;
}
} // isBarrier = 1, isReturn = 1, isTerminator = 1

def WFI : Priv<"wfi", 0b0001000>, Sched<[]> {
  let rd = 0;
  let rs1 = 0;
  let rs2 = 0b00101;
}

let hasSideEffects = 1, mayLoad = 0, mayStore = 0 in
def SFENCE_VMA : RVInstR<0b0001001, 0b000, OPC_SYSTEM, (outs),
                         (ins GPR:$rs1, GPR:$rs2),
                         "sfence.vma", "$rs1, $rs2">, Sched<[]> {
  let rd = 0;
}

//===----------------------------------------------------------------------===//
// Assembler Pseudo Instructions (User-Level ISA, Version 2.2, Chapter 20)
//===----------------------------------------------------------------------===//

def : InstAlias<"nop",           (ADDI      X0,      X0,       0)>;

// Note that the size is 32 because up to 8 32-bit instructions are needed to
// generate an arbitrary 64-bit immediate. However, the size does not really
// matter since PseudoLI is currently only used in the AsmParser where it gets
// expanded to real instructions immediately.
let hasSideEffects = 0, mayLoad = 0, mayStore = 0, Size = 32,
    isCodeGenOnly = 0, isAsmParserOnly = 1 in
def PseudoLI : Pseudo<(outs GPR:$rd), (ins ixlenimm_li:$imm), [],
                      "li", "$rd, $imm">;

def PseudoLB  : PseudoLoad<"lb">;
def PseudoLBU : PseudoLoad<"lbu">;
def PseudoLH  : PseudoLoad<"lh">;
def PseudoLHU : PseudoLoad<"lhu">;
def PseudoLW  : PseudoLoad<"lw">;

def PseudoSB  : PseudoStore<"sb">;
def PseudoSH  : PseudoStore<"sh">;
def PseudoSW  : PseudoStore<"sw">;

let Predicates = [IsRV64] in {
def PseudoLWU : PseudoLoad<"lwu">;
def PseudoLD  : PseudoLoad<"ld">;
def PseudoSD  : PseudoStore<"sd">;
} // Predicates = [IsRV64]

def : InstAlias<"mv $rd, $rs",   (ADDI GPR:$rd, GPR:$rs,       0)>;
def : InstAlias<"not $rd, $rs",  (XORI GPR:$rd, GPR:$rs,      -1)>;
def : InstAlias<"neg $rd, $rs",  (SUB  GPR:$rd,      X0, GPR:$rs)>;

let Predicates = [IsRV64] in {
def : InstAlias<"negw $rd, $rs",   (SUBW  GPR:$rd,      X0, GPR:$rs)>;
def : InstAlias<"sext.w $rd, $rs", (ADDIW GPR:$rd, GPR:$rs,       0)>;
} // Predicates = [IsRV64]

def : InstAlias<"seqz $rd, $rs", (SLTIU GPR:$rd, GPR:$rs,       1)>;
def : InstAlias<"snez $rd, $rs", (SLTU  GPR:$rd,      X0, GPR:$rs)>;
def : InstAlias<"sltz $rd, $rs", (SLT   GPR:$rd, GPR:$rs,      X0)>;
def : InstAlias<"sgtz $rd, $rs", (SLT   GPR:$rd,      X0, GPR:$rs)>;

// sgt/sgtu are recognised by the GNU assembler but the canonical slt/sltu
// form will always be printed. Therefore, set a zero weight.
def : InstAlias<"sgt $rd, $rs, $rt", (SLT GPR:$rd, GPR:$rt, GPR:$rs), 0>;
def : InstAlias<"sgtu $rd, $rs, $rt", (SLTU GPR:$rd, GPR:$rt, GPR:$rs), 0>;

def : InstAlias<"beqz $rs, $offset",
                (BEQ GPR:$rs,      X0, simm13_lsb0:$offset)>;
def : InstAlias<"bnez $rs, $offset",
                (BNE GPR:$rs,      X0, simm13_lsb0:$offset)>;
def : InstAlias<"blez $rs, $offset",
                (BGE      X0, GPR:$rs, simm13_lsb0:$offset)>;
def : InstAlias<"bgez $rs, $offset",
                (BGE GPR:$rs,      X0, simm13_lsb0:$offset)>;
def : InstAlias<"bltz $rs, $offset",
                (BLT GPR:$rs,      X0, simm13_lsb0:$offset)>;
def : InstAlias<"bgtz $rs, $offset",
                (BLT      X0, GPR:$rs, simm13_lsb0:$offset)>;

// Always output the canonical mnemonic for the pseudo branch instructions.
// The GNU tools emit the canonical mnemonic for the branch pseudo instructions
// as well (e.g. "bgt" will be recognised by the assembler but never printed by
// objdump). Match this behaviour by setting a zero weight.
def : InstAlias<"bgt $rs, $rt, $offset",
                (BLT  GPR:$rt, GPR:$rs, simm13_lsb0:$offset), 0>;
def : InstAlias<"ble $rs, $rt, $offset",
                (BGE  GPR:$rt, GPR:$rs, simm13_lsb0:$offset), 0>;
def : InstAlias<"bgtu $rs, $rt, $offset",
                (BLTU GPR:$rt, GPR:$rs, simm13_lsb0:$offset), 0>;
def : InstAlias<"bleu $rs, $rt, $offset",
                (BGEU GPR:$rt, GPR:$rs, simm13_lsb0:$offset), 0>;

def : InstAlias<"j $offset",   (JAL X0, simm21_lsb0_jal:$offset)>;
def : InstAlias<"jal $offset", (JAL X1, simm21_lsb0_jal:$offset)>;

// Non-zero offset aliases of "jalr" are the lowest weight, followed by the
// two-register form, then the one-register forms and finally "ret".
def : InstAlias<"jr $rs",                (JALR      X0, GPR:$rs, 0), 3>;
def : InstAlias<"jr ${offset}(${rs})",   (JALR      X0, GPR:$rs, simm12:$offset)>;
def : InstAlias<"jalr $rs",              (JALR      X1, GPR:$rs, 0), 3>;
def : InstAlias<"jalr ${offset}(${rs})", (JALR      X1, GPR:$rs, simm12:$offset)>;
def : InstAlias<"jalr $rd, $rs",         (JALR GPR:$rd, GPR:$rs, 0), 2>;
def : InstAlias<"ret",                   (JALR      X0,      X1, 0), 4>;

// Non-canonical forms for jump targets also accepted by the assembler.
def : InstAlias<"jr $rs, $offset",        (JALR      X0, GPR:$rs, simm12:$offset), 0>;
def : InstAlias<"jalr $rs, $offset",      (JALR      X1, GPR:$rs, simm12:$offset), 0>;
def : InstAlias<"jalr $rd, $rs, $offset", (JALR GPR:$rd, GPR:$rs, simm12:$offset), 0>;

def : InstAlias<"fence", (FENCE 0xF, 0xF)>; // 0xF == iorw

def : InstAlias<"rdinstret $rd", (CSRRS GPR:$rd, INSTRET.Encoding, X0)>;
def : InstAlias<"rdcycle $rd",   (CSRRS GPR:$rd, CYCLE.Encoding, X0)>;
def : InstAlias<"rdtime $rd",    (CSRRS GPR:$rd, TIME.Encoding, X0)>;

let Predicates = [IsRV32] in {
def : InstAlias<"rdinstreth $rd", (CSRRS GPR:$rd, INSTRETH.Encoding, X0)>;
def : InstAlias<"rdcycleh $rd",   (CSRRS GPR:$rd, CYCLEH.Encoding, X0)>;
def : InstAlias<"rdtimeh $rd",    (CSRRS GPR:$rd, TIMEH.Encoding, X0)>;
} // Predicates = [IsRV32]

def : InstAlias<"csrr $rd, $csr", (CSRRS GPR:$rd, csr_sysreg:$csr,      X0)>;
def : InstAlias<"csrw $csr, $rs", (CSRRW      X0, csr_sysreg:$csr, GPR:$rs)>;
def : InstAlias<"csrs $csr, $rs", (CSRRS      X0, csr_sysreg:$csr, GPR:$rs)>;
def : InstAlias<"csrc $csr, $rs", (CSRRC      X0, csr_sysreg:$csr, GPR:$rs)>;

def : InstAlias<"csrwi $csr, $imm", (CSRRWI X0, csr_sysreg:$csr, uimm5:$imm)>;
def : InstAlias<"csrsi $csr, $imm", (CSRRSI X0, csr_sysreg:$csr, uimm5:$imm)>;
def : InstAlias<"csrci $csr, $imm", (CSRRCI X0, csr_sysreg:$csr, uimm5:$imm)>;

let EmitPriority = 0 in {
def : InstAlias<"csrw $csr, $imm", (CSRRWI X0, csr_sysreg:$csr, uimm5:$imm)>;
def : InstAlias<"csrs $csr, $imm", (CSRRSI X0, csr_sysreg:$csr, uimm5:$imm)>;
def : InstAlias<"csrc $csr, $imm", (CSRRCI X0, csr_sysreg:$csr, uimm5:$imm)>;

def : InstAlias<"csrrw $rd, $csr, $imm", (CSRRWI GPR:$rd, csr_sysreg:$csr, uimm5:$imm)>;
def : InstAlias<"csrrs $rd, $csr, $imm", (CSRRSI GPR:$rd, csr_sysreg:$csr, uimm5:$imm)>;
def : InstAlias<"csrrc $rd, $csr, $imm", (CSRRCI GPR:$rd, csr_sysreg:$csr, uimm5:$imm)>;
}

def : InstAlias<"sfence.vma",     (SFENCE_VMA      X0, X0)>;
def : InstAlias<"sfence.vma $rs", (SFENCE_VMA GPR:$rs, X0)>;

let EmitPriority = 0 in {
def : InstAlias<"lb $rd, (${rs1})",
                (LB  GPR:$rd, GPR:$rs1, 0)>;
def : InstAlias<"lh $rd, (${rs1})",
                (LH  GPR:$rd, GPR:$rs1, 0)>;
def : InstAlias<"lw $rd, (${rs1})",
                (LW  GPR:$rd, GPR:$rs1, 0)>;
def : InstAlias<"lbu $rd, (${rs1})",
                (LBU  GPR:$rd, GPR:$rs1, 0)>;
def : InstAlias<"lhu $rd, (${rs1})",
                (LHU  GPR:$rd, GPR:$rs1, 0)>;

def : InstAlias<"sb $rs2, (${rs1})",
                (SB  GPR:$rs2, GPR:$rs1, 0)>;
def : InstAlias<"sh $rs2, (${rs1})",
                (SH  GPR:$rs2, GPR:$rs1, 0)>;
def : InstAlias<"sw $rs2, (${rs1})",
                (SW  GPR:$rs2, GPR:$rs1, 0)>;

def : InstAlias<"add $rd, $rs1, $imm12",
                (ADDI  GPR:$rd, GPR:$rs1, simm12:$imm12)>;
def : InstAlias<"and $rd, $rs1, $imm12",
                (ANDI  GPR:$rd, GPR:$rs1, simm12:$imm12)>;
def : InstAlias<"xor $rd, $rs1, $imm12",
                (XORI  GPR:$rd, GPR:$rs1, simm12:$imm12)>;
def : InstAlias<"or $rd, $rs1, $imm12",
                (ORI  GPR:$rd, GPR:$rs1, simm12:$imm12)>;
def : InstAlias<"sll $rd, $rs1, $shamt",
                (SLLI  GPR:$rd, GPR:$rs1, uimmlog2xlen:$shamt)>;
def : InstAlias<"srl $rd, $rs1, $shamt",
                (SRLI  GPR:$rd, GPR:$rs1, uimmlog2xlen:$shamt)>;
def : InstAlias<"sra $rd, $rs1, $shamt",
                (SRAI  GPR:$rd, GPR:$rs1, uimmlog2xlen:$shamt)>;
let Predicates = [IsRV64] in {
def : InstAlias<"lwu $rd, (${rs1})",
                (LWU  GPR:$rd, GPR:$rs1, 0)>;
def : InstAlias<"ld $rd, (${rs1})",
                (LD  GPR:$rd, GPR:$rs1, 0)>;
def : InstAlias<"sd $rs2, (${rs1})",
                (SD  GPR:$rs2, GPR:$rs1, 0)>;

def : InstAlias<"addw $rd, $rs1, $imm12",
                (ADDIW  GPR:$rd, GPR:$rs1, simm12:$imm12)>;
def : InstAlias<"sllw $rd, $rs1, $shamt",
                (SLLIW  GPR:$rd, GPR:$rs1, uimm5:$shamt)>;
def : InstAlias<"srlw $rd, $rs1, $shamt",
                (SRLIW  GPR:$rd, GPR:$rs1, uimm5:$shamt)>;
def : InstAlias<"sraw $rd, $rs1, $shamt",
                (SRAIW  GPR:$rd, GPR:$rs1, uimm5:$shamt)>;
} // Predicates = [IsRV64]
def : InstAlias<"slt $rd, $rs1, $imm12",
                (SLTI  GPR:$rd, GPR:$rs1, simm12:$imm12)>;
def : InstAlias<"sltu $rd, $rs1, $imm12",
                (SLTIU  GPR:$rd, GPR:$rs1, simm12:$imm12)>;
}

def : MnemonicAlias<"move", "mv">;

// The SCALL and SBREAK instructions wererenamed to ECALL and EBREAK in
// version 2.1 of the user-level ISA. Like the GNU toolchain, we still accept
// the old name for backwards compatibility.
def : MnemonicAlias<"scall", "ecall">;
def : MnemonicAlias<"sbreak", "ebreak">;

//===----------------------------------------------------------------------===//
// Pseudo-instructions and codegen patterns
//
// Naming convention: For 'generic' pattern classes, we use the naming
// convention PatTy1Ty2. For pattern classes which offer a more complex
// expension, prefix the class name, e.g. BccPat.
//===----------------------------------------------------------------------===//

/// Generic pattern classes

class PatGpr<SDPatternOperator OpNode, RVInst Inst>
    : Pat<(OpNode GPR:$rs1), (Inst GPR:$rs1)>;
class PatGprGpr<SDPatternOperator OpNode, RVInst Inst>
    : Pat<(OpNode GPR:$rs1, GPR:$rs2), (Inst GPR:$rs1, GPR:$rs2)>;
class PatGprSimm12<SDPatternOperator OpNode, RVInstI Inst>
    : Pat<(OpNode GPR:$rs1, simm12:$imm12), (Inst GPR:$rs1, simm12:$imm12)>;

class PatGprImm<SDPatternOperator OpNode, RVInst Inst, ImmLeaf ImmType>
    : Pat<(XLenVT (OpNode (XLenVT GPR:$rs1), ImmType:$imm)),
          (Inst GPR:$rs1, ImmType:$imm)>;
class PatGprUimmLog2XLen<SDPatternOperator OpNode, RVInstIShift Inst>
    : Pat<(OpNode GPR:$rs1, uimmlog2xlen:$shamt),
          (Inst GPR:$rs1, uimmlog2xlen:$shamt)>;

/// Predicates

def IsOrAdd: PatFrag<(ops node:$A, node:$B), (or node:$A, node:$B), [{
  return isOrEquivalentToAdd(N);
}]>;
def assertsexti32 : PatFrag<(ops node:$src), (assertsext node:$src), [{
  return cast<VTSDNode>(N->getOperand(1))->getVT() == MVT::i32;
}]>;
def sexti32 : PatFrags<(ops node:$src),
                       [(sext_inreg node:$src, i32),
                        (assertsexti32 node:$src)]>;
def assertzexti32 : PatFrag<(ops node:$src), (assertzext node:$src), [{
  return cast<VTSDNode>(N->getOperand(1))->getVT() == MVT::i32;
}]>;
def zexti32 : PatFrags<(ops node:$src),
                       [(and node:$src, 0xffffffff),
                        (assertzexti32 node:$src)]>;

/// Immediates

def : Pat<(simm12:$imm), (ADDI X0, simm12:$imm)>;
def : Pat<(simm32hi20:$imm), (LUI (HI20 imm:$imm))>;
def : Pat<(simm32:$imm), (ADDI (LUI (HI20 imm:$imm)), (LO12Sext imm:$imm))>,
      Requires<[IsRV32]>;

/// Simple arithmetic operations

def : PatGprGpr<add, ADD>;
def : PatGprSimm12<add, ADDI>;
def : PatGprGpr<sub, SUB>;
def : PatGprGpr<or, OR>;
def : PatGprSimm12<or, ORI>;
def : PatGprGpr<and, AND>;
def : PatGprSimm12<and, ANDI>;
def : PatGprGpr<xor, XOR>;
def : PatGprSimm12<xor, XORI>;
def : PatGprUimmLog2XLen<shl, SLLI>;
def : PatGprUimmLog2XLen<srl, SRLI>;
def : PatGprUimmLog2XLen<sra, SRAI>;

// Match both a plain shift and one where the shift amount is masked (this is
// typically introduced when the legalizer promotes the shift amount and
// zero-extends it). For RISC-V, the mask is unnecessary as shifts in the base
// ISA only read the least significant 5 bits (RV32I) or 6 bits (RV64I).
class shiftop<SDPatternOperator operator>
    : PatFrags<(ops node:$val, node:$count),
               [(operator node:$val, node:$count),
                (operator node:$val, (and node:$count, immbottomxlenset))]>;

def : PatGprGpr<shiftop<shl>, SLL>;
def : PatGprGpr<shiftop<srl>, SRL>;
def : PatGprGpr<shiftop<sra>, SRA>;

// This is a special case of the ADD instruction used to facilitate the use of a
// fourth operand to emit a relocation on a symbol relating to this instruction.
// The relocation does not affect any bits of the instruction itself but is used
// as a hint to the linker.
let hasSideEffects = 0, mayLoad = 0, mayStore = 0, isCodeGenOnly = 0 in
def PseudoAddTPRel : Pseudo<(outs GPR:$rd),
                            (ins GPR:$rs1, GPR:$rs2, tprel_add_symbol:$src), [],
                            "add", "$rd, $rs1, $rs2, $src">;

/// FrameIndex calculations

def : Pat<(add (i32 AddrFI:$Rs), simm12:$imm12),
          (ADDI (i32 AddrFI:$Rs), simm12:$imm12)>;
def : Pat<(IsOrAdd (i32 AddrFI:$Rs), simm12:$imm12),
          (ADDI (i32 AddrFI:$Rs), simm12:$imm12)>;

/// Setcc

def : PatGprGpr<setlt, SLT>;
def : PatGprSimm12<setlt, SLTI>;
def : PatGprGpr<setult, SLTU>;
def : PatGprSimm12<setult, SLTIU>;

// Define pattern expansions for setcc operations that aren't directly
// handled by a RISC-V instruction.
def : Pat<(seteq GPR:$rs1, 0), (SLTIU GPR:$rs1, 1)>;
def : Pat<(seteq GPR:$rs1, GPR:$rs2), (SLTIU (XOR GPR:$rs1, GPR:$rs2), 1)>;
def : Pat<(seteq GPR:$rs1, simm12:$imm12),
          (SLTIU (XORI GPR:$rs1, simm12:$imm12), 1)>;
def : Pat<(setne GPR:$rs1, 0), (SLTU X0, GPR:$rs1)>;
def : Pat<(setne GPR:$rs1, GPR:$rs2), (SLTU X0, (XOR GPR:$rs1, GPR:$rs2))>;
def : Pat<(setne GPR:$rs1, simm12:$imm12),
          (SLTU X0, (XORI GPR:$rs1, simm12:$imm12))>;
def : Pat<(setugt GPR:$rs1, GPR:$rs2), (SLTU GPR:$rs2, GPR:$rs1)>;
def : Pat<(setuge GPR:$rs1, GPR:$rs2), (XORI (SLTU GPR:$rs1, GPR:$rs2), 1)>;
def : Pat<(setule GPR:$rs1, GPR:$rs2), (XORI (SLTU GPR:$rs2, GPR:$rs1), 1)>;
def : Pat<(setgt GPR:$rs1, GPR:$rs2), (SLT GPR:$rs2, GPR:$rs1)>;
def : Pat<(setge GPR:$rs1, GPR:$rs2), (XORI (SLT GPR:$rs1, GPR:$rs2), 1)>;
def : Pat<(setle GPR:$rs1, GPR:$rs2), (XORI (SLT GPR:$rs2, GPR:$rs1), 1)>;

let usesCustomInserter = 1 in
class SelectCC_rrirr<RegisterClass valty, RegisterClass cmpty>
    : Pseudo<(outs valty:$dst),
             (ins cmpty:$lhs, cmpty:$rhs, ixlenimm:$imm,
              valty:$truev, valty:$falsev),
             [(set valty:$dst, (riscv_selectcc cmpty:$lhs, cmpty:$rhs,
              (XLenVT imm:$imm), valty:$truev, valty:$falsev))]>;

def Select_GPR_Using_CC_GPR : SelectCC_rrirr<GPR, GPR>;

/// Branches and jumps

// Match `(brcond (CondOp ..), ..)` and lower to the appropriate RISC-V branch
// instruction.
class BccPat<PatFrag CondOp, RVInstB Inst>
    : Pat<(brcond (XLenVT (CondOp GPR:$rs1, GPR:$rs2)), bb:$imm12),
          (Inst GPR:$rs1, GPR:$rs2, simm13_lsb0:$imm12)>;

def : BccPat<seteq, BEQ>;
def : BccPat<setne, BNE>;
def : BccPat<setlt, BLT>;
def : BccPat<setge, BGE>;
def : BccPat<setult, BLTU>;
def : BccPat<setuge, BGEU>;

class BccSwapPat<PatFrag CondOp, RVInst InstBcc>
    : Pat<(brcond (XLenVT (CondOp GPR:$rs1, GPR:$rs2)), bb:$imm12),
          (InstBcc GPR:$rs2, GPR:$rs1, bb:$imm12)>;

// Condition codes that don't have matching RISC-V branch instructions, but
// are trivially supported by swapping the two input operands
def : BccSwapPat<setgt, BLT>;
def : BccSwapPat<setle, BGE>;
def : BccSwapPat<setugt, BLTU>;
def : BccSwapPat<setule, BGEU>;

// An extra pattern is needed for a brcond without a setcc (i.e. where the
// condition was calculated elsewhere).
def : Pat<(brcond GPR:$cond, bb:$imm12), (BNE GPR:$cond, X0, bb:$imm12)>;

let isBarrier = 1, isBranch = 1, isTerminator = 1 in
def PseudoBR : Pseudo<(outs), (ins simm21_lsb0_jal:$imm20), [(br bb:$imm20)]>,
               PseudoInstExpansion<(JAL X0, simm21_lsb0_jal:$imm20)>;

let isCall = 1, Defs=[X1] in
let isBarrier = 1, isBranch = 1, isIndirectBranch = 1, isTerminator = 1 in
def PseudoBRIND : Pseudo<(outs), (ins GPR:$rs1, simm12:$imm12), []>,
                  PseudoInstExpansion<(JALR X0, GPR:$rs1, simm12:$imm12)>;

def : Pat<(brind GPR:$rs1), (PseudoBRIND GPR:$rs1, 0)>;
def : Pat<(brind (add GPR:$rs1, simm12:$imm12)),
          (PseudoBRIND GPR:$rs1, simm12:$imm12)>;

// PseudoCALLReg is a generic pseudo instruction for calls which will eventually
// expand to auipc and jalr while encoding, with any given register used as the
// destination.
// Define AsmString to print "call" when compile with -S flag.
// Define isCodeGenOnly = 0 to support parsing assembly "call" instruction.
let isCall = 1, isBarrier = 1, isCodeGenOnly = 0, hasSideEffects = 0,
    mayStore = 0, mayLoad = 0 in
def PseudoCALLReg : Pseudo<(outs GPR:$rd), (ins call_symbol:$func), []> {
  let AsmString = "call\t$rd, $func";
}

// PseudoCALL is a pseudo instruction which will eventually expand to auipc
// and jalr while encoding. This is desirable, as an auipc+jalr pair with
// R_RISCV_CALL and R_RISCV_RELAX relocations can be be relaxed by the linker
// if the offset fits in a signed 21-bit immediate.
// Define AsmString to print "call" when compile with -S flag.
// Define isCodeGenOnly = 0 to support parsing assembly "call" instruction.
let isCall = 1, Defs = [X1], isCodeGenOnly = 0 in
def PseudoCALL : Pseudo<(outs), (ins call_symbol:$func), []> {
  let AsmString = "call\t$func";
}

def : Pat<(riscv_call tglobaladdr:$func), (PseudoCALL tglobaladdr:$func)>;
def : Pat<(riscv_call texternalsym:$func), (PseudoCALL texternalsym:$func)>;

def : Pat<(riscv_uret_flag), (URET X0, X0)>;
def : Pat<(riscv_sret_flag), (SRET X0, X0)>;
def : Pat<(riscv_mret_flag), (MRET X0, X0)>;

let isCall = 1, Defs = [X1] in
def PseudoCALLIndirect : Pseudo<(outs), (ins GPR:$rs1),
                                [(riscv_call GPR:$rs1)]>,
                         PseudoInstExpansion<(JALR X1, GPR:$rs1, 0)>;

let isBarrier = 1, isReturn = 1, isTerminator = 1 in
def PseudoRET : Pseudo<(outs), (ins), [(riscv_ret_flag)]>,
                PseudoInstExpansion<(JALR X0, X1, 0)>;

// PseudoTAIL is a pseudo instruction similar to PseudoCALL and will eventually
// expand to auipc and jalr while encoding.
// Define AsmString to print "tail" when compile with -S flag.
let isCall = 1, isTerminator = 1, isReturn = 1, isBarrier = 1, Uses = [X2],
    isCodeGenOnly = 0 in
def PseudoTAIL : Pseudo<(outs), (ins call_symbol:$dst), []> {
  let AsmString = "tail\t$dst";
}

let isCall = 1, isTerminator = 1, isReturn = 1, isBarrier = 1, Uses = [X2] in
def PseudoTAILIndirect : Pseudo<(outs), (ins GPRTC:$rs1),
                                [(riscv_tail GPRTC:$rs1)]>,
                         PseudoInstExpansion<(JALR X0, GPR:$rs1, 0)>;

def : Pat<(riscv_tail (iPTR tglobaladdr:$dst)),
          (PseudoTAIL texternalsym:$dst)>;
def : Pat<(riscv_tail (iPTR texternalsym:$dst)),
          (PseudoTAIL texternalsym:$dst)>;

let hasSideEffects = 0, mayLoad = 0, mayStore = 0, isCodeGenOnly = 0,
    isAsmParserOnly = 1 in
def PseudoLLA : Pseudo<(outs GPR:$dst), (ins bare_symbol:$src), [],
                       "lla", "$dst, $src">;

let hasSideEffects = 0, mayLoad = 1, mayStore = 0, isCodeGenOnly = 0,
    isAsmParserOnly = 1 in
def PseudoLA : Pseudo<(outs GPR:$dst), (ins bare_symbol:$src), [],
                      "la", "$dst, $src">;

let hasSideEffects = 0, mayLoad = 1, mayStore = 0, isCodeGenOnly = 0,
    isAsmParserOnly = 1 in
def PseudoLA_TLS_IE : Pseudo<(outs GPR:$dst), (ins bare_symbol:$src), [],
                             "la.tls.ie", "$dst, $src">;

let hasSideEffects = 0, mayLoad = 1, mayStore = 0, isCodeGenOnly = 0,
    isAsmParserOnly = 1 in
def PseudoLA_TLS_GD : Pseudo<(outs GPR:$dst), (ins bare_symbol:$src), [],
                             "la.tls.gd", "$dst, $src">;

/// Loads

multiclass LdPat<PatFrag LoadOp, RVInst Inst> {
  def : Pat<(LoadOp GPR:$rs1), (Inst GPR:$rs1, 0)>;
  def : Pat<(LoadOp AddrFI:$rs1), (Inst AddrFI:$rs1, 0)>;
  def : Pat<(LoadOp (add GPR:$rs1, simm12:$imm12)),
            (Inst GPR:$rs1, simm12:$imm12)>;
  def : Pat<(LoadOp (add AddrFI:$rs1, simm12:$imm12)),
            (Inst AddrFI:$rs1, simm12:$imm12)>;
  def : Pat<(LoadOp (IsOrAdd AddrFI:$rs1, simm12:$imm12)),
            (Inst AddrFI:$rs1, simm12:$imm12)>;
}

defm : LdPat<sextloadi8, LB>;
defm : LdPat<extloadi8, LB>;
defm : LdPat<sextloadi16, LH>;
defm : LdPat<extloadi16, LH>;
defm : LdPat<load, LW>, Requires<[IsRV32]>;
defm : LdPat<zextloadi8, LBU>;
defm : LdPat<zextloadi16, LHU>;

/// Stores

multiclass StPat<PatFrag StoreOp, RVInst Inst, RegisterClass StTy> {
  def : Pat<(StoreOp StTy:$rs2, GPR:$rs1), (Inst StTy:$rs2, GPR:$rs1, 0)>;
  def : Pat<(StoreOp StTy:$rs2, AddrFI:$rs1), (Inst StTy:$rs2, AddrFI:$rs1, 0)>;
  def : Pat<(StoreOp StTy:$rs2, (add GPR:$rs1, simm12:$imm12)),
            (Inst StTy:$rs2, GPR:$rs1, simm12:$imm12)>;
  def : Pat<(StoreOp StTy:$rs2, (add AddrFI:$rs1, simm12:$imm12)),
            (Inst StTy:$rs2, AddrFI:$rs1, simm12:$imm12)>;
  def : Pat<(StoreOp StTy:$rs2, (IsOrAdd AddrFI:$rs1, simm12:$imm12)),
            (Inst StTy:$rs2, AddrFI:$rs1, simm12:$imm12)>;
}

defm : StPat<truncstorei8, SB, GPR>;
defm : StPat<truncstorei16, SH, GPR>;
defm : StPat<store, SW, GPR>, Requires<[IsRV32]>;

/// Fences

// Refer to Table A.6 in the version 2.3 draft of the RISC-V Instruction Set
// Manual: Volume I.

// fence acquire -> fence r, rw
def : Pat<(atomic_fence (XLenVT 4), (timm)), (FENCE 0b10, 0b11)>;
// fence release -> fence rw, w
def : Pat<(atomic_fence (XLenVT 5), (timm)), (FENCE 0b11, 0b1)>;
// fence acq_rel -> fence.tso
def : Pat<(atomic_fence (XLenVT 6), (timm)), (FENCE_TSO)>;
// fence seq_cst -> fence rw, rw
def : Pat<(atomic_fence (XLenVT 7), (timm)), (FENCE 0b11, 0b11)>;

// Lowering for atomic load and store is defined in RISCVInstrInfoA.td.
// Although these are lowered to fence+load/store instructions defined in the
// base RV32I/RV64I ISA, this lowering is only used when the A extension is
// present. This is necessary as it isn't valid to mix __atomic_* libcalls
// with inline atomic operations for the same object.

/// Other pseudo-instructions

// Pessimistically assume the stack pointer will be clobbered
let Defs = [X2], Uses = [X2] in {
def ADJCALLSTACKDOWN : Pseudo<(outs), (ins i32imm:$amt1, i32imm:$amt2),
                              [(callseq_start timm:$amt1, timm:$amt2)]>;
def ADJCALLSTACKUP   : Pseudo<(outs), (ins i32imm:$amt1, i32imm:$amt2),
                              [(callseq_end timm:$amt1, timm:$amt2)]>;
} // Defs = [X2], Uses = [X2]

/// RV64 patterns

let Predicates = [IsRV64] in {

/// sext and zext

def : Pat<(sext_inreg GPR:$rs1, i32), (ADDIW GPR:$rs1, 0)>;
def : Pat<(and GPR:$rs1, 0xffffffff), (SRLI (SLLI GPR:$rs1, 32), 32)>;

/// ALU operations

def : Pat<(sext_inreg (add GPR:$rs1, GPR:$rs2), i32),
          (ADDW GPR:$rs1, GPR:$rs2)>;
def : Pat<(sext_inreg (add GPR:$rs1, simm12:$imm12), i32),
          (ADDIW GPR:$rs1, simm12:$imm12)>;
def : Pat<(sext_inreg (sub GPR:$rs1, GPR:$rs2), i32),
          (SUBW GPR:$rs1, GPR:$rs2)>;
def : Pat<(sext_inreg (shl GPR:$rs1, uimm5:$shamt), i32),
          (SLLIW GPR:$rs1, uimm5:$shamt)>;
// (srl (zexti32 ...), uimm5:$shamt) is matched with custom code due to the
// need to undo manipulation of the mask value performed by DAGCombine.
def : Pat<(sra (sext_inreg GPR:$rs1, i32), uimm5:$shamt),
          (SRAIW GPR:$rs1, uimm5:$shamt)>;

def : PatGprGpr<riscv_sllw, SLLW>;
def : PatGprGpr<riscv_srlw, SRLW>;
def : PatGprGpr<riscv_sraw, SRAW>;

/// Loads

defm : LdPat<sextloadi32, LW>;
defm : LdPat<extloadi32, LW>;
defm : LdPat<zextloadi32, LWU>;
defm : LdPat<load, LD>;

/// Stores

defm : StPat<truncstorei32, SW, GPR>;
defm : StPat<store, SD, GPR>;
} // Predicates = [IsRV64]

/// readcyclecounter
// On RV64, we can directly read the 64-bit "cycle" CSR.
let Predicates = [IsRV64] in
def : Pat<(readcyclecounter), (CSRRS CYCLE.Encoding, X0)>;
// On RV32, ReadCycleWide will be expanded to the suggested loop reading both
// halves of the 64-bit "cycle" CSR.
let Predicates = [IsRV32], usesCustomInserter = 1, hasSideEffects = 0,
mayLoad = 0, mayStore = 0, hasNoSchedulingInfo = 1 in
def ReadCycleWide : Pseudo<(outs GPR:$lo, GPR:$hi), (ins), [], "", "">;

/// traps

// We lower `trap` to `unimp`, as this causes a hard exception on nearly all
// systems.
def : Pat<(trap), (UNIMP)>;

// We lower `debugtrap` to `ebreak`, as this will get the attention of the
// debugger if possible.
def : Pat<(debugtrap), (EBREAK)>;

//===----------------------------------------------------------------------===//
// Standard extensions
//===----------------------------------------------------------------------===//

include "RISCVInstrInfoM.td"
include "RISCVInstrInfoA.td"
include "RISCVInstrInfoF.td"
include "RISCVInstrInfoD.td"
include "RISCVInstrInfoC.td"

// if defined(TENSILICA) || 1

// Shift+add instructions from Zba extension (RISCVInstrInfoB.td)
// We may need a separate feature from it after updating llvm, because
// the HasStdExtZba feature includes other instructions as well.

def mul_oneuse : PatFrag<(ops node:$A, node:$B), (mul node:$A, node:$B), [{
  return N->hasOneUse();
}]>;

// Pattern to exclude simm12 immediates from matching.
def non_imm12 : PatLeaf<(XLenVT GPR:$a), [{
  auto *C = dyn_cast<ConstantSDNode>(N);
  return !C || !isInt<12>(C->getSExtValue());
}]>;

def TrailingZerosXForm : SDNodeXForm<imm, [{
  uint64_t I = N->getZExtValue();
  return CurDAG->getTargetConstant(countTrailingZeros(I), SDLoc(N),
                                   N->getValueType(0));
}]>;

// Return an immediate subtracted from XLen.
def ImmSubFromXLen : SDNodeXForm<imm, [{
  uint64_t XLen = Subtarget->getXLen();
  return CurDAG->getTargetConstant(XLen - N->getZExtValue(), SDLoc(N),
                                   N->getValueType(0));
}]>;

def C3LeftShift : PatLeaf<(imm), [{
  uint64_t C = N->getZExtValue();
  return C > 3 && ((C % 3) == 0) && isPowerOf2_64(C / 3);
}]>;

def C5LeftShift : PatLeaf<(imm), [{
  uint64_t C = N->getZExtValue();
  return C > 5 && ((C % 5) == 0) && isPowerOf2_64(C / 5);
}]>;

def C9LeftShift : PatLeaf<(imm), [{
  uint64_t C = N->getZExtValue();
  return C > 9 && ((C % 9) == 0) && isPowerOf2_64(C / 9);
}]>;

def SH1ADD : ALU_rr<0b0010000, 0b010, "sh1add">, Sched<[]>;
def SH2ADD : ALU_rr<0b0010000, 0b100, "sh2add">, Sched<[]>;
def SH3ADD : ALU_rr<0b0010000, 0b110, "sh3add">, Sched<[]>;

def MIN  : ALU_rr<0b0000101, 0b100, "min">, Sched<[]>;
def MINU : ALU_rr<0b0000101, 0b101, "minu">, Sched<[]>;
def MAX  : ALU_rr<0b0000101, 0b110, "max">, Sched<[]>;
def MAXU : ALU_rr<0b0000101, 0b111, "maxu">, Sched<[]>;

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
class RVBUnary<bits<7> funct7, bits<5> funct5, bits<3> funct3,
               RISCVOpcode opcode, string opcodestr>
    : RVInstR<funct7, funct3, opcode, (outs GPR:$rd), (ins GPR:$rs1),
              opcodestr, "$rd, $rs1"> {
  let rs2 = funct5;
}
let Predicates = [HasSext] in {
// SEXTB, SEXTH
def SEXT_B : RVBUnary<0b0110000, 0b00100, 0b001, RISCVOpcode<0b0010011>,
                     "sext.b">, Sched<[]>;
def SEXT_H : RVBUnary<0b0110000, 0b00101, 0b001, RISCVOpcode<0b0010011>,
                     "sext.h">, Sched<[]>;
}
let Predicates = [HasSext, IsRV32] in {
// ZEXTH_RV32
let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
def ZEXT_H : RVInstR<0b0000100, 0b100, OPC_OP, (outs GPR:$rd),
                         (ins GPR:$rs1), "zext.h", "$rd, $rs1">,
                         Sched<[]> {
  let rs2 = 0b00000;
}
}

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
class RVBShift_ri<bits<5> imm11_7, bits<3> funct3, RISCVOpcode opcode,
                  string opcodestr>
    : RVInstIShift<imm11_7, funct3, opcode, (outs GPR:$rd),
                   (ins GPR:$rs1, uimmlog2xlen:$shamt), opcodestr,
                   "$rd, $rs1, $shamt">;
let Predicates = [HasStdExtZbbOrZbp] in {
def ANDN  : ALU_rr<0b0100000, 0b111, "andn">,
            Sched<[]>;
def ORN   : ALU_rr<0b0100000, 0b110, "orn">,
            Sched<[]>;
def XNOR  : ALU_rr<0b0100000, 0b100, "xnor">,
            Sched<[]>;
def ROL   : ALU_rr<0b0110000, 0b001, "rol">,
            Sched<[]>;
def ROR   : ALU_rr<0b0110000, 0b101, "ror">,
            Sched<[]>;
def RORI  : RVBShift_ri<0b01100, 0b101, OPC_OP_IMM, "rori">,
            Sched<[]>;
def CLZ  : RVBUnary<0b0110000, 0b00000, 0b001, RISCVOpcode<0b0010011>, "clz">,
           Sched<[]>;
def CTZ  : RVBUnary<0b0110000, 0b00001, 0b001, RISCVOpcode<0b0010011>, "ctz">,
           Sched<[]>;
def CPOP : RVBUnary<0b0110000, 0b00010, 0b001, RISCVOpcode<0b0010011>, "cpop">,
           Sched<[]>;
} // Predicates = [HasStdExtZbbOrZbp]

let Predicates = [HasStdExtZbbOrZbp, IsRV32] in {
// REV8_RV32
let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
def REV8 : RVInstI<0b101, OPC_OP_IMM, (outs GPR:$rd), (ins GPR:$rs1),
                        "rev8", "$rd, $rs1">, Sched<[]> {
  let imm12 = { 0b01101, 0b0011000 };
}
} // Predicates = [HasStdExtZbbOrZbp, IsRV32]

let Predicates = [HasStdExtZbbOrZbp] in {
let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
// ORCB
def ORC_B : RVInstI<0b101, OPC_OP_IMM, (outs GPR:$rd), (ins GPR:$rs1),
                   "orc.b", "$rd, $rs1">, Sched<[]> {
  let imm12 = { 0b00101, 0b0000111 };
}
} // Predicates = [HasStdExtZbbOrZbp]


def : Pat<(add (shl GPR:$rs1, (XLenVT 1)), non_imm12:$rs2),
          (SH1ADD GPR:$rs1, GPR:$rs2)>;
def : Pat<(add (shl GPR:$rs1, (XLenVT 2)), non_imm12:$rs2),
          (SH2ADD GPR:$rs1, GPR:$rs2)>;
def : Pat<(add (shl GPR:$rs1, (XLenVT 3)), non_imm12:$rs2),
          (SH3ADD GPR:$rs1, GPR:$rs2)>;

def : Pat<(add (mul_oneuse GPR:$rs1, (XLenVT 6)), GPR:$rs2),
          (SH1ADD (SH1ADD GPR:$rs1, GPR:$rs1), GPR:$rs2)>;
def : Pat<(add (mul_oneuse GPR:$rs1, (XLenVT 10)), GPR:$rs2),
          (SH1ADD (SH2ADD GPR:$rs1, GPR:$rs1), GPR:$rs2)>;
def : Pat<(add (mul_oneuse GPR:$rs1, (XLenVT 18)), GPR:$rs2),
          (SH1ADD (SH3ADD GPR:$rs1, GPR:$rs1), GPR:$rs2)>;
def : Pat<(add (mul_oneuse GPR:$rs1, (XLenVT 12)), GPR:$rs2),
          (SH2ADD (SH1ADD GPR:$rs1, GPR:$rs1), GPR:$rs2)>;
def : Pat<(add (mul_oneuse GPR:$rs1, (XLenVT 20)), GPR:$rs2),
          (SH2ADD (SH2ADD GPR:$rs1, GPR:$rs1), GPR:$rs2)>;
def : Pat<(add (mul_oneuse GPR:$rs1, (XLenVT 36)), GPR:$rs2),
          (SH2ADD (SH3ADD GPR:$rs1, GPR:$rs1), GPR:$rs2)>;
def : Pat<(add (mul_oneuse GPR:$rs1, (XLenVT 24)), GPR:$rs2),
          (SH3ADD (SH1ADD GPR:$rs1, GPR:$rs1), GPR:$rs2)>;
def : Pat<(add (mul_oneuse GPR:$rs1, (XLenVT 40)), GPR:$rs2),
          (SH3ADD (SH2ADD GPR:$rs1, GPR:$rs1), GPR:$rs2)>;
def : Pat<(add (mul_oneuse GPR:$rs1, (XLenVT 72)), GPR:$rs2),
          (SH3ADD (SH3ADD GPR:$rs1, GPR:$rs1), GPR:$rs2)>;

def : Pat<(mul GPR:$r, C3LeftShift:$i),
          (SLLI (SH1ADD GPR:$r, GPR:$r),
                (TrailingZerosXForm C3LeftShift:$i))>;
def : Pat<(mul GPR:$r, C5LeftShift:$i),
          (SLLI (SH2ADD GPR:$r, GPR:$r),
                (TrailingZerosXForm C5LeftShift:$i))>;
def : Pat<(mul GPR:$r, C9LeftShift:$i),
          (SLLI (SH3ADD GPR:$r, GPR:$r),
                (TrailingZerosXForm C9LeftShift:$i))>;

let Predicates = [HasMinMax] in { // HasStdExtZbb min/max
def : PatGprGpr<smin, MIN>;
def : PatGprGpr<smax, MAX>;
def : PatGprGpr<umin, MINU>;
def : PatGprGpr<umax, MAXU>;
} // Predicates = [HasMinMax]

let Predicates = [HasSext] in {
def : Pat<(sext_inreg GPR:$rs1, i8), (SEXT_B GPR:$rs1)>;
def : Pat<(sext_inreg GPR:$rs1, i16), (SEXT_H GPR:$rs1)>;
}
let Predicates = [HasSext, IsRV32] in
def : Pat<(i32 (and GPR:$rs, 0xFFFF)), (ZEXT_H GPR:$rs)>;

let Predicates = [HasStdExtZbbOrZbp] in {
def : Pat<(and GPR:$rs1, (not GPR:$rs2)), (ANDN GPR:$rs1, GPR:$rs2)>;
def : Pat<(or  GPR:$rs1, (not GPR:$rs2)), (ORN  GPR:$rs1, GPR:$rs2)>;
def : Pat<(xor GPR:$rs1, (not GPR:$rs2)), (XNOR GPR:$rs1, GPR:$rs2)>;
} // Predicates = [HasStdExtZbbOrZbp]

let Predicates = [HasStdExtZbbOrZbp] in {
def : PatGprGpr<rotl, ROL>;
def : PatGprGpr<rotr, ROR>;
} // Predicates = [HasStdExtZbbOrZbp]

// There's no encoding for roli in the the 'B' extension as it can be
// implemented with rori by negating the immediate.
let Predicates = [HasStdExtZbbOrZbp] in {
def : PatGprImm<rotr, RORI, uimmlog2xlen>;
def : Pat<(rotl GPR:$rs1, uimmlog2xlen:$shamt),
          (RORI GPR:$rs1, (ImmSubFromXLen uimmlog2xlen:$shamt))>;

// TODO In trunk: combineORToGORC.
// def : Pat<(riscv_gorc GPR:$rs1, 7), (ORC_B GPR:$rs1)>;

// We treat rev8 as a separate instruction, so match it directly.
// In trunk: Pat<(i32 (riscv_grev GPR:$rs1, 24)), (REV8 GPR:$rs1)>;
def : Pat<(bswap GPR:$rs1), (REV8 GPR:$rs1)>;

def : PatGpr<ctlz, CLZ>;
def : PatGpr<cttz, CTZ>;
def : PatGpr<ctpop, CPOP>;
}

// Loop
let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
class Loop_ri<string opcodestr>
      : CoreInstTensilica<(outs),
                (ins GPR:$rs1, bare_symbol:$imm15),
                opcodestr # "\t$rs1, $imm15", []> {
  let isTerminator = 1;
  let isNotDuplicable = 1;
  let Defs = [XTS_CORE_LBEG, XTS_CORE_LEND, XTS_CORE_LCOUNT];
}

def XT_LOOPGTZ : Loop_ri<"xt_loopgtz">;
def XT_LOOPNEZ : Loop_ri<"xt_loopnez">;
def XT_LOOP : Loop_ri<"xt_loop">;

def ENDLOOP:
        CorePseudoInstTensilica<(outs),
                   (ins bare_symbol:$a),
                   "# the end of zero overhead loop",
                   []> {
  let Uses = [XTS_CORE_LBEG, XTS_CORE_LCOUNT, XTS_CORE_LEND];
  let isTerminator = 1;
  let isBranch = 1;
  let isNotDuplicable = 1;
}

let hasSideEffects = 1, mayLoad = 1, mayStore = 1 in {
def LOOP_SET:
        CorePseudoInstTensilica<(outs),
                   (ins GPR:$rS),
                   " # set number of loop iterations",
                   [(int_set_loop_iterations i32:$rS)]>;
def LOOP_DEC:
        CorePseudoInstTensilica<(outs GPR:$dst),
                   (ins GPR:$r1),
                   " # check and decrement number of iterations",
                   [(set i32:$dst,
                    (int_loop_decrement_reg i32:$r1, (i32 1)))]> {
    let isNotDuplicable = 1;
  }
}

def LOAD_CONST : CorePseudoInstTensilica<(outs GPR:$dst), (ins i32imm:$imm),
                                  "# LOAD_CONST $dst,$imm",
                                  []>
{
 let isMoveImm = 1;
 let isReMaterializable = 1;
}

// Conditional moves
let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
class Cmov_rrr<string opcodestr>
      : CoreInstTensilica<(outs GPR:$rd),
                (ins GPR:$rd1, GPR:$rs1, GPR:$rs2),
                opcodestr # "\t$rd, $rs1, $rs2", []> {
  let Constraints = "$rd = $rd1";
  let isCommutable = 1;
  let isSelect = 1;
}

def XT_MOVEQZ : Cmov_rrr<"xt_moveqz">;
def XT_MOVNEZ : Cmov_rrr<"xt_movnez">;
def XT_MOVGEZ : Cmov_rrr<"xt_movgez">;
def XT_MOVLTZ : Cmov_rrr<"xt_movltz">;

// Generics
def MOVE_AR : CorePseudoInstTensilica<(outs GPR:$a),
                                      (ins GPR:$b),
                                      "mv\t$a,$b", []>;

// SELECT_CC_* - Used to implement the SELECT_CC DAG operation.  Expanded after
// instruction selection into a branch sequence.
let usesCustomInserter = 1 in {
  def SELECT_CC : CorePseudoInstTensilica<(outs GPR:$dst),
                              (ins GPR:$cond, GPR:$T, GPR:$F),
                              "# SELECT_CC PSEUDO!",
                              [(set I32:$dst,
                                 (select I32:$cond, I32:$T, I32:$F))]>;

  def SELECT_EQ : CorePseudoInstTensilica<(outs GPR:$dst),
                              (ins GPR:$L, GPR:$R, GPR:$T, GPR:$F),
                              "# SELECT_EQ PSEUDO!",
                              [(set I32:$dst,
                                 (selectcc I32:$L, I32:$R,
                                         I32:$T, I32:$F, SETEQ))]>;

  def SELECT_LT : CorePseudoInstTensilica<(outs GPR:$dst),
                              (ins GPR:$L, GPR:$R, GPR:$T, GPR:$F),
                              "# SELECT_LT PSEUDO!",
                              [(set I32:$dst,
                                 (selectcc I32:$L, I32:$R,
                                         I32:$T, I32:$F, SETLT))]>;

  def SELECT_ULT : CorePseudoInstTensilica<(outs GPR:$dst),
                              (ins GPR:$L, GPR:$R, GPR:$T, GPR:$F),
                              "# SELECT_LTU PSEUDO!",
                              [(set I32:$dst,
                                 (selectcc I32:$L, I32:$R,
                                         I32:$T, I32:$F, SETULT))]>;
}

def XT_BRDEC_P:
        CoreInstTensilica<(outs GPR:$a0),
                          (ins GPR:$a1, simm13_lsb0:$imm12),
                          "xt_brdec.p\t$a0, $imm12",
                          []> {
  let Constraints = "$a0 = $a1";
  let isTerminator = 1;
  let isBranch = 1;
}

// endif

