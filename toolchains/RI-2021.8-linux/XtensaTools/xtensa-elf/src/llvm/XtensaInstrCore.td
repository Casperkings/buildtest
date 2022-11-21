//===-- XtensaInstrCore.td - Xtensa Generic Instructions ---*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// This files defines Xtensa core instructions, common between all configs
//===----------------------------------------------------------------------===//

def ABS:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b),
                   "abs\t$a,$b",
                   []>;

def ADD:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, AR:$c),
                   "add\t$a,$b,$c",
                   []>;

let isReMaterializable = 1 in {

def ADDI:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, i32imm:$c),
                   "addi\t$a,$b,$c",
                   []>;

def ADDI_N:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, i32imm:$c),
                   "addi.n\t$a,$b,$c",
                   []>,
        Requires<[HasDensity]>;

def ADDI_N_GENERIC:
  CorePseudoInstXtensa<(outs AR:$a),
                   (ins AR:$b, i32imm:$c),
                   "",
                   []>;

def ADDMI:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, i32imm:$c),
                   "addmi\t$a,$b,$c",
                   []>;

}

def ADDX2:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, AR:$c),
                   "addx2\t$a,$b,$c",
                   []>;

def ADDX4:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, AR:$c),
                   "addx4\t$a,$b,$c",
                   []>;

def ADDX8:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, AR:$c),
                   "addx8\t$a,$b,$c",
                   []>;

def ADD_N:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, AR:$c),
                   "add.n\t$a,$b,$c",
                   []>,
        Requires<[HasDensity]>;

def ADD_N_GENERIC:
  CorePseudoInstXtensa<(outs AR:$a),
                   (ins AR:$b, AR:$c),
                   "",
                   []>;

def AND:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, AR:$c),
                   "and\t$a,$b,$c",
                   []>;

def BALL:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "ball\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BALL_GENERIC:
  CorePseudoInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BALL_W15:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "ball.w15\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BALL_W18:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "ball.w18\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BANY:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "bany\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BANY_GENERIC:
  CorePseudoInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BANY_W15:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "bany.w15\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BANY_W18:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "bany.w18\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BBC:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "bbc\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BBCI:
        CoreInstXtensa<(outs),
                   (ins AR:$a, i32imm:$b, brtarget:$c),
                   "bbci\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BBCI_GENERIC:
  CorePseudoInstXtensa<(outs),
                   (ins AR:$a, i32imm:$b, brtarget:$c),
                   "",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BBCI_W15:
        CoreInstXtensa<(outs),
                   (ins AR:$a, i32imm:$b, brtarget:$c),
                   "bbci.w15\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BBCI_W18:
        CoreInstXtensa<(outs),
                   (ins AR:$a, i32imm:$b, brtarget:$c),
                   "bbci.w18\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BBC_GENERIC:
  CorePseudoInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BBC_W15:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "bbc.w15\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BBC_W18:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "bbc.w18\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BBS:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "bbs\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BBSI:
        CoreInstXtensa<(outs),
                   (ins AR:$a, i32imm:$b, brtarget:$c),
                   "bbsi\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BBSI_GENERIC:
  CorePseudoInstXtensa<(outs),
                   (ins AR:$a, i32imm:$b, brtarget:$c),
                   "",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BBSI_W15:
        CoreInstXtensa<(outs),
                   (ins AR:$a, i32imm:$b, brtarget:$c),
                   "bbsi.w15\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BBSI_W18:
        CoreInstXtensa<(outs),
                   (ins AR:$a, i32imm:$b, brtarget:$c),
                   "bbsi.w18\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BBS_GENERIC:
  CorePseudoInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BBS_W15:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "bbs.w15\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BBS_W18:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "bbs.w18\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BEQ:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "beq\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BEQI:
        CoreInstXtensa<(outs),
                   (ins AR:$a, i32imm:$b, brtarget:$c),
                   "beqi\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BEQI_GENERIC:
  CorePseudoInstXtensa<(outs),
                   (ins AR:$a, i32imm:$b, brtarget:$c),
                   "",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BEQI_W15:
        CoreInstXtensa<(outs),
                   (ins AR:$a, i32imm:$b, brtarget:$c),
                   "beqi.w15\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BEQI_W18:
        CoreInstXtensa<(outs),
                   (ins AR:$a, i32imm:$b, brtarget:$c),
                   "beqi.w18\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BEQZ:
        CoreInstXtensa<(outs),
                   (ins AR:$a, brtarget:$b),
                   "beqz\t$a,$b",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BEQZ_GENERIC:
  CorePseudoInstXtensa<(outs),
                   (ins AR:$a, brtarget:$b),
                   "",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BEQZ_N:
        CoreInstXtensa<(outs),
                   (ins AR:$a, brtarget:$b),
                   "beqz.n\t$a,$b",
                   []>,
        Requires<[HasDensity]>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BEQZ_W15:
        CoreInstXtensa<(outs),
                   (ins AR:$a, brtarget:$b),
                   "beqz.w15\t$a,$b",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BEQZ_W18:
        CoreInstXtensa<(outs),
                   (ins AR:$a, brtarget:$b),
                   "beqz.w18\t$a,$b",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BEQ_GENERIC:
  CorePseudoInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BEQ_W15:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "beq.w15\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BEQ_W18:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "beq.w18\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BGE:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "bge\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BGEI:
        CoreInstXtensa<(outs),
                   (ins AR:$a, i32imm:$b, brtarget:$c),
                   "bgei\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BGEI_GENERIC:
  CorePseudoInstXtensa<(outs),
                   (ins AR:$a, i32imm:$b, brtarget:$c),
                   "",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BGEI_W15:
        CoreInstXtensa<(outs),
                   (ins AR:$a, i32imm:$b, brtarget:$c),
                   "bgei.w15\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BGEI_W18:
        CoreInstXtensa<(outs),
                   (ins AR:$a, i32imm:$b, brtarget:$c),
                   "bgei.w18\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BGEU:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "bgeu\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BGEUI:
        CoreInstXtensa<(outs),
                   (ins AR:$a, i32imm:$b, brtarget:$c),
                   "bgeui\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BGEUI_GENERIC:
  CorePseudoInstXtensa<(outs),
                   (ins AR:$a, i32imm:$b, brtarget:$c),
                   "",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BGEUI_W15:
        CoreInstXtensa<(outs),
                   (ins AR:$a, i32imm:$b, brtarget:$c),
                   "bgeui.w15\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BGEUI_W18:
        CoreInstXtensa<(outs),
                   (ins AR:$a, i32imm:$b, brtarget:$c),
                   "bgeui.w18\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BGEU_GENERIC:
  CorePseudoInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BGEU_W15:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "bgeu.w15\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BGEU_W18:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "bgeu.w18\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BGEZ:
        CoreInstXtensa<(outs),
                   (ins AR:$a, brtarget:$b),
                   "bgez\t$a,$b",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BGEZ_GENERIC:
  CorePseudoInstXtensa<(outs),
                   (ins AR:$a, brtarget:$b),
                   "",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BGEZ_W15:
        CoreInstXtensa<(outs),
                   (ins AR:$a, brtarget:$b),
                   "bgez.w15\t$a,$b",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BGEZ_W18:
        CoreInstXtensa<(outs),
                   (ins AR:$a, brtarget:$b),
                   "bgez.w18\t$a,$b",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BGE_GENERIC:
  CorePseudoInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BGE_W15:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "bge.w15\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BGE_W18:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "bge.w18\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BLT:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "blt\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BLTI:
        CoreInstXtensa<(outs),
                   (ins AR:$a, i32imm:$b, brtarget:$c),
                   "blti\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BLTI_GENERIC:
  CorePseudoInstXtensa<(outs),
                   (ins AR:$a, i32imm:$b, brtarget:$c),
                   "",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BLTI_W15:
        CoreInstXtensa<(outs),
                   (ins AR:$a, i32imm:$b, brtarget:$c),
                   "blti.w15\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BLTI_W18:
        CoreInstXtensa<(outs),
                   (ins AR:$a, i32imm:$b, brtarget:$c),
                   "blti.w18\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BLTU:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "bltu\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BLTUI:
        CoreInstXtensa<(outs),
                   (ins AR:$a, i32imm:$b, brtarget:$c),
                   "bltui\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BLTUI_GENERIC:
  CorePseudoInstXtensa<(outs),
                   (ins AR:$a, i32imm:$b, brtarget:$c),
                   "",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BLTUI_W15:
        CoreInstXtensa<(outs),
                   (ins AR:$a, i32imm:$b, brtarget:$c),
                   "bltui.w15\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BLTUI_W18:
        CoreInstXtensa<(outs),
                   (ins AR:$a, i32imm:$b, brtarget:$c),
                   "bltui.w18\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BLTU_GENERIC:
  CorePseudoInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BLTU_W15:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "bltu.w15\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BLTU_W18:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "bltu.w18\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BLTZ:
        CoreInstXtensa<(outs),
                   (ins AR:$a, brtarget:$b),
                   "bltz\t$a,$b",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BLTZ_GENERIC:
  CorePseudoInstXtensa<(outs),
                   (ins AR:$a, brtarget:$b),
                   "",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BLTZ_W15:
        CoreInstXtensa<(outs),
                   (ins AR:$a, brtarget:$b),
                   "bltz.w15\t$a,$b",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BLTZ_W18:
        CoreInstXtensa<(outs),
                   (ins AR:$a, brtarget:$b),
                   "bltz.w18\t$a,$b",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BLT_GENERIC:
  CorePseudoInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BLT_W15:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "blt.w15\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BLT_W18:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "blt.w18\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BNALL:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "bnall\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BNALL_GENERIC:
  CorePseudoInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BNALL_W15:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "bnall.w15\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BNALL_W18:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "bnall.w18\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BNE:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "bne\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BNEI:
        CoreInstXtensa<(outs),
                   (ins AR:$a, i32imm:$b, brtarget:$c),
                   "bnei\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BNEI_GENERIC:
  CorePseudoInstXtensa<(outs),
                   (ins AR:$a, i32imm:$b, brtarget:$c),
                   "",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BNEI_W15:
        CoreInstXtensa<(outs),
                   (ins AR:$a, i32imm:$b, brtarget:$c),
                   "bnei.w15\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BNEI_W18:
        CoreInstXtensa<(outs),
                   (ins AR:$a, i32imm:$b, brtarget:$c),
                   "bnei.w18\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BNEZ:
        CoreInstXtensa<(outs),
                   (ins AR:$a, brtarget:$b),
                   "bnez\t$a,$b",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BNEZ_GENERIC:
  CorePseudoInstXtensa<(outs),
                   (ins AR:$a, brtarget:$b),
                   "",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BNEZ_N:
        CoreInstXtensa<(outs),
                   (ins AR:$a, brtarget:$b),
                   "bnez.n\t$a,$b",
                   []>,
        Requires<[HasDensity]>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BNEZ_W15:
        CoreInstXtensa<(outs),
                   (ins AR:$a, brtarget:$b),
                   "bnez.w15\t$a,$b",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BNEZ_W18:
        CoreInstXtensa<(outs),
                   (ins AR:$a, brtarget:$b),
                   "bnez.w18\t$a,$b",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BNE_GENERIC:
  CorePseudoInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BNE_W15:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "bne.w15\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BNE_W18:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "bne.w18\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BNONE:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "bnone\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BNONE_GENERIC:
  CorePseudoInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BRDEC_P:
        CoreInstXtensa<(outs AR:$a0),
                   (ins AR:$a1, brtarget:$b),
                   "brdec.p\t$a0,$b",
                   []>,
        Requires<[IsBrdec]>
{
  let Constraints = "$a0 = $a1";
  let isTerminator = 1;
  let isBranch = 1;
}

def BNONE_W15:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "bnone.w15\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def BNONE_W18:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, brtarget:$c),
                   "bnone.w18\t$a,$b,$c",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
}

def CLAMPS:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, i32imm:$c),
                   "clamps\t$a,$b,$c",
                   []>,
        Requires<[HasClamps]>;

def CLREX:
        CoreInstXtensa<(outs),
                   (ins),
                   "clrex\t",
                   []>
{
  let Defs = [XTS_CORE_XTSYNC];
  let Uses = [XTS_CORE_XTSYNC];
  let hasSideEffects = 1;
}

def CONST16:
        CoreInstXtensa<(outs AR:$a0),
                   (ins AR:$a1, i32imm:$b),
                   "const16\t$a0,$b",
                   []>,
        Requires<[HasConst16]>
{
  let Constraints = "$a0 = $a1";
  let isReMaterializable = 1;
}

def DEPBITS:
        CoreInstXtensa<(outs AR:$a0),
                   (ins AR:$a1, AR:$b, i32imm:$c, i32imm:$d),
                   "depbits\t$a0,$b,$c,$d",
                   []>
{
  let Constraints = "$a0 = $a1";
}

def ENTRY:
        CoreInstXtensa<(outs),
                   (ins AR:$a, i32imm:$b),
                   "entry\t$a,$b",
                   []>
{
  let Defs = [SP, XTS_CORE_WINDOWBASE, XTS_CORE_WINDOWSTART];
  let Uses = [XTS_CORE_PSCALLINC, XTS_CORE_PSEXCM, XTS_CORE_PSWOE, XTS_CORE_WINDOWBASE, XTS_CORE_WINDOWSTART];
}

def DIWBUI_P:
        CoreInstXtensa<(outs AR:$a0),
                   (ins AR:$a1),
                   "diwbui.p\t$a0",
                   []>
{
  let Constraints = "$a0 = $a1";
  let Uses = [XTS_CORE_MS_DISPST, XTS_CORE_PSRING, XTS_CORE_INOCDMODE];
  let hasSideEffects = 1;
}

def EXIT:
        CoreInstXtensa<(outs),
                   (ins),
                   "exit\t",
                   []>
{
  let Defs = [XTS_CORE_WB_C, XTS_CORE_WB_P];
  let Uses = [XTS_CORE_WB_C, XTS_CORE_WB_N, XTS_CORE_WB_S, XTS_CORE_MS_DISPST];
  let hasSideEffects = 1;
}

def EXTUI:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, i32imm:$c, i32imm:$d),
                   "extui\t$a,$b,$c,$d",
                   []>;

def EXTW:
        CoreInstXtensa<(outs),
                   (ins),
                   "extw\t",
                   []>
{
  let hasSideEffects = 1;
}

def GETEX:
        CoreInstXtensa<(outs AR:$a0),
                   (ins AR:$a1),
                   "getex\t$a0",
                   []>
{
  let Constraints = "$a0 = $a1";
  let Defs = [XTS_CORE_XTSYNC];
  let Uses = [XTS_CORE_XTSYNC];
  let hasSideEffects = 1;
}

def HALT:
        CoreInstXtensa<(outs),
                   (ins i32imm:$a),
                   "halt\t$a",
                   []>
{
  let Uses = [XTS_CORE_INOCDMODE];
  let hasSideEffects = 1;
}

def HALT_N:
        CoreInstXtensa<(outs),
                   (ins),
                   "halt.n\t",
                   []>
{
  let Uses = [XTS_CORE_INOCDMODE];
  let hasSideEffects = 1;
}

def IDTLB:
        CoreInstXtensa<
                   (outs),
                   (ins AR:$a),
                   "idtlb\t$a",
                   []>
{
  let Defs = [XTS_CORE_XTSYNC];
  let hasSideEffects = 1;
}

def ILL:
           InstXtensa<
                   (outs),
                   (ins),
                   "ill\t",
                   []>
{
  let hasSideEffects = 1;
}

def ILL_N:
           InstXtensa<
                   (outs),
                   (ins),
                   "ill.n\t",
                   []>
{
  let hasSideEffects = 1;
}

def ILL_N_GENERIC:
     PseudoInstXtensa<
                   (outs),
                   (ins),
                   "",
                   []>
{
  let hasSideEffects = 1;
}

def EXTW_NOREORDER:
        CoreInstXtensa<(outs),
                   (ins),
                   "# extw\t",
                   []>
{
  let hasSideEffects = 1;
}

def J:
        CoreInstXtensa<(outs),
                   (ins brtarget:$a),
                   "j\t$a",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
  let isBarrier = 1;
}

def JX:
        CoreInstXtensa<(outs),
                   (ins AR:$a),
                   "jx\t$a",
                   []>
{
  let isTerminator = 1;
  let isBranch = 1;
  let isBarrier = 1;
  let isIndirectBranch = 1;
}

def L16SI:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, i32imm:$c),
                   "l16si\t$a,$b,$c",
                   []>
{
  let mayLoad = 1;
}

def L16UI:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, i32imm:$c),
                   "l16ui\t$a,$b,$c",
                   []>
{
  let mayLoad = 1;
}

def L32AI:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, i32imm:$c),
                   "l32ai\t$a,$b,$c",
                   []>
{
  let hasSideEffects = 1;
  let mayLoad = 1;
}

def L32E:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, i32imm:$c),
                   "l32e\t$a,$b,$c",
                   []>
{
  let Uses = [XTS_CORE_PSEXCM, XTS_CORE_PSRING];
  let hasSideEffects = 1;
  let mayLoad = 1;
}

def L32EX:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b),
                   "l32ex\t$a,$b",
                   []>
{
  let Defs = [XTS_CORE_XTSYNC];
  let Uses = [XTS_CORE_XTSYNC];
  let hasSideEffects = 1;
  let mayLoad = 1;
}

def L32I:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, i32imm:$c),
                   "l32i\t$a,$b,$c",
                   []>
{
  let mayLoad = 1;
}

def L32I_N:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, i32imm:$c),
                   "l32i.n\t$a,$b,$c",
                   []>,
        Requires<[HasDensity]>
{
  let mayLoad = 1;
}

def L32I_N_GENERIC:
  CorePseudoInstXtensa<(outs AR:$a),
                   (ins AR:$b, i32imm:$c),
                   "",
                   []>
{
  let mayLoad = 1;
}

def L32R:
        CoreInstXtensa<(outs AR:$a),
                   (ins pcrel_imm:$b),
                   "l32r\t$a,$b",
                   []>,
        Requires<[HasL32r]>
{
  let mayLoad = 1;
  let isReMaterializable = 1;
}

def L8UI:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, i32imm:$c),
                   "l8ui\t$a,$b,$c",
                   []>
{
  let mayLoad = 1;
}

def LDCT:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b),
                   "ldct\t$a,$b",
                   []>
{
  let Uses = [XTS_CORE_MS_DISPST, XTS_CORE_PSRING, XTS_CORE_INOCDMODE];
  let hasSideEffects = 1;
  let mayLoad = 1;
}

def LDCW:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b),
                   "ldcw\t$a,$b",
                   []>
{
  let Uses = [XTS_CORE_MS_DISPST, XTS_CORE_PSRING, XTS_CORE_INOCDMODE];
  let hasSideEffects = 1;
  let mayLoad = 1;
}

def LDDR32_P:
        CoreInstXtensa<(outs AR:$a0),
                   (ins AR:$a1),
                   "lddr32.p\t$a0",
                   []>
{
  let Constraints = "$a0 = $a1";
  let Defs = [XTS_CORE_XTSYNC, XTS_CORE_DDR];
  let Uses = [XTS_CORE_PSEXCM, XTS_CORE_PSRING, XTS_CORE_INOCDMODE];
  let hasSideEffects = 1;
  let mayLoad = 1;
}

def LICT:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b),
                   "lict\t$a,$b",
                   []>
{
  let Uses = [XTS_CORE_PSEXCM, XTS_CORE_PSRING];
  let hasSideEffects = 1;
  let mayLoad = 1;
}

def LICW:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b),
                   "licw\t$a,$b",
                   []>
{
  let Uses = [XTS_CORE_PSEXCM, XTS_CORE_PSRING];
  let hasSideEffects = 1;
  let mayLoad = 1;
}

def LOOP:
        CoreInstXtensa<(outs),
                   (ins AR:$a, brtarget:$b),
                   "loop\t$a,$b",
                   []>
{
  let isTerminator = 1;
  let isNotDuplicable = 1;
  let Defs = [XTS_CORE_LBEG, XTS_CORE_LCOUNT, XTS_CORE_LEND];
}

def LOOPGTZ:
        CoreInstXtensa<(outs),
                   (ins AR:$a, brtarget:$b),
                   "loopgtz\t$a,$b",
                   []>
{
  let isTerminator = 1;
  let isNotDuplicable = 1;
  let Defs = [XTS_CORE_LBEG, XTS_CORE_LCOUNT, XTS_CORE_LEND];
}

def LOOPNEZ:
        CoreInstXtensa<(outs),
                   (ins AR:$a, brtarget:$b),
                   "loopnez\t$a,$b",
                   []>
{
  let isTerminator = 1;
  let isNotDuplicable = 1;
  let Defs = [XTS_CORE_LBEG, XTS_CORE_LCOUNT, XTS_CORE_LEND];
}

def LOOP_GENERIC:
        CoreInstXtensa<(outs),
                   (ins AR:$a, brtarget:$b),
                   "loop\t$a,$b",
                   []>
{
  let isTerminator = 1;
  let isNotDuplicable = 1;
  let Defs = [XTS_CORE_LBEG, XTS_CORE_LCOUNT, XTS_CORE_LEND];
}

def LOOPGTZ_GENERIC:
        CoreInstXtensa<(outs),
                   (ins AR:$a, brtarget:$b),
                   "loopgtz\t$a,$b",
                   []>
{
  let isTerminator = 1;
  let isNotDuplicable = 1;
  let Defs = [XTS_CORE_LBEG, XTS_CORE_LCOUNT, XTS_CORE_LEND];
}

def LOOPNEZ_GENERIC:
        CoreInstXtensa<(outs),
                   (ins AR:$a, brtarget:$b),
                   "loopnez\t$a,$b",
                   []>
{
  let isTerminator = 1;
  let isNotDuplicable = 1;
  let Defs = [XTS_CORE_LBEG, XTS_CORE_LCOUNT, XTS_CORE_LEND];
}

def MAX:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, AR:$c),
                   "max\t$a,$b,$c",
                   []>,
        Requires<[HasMinMax]>;

def MAXU:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, AR:$c),
                   "maxu\t$a,$b,$c",
                   []>,
        Requires<[HasMinMax]>;

def MEMW:
        CoreInstXtensa<(outs),
                   (ins),
                   "memw\t",
                   []>
{
  let hasSideEffects = 1;
}

def MEMW_NOREORDER:
        CoreInstXtensa<(outs),
                   (ins),
                   "# memw\t",
                   []>
{
  let hasSideEffects = 1;
}

def MIN:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, AR:$c),
                   "min\t$a,$b,$c",
                   []>,
        Requires<[HasMinMax]>;

def MINU:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, AR:$c),
                   "minu\t$a,$b,$c",
                   []>,
        Requires<[HasMinMax]>;

def MOVEQZ:
        CoreInstXtensa<(outs AR:$a0),
                   (ins AR:$a1, AR:$b, AR:$c),
                   "moveqz\t$a0,$b,$c",
                   []>
{
  let Constraints = "$a0 = $a1";
  let isCommutable = 1;
  let isSelect = 1;
}

def MOVGEZ:
        CoreInstXtensa<(outs AR:$a0),
                   (ins AR:$a1, AR:$b, AR:$c),
                   "movgez\t$a0,$b,$c",
                   []>
{
  let Constraints = "$a0 = $a1";
  let isCommutable = 1;
}

def MOVI:
        CoreInstXtensa<(outs AR:$a),
                   (ins i32imm:$b),
                   "movi\t$a,$b",
                   []>
{
  let isReMaterializable = 1;
  let isAsCheapAsAMove = 1;
  let isMoveImm  = 1;
}

def MOVI_N:
        CoreInstXtensa<(outs AR:$a),
                   (ins i32imm:$b),
                   "movi.n\t$a,$b",
                   []>,
        Requires<[HasDensity]>
{
  let isReMaterializable = 1;
  let isAsCheapAsAMove = 1;
  let isMoveImm  = 1;
}

def MOVI_N_GENERIC:
  CorePseudoInstXtensa<(outs AR:$a),
                   (ins i32imm:$b),
                   "",
                   []>
{
  let isReMaterializable = 1;
  let isAsCheapAsAMove = 1;
  let isMoveImm  = 1;
}

def MOVLTZ:
        CoreInstXtensa<(outs AR:$a0),
                   (ins AR:$a1, AR:$b, AR:$c),
                   "movltz\t$a0,$b,$c",
                   []>
{
  let Constraints = "$a0 = $a1";
  let isCommutable = 1;
}

def MOVNEZ:
        CoreInstXtensa<(outs AR:$a0),
                   (ins AR:$a1, AR:$b, AR:$c),
                   "movnez\t$a0,$b,$c",
                   []>
{
  let Constraints = "$a0 = $a1";
  let isCommutable = 1;
  let isSelect = 1;
}

def MOVSP:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b),
                   "movsp\t$a,$b",
                   []>
{
  let Uses = [XTS_CORE_WINDOWBASE, XTS_CORE_WINDOWSTART];
  let hasSideEffects = 1;
}

def MOV_N:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b),
                   "mov.n\t$a,$b",
                   []>,
        Requires<[HasDensity]>;

def MUL16S:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, AR:$c),
                   "mul16s\t$a,$b,$c",
                   []>,
        Requires<[HasMul16]>;

def MUL16U:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, AR:$c),
                   "mul16u\t$a,$b,$c",
                   []>,
        Requires<[HasMul16]>;

def MULA_AA_HH:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b),
                   "mula.aa.hh\t$a,$b",
                   []>,
        Requires<[HasMac16]>
{
  let Defs = [XTS_CORE_ACC];
  let Uses = [XTS_CORE_ACC];
}

def MULA_AA_HL:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b),
                   "mula.aa.hl\t$a,$b",
                   []>,
        Requires<[HasMac16]>
{
  let Defs = [XTS_CORE_ACC];
  let Uses = [XTS_CORE_ACC];
}

def MULA_AA_LH:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b),
                   "mula.aa.lh\t$a,$b",
                   []>,
        Requires<[HasMac16]>
{
  let Defs = [XTS_CORE_ACC];
  let Uses = [XTS_CORE_ACC];
}

def MULA_AA_LL:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b),
                   "mula.aa.ll\t$a,$b",
                   []>,
        Requires<[HasMac16]>
{
  let Defs = [XTS_CORE_ACC];
  let Uses = [XTS_CORE_ACC];
}

def MULL:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, AR:$c),
                   "mull\t$a,$b,$c",
                   []>,
        Requires<[HasMul32]>;

def MULSH:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, AR:$c),
                   "mulsh\t$a,$b,$c",
                   []>,
        Requires<[HasMul32h]>;

def MULS_AA_HH:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b),
                   "muls.aa.hh\t$a,$b",
                   []>,
        Requires<[HasMac16]>
{
  let Defs = [XTS_CORE_ACC];
  let Uses = [XTS_CORE_ACC];
}

def MULS_AA_HL:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b),
                   "muls.aa.hl\t$a,$b",
                   []>,
        Requires<[HasMac16]>
{
  let Defs = [XTS_CORE_ACC];
  let Uses = [XTS_CORE_ACC];
}

def MULS_AA_LH:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b),
                   "muls.aa.lh\t$a,$b",
                   []>,
        Requires<[HasMac16]>
{
  let Defs = [XTS_CORE_ACC];
  let Uses = [XTS_CORE_ACC];
}

def MULS_AA_LL:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b),
                   "muls.aa.ll\t$a,$b",
                   []>,
        Requires<[HasMac16]>
{
  let Defs = [XTS_CORE_ACC];
  let Uses = [XTS_CORE_ACC];
}

def MULUH:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, AR:$c),
                   "muluh\t$a,$b,$c",
                   []>,
        Requires<[HasMul32h]>;

def MUL_AA_HH:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b),
                   "mul.aa.hh\t$a,$b",
                   []>,
        Requires<[HasMac16]>
{
  let Defs = [XTS_CORE_ACC];
}

def MUL_AA_HL:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b),
                   "mul.aa.hl\t$a,$b",
                   []>,
        Requires<[HasMac16]>
{
  let Defs = [XTS_CORE_ACC];
}

def MUL_AA_LH:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b),
                   "mul.aa.lh\t$a,$b",
                   []>,
        Requires<[HasMac16]>
{
  let Defs = [XTS_CORE_ACC];
}

def MUL_AA_LL:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b),
                   "mul.aa.ll\t$a,$b",
                   []>,
        Requires<[HasMac16]>
{
  let Defs = [XTS_CORE_ACC];
}

def NEG:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b),
                   "neg\t$a,$b",
                   []>;

def NOP:
        CoreInstXtensa<(outs),
                   (ins),
                   "nop\t",
                   []>
{
  let hasSideEffects = 1;
}

def NOP_N:
        CoreInstXtensa<(outs),
                   (ins),
                   "nop.n\t",
                   []>,
        Requires<[HasDensity]>
{
  let hasSideEffects = 1;
}

def NSA:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b),
                   "nsa\t$a,$b",
                   []>,
        Requires<[HasNsa]>;

def NSAU:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b),
                   "nsau\t$a,$b",
                   []>,
        Requires<[HasNsa]>;

def OR:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, AR:$c),
                   "or\t$a,$b,$c",
                   []>;

def PDTLB:
        CoreInstXtensa<
                   (outs AR:$a),
                   (ins AR:$b),
                   "pdtlb\t$a,$b",
                   []>
{
  let hasSideEffects = 1;
}

def PITLB:
        CoreInstXtensa<
                   (outs AR:$a),
                   (ins AR:$b),
                   "pitlb\t$a,$b",
                   []>
{
  let hasSideEffects = 1;
}

def RDTLB0:
        CoreInstXtensa<
                   (outs AR:$a),
                   (ins AR:$b),
                   "rdtlb0\t$a,$b",
                   []>
{
  let hasSideEffects = 1;
}

def RDTLB1:
        CoreInstXtensa<
                   (outs AR:$a),
                   (ins AR:$b),
                   "rdtlb1\t$a,$b",
                   []>
{
  let hasSideEffects = 1;
}

def PPTLB:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b),
                   "pptlb\t$a,$b",
                   []>
{
  let Uses = [XTS_CORE_PSEXCM, XTS_CORE_PSRING];
  let hasSideEffects = 1;
}

def QUOS:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, AR:$c),
                   "quos\t$a,$b,$c",
                   []>;

def QUOU:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, AR:$c),
                   "quou\t$a,$b,$c",
                   []>;

def RBTB0:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b),
                   "rbtb0\t$a,$b",
                   []>
{
  let Uses = [XTS_CORE_MS_DISPST, XTS_CORE_PSRING, XTS_CORE_INOCDMODE];
  let hasSideEffects = 1;
}

def RBTB1:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b),
                   "rbtb1\t$a,$b",
                   []>
{
  let Uses = [XTS_CORE_MS_DISPST, XTS_CORE_PSRING, XTS_CORE_INOCDMODE];
  let hasSideEffects = 1;
}

def RBTB2:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b),
                   "rbtb2\t$a,$b",
                   []>
{
  let Uses = [XTS_CORE_MS_DISPST, XTS_CORE_PSRING, XTS_CORE_INOCDMODE];
  let hasSideEffects = 1;
}

def REMS:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, AR:$c),
                   "rems\t$a,$b,$c",
                   []>;

def REMU:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, AR:$c),
                   "remu\t$a,$b,$c",
                   []>;

def RET:
        CoreInstXtensa<(outs),
                   (ins),
                   "ret\t",
                   []>
{
  let isTerminator = 1;
  let isBarrier = 1;
  let isReturn = 1;
  let Uses = [LR];
}

def RETW:
        CoreInstXtensa<(outs),
                   (ins),
                   "retw\t",
                   []>
{
  let isTerminator = 1;
  let isBarrier = 1;
  let isReturn = 1;
  let Defs = [XTS_CORE_PSCALLINC, XTS_CORE_WINDOWBASE, XTS_CORE_WINDOWSTART];
  let Uses = [LR, XTS_CORE_PSEXCM, XTS_CORE_PSWOE, XTS_CORE_WINDOWBASE, XTS_CORE_WINDOWSTART];
}

def RETW_N:
        CoreInstXtensa<(outs),
                   (ins),
                   "retw.n\t",
                   []>,
        Requires<[HasDensity]>
{
  let isTerminator = 1;
  let isBarrier = 1;
  let isReturn = 1;
  let Defs = [XTS_CORE_PSCALLINC, XTS_CORE_WINDOWBASE, XTS_CORE_WINDOWSTART];
  let Uses = [LR, XTS_CORE_PSEXCM, XTS_CORE_PSWOE, XTS_CORE_WINDOWBASE, XTS_CORE_WINDOWSTART];
}

def RETW_N_GENERIC:
  CorePseudoInstXtensa<(outs),
                   (ins),
                   "",
                   []>
{
  let isTerminator = 1;
  let isBarrier = 1;
  let isReturn = 1;
  let Defs = [XTS_CORE_PSCALLINC, XTS_CORE_WINDOWBASE, XTS_CORE_WINDOWSTART];
  let Uses = [LR, XTS_CORE_PSEXCM, XTS_CORE_PSWOE, XTS_CORE_WINDOWBASE, XTS_CORE_WINDOWSTART];
}

def RET_N:
        CoreInstXtensa<(outs),
                   (ins),
                   "ret.n\t",
                   []>,
        Requires<[HasDensity]>
{
  let isTerminator = 1;
  let isBarrier = 1;
  let isReturn = 1;
  let Uses = [LR];
}

def RET_N_GENERIC:
  CorePseudoInstXtensa<(outs),
                   (ins),
                   "",
                   []>
{
  let isTerminator = 1;
  let isBarrier = 1;
  let isReturn = 1;
  let Uses = [LR];
}

def RITLB0:
        CoreInstXtensa<
                   (outs AR:$a),
                   (ins AR:$b),
                   "ritlb0\t$a,$b",
                   []>
{
  let hasSideEffects = 1;
}

def RITLB1:
        CoreInstXtensa<
                   (outs AR:$a),
                   (ins AR:$b),
                   "ritlb1\t$a,$b",
                   []>
{
  let hasSideEffects = 1;
}

def ROTW:
        CoreInstXtensa<(outs),
                   (ins i32imm:$a),
                   "rotw\t$a",
                   []>
{
  let Defs = [XTS_CORE_WINDOWBASE];
  let Uses = [XTS_CORE_PSEXCM, XTS_CORE_PSRING, XTS_CORE_WINDOWBASE];
  let hasSideEffects = 1;
}

def RPTLB0:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b),
                   "rptlb0\t$a,$b",
                   []>
{
  let Uses = [XTS_CORE_PSEXCM, XTS_CORE_PSRING, XTS_CORE_MPUENB];
  let hasSideEffects = 1;
}

def RPTLB1:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b),
                   "rptlb1\t$a,$b",
                   []>
{
  let Uses = [XTS_CORE_PSEXCM, XTS_CORE_PSRING];
  let hasSideEffects = 1;
}

def RSIL:
        CoreInstXtensa<
                   (outs AR:$a),
                   (ins i32imm:$b),
                   "rsil\t$a,$b",
                   []>
{
  let hasSideEffects = 1;
  let Defs = [XTS_CORE_PSINTLEVEL];
  let Uses = [XTS_CORE_PSWOE, XTS_CORE_PSCALLINC, XTS_CORE_PSOWB, XTS_CORE_PSRING, XTS_CORE_PSUM, XTS_CORE_PSEXCM, XTS_CORE_PSINTLEVEL];
}

def RSR_ACCHI:
        CoreInstXtensa<(outs AR:$a),
                   (ins),
                   "rsr.acchi\t$a",
                   []>
{
  let Uses = [XTS_CORE_ACC];
}

def RSR_ACCLO:
        CoreInstXtensa<(outs AR:$a),
                   (ins),
                   "rsr.acclo\t$a",
                   []>
{
  let Uses = [XTS_CORE_ACC];
}

def RSR_CCOUNT:
        CoreInstXtensa<(outs AR:$a),
                   (ins),
                   "rsr.ccount\t$a",
                   []>
{
  let hasSideEffects = 1;
  let Uses = [XTS_CORE_CCOUNT, XTS_CORE_PSEXCM, XTS_CORE_PSRING];
}

def RSR_LBEG:
        CoreInstXtensa<(outs AR:$a),
                   (ins),
                   "rsr.lbeg\t$a",
                   []>
{
  let Uses = [XTS_CORE_LBEG];
}

def RSR_LCOUNT:
        CoreInstXtensa<(outs AR:$a),
                   (ins),
                   "rsr.lcount\t$a",
                   []>
{
  let Uses = [XTS_CORE_LCOUNT];
}

def RSR_LEND:
        CoreInstXtensa<(outs AR:$a),
                   (ins),
                   "rsr.lend\t$a",
                   []>
{
  let Uses = [XTS_CORE_LEND];
}

def RSR_PS:
        CoreInstXtensa<
                   (outs AR:$a),
                   (ins),
                   "rsr.ps\t$a",
                   []>
{
  let hasSideEffects = 1;
  let Uses = [XTS_CORE_PSENTRYNR, XTS_CORE_PSSS, XTS_CORE_PSSTACK, XTS_CORE_PSRING, XTS_CORE_PSDI, XTS_CORE_PSDIEXC, XTS_CORE_INOCDMODE, XTS_CORE_MS_DISPST];
}

def RSR_SAR:
        CoreInstXtensa<(outs AR:$a),
                   (ins),
                   "rsr.sar\t$a",
                   []>
{
  let Uses = [XTS_CORE_SAR];
}

def S16I:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, i32imm:$c),
                   "s16i\t$a,$b,$c",
                   []>
{
  let mayStore = 1;
}

def S32C1I:
        CoreInstXtensa<
                   (outs AR:$a0),
                   (ins AR:$a1, AR:$b, i32imm:$c),
                   "s32c1i\t$a0,$b,$c",
                   []>
{
  let Constraints = "$a0 = $a1";
  let Uses = [XTS_CORE_SCOMPARE1, XTS_CORE_XTSYNC, XTS_CORE_SCOMPARE1];
  let hasSideEffects = 1;
  let mayLoad = 1;
}

def S32E:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, i32imm:$c),
                   "s32e\t$a,$b,$c",
                   []>
{
  let Uses = [XTS_CORE_PSEXCM, XTS_CORE_PSRING];
  let hasSideEffects = 1;
  let mayStore = 1;
}

def S32EX:
        CoreInstXtensa<(outs AR:$a0),
                   (ins AR:$a1, AR:$b),
                   "s32ex\t$a0,$b",
                   []>
{
  let Constraints = "$a0 = $a1";
  let Defs = [XTS_CORE_XTSYNC];
  let Uses = [XTS_CORE_XTSYNC];
  let hasSideEffects = 1;
  let mayStore = 1;
}

def S32I:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, i32imm:$c),
                   "s32i\t$a,$b,$c",
                   []>
{
  let mayStore = 1;
}

def S32I_N:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, i32imm:$c),
                   "s32i.n\t$a,$b,$c",
                   []>,
        Requires<[HasDensity]>
{
  let mayStore = 1;
}

def S32I_N_GENERIC:
  CorePseudoInstXtensa<(outs),
                   (ins AR:$a, AR:$b, i32imm:$c),
                   "",
                   []>
{
  let mayStore = 1;
}

def S32RI:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, i32imm:$c),
                   "s32ri\t$a,$b,$c",
                   []>
{
  let hasSideEffects = 1;
  let mayStore = 1;
}

def S32STK:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, i32imm:$c),
                   "s32stk\t$a,$b,$c",
                   []>
{
  let Defs = [XTS_CORE_PSSTACK];
  let Uses = [XTS_CORE_MS_DISPST, XTS_CORE_PSSTACK, XTS_CORE_ISB, XTS_CORE_ISL, XTS_CORE_KSL, XTS_CORE_EXCCAUSE];
  let hasSideEffects = 1;
  let mayStore = 1;
}

def S8I:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b, i32imm:$c),
                   "s8i\t$a,$b,$c",
                   []>
{
  let mayStore = 1;
}

def SALT:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, AR:$c),
                   "salt\t$a,$b,$c",
                   []>;

def SALTU:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, AR:$c),
                   "saltu\t$a,$b,$c",
                   []>;

def SDCT:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b),
                   "sdct\t$a,$b",
                   []>
{
  let Uses = [XTS_CORE_MS_DISPST, XTS_CORE_PSRING, XTS_CORE_INOCDMODE];
  let hasSideEffects = 1;
  let mayStore = 1;
}

def SDCW:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b),
                   "sdcw\t$a,$b",
                   []>
{
  let Uses = [XTS_CORE_MS_DISPST, XTS_CORE_PSRING, XTS_CORE_INOCDMODE];
  let hasSideEffects = 1;
  let mayStore = 1;
}

def SDDR32_P:
        CoreInstXtensa<(outs AR:$a0),
                   (ins AR:$a1),
                   "sddr32.p\t$a0",
                   []>
{
  let Constraints = "$a0 = $a1";
  let Uses = [XTS_CORE_PSEXCM, XTS_CORE_PSRING, XTS_CORE_INOCDMODE, XTS_CORE_DDR];
  let hasSideEffects = 1;
  let mayStore = 1;
}

def SETW:
        CoreInstXtensa<(outs),
                   (ins i32imm:$a),
                   "setw\t$a",
                   []>
{
  let Defs = [XTS_CORE_WB_P];
  let Uses = [XTS_CORE_MS_DISPST, XTS_CORE_PSRING, XTS_CORE_INOCDMODE];
  let hasSideEffects = 1;
}

def SEXT:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, i32imm:$c),
                   "sext\t$a,$b,$c",
                   []>,
        Requires<[HasSext]>;

def SICT:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b),
                   "sict\t$a,$b",
                   []>
{
  let Uses = [XTS_CORE_MS_DISPST, XTS_CORE_PSRING, XTS_CORE_INOCDMODE];
  let hasSideEffects = 1;
  let mayStore = 1;
}

def SICW:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b),
                   "sicw\t$a,$b",
                   []>
{
  let Uses = [XTS_CORE_MS_DISPST, XTS_CORE_PSRING, XTS_CORE_INOCDMODE];
  let hasSideEffects = 1;
  let mayStore = 1;
}

def SLL:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b),
                   "sll\t$a,$b",
                   []>
{
  let Uses = [XTS_CORE_SAR];
}

def SLLI:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, i32imm:$c),
                   "slli\t$a,$b,$c",
                   []>;

def SPILLW:
        CoreInstXtensa<(outs),
                   (ins),
                   "spillw\t",
                   []>
{
  let Uses = [XTS_CORE_WB_C, XTS_CORE_WB_N, XTS_CORE_WB_S, XTS_CORE_SAR, XTS_CORE_MS_DISPST];
  let hasSideEffects = 1;
}

def SRA:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b),
                   "sra\t$a,$b",
                   []>
{
  let Uses = [XTS_CORE_SAR];
}

def SRAI:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, i32imm:$c),
                   "srai\t$a,$b,$c",
                   []>;

def SRC:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, AR:$c),
                   "src\t$a,$b,$c",
                   []>
{
  let Uses = [XTS_CORE_SAR];
}

def SRL:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b),
                   "srl\t$a,$b",
                   []>
{
  let Uses = [XTS_CORE_SAR];
}

def SRLI:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, i32imm:$c),
                   "srli\t$a,$b,$c",
                   []>;

def SSA8B:
        CoreInstXtensa<(outs),
                   (ins AR:$a),
                   "ssa8b\t$a",
                   []>
{
  let Defs = [XTS_CORE_SAR];
}

def SSA8L:
        CoreInstXtensa<(outs),
                   (ins AR:$a),
                   "ssa8l\t$a",
                   []>
{
  let Defs = [XTS_CORE_SAR];
}

def SSAI:
        CoreInstXtensa<(outs),
                   (ins i32imm:$a),
                   "ssai\t$a",
                   []>
{
  let Defs = [XTS_CORE_SAR];
}

def SSL:
        CoreInstXtensa<(outs),
                   (ins AR:$a),
                   "ssl\t$a",
                   []>
{
  let Defs = [XTS_CORE_SAR];
}

def SSR:
        CoreInstXtensa<(outs),
                   (ins AR:$a),
                   "ssr\t$a",
                   []>
{
  let Defs = [XTS_CORE_SAR];
}

def SUB:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, AR:$c),
                   "sub\t$a,$b,$c",
                   []>;

def SUBX2:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, AR:$c),
                   "subx2\t$a,$b,$c",
                   []>;

def SUBX4:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, AR:$c),
                   "subx4\t$a,$b,$c",
                   []>;

def SUBX8:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, AR:$c),
                   "subx8\t$a,$b,$c",
                   []>;

def TOSSW:
        CoreInstXtensa<(outs),
                   (ins),
                   "tossw\t",
                   []>
{
  let Defs = [XTS_CORE_WB_N];
  let Uses = [XTS_CORE_WB_N, XTS_CORE_WB_C, XTS_CORE_WB_S, XTS_CORE_SAR];
  let hasSideEffects = 1;
}

def WBTB0:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b),
                   "wbtb0\t$a,$b",
                   []>
{
  let Uses = [XTS_CORE_MS_DISPST, XTS_CORE_PSRING, XTS_CORE_INOCDMODE];
  let hasSideEffects = 1;
}

def WBTB1:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b),
                   "wbtb1\t$a,$b",
                   []>
{
  let Uses = [XTS_CORE_MS_DISPST, XTS_CORE_PSRING, XTS_CORE_INOCDMODE];
  let hasSideEffects = 1;
}

def WBTB2:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b),
                   "wbtb2\t$a,$b",
                   []>
{
  let Uses = [XTS_CORE_MS_DISPST, XTS_CORE_PSRING, XTS_CORE_INOCDMODE];
  let hasSideEffects = 1;
}

def WDTLB:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b),
                   "wdtlb\t$a,$b",
                   []>
{
  let Defs = [XTS_CORE_MPUENB, XTS_CORE_XTSYNC];
  let Uses = [XTS_CORE_PSEXCM, XTS_CORE_PSRING, XTS_CORE_MPUENB];
  let hasSideEffects = 1;
}

def WPTLB:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b),
                   "wptlb\t$a,$b",
                   []>
{
  let Defs = [XTS_CORE_MPUENB, XTS_CORE_XTSYNC];
  let Uses = [XTS_CORE_PSEXCM, XTS_CORE_PSRING, XTS_CORE_MPUENB];
  let hasSideEffects = 1;
}

def UMUL_AA_HH:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b),
                   "umul.aa.hh\t$a,$b",
                   []>,
        Requires<[HasMac16]>
{
  let Defs = [XTS_CORE_ACC];
}

def UMUL_AA_HL:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b),
                   "umul.aa.hl\t$a,$b",
                   []>,
        Requires<[HasMac16]>
{
  let Defs = [XTS_CORE_ACC];
}

def UMUL_AA_LH:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b),
                   "umul.aa.lh\t$a,$b",
                   []>,
        Requires<[HasMac16]>
{
  let Defs = [XTS_CORE_ACC];
}

def UMUL_AA_LL:
        CoreInstXtensa<(outs),
                   (ins AR:$a, AR:$b),
                   "umul.aa.ll\t$a,$b",
                   []>,
        Requires<[HasMac16]>
{
  let Defs = [XTS_CORE_ACC];
}

def WSR_ACCHI:
        CoreInstXtensa<(outs),
                   (ins AR:$a),
                   "wsr.acchi\t$a",
                   []>
{
  let Defs = [XTS_CORE_ACC];
  let Uses = [XTS_CORE_ACC];
}

def WSR_ACCLO:
        CoreInstXtensa<(outs),
                   (ins AR:$a),
                   "wsr.acclo\t$a",
                   []>
{
  let Defs = [XTS_CORE_ACC];
  let Uses = [XTS_CORE_ACC];
}

def WSR_INTENABLE:
        CoreInstXtensa<
                   (outs),
                   (ins AR:$a),
                   "wsr.intenable\t$a",
                   []>
{
  let Defs = [XTS_CORE_INTENABLE];
  let Uses = [XTS_CORE_PSEXCM, XTS_CORE_PSRING];
  let hasSideEffects = 1;
}

def WSR_CCOUNT:
        CoreInstXtensa<(outs),
                   (ins AR:$a),
                   "wsr.ccount\t$a",
                   []>
{
  let hasSideEffects = 1;
  let Defs = [XTS_CORE_CCOUNT, XTS_CORE_XTSYNC];
  let Uses = [XTS_CORE_PSEXCM, XTS_CORE_PSRING];
}

def WSR_LBEG:
        CoreInstXtensa<(outs),
                   (ins AR:$a),
                   "wsr.lbeg\t$a",
                   []>
{
  let Defs = [XTS_CORE_LBEG];
}

def WSR_LCOUNT:
        CoreInstXtensa<(outs),
                   (ins AR:$a),
                   "wsr.lcount\t$a",
                   []>
{
  let Defs = [XTS_CORE_LCOUNT, XTS_CORE_XTSYNC];
}

def WSR_LEND:
        CoreInstXtensa<(outs),
                   (ins AR:$a),
                   "wsr.lend\t$a",
                   []>
{
  let Defs = [XTS_CORE_LEND];
}

def WSR_PS:
        CoreInstXtensa<
                   (outs),
                   (ins AR:$a),
                   "wsr.ps\t$a",
                   []>
{
  let hasSideEffects = 1;
  let Defs = [XTS_CORE_XTSYNC, XTS_CORE_PSENTRYNR, XTS_CORE_PSSS, XTS_CORE_PSSTACK, XTS_CORE_PSRING, XTS_CORE_PSDI, XTS_CORE_PSDIEXC];
  let Uses = [XTS_CORE_INOCDMODE, XTS_CORE_PSRING, XTS_CORE_MS_DISPST];
}

def WSR_SAR:
        CoreInstXtensa<(outs),
                   (ins AR:$a),
                   "wsr.sar\t$a",
                   []>
{
  let Defs = [XTS_CORE_SAR, XTS_CORE_XTSYNC];
}

def XOR:
        CoreInstXtensa<(outs AR:$a),
                   (ins AR:$b, AR:$c),
                   "xor\t$a,$b,$c",
                   []>;

def XPS:
        CoreInstXtensa<
                   (outs AR:$a0),
                   (ins AR:$a1, AR:$b),
                   "xps\t$a0,$b",
                   []>
{
  let Constraints = "$a0 = $a1";
  let hasSideEffects = 1;
  let Defs = [XTS_CORE_PSENTRYNR, XTS_CORE_PSSS, XTS_CORE_PSSTACK, XTS_CORE_PSRING, XTS_CORE_PSDI, XTS_CORE_PSDIEXC];
  let Uses = [XTS_CORE_PSENTRYNR, XTS_CORE_PSSS, XTS_CORE_PSSTACK, XTS_CORE_PSRING, XTS_CORE_PSDI, XTS_CORE_PSDIEXC, XTS_CORE_MS_DISPST, XTS_CORE_INOCDMODE];
}

def XSR_ACCHI:
        CoreInstXtensa<(outs AR:$a0),
                   (ins AR:$a1),
                   "xsr.acchi\t$a0",
                   []>
{
  let Constraints = "$a0 = $a1";
  let Defs = [XTS_CORE_ACC];
  let Uses = [XTS_CORE_ACC];
}

def XSR_ACCLO:
        CoreInstXtensa<(outs AR:$a0),
                   (ins AR:$a1),
                   "xsr.acclo\t$a0",
                   []>
{
  let Constraints = "$a0 = $a1";
  let Defs = [XTS_CORE_ACC];
  let Uses = [XTS_CORE_ACC];
}

def XSR_LBEG:
        CoreInstXtensa<(outs AR:$a0),
                   (ins AR:$a1),
                   "xsr.lbeg\t$a0",
                   []>
{
  let Constraints = "$a0 = $a1";
  let Defs = [XTS_CORE_LBEG];
  let Uses = [XTS_CORE_LBEG];
}

def XSR_LCOUNT:
        CoreInstXtensa<(outs AR:$a0),
                   (ins AR:$a1),
                   "xsr.lcount\t$a0",
                   []>
{
  let Constraints = "$a0 = $a1";
  let Defs = [XTS_CORE_LCOUNT, XTS_CORE_XTSYNC];
  let Uses = [XTS_CORE_LCOUNT];
}

def XSR_LEND:
        CoreInstXtensa<(outs AR:$a0),
                   (ins AR:$a1),
                   "xsr.lend\t$a0",
                   []>
{
  let Constraints = "$a0 = $a1";
  let Defs = [XTS_CORE_LEND];
  let Uses = [XTS_CORE_LEND];
}

def XSR_SAR:
        CoreInstXtensa<(outs AR:$a0),
                   (ins AR:$a1),
                   "xsr.sar\t$a0",
                   []>
{
  let Constraints = "$a0 = $a1";
  let Defs = [XTS_CORE_SAR];
  let Uses = [XTS_CORE_SAR];
}

