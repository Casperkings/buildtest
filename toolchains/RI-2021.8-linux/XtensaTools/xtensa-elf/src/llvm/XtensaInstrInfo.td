//===-- XtensaInstrInfo.td - Target Description for Xtensa ---*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file describes the Xtensa instructions in TableGen format.
//
//===----------------------------------------------------------------------===//

// Uses of CP, DP are not currently reflected in the patterns, since
// having a physical register as an operand prevents loop hoisting and
// since the value of these registers never changes during the life of the
// function.

//===----------------------------------------------------------------------===//
// Instruction format superclass.
//===----------------------------------------------------------------------===//

include "XtensaInstrFormats.td"

// Predicates
def HasMul16    : Predicate<"Subtarget->hasMul16()">;
def HasMul32    : Predicate<"Subtarget->hasMul32()">;
def HasMul32h   : Predicate<"Subtarget->hasMul32h()">;
def HardwareF32 : Predicate<"Subtarget->hasHWF32()">;
def HasClamps   : Predicate<"Subtarget->hasClamps()">;
def HasConst16  : Predicate<"Subtarget->hasConst16()">;
def HasDensity  : Predicate<"Subtarget->hasDensity()">;
def HasDepbits  : Predicate<"Subtarget->hasDepbits()">;
def HasDiv32    : Predicate<"Subtarget->hasDiv32()">;
def HasL32r     : Predicate<"Subtarget->hasL32r()">;
def HasMac16    : Predicate<"Subtarget->hasMac16()">;
def HasMinMax   : Predicate<"Subtarget->hasMinMax()">;
def HasNsa      : Predicate<"Subtarget->hasNsa()">;
def HasSalt     : Predicate<"Subtarget->hasSalt()">;
def HasSext     : Predicate<"Subtarget->hasSext()">;
def HasLoop     : Predicate<"Subtarget->hasLoop()">;
def HasSync     : Predicate<"Subtarget->hasSync()">;
def HasS32C1I   : Predicate<"Subtarget->hasS32C1I()">;
def HasBooleans : Predicate<"Subtarget->hasBooleans()">;
def HasExcl     : Predicate<"Subtarget->hasExcl()">;
def IsWindowedABI : Predicate<"Subtarget->isWindowedABI()">;
def IsCall0ABI    : Predicate<"Subtarget->isCall0ABI()">;
def L32rFlix    : Predicate<"Subtarget->L32RFlix()">;
def NoL32rFlix  : Predicate<"!Subtarget->L32RFlix()">;
def IsBrdec     : Predicate<"Subtarget->isBrdec()">;

//===----------------------------------------------------------------------===//
// Xtensa specific DAG Nodes.
//

// Pattern fragment to specify i32 value type in AR register.
def I32 : PatLeaf<(i32 AR:$R)>;

// Call
def SDT_XtensaBranchLink : SDTypeProfile<0, 1, [SDTCisPtrTy<0>]>;
def XtensaBranchLink     : SDNode<"XtensaISD::BL",SDT_XtensaBranchLink,
                            [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue,
                             SDNPVariadic]>;

// This node is required for overlays support and will end up in CALL4 instruction.
def XtensaOvrlOutCall4     : SDNode<"XtensaISD::CALL4_OVRL_OUT",SDT_XtensaBranchLink,
                            [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue,
                             SDNPVariadic]>;

def XtensaRet : SDNode<"XtensaISD::RET", SDTNone,
                      [SDNPHasChain, SDNPOptInGlue, SDNPMayLoad, SDNPVariadic]>;

def SDT_XtensaEhRet : SDTypeProfile<0, 2,
                            [SDTCisSameAs<0, 1>, SDTCisPtrTy<0>]>;
def XtensaEhRet       : SDNode<"XtensaISD::EH_RETURN", SDT_XtensaEhRet,
                         [SDNPHasChain, SDNPOptInGlue]>;

def SDT_XtensaFrameAddr
    : SDTypeProfile<1, 2,
                    [ SDTCisSameAs<0, 1>, SDTCisSameAs<0, 2>, SDTCisPtrTy<0> ]>;
def XtensaFrameAddr       : SDNode<"XtensaISD::FRAMEADDR", SDT_XtensaFrameAddr,
                         []>;

// def SDT_XtensaBR_JT    : SDTypeProfile<0, 2,
//                                       [SDTCisVT<0, i32>, SDTCisVT<1, i32>]>;

// def XtensaBR_JT : SDNode<"XtensaISD::BR_JT", SDT_XtensaBR_JT,
//                         [SDNPHasChain]>;

// def XtensaBR_JT32 : SDNode<"XtensaISD::BR_JT32", SDT_XtensaBR_JT,
//                         [SDNPHasChain]>;

def SDT_XtensaAddress    : SDTypeProfile<1, 1,
                            [SDTCisSameAs<0, 1>, SDTCisPtrTy<0>]>;

def pcrelwrapper : SDNode<"XtensaISD::PCRelativeWrapper", SDT_XtensaAddress,
                           []>;

def frametoargsoffset : SDNode<"XtensaISD::FRAME_TO_ARGS_OFFSET", SDTIntLeaf,
                               []>;

def SDT_XtensaLdwsp    : SDTypeProfile<1, 1, [SDTCisInt<1>]>;
def XtensaLdwsp        : SDNode<"XtensaISD::LDWSP", SDT_XtensaLdwsp,
                               [SDNPHasChain, SDNPMayLoad]>;

// These are target-independent nodes, but have target-specific formats.
def SDT_XtensaCallSeqStart : SDCallSeqStart<[ SDTCisVT<0, i32>, SDTCisVT<1, i32> ]>;
def SDT_XtensaCallSeqEnd   : SDCallSeqEnd<[ SDTCisVT<0, i32>, SDTCisVT<1, i32> ]>;

def callseq_start : SDNode<"ISD::CALLSEQ_START", SDT_XtensaCallSeqStart,
                           [SDNPHasChain, SDNPOutGlue]>;
def callseq_end   : SDNode<"ISD::CALLSEQ_END",   SDT_XtensaCallSeqEnd,
                           [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;

def XtensaAbs : SDNode<"XtensaISD::ABS", SDTIntUnaryOp>;

//===----------------------------------------------------------------------===//
// Instruction Pattern Stuff
//===----------------------------------------------------------------------===//

// An 'or' node with no common bits set
def or_ncb : PatFrag<(ops node:$lhs, node:$rhs), (or node:$lhs, node:$rhs), [{
  return CurDAG->haveNoCommonBitsSet(N->getOperand(0), N->getOperand(1));
}]>;

def immU16 : ImmLeaf<i32, [{
  return isUInt<16>(Imm);
}]>;

def immMOVI : ImmLeaf<i32, [{
  return isInt<12>(Imm);
}]>;

def immADDI : ImmLeaf<i32, [{
  return isInt<8>(Imm);
}]>;

def immADDMI : ImmLeaf<i32, [{
  return isShiftedInt<8,8>(Imm);
}]>;

def immL8UI : ImmLeaf<i32, [{
  return isUInt<8>(Imm);
}]>;

def immL16UI : ImmLeaf<i32, [{
  return isShiftedUInt<8,1>(Imm);
}]>;

def immL32I : ImmLeaf<i32, [{
  return isShiftedUInt<8,2>(Imm);
}]>;

// SRL accepts 1 through 15 as the shift count.
// EXTUI handles logical right shifts by 16 bits or more.
def immSRLI : ImmLeaf<i32, [{
  return 1 <= Imm && Imm <= 15;
}]>;

// SLLI and SRAI accept 1 through 31 as the shift count.
def immSLLI : ImmLeaf<i32, [{
  return 1 <= Imm && Imm <= 31;
}]>;

// EXTUI can be used to shift right by 16 to 31 bits.
def immEXTUIShift : ImmLeaf<i32, [{
  return 16 <= Imm && Imm <= 31;
}]>;

def imm16_24 : ImmLeaf<i32, [{
  return 16 <= Imm && Imm <= 24;
}]>;

// EXTUI can be used to AND with 0x0001 through 0xffff.
def immEXTUIMask : ImmLeaf<i32, [{
  return isMask_32(Imm) && Imm <= 0xffff;
}]>;

def immSEXTShift : ImmLeaf<i32, [{
  return 9 <= Imm && Imm <= 24;
}]>;

def immB4CONST : ImmLeaf<i32, [{
  return (Imm == -1 ||
	  (1 <= Imm && Imm <= 8) ||
	  Imm == 10 ||
	  Imm == 12 ||
	  Imm == 16 ||
	  Imm == 32 ||
	  Imm == 64 ||
	  Imm == 128 ||
	  Imm == 256);
}]>;

def immB4CONSTMinus1 : ImmLeaf<i32, [{
  return (Imm == -2 ||
	  (0 <= Imm && Imm <= 7) ||
	  Imm == 9 ||
	  Imm == 11 ||
	  Imm == 15 ||
	  Imm == 31 ||
	  Imm == 63 ||
	  Imm == 127 ||
	  Imm == 255);
}]>;

def immB4CONSTU : ImmLeaf<i32, [{
  return ((2 <= Imm && Imm <= 8) ||
	  Imm == 10 ||
	  Imm == 12 ||
	  Imm == 16 ||
	  Imm == 32 ||
	  Imm == 64 ||
	  Imm == 128 ||
	  Imm == 256 ||
	  Imm == 32768 ||
	  Imm == 65536);
}]>;

def immB4CONSTUMinus1 : ImmLeaf<i32, [{
  return ((1 <= Imm && Imm <= 7) ||
	  Imm == 9 ||
	  Imm == 11 ||
	  Imm == 15 ||
	  Imm == 31 ||
	  Imm == 63 ||
	  Imm == 127 ||
	  Imm == 255 ||
	  Imm == 32767 ||
	  Imm == 65535);
}]>;

def immPowerOf2 : ImmLeaf<i32, [{
  return isPowerOf2_32(Imm);
}]>;

// Instruction operand types
def pcrel_imm  : Operand<i32>;
def pcrel_imm_neg  : Operand<i32> {
  let DecoderMethod = "DecodeNegImmOperand";
}
def brtarget : Operand<OtherVT>;

// Addressing modes
def ADDRspii : ComplexPattern<i32, 2, "SelectADDRspii", [add, or, frameindex], []>;
def addr8 : ComplexPattern<i32, 2, "SelectAddr8", [add], []>;
def addr16 : ComplexPattern<i32, 2, "SelectAddr16", [add], []>;
def addr32 : ComplexPattern<i32, 2, "SelectAddr32", [add], []>;
def addr32fia : ComplexPattern<i32, 3, "SelectAddrFIA", [add, or, frameindex], []>;

// Address operands
def MEMii : Operand<i32> {
  let MIOperandInfo = (ops i32imm, i32imm);
}

//===----------------------------------------------------------------------===//
// Core Instructions
//===----------------------------------------------------------------------===//

include "XtensaInstrCore.td"


//===----------------------------------------------------------------------===//
// Pseudo Instructions
//===----------------------------------------------------------------------===//

// BBC_BE is simply BBC on big endian config, but as the semantic is different,
// it is named differently. Similarly for BBS_BE.
// Also note that BBCI_BE does not need to exist, as BBCI_LE will be printed
// as BBCI_BE at MCInst level.

def BBC_BE:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "bbc\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BBC_BE_GENERIC:
  CorePseudoInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BBC_BE_W15:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "bbc.w15\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BBC_BE_W18:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "bbc.w18\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BBS_BE:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "bbs\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BBS_BE_GENERIC:
  CorePseudoInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BBS_BE_W15:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "bbs.w15\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BBS_BE_W18:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "bbs.w18\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

let hasSideEffects = 1 in {
def LOOP_SET:
        CorePseudoInstXtensa<(outs),
                   (ins AR:$rS),
                   " # set number of loop iterations",
                   [(int_set_loop_iterations i32:$rS)]>;
def LOOP_DEC:
        CorePseudoInstXtensa<(outs AR:$dst),
                   (ins AR:$r1),
                   " # check and decrement number of iterations",
                   [(set i32:$dst,
                    (int_loop_decrement_reg i32:$r1, (i32 1)))]> {
    let isNotDuplicable = 1;
  }
}

let Defs = [SP, a8], Uses = [SP] in {
def ADJCALLSTACKDOWN : CorePseudoInstXtensa<(outs), (ins i32imm:$amt1, i32imm:$amt2),
                               "# ADJCALLSTACKDOWN $amt1, $amt2",
                               [(callseq_start timm:$amt1, timm:$amt2)]>;
def ADJCALLSTACKUP : CorePseudoInstXtensa<(outs), (ins i32imm:$amt1, i32imm:$amt2),
                            "# ADJCALLSTACKUP $amt1",
                            [(callseq_end timm:$amt1, timm:$amt2)]>;
}

let isReMaterializable = 1 in
def FRAME_TO_ARGS_OFFSET : CorePseudoInstXtensa<(outs AR:$dst), (ins),
                               "# FRAME_TO_ARGS_OFFSET $dst",
                               [(set AR:$dst, (i32 frametoargsoffset))]>;

let isReturn = 1, isTerminator = 1, isBarrier = 1 in
def EH_RETURN : CorePseudoInstXtensa<(outs), (ins AR:$s, AR:$handler),
                               "# EH_RETURN $s, $handler",
                               [(XtensaEhRet AR:$s, AR:$handler)]>;

def FRAMEADDR : CorePseudoInstXtensa<(outs AR:$dst), (ins AR:$level, AR:$sp),
                               "",
                               [(set AR:$dst, (XtensaFrameAddr AR:$level, AR:$sp))]> {
  let usesCustomInserter = 1;
}

// We use LDAWFI to compute addresses involving FrameIndex for local
// variables on the stack and spill objects.  For incoming arguments
// on the stack, we use LDAWFIA, defined immediately below.
def LDAWFI : CorePseudoInstXtensa<(outs AR:$dst), (ins MEMii:$addr),
                             "# LDAWFI $dst, $addr",
                             [(set AR:$dst, ADDRspii:$addr)]>;

// LDAWFIA is just like LDAWFI except that it is used to compute
// addresses involving FrameIndex for incoming arguments.  Since we
// may need to access incoming arguments on the stack using the
// original stack pointer, we keep $osp as one of the inputs.  This
// way, the register allocator will allocate a register for us.
//
// One downside with this design is that we allocate a physical
// register for $osp, even when we do not need the original stack
// pointer.  Note that when we do not require a wide stack alignment,
// we can access incoming arguments on the stack using FP or SP as the
// base pointer.
def LDAWFIA : CorePseudoInstXtensa<(outs AR:$dst),
                               (ins AR:$osp, AR:$base, i32imm:$offset),
                               "# LDAWFIA $dst,$osp,$base,$offset",
                               [(set I32:$dst,
                                 (addr32fia I32:$osp, I32:$base, i32imm:$offset))]>;

// SELECT_CC_* - Used to implement the SELECT_CC DAG operation.  Expanded after
// instruction selection into a branch sequence.
let usesCustomInserter = 1 in {
  def SELECT_CC : CorePseudoInstXtensa<(outs AR:$dst),
                              (ins AR:$cond, AR:$T, AR:$F),
                              "# SELECT_CC PSEUDO!",
                              [(set I32:$dst,
                                 (select I32:$cond, I32:$T, I32:$F))]>;

  def SELECT_EQ : CorePseudoInstXtensa<(outs AR:$dst),
                              (ins AR:$L, AR:$R, AR:$T, AR:$F),
                              "# SELECT_EQ PSEUDO!",
                              [(set I32:$dst,
                                 (selectcc I32:$L, I32:$R,
                                         I32:$T, I32:$F, SETEQ))]>;

  def SELECT_LT : CorePseudoInstXtensa<(outs AR:$dst),
                              (ins AR:$L, AR:$R, AR:$T, AR:$F),
                              "# SELECT_LT PSEUDO!",
                              [(set I32:$dst,
                                 (selectcc I32:$L, I32:$R,
                                         I32:$T, I32:$F, SETLT))]>;

  def SELECT_ULT : CorePseudoInstXtensa<(outs AR:$dst),
                              (ins AR:$L, AR:$R, AR:$T, AR:$F),
                              "# SELECT_LTU PSEUDO!",
                              [(set I32:$dst,
                                 (selectcc I32:$L, I32:$R,
                                         I32:$T, I32:$F, SETULT))]>;
}

// Generics 
def MOVE_AR : CorePseudoInstXtensa<(outs AR:$a), (ins AR:$b), "mov.n\t$a,$b", []>;

def L32R_NOFLIX:
        CoreInstXtensa<(outs AR:$a),
                   (ins pcrel_imm:$b),
                   "l32r\t$a,$b",
                   []>,
        Requires<[HasL32r]>
{
  let mayLoad = 1;
  let isReMaterializable = 1;
}

//===----------------------------------------------------------------------===//
// Instructions to Load Immediates
//===----------------------------------------------------------------------===//

def : Pat<(immMOVI:$b), (MOVI immMOVI:$b)>;

def LOAD_CONST : CorePseudoInstXtensa<(outs AR:$dst), (ins i32imm:$imm),
                                  "# LOAD_CONST $dst,$imm",
                                  []>
{
 let isMoveImm = 1;
 let isReMaterializable = 1;
}

//===----------------------------------------------------------------------===//
// Arithmetic Instructions
//===----------------------------------------------------------------------===//

def : Pat<(add AR:$b, (and AR:$c, (i32 -2))),
          (ADDX2 (SRLI AR:$c, (i32 1)), AR:$b)>;
def : Pat<(add AR:$b, (and AR:$c, (i32 -4))),
          (ADDX4 (SRLI AR:$c, (i32 2)), AR:$b)>;
def : Pat<(add AR:$b, (and AR:$c, (i32 -8))),
          (ADDX8 (SRLI AR:$c, (i32 3)), AR:$b)>;

def : Pat<(or_ncb AR:$b, (and AR:$c, (i32 -2))),
          (ADDX2 (SRLI AR:$c, (i32 1)), AR:$b)>;
def : Pat<(or_ncb AR:$b, (and AR:$c, (i32 -4))),
          (ADDX4 (SRLI AR:$c, (i32 2)), AR:$b)>;
def : Pat<(or_ncb AR:$b, (and AR:$c, (i32 -8))),
          (ADDX8 (SRLI AR:$c, (i32 3)), AR:$b)>;

def : Pat<(sub (and AR:$b, (i32 -2)), AR:$c),
          (SUBX2 (SRLI AR:$b, (i32 1)), AR:$c)>;
def : Pat<(sub (and AR:$b, (i32 -4)), AR:$c),
          (SUBX4 (SRLI AR:$b, (i32 2)), AR:$c)>;
def : Pat<(sub (and AR:$b, (i32 -8)), AR:$c),
          (SUBX8 (SRLI AR:$b, (i32 3)), AR:$c)>;

def : Pat<(add I32:$b, I32:$c), (ADD AR:$b, AR:$c)>;

// Prefer ADD to OR so that we can later convert it to ADD.N.
def : Pat<(or_ncb I32:$b, I32:$c), (ADD AR:$b, AR:$c)>;

def : Pat<(add (shl I32:$b, (i32 1)), AR:$c), (ADDX2 AR:$b, AR:$c)>;
def : Pat<(add (shl I32:$b, (i32 2)), AR:$c), (ADDX4 AR:$b, AR:$c)>;
def : Pat<(add (shl I32:$b, (i32 3)), AR:$c), (ADDX8 AR:$b, AR:$c)>;

def : Pat<(or_ncb (shl I32:$b, (i32 1)), AR:$c), (ADDX2 AR:$b, AR:$c)>;
def : Pat<(or_ncb (shl I32:$b, (i32 2)), AR:$c), (ADDX4 AR:$b, AR:$c)>;
def : Pat<(or_ncb (shl I32:$b, (i32 3)), AR:$c), (ADDX8 AR:$b, AR:$c)>;

def : Pat<(add I32:$b, immADDI:$c), (ADDI AR:$b, immADDI:$c)>;
def : Pat<(or_ncb AR:$b, immADDI:$c), (ADDI AR:$b, immADDI:$c)>;

def : Pat<(add AR:$b, immADDMI:$c), (ADDMI AR:$b, immADDMI:$c)>;
def : Pat<(or_ncb AR:$b, immADDMI:$c), (ADDMI AR:$b, immADDMI:$c)>;

def : Pat<(sub I32:$b, AR:$c), (SUB AR:$b, AR:$c)>;

def : Pat<(sub (shl I32:$b, (i32 1)), AR:$c), (SUBX2 AR:$b, AR:$c)>;
def : Pat<(sub (shl I32:$b, (i32 2)), AR:$c), (SUBX4 AR:$b, AR:$c)>;
def : Pat<(sub (shl I32:$b, (i32 3)), AR:$c), (SUBX8 AR:$b, AR:$c)>;

def : Pat<(ineg I32:$b), (NEG I32:$b)>;

def : Pat<(mul I32:$b, I32:$c), (MULL AR:$b, AR:$c)>, Requires<[HasMul32]>;

def : Pat<(mul I32:$b, (i32 3)), (ADDX2 AR:$b, AR:$b)>;
def : Pat<(mul I32:$b, (i32 5)), (ADDX4 AR:$b, AR:$b)>;
def : Pat<(mul I32:$b, (i32 7)), (SUBX8 AR:$b, AR:$b)>;
def : Pat<(mul I32:$b, (i32 9)), (ADDX8 AR:$b, AR:$b)>;

def : Pat<(mulhu I32:$b, I32:$c), (MULUH AR:$b, AR:$c)>, Requires<[HasMul32h]>;
def : Pat<(mulhs I32:$b, I32:$c), (MULSH AR:$b, AR:$c)>, Requires<[HasMul32h]>;

def : Pat<(sdiv I32:$b, I32:$c), (QUOS AR:$b, AR:$c)>, Requires<[HasDiv32]>;
def : Pat<(srem I32:$b, I32:$c), (REMS AR:$b, AR:$c)>, Requires<[HasDiv32]>;
def : Pat<(udiv I32:$b, I32:$c), (QUOU AR:$b, AR:$c)>, Requires<[HasDiv32]>;
def : Pat<(urem I32:$b, I32:$c), (REMU AR:$b, AR:$c)>, Requires<[HasDiv32]>;

def : Pat<(XtensaAbs I32:$b), (ABS AR:$b)>;

//===----------------------------------------------------------------------===//
// Bitwise Logical Instructions
//===----------------------------------------------------------------------===//

def : Pat<(and I32:$b, (xor I32:$c, (i32 -1))),
          (XOR AR:$b, (AND AR:$b, AR:$c))>;

def : Pat<(and I32:$b, I32:$c), (AND AR:$b, AR:$c)>;
def : Pat<(or  I32:$b, I32:$c), (OR  AR:$b, AR:$c)>;
def : Pat<(xor I32:$b, I32:$c), (XOR AR:$b, AR:$c)>;

// Compute the second immediate in the EXTUI instruction.
def extuiMask_xform : SDNodeXForm<imm, [{
  uint32_t value = N->getZExtValue();
  return getI32Imm(countTrailingOnes(value), SDLoc(N));
}]>;

def : Pat<(and I32:$b, immEXTUIMask:$c),
          (EXTUI AR:$b, (i32 0), (extuiMask_xform immEXTUIMask:$c))>;

//===----------------------------------------------------------------------===//
// Shift Instructions
//===----------------------------------------------------------------------===//

def : Pat<(shl I32:$b, immSLLI:$c), (SLLI AR:$b, immSLLI:$c)>;

def : Pat<(srl I32:$b, immSRLI:$c), (SRLI AR:$b, immSRLI:$c)>;

// Compute the second immediate in the EXTUI instruction.
def extuiShift_xform : SDNodeXForm<imm, [{
  // Transformation function: 32 - imm
  uint32_t value = N->getZExtValue();
  return getI32Imm(32 - value, SDLoc(N));
}]>;

def : Pat<(srl I32:$src, immEXTUIShift:$imm),
          (EXTUI AR:$src, immEXTUIShift:$imm,
                 (extuiShift_xform immEXTUIShift:$imm))>;

def : Pat<(sra I32:$b, immSLLI:$c), (SRAI AR:$b, immSLLI:$c)>;

//===----------------------------------------------------------------------===//
// Instructions for Miscellaneous Operations
//===----------------------------------------------------------------------===//

def : Pat<(smax I32:$a, AR:$b), (MAX  AR:$a, AR:$b)>, Requires<[HasMinMax]>;
def : Pat<(smin I32:$a, AR:$b), (MIN  AR:$a, AR:$b)>, Requires<[HasMinMax]>;
def : Pat<(umax I32:$a, AR:$b), (MAXU AR:$a, AR:$b)>, Requires<[HasMinMax]>;
def : Pat<(umin I32:$a, AR:$b), (MINU AR:$a, AR:$b)>, Requires<[HasMinMax]>;

def : Pat<(i32 (ctlz I32:$src)), (NSAU AR:$src)>, Requires<[HasNsa]>;

/// sext_inreg
def : Pat<(sext_inreg I32:$b, i8),  (SEXT AR:$b,  7)>, Requires<[HasSext]>;
def : Pat<(sext_inreg I32:$b, i16), (SEXT AR:$b, 15)>, Requires<[HasSext]>;

// Compute the second immediate in the EXTUI instruction.
def sextShift_xform : SDNodeXForm<imm, [{
  // Transformation function: 31 - imm
  uint32_t value = N->getZExtValue();
  return getI32Imm(31 - value, SDLoc(N));
}]>;

def : Pat<(sra (shl I32:$b, immSEXTShift:$c), immSEXTShift:$c),
          (SEXT AR:$b, (sextShift_xform immSEXTShift:$c))>,
      Requires<[HasSext]>;

/// trap
def : Pat<(trap), (ILL)>;

//===----------------------------------------------------------------------===//
// Load/Store Instructions
//===----------------------------------------------------------------------===//

/// loads
def : Pat<(i32 (zextloadi8 (addr8 I32:$base, i32imm:$offset))),
          (i32 (L8UI I32:$base, i32imm:$offset))>;

def : Pat<(i32 (sextloadi16 (addr16 I32:$base, i32imm:$offset))),
          (i32 (L16SI I32:$base, i32imm:$offset))>;

def : Pat<(i32 (zextloadi16 (addr16 I32:$base, i32imm:$offset))),
          (i32 (L16UI I32:$base, i32imm:$offset))>;

def : Pat<(i32 (load (addr32 I32:$base, i32imm:$offset))),
          (i32 (L32I I32:$base, i32imm:$offset))>;

// Used to access incoming arguments on the stack.  $osp holds the
// original stack pointer.
// let mayLoad = 1 in
def L32I_FIA: CorePseudoInstXtensa<(outs AR:$a),
                               (ins AR:$osp, AR:$b, i32imm:$c),
                               "# L32I_FIA\t$a,$osp,$b,$c",
                               [(set I32:$a,
                                 (load (addr32fia I32:$osp, I32:$b, i32imm:$c)))]>;

/// anyext
def : Pat<(i32 (extloadi8 (addr8 I32:$base, i32imm:$offset))),
          (i32 (L8UI AR:$base, i32imm:$offset))>;

def : Pat<(i32 (extloadi16 (addr16 I32:$base, i32imm:$offset))),
          (i32 (L16SI I32:$base, i32imm:$offset))>;

/// stores
def : Pat<(truncstorei8 I32:$val, (addr8 I32:$base, i32imm:$offset)),
          (S8I I32:$val, I32:$base, i32imm:$offset)>;

def : Pat<(truncstorei8 (srl I32:$val, imm16_24:$imm),
                        (addr8 I32:$base, i32imm:$offset)),
          (S8I (SRAI I32:$val, imm16_24:$imm),
               I32:$base, i32imm:$offset)>;

def : Pat<(truncstorei16 (srl I32:$val, (i32 16)),
                         (addr8 I32:$base, i32imm:$offset)),
          (S16I (SRAI I32:$val, (i32 16)),
                I32:$base, i32imm:$offset)>;

def : Pat<(truncstorei16 I32:$val, (addr16 I32:$base, i32imm:$offset)),
          (S16I I32:$val, I32:$base, i32imm:$offset)>;

def : Pat<(store I32:$val, (addr32 I32:$base, i32imm:$offset)),
          (S32I I32:$val, I32:$base, i32imm:$offset)>;

def : Pat<(i32 (load (pcrelwrapper tconstpool:$b))),
          (i32 (L32R pcrel_imm:$b))>, Requires<[HasL32r, L32rFlix]>;

def : Pat<(i32 (load (pcrelwrapper tconstpool:$b))),
          (i32 (L32R_NOFLIX pcrel_imm:$b))>, Requires<[HasL32r, NoL32rFlix]>;

let Uses = [SP] in {
let mayLoad=1 in {
def L32I_SP : CoreInstXtensa<(outs AR:$a), (ins i32imm:$b),
                        "l32i\t$a,a1,$b",
                        [(set I32:$a, (XtensaLdwsp immU16:$b))]>;
}
}

def MOVI_ADDR: CorePseudoInstXtensa<(outs AR:$a),
                                (ins pcrel_imm:$b),
                                "movi\t$a,$b",
                                []>, Requires<[HasL32r]> {
  let mayLoad = 1;
  let isReMaterializable = 1;
}

//===----------------------------------------------------------------------===//
// Branch Instructions
//===----------------------------------------------------------------------===//

def ENDLOOP:
        CorePseudoInstXtensa<(outs),
                   (ins brtarget:$a),
                   "# the end of zero overhead loop",
                   []>
{
  let Uses = [XTS_CORE_LBEG, XTS_CORE_LCOUNT, XTS_CORE_LEND];
  let isTerminator = 1;
  let isBranch = 1;
  let isNotDuplicable = 1;
}

def : Pat<(brind AR:$a), (JX AR:$a)>;

///
/// branch patterns
///

// unconditional branch
def : Pat<(br bb:$addr), (J bb:$addr)>;

// equality comparisons

def : Pat<(brcond (i32 (seteq i32:$lhs, 0)), bb:$dst),
          (BEQZ AR:$lhs, bb:$dst)>;
def : Pat<(brcond (i32 (setne i32:$lhs, 0)), bb:$dst),
          (BNEZ AR:$lhs, bb:$dst)>;

def : Pat<(brcond (i32 (seteq i32:$lhs, immB4CONST:$rhs)), bb:$dst),
          (BEQI AR:$lhs, immB4CONST:$rhs, bb:$dst)>;
def : Pat<(brcond (i32 (setne i32:$lhs, immB4CONST:$rhs)), bb:$dst),
          (BNEI AR:$lhs, immB4CONST:$rhs, bb:$dst)>;

def : Pat<(brcond (i32 (seteq I32:$lhs, I32:$rhs)), bb:$dst),
          (BEQ I32:$lhs, I32:$rhs, bb:$dst)>;
def : Pat<(brcond (i32 (setne I32:$lhs, I32:$rhs)), bb:$dst),
          (BNE I32:$lhs, I32:$rhs, bb:$dst)>;

// signed comparisons

def : Pat<(brcond (i32 (setlt i32:$lhs, (i32 0))), bb:$dst),
          (BLTZ AR:$lhs, bb:$dst)>;
def : Pat<(brcond (i32 (setgt i32:$lhs, (i32 -1))), bb:$dst),
          (BGEZ AR:$lhs, bb:$dst)>;

// Compute imm + 1.
def immPlus1_xform : SDNodeXForm<imm, [{
  // Transformation function: imm + 1
  int64_t value = N->getSExtValue();
  return getI32Imm(value + 1, SDLoc(N));
}]>;

def : Pat<(brcond (i32 (setlt AR:$lhs, immB4CONST:$rhs)), bb:$dst),
          (BLTI AR:$lhs, immB4CONST:$rhs, bb:$dst)>;
def : Pat<(brcond (i32 (setgt AR:$lhs, immB4CONSTMinus1:$rhs)), bb:$dst),
          (BGEI AR:$lhs, (immPlus1_xform immB4CONSTMinus1:$rhs), bb:$dst)>;

def : Pat<(brcond (i32 (setlt I32:$lhs, I32:$rhs)), bb:$dst),
          (BLT I32:$lhs, I32:$rhs, bb:$dst)>;
def : Pat<(brcond (i32 (setge I32:$lhs, I32:$rhs)), bb:$dst),
          (BGE I32:$lhs, I32:$rhs, bb:$dst)>;

def : Pat<(brcond (i32 (setgt I32:$lhs, I32:$rhs)), bb:$dst),
          (BLT I32:$rhs, I32:$lhs, bb:$dst)>;
def : Pat<(brcond (i32 (setle I32:$lhs, I32:$rhs)), bb:$dst),
          (BGE I32:$rhs, I32:$lhs, bb:$dst)>;

// unsigned comparisons

def : Pat<(brcond (i32 (setult AR:$lhs, immB4CONSTU:$rhs)), bb:$dst),
          (BLTUI AR:$lhs, immB4CONSTU:$rhs, bb:$dst)>;
def : Pat<(brcond (i32 (setugt AR:$lhs, immB4CONSTUMinus1:$rhs)), bb:$dst),
          (BGEUI AR:$lhs, (immPlus1_xform immB4CONSTUMinus1:$rhs), bb:$dst)>;

def : Pat<(brcond (i32 (setult I32:$lhs, I32:$rhs)), bb:$dst),
          (BLTU I32:$lhs, I32:$rhs, bb:$dst)>;
def : Pat<(brcond (i32 (setuge I32:$lhs, I32:$rhs)), bb:$dst),
          (BGEU I32:$lhs, I32:$rhs, bb:$dst)>;

def : Pat<(brcond (i32 (setugt I32:$lhs, I32:$rhs)), bb:$dst),
          (BLTU I32:$rhs, I32:$lhs, bb:$dst)>;
def : Pat<(brcond (i32 (setule I32:$lhs, I32:$rhs)), bb:$dst),
          (BGEU I32:$rhs, I32:$lhs, bb:$dst)>;

// bnone/bany

def : Pat<(brcond (i32 (seteq (and AR:$lhs, AR:$rhs), (i32 0))), bb:$dst),
          (BNONE AR:$rhs, AR:$lhs, bb:$dst)>;
def : Pat<(brcond (i32 (setne (and AR:$lhs, AR:$rhs), (i32 0))), bb:$dst),
          (BANY AR:$rhs, AR:$lhs, bb:$dst)>;

// ball/bnall

def : Pat<(brcond (i32 (seteq (and (xor AR:$lhs, (i32 -1)), AR:$rhs), (i32 0))),
           bb:$dst),
          (BALL AR:$lhs, AR:$rhs, bb:$dst)>;
def : Pat<(brcond (i32 (setne (and (xor AR:$lhs, (i32 -1)), AR:$rhs), (i32 0))),
           bb:$dst),
          (BNALL AR:$lhs, AR:$rhs, bb:$dst)>;

def : Pat<(brcond (i32 (seteq (and I32:$lhs, I32:$rhs), I32:$rhs)), bb:$dst),
          (BALL AR:$lhs, AR:$rhs, bb:$dst)>;
def : Pat<(brcond (i32 (setne (and I32:$lhs, I32:$rhs), I32:$rhs)), bb:$dst),
          (BNALL AR:$lhs, AR:$rhs, bb:$dst)>;

// bbc/bbs

def : Pat<(brcond (i32 (seteq (and I32:$lhs, (shl (i32 1), I32:$rhs)), 
                              (i32 0))), bb:$dst),
          (BBC AR:$lhs, AR:$rhs, bb:$dst)>;
def : Pat<(brcond (i32 (setne (and I32:$lhs, (shl (i32 1), I32:$rhs)), 
                              (i32 0))), bb:$dst),
          (BBS AR:$lhs, AR:$rhs, bb:$dst)>;
def : Pat<(brcond (i32 (seteq (and (srl I32:$lhs, I32:$rhs), (i32 1)),
                              (i32 0))), bb:$dst),
          (BBC AR:$lhs, AR:$rhs, bb:$dst)>;
def : Pat<(brcond (i32 (and (srl I32:$lhs, I32:$rhs), (i32 1))), bb:$dst),
          (BBS AR:$lhs, AR:$rhs, bb:$dst)>;

// bbci/bbsi

// Compute log2(imm).
def immLog2_xform : SDNodeXForm<imm, [{
  // Transformation function: log2(imm)
  return getI32Imm(Log2_32(N->getZExtValue()), SDLoc(N));
}]>;

def : Pat<(brcond (i32 (seteq (and AR:$lhs, immPowerOf2:$rhs), (i32 0))), 
           bb:$dst),
          (BBCI AR:$lhs, (immLog2_xform immPowerOf2:$rhs), bb:$dst)>;
def : Pat<(brcond (i32 (setne (and AR:$lhs, immPowerOf2:$rhs), (i32 0))), 
           bb:$dst),
          (BBSI AR:$lhs, (immLog2_xform immPowerOf2:$rhs), bb:$dst)>;

def : Pat<(brcond (and AR:$lhs, (i32 1)), bb:$dst),
          (BBSI AR:$lhs, 0, bb:$dst)>;

// generic brcond pattern
def : Pat<(brcond I32:$cond, bb:$addr), (BNEZ AR:$cond, bb:$addr)>;

//===----------------------------------------------------------------------===//
// Setcc Instructions
//===----------------------------------------------------------------------===//

def : Pat<(i32 (seteq AR:$lhs, (i32 0))),
          (SALTU AR:$lhs, (MOVI (i32 1)))>,
      Requires<[HasSalt]>;
def : Pat<(i32 (setne I32:$lhs, (i32 0))),
          (SALTU (MOVI (i32 0)), AR:$lhs)>,
      Requires<[HasSalt]>;

def : Pat<(i32 (seteq I32:$lhs, I32:$rhs)),
          (SALTU (SUB I32:$lhs, I32:$rhs), (MOVI (i32 1)))>,
      Requires<[HasSalt]>;
def : Pat<(i32 (setne I32:$lhs, I32:$rhs)),
          (SALTU (MOVI (i32 0)), (SUB I32:$lhs, I32:$rhs))>, // ???
      Requires<[HasSalt]>;

def : Pat<(i32 (setlt I32:$lhs, I32:$rhs)),
          (SALT  I32:$lhs, I32:$rhs)>,
      Requires<[HasSalt]>;
def : Pat<(i32 (setult I32:$lhs, I32:$rhs)),
          (SALTU  I32:$lhs, I32:$rhs)>,
      Requires<[HasSalt]>;

def : Pat<(i32 (setgt I32:$lhs, I32:$rhs)),
          (SALT  I32:$rhs, I32:$lhs)>,
      Requires<[HasSalt]>;
def : Pat<(i32 (setugt I32:$lhs, I32:$rhs)),
          (SALTU  I32:$rhs, I32:$lhs)>,
      Requires<[HasSalt]>;

def : Pat<(i32 (setle      I32:$lhs, I32:$rhs)),
          (XOR (SALT  I32:$rhs, I32:$lhs), (MOVI 1))>,
      Requires<[HasSalt]>;
def : Pat<(i32 (setule     I32:$lhs, I32:$rhs)),
          (XOR (SALTU I32:$rhs, I32:$lhs), (MOVI 1))>,
      Requires<[HasSalt]>;

def : Pat<(i32 (setge      I32:$lhs, I32:$rhs)),
          (XOR (SALT  I32:$lhs, I32:$rhs), (MOVI 1))>,
      Requires<[HasSalt]>;
def : Pat<(i32 (setuge     I32:$lhs, I32:$rhs)),
          (XOR (SALTU I32:$lhs, I32:$rhs), (MOVI 1))>,
      Requires<[HasSalt]>;

def : Pat<(i32 (setle      I32:$lhs, I32:$rhs)),
          (SELECT_LT  I32:$rhs, I32:$lhs, (i32 0), (i32 1))>;
def : Pat<(i32 (setule     I32:$lhs, I32:$rhs)),
          (SELECT_ULT I32:$rhs, I32:$lhs, (i32 0), (i32 1))>;

def : Pat<(i32 (setgt      I32:$lhs, I32:$rhs)),
          (SELECT_LT  I32:$rhs, I32:$lhs, (i32 1), (i32 0))>;
def : Pat<(i32 (setugt     I32:$lhs, I32:$rhs)),
          (SELECT_ULT I32:$rhs, I32:$lhs, (i32 1), (i32 0))>;

def : Pat<(i32 (setge      I32:$lhs, I32:$rhs)),
          (SELECT_LT  I32:$lhs, I32:$rhs, (i32 0), (i32 1))>;
def : Pat<(i32 (setuge     I32:$lhs, I32:$rhs)),
          (SELECT_ULT I32:$lhs, I32:$rhs, (i32 0), (i32 1))>;

def : Pat<(i32 (setlt      I32:$lhs, I32:$rhs)),
          (SELECT_LT  I32:$lhs, I32:$rhs, (i32 1), (i32 0))>;
def : Pat<(i32 (setult     I32:$lhs, I32:$rhs)),
          (SELECT_ULT I32:$lhs, I32:$rhs, (i32 1), (i32 0))>;

def : Pat<(i32 (setne     I32:$lhs, I32:$rhs)),
          (SELECT_EQ I32:$lhs, I32:$rhs, (i32 0), (i32 1))>;

def : Pat<(i32 (seteq     I32:$lhs, I32:$rhs)),
          (SELECT_EQ I32:$lhs, I32:$rhs, (i32 1), (i32 0))>;

//===----------------------------------------------------------------------===//
// Select Instructions
//===----------------------------------------------------------------------===//

def : Pat<(selectcc   I32:$lhs, I32:$rhs, I32:$T, I32:$F, SETNE),
          (SELECT_EQ  I32:$lhs, I32:$rhs, I32:$F, I32:$T)>;

// def : Pat<(selectcc   I32:$lhs, I32:$rhs, I32:$T, I32:$F, SETLT),
//           (SELECT_LT  I32:$lhs, I32:$rhs, I32:$T, I32:$F)>;
// def : Pat<(selectcc   I32:$lhs, I32:$rhs, I32:$T, I32:$F, SETULT),
//           (SELECT_ULT I32:$lhs, I32:$rhs, I32:$T, I32:$F)>;

def : Pat<(selectcc   I32:$lhs, I32:$rhs, I32:$T, I32:$F, SETGT),
          (SELECT_LT  I32:$rhs, I32:$lhs, I32:$T, I32:$F)>;
def : Pat<(selectcc   I32:$lhs, I32:$rhs, I32:$T, I32:$F, SETUGT),
          (SELECT_ULT I32:$rhs, I32:$lhs, I32:$T, I32:$F)>;

def : Pat<(selectcc   I32:$lhs, I32:$rhs, I32:$T, I32:$F, SETLE),
          (SELECT_LT  I32:$rhs, I32:$lhs, I32:$F, I32:$T)>;
def : Pat<(selectcc   I32:$lhs, I32:$rhs, I32:$T, I32:$F, SETULE),
          (SELECT_ULT I32:$rhs, I32:$lhs, I32:$F, I32:$T)>;

def : Pat<(selectcc   I32:$lhs, I32:$rhs, I32:$T, I32:$F, SETGE),
          (SELECT_LT  I32:$lhs, I32:$rhs, I32:$F, I32:$T)>;
def : Pat<(selectcc   I32:$lhs, I32:$rhs, I32:$T, I32:$F, SETUGE),
          (SELECT_ULT I32:$lhs, I32:$rhs, I32:$F, I32:$T)>;

//===----------------------------------------------------------------------===//
// Instructions for the Windowed ABI
//===----------------------------------------------------------------------===//

def : Pat<(XtensaRet), (RETW)>,
      Requires<[IsWindowedABI]>;

let hasCtrlDep = 1, hasSideEffects = 1 in
def CALLX8_ABORT : CoreInstXtensa<(outs), (ins),
                 "call8\tabort",
                 []>,
                 Requires<[IsWindowedABI]>;

// This pseudo instruction is a lot like MOVSP except that this one
// does not have an explicit destination.  We use it during the
// selection DAG phase to indicate writes to SP.
// EmitInstrWithCustomInserter replaces it with MOVSP or COPY
// depending on whether we are using the Windowed ABI or not.
let usesCustomInserter = 1, Defs = [SP] in
def SETSP : CorePseudoInstXtensa<(outs), (ins AR:$b), "# SETSP", []>;

//===----------------------------------------------------------------------===//
// Instructions for the Call0 ABI
//===----------------------------------------------------------------------===//

def : Pat<(XtensaRet), (RET)>,
      Requires<[IsCall0ABI]>;

let hasCtrlDep = 1, hasSideEffects = 1 in
def CALLX0_ABORT : CoreInstXtensa<(outs), (ins),
                 "call0\tabort",
                 []>,
                 Requires<[IsCall0ABI]>;

//===----------------------------------------------------------------------===//
// Instructions for atomics
//===----------------------------------------------------------------------===//

// A atomic load operation that actually needs acquire semantics.
class acquiring_load<PatFrag base>
  : PatFrag<(ops node:$ptr), (base node:$ptr), [{
  AtomicOrdering Ordering = cast<AtomicSDNode>(N)->getOrdering();
  return isAcquireOrStronger(Ordering);
}]>;

// A store operation that actually needs release semantics.
class releasing_store<PatFrag base>
  : PatFrag<(ops node:$ptr, node:$val), (base node:$ptr, node:$val), [{
  AtomicOrdering Ordering = cast<AtomicSDNode>(N)->getOrdering();
  assert(Ordering != AtomicOrdering::AcquireRelease &&
         "unexpected store ordering");
  return isReleaseOrStronger(Ordering);
}]>;

// Load-acquire, store-release
def : Pat<(i32 (acquiring_load<atomic_load_32> (addr32 I32:$base, i32imm:$offset))), 
          (i32 (L32AI I32:$base, i32imm:$offset))>,
          Requires<[HasSync]>;

class SwapSt<PatFrag F>
  : PatFrag<(ops node:$val, node:$ptr), !head(F.Fragments), F.PredicateCode,
            F.OperandTransform>;

def : Pat<(releasing_store<SwapSt<atomic_store_32>> 
             I32:$val, (i32 (addr32 I32:$base, i32imm:$offset))),
          (S32RI AR:$val, I32:$base, i32imm:$offset)>,
          Requires<[HasSync]>;

// Atomic fences 
def : Pat<(atomic_fence (timm), (timm)), (MEMW)>;

//===----------------------------------------------------------------------===//
// Non-Instruction Patterns
//===----------------------------------------------------------------------===//

///
/// Some peepholes
///

def : Pat<(sra (add (shl I32:$src, (i32 16)), 131072), (i32 16)),
          (SEXT (ADDI AR:$src, 2), 15)>;
