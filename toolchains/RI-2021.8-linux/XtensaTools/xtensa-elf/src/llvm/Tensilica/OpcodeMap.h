//===-- OpcodeMap.h - Tensilica Opcodes --------------------------- -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
//===----------------------------------------------------------------------===//

#if defined TENSILICA_OPCODE
#   define MAPOPCODE(X,Y,Z) X,
#elif defined XTENSA_ENUM_TO_OPCODE
#   define MAPOPCODE(X,Y,Z) Y,
#elif defined XTENSA_OPCODE_TO_ENUM 
#   define MAPOPCODE(X,Y,Z) {Y, Tensilica::MapOpcode::X},
#elif defined RISCV_ENUM_TO_OPCODE
#   define MAPOPCODE(X,Y,Z) Z,
#elif defined RISCV_OPCODE_TO_ENUM 
#   define MAPOPCODE(X,Y,Z) {Z, Tensilica::MapOpcode::X},
#else
//  error
#   define MAPOPCODE(X,Y,Z)
#   error No define
#endif
//  both Xtensa and RISC
MAPOPCODE(NOP,        Xtensa::NOP,        RISCV::NOP       )
MAPOPCODE(LOOP,       Xtensa::LOOP,       RISCV::XT_LOOP   )
MAPOPCODE(LOOPNEZ,    Xtensa::LOOPNEZ,    RISCV::XT_LOOPNEZ)
MAPOPCODE(LOOPGTZ,    Xtensa::LOOPGTZ,    RISCV::XT_LOOPGTZ)
MAPOPCODE(ENDLOOP,    Xtensa::ENDLOOP,    RISCV::ENDLOOP   )
MAPOPCODE(J,          Xtensa::J,          RISCV::PseudoBR  )
MAPOPCODE(LOOP_SET,   Xtensa::LOOP_SET,   RISCV::LOOP_SET  )
MAPOPCODE(LOOP_DEC,   Xtensa::LOOP_DEC,   RISCV::LOOP_DEC  )
MAPOPCODE(MOVE_PTR,   Xtensa::MOVE_AR,    RISCV::MOVE_AR   )
MAPOPCODE(LOAD_PTR,   Xtensa::L32I,       RISCV::LW        )
MAPOPCODE(STORE_PTR,  Xtensa::S32I,       RISCV::SW        )
MAPOPCODE(ADD,        Xtensa::ADD,        RISCV::ADD       )
MAPOPCODE(SUB,        Xtensa::SUB,        RISCV::SUB       )
MAPOPCODE(SH1ADD,     Xtensa::ADDX2,      RISCV::SH1ADD    )
MAPOPCODE(SH2ADD,     Xtensa::ADDX4,      RISCV::SH2ADD    )
MAPOPCODE(SH3ADD,     Xtensa::ADDX8,      RISCV::SH3ADD    )
MAPOPCODE(SLLI,       Xtensa::SLLI,       RISCV::SLLI      )
MAPOPCODE(ADDI,       Xtensa::ADDI,       RISCV::ADDI      )
MAPOPCODE(LOAD_CONST, Xtensa::LOAD_CONST, RISCV::LOAD_CONST)
MAPOPCODE(MIN,        Xtensa::MIN,        RISCV::MIN       )
MAPOPCODE(MAX,        Xtensa::MAX,        RISCV::MAX       )
MAPOPCODE(MINU,       Xtensa::MINU,       RISCV::MINU      )
MAPOPCODE(MAXU,       Xtensa::MAXU,       RISCV::MAXU      )
MAPOPCODE(SRLI,       Xtensa::SRLI,       RISCV::SRLI      )
MAPOPCODE(BEQ,        Xtensa::BEQ,        RISCV::BEQ       )
MAPOPCODE(BNE,        Xtensa::BNE,        RISCV::BNE       )
MAPOPCODE(BGE,        Xtensa::BGE,        RISCV::BGE       )
MAPOPCODE(BLT,        Xtensa::BLT,        RISCV::BLT       )
MAPOPCODE(BGEU,       Xtensa::BGEU,       RISCV::BGEU      )
MAPOPCODE(BLTU,       Xtensa::BLTU,       RISCV::BLTU      )
MAPOPCODE(LBU,        Xtensa::L8UI,       RISCV::LBU       )
MAPOPCODE(LH,         Xtensa::L16SI,      RISCV::LH        )
MAPOPCODE(LHU,        Xtensa::L16UI,      RISCV::LHU       )

// Xtensa only
#if !defined RISCV_OPCODE_TO_ENUM 
MAPOPCODE(LDAWFI,     Xtensa::LDAWFI,     OPC_NULL         )
MAPOPCODE(SH1SUB,     Xtensa::SUBX2,      OPC_NULL         )
MAPOPCODE(SH2SUB,     Xtensa::SUBX4,      OPC_NULL         )
MAPOPCODE(SH3SUB,     Xtensa::SUBX8,      OPC_NULL         )
MAPOPCODE(CONST16,    Xtensa::CONST16,    OPC_NULL         )
MAPOPCODE(SEXT,       Xtensa::SEXT,       OPC_NULL         )
MAPOPCODE(ABS,        Xtensa::ABS,        OPC_NULL         )
MAPOPCODE(DEPBITS,    Xtensa::DEPBITS,    OPC_NULL         )
MAPOPCODE(MULL,       Xtensa::MULL,       OPC_NULL         )
MAPOPCODE(MULSH,      Xtensa::MULSH,      OPC_NULL         )
MAPOPCODE(MULUH,      Xtensa::MULUH,      OPC_NULL         )
MAPOPCODE(MUL16S,     Xtensa::MUL16S,     OPC_NULL         )
MAPOPCODE(MUL16U,     Xtensa::MUL16U,     OPC_NULL         )
MAPOPCODE(NEG,        Xtensa::NEG,        OPC_NULL         )
MAPOPCODE(SSL,        Xtensa::SSL,        OPC_NULL         )
MAPOPCODE(SRL,        Xtensa::SRL,        OPC_NULL         )
MAPOPCODE(SRA,        Xtensa::SRA,        OPC_NULL         )
MAPOPCODE(SRAI,       Xtensa::SRAI,       OPC_NULL         )
MAPOPCODE(EXTUI,      Xtensa::EXTUI,      OPC_NULL         )
MAPOPCODE(AND,        Xtensa::AND,        OPC_NULL         )
MAPOPCODE(OR,         Xtensa::OR,         OPC_NULL         )
MAPOPCODE(XOR,        Xtensa::XOR,        OPC_NULL         )
MAPOPCODE(SALT,       Xtensa::SALT,       OPC_NULL         )
MAPOPCODE(SALTU,      Xtensa::SALTU,      OPC_NULL         )
MAPOPCODE(MOVEQZ,     Xtensa::MOVEQZ,     OPC_NULL         )
MAPOPCODE(MOVNEZ,     Xtensa::MOVNEZ,     OPC_NULL         )
MAPOPCODE(MOVGEZ,     Xtensa::MOVGEZ,     OPC_NULL         )
MAPOPCODE(MOVLTZ,     Xtensa::MOVLTZ,     OPC_NULL         )
MAPOPCODE(MOVT,       Xtensa::MOVT,       OPC_NULL         )
MAPOPCODE(MOVF,       Xtensa::MOVF,       OPC_NULL         )
MAPOPCODE(MOVARBR,    Xtensa::MOVARBR,    OPC_NULL         )
MAPOPCODE(MOVARBR2,   Xtensa::MOVARBR2,   OPC_NULL         )
MAPOPCODE(MOVARBR4,   Xtensa::MOVARBR4,   OPC_NULL         )
MAPOPCODE(MOVARBR8,   Xtensa::MOVARBR8,   OPC_NULL         )
MAPOPCODE(MOVARBR16,  Xtensa::MOVARBR16,  OPC_NULL         )
MAPOPCODE(BNEZ,       Xtensa::BNEZ,       OPC_NULL         )
MAPOPCODE(BEQZ,       Xtensa::BEQZ,       OPC_NULL         )
MAPOPCODE(BGEZ,       Xtensa::BGEZ,       OPC_NULL         )
MAPOPCODE(BLTZ,       Xtensa::BLTZ,       OPC_NULL         )
MAPOPCODE(BALL,       Xtensa::BALL,       OPC_NULL         )
MAPOPCODE(BNALL,      Xtensa::BNALL,      OPC_NULL         )
MAPOPCODE(BNONE,      Xtensa::BNONE,      OPC_NULL         )
MAPOPCODE(BANY,       Xtensa::BANY,       OPC_NULL         )
MAPOPCODE(BBC,        Xtensa::BBC,        OPC_NULL         )
MAPOPCODE(BBCI,       Xtensa::BBCI,       OPC_NULL         )
MAPOPCODE(BBS,        Xtensa::BBS,        OPC_NULL         )
MAPOPCODE(BBSI,       Xtensa::BBSI,       OPC_NULL         )
MAPOPCODE(BEQI,       Xtensa::BEQI,       OPC_NULL         )
MAPOPCODE(BNEI,       Xtensa::BNEI,       OPC_NULL         )
MAPOPCODE(BGEI,       Xtensa::BGEI,       OPC_NULL         )
MAPOPCODE(BLTI,       Xtensa::BLTI,       OPC_NULL         )
MAPOPCODE(BGEUI,      Xtensa::BGEUI,      OPC_NULL         )
MAPOPCODE(BLTUI,      Xtensa::BLTUI,      OPC_NULL         )
MAPOPCODE(SB,         Xtensa::S8I,        OPC_NULL         )
MAPOPCODE(SH,         Xtensa::S16I,       OPC_NULL         )
MAPOPCODE(L32R,       Xtensa::L32R,       OPC_NULL         )
#if defined XTENSA_OPCODE_TO_ENUM 
// Duplicate
MAPOPCODE(ADDI,       Xtensa::ADDMI,      OPC_NULL         )
MAPOPCODE(LOAD_CONST, Xtensa::MOVI,       OPC_NULL         )
#endif
#endif // Xtensa only
// RISCV only
#if !defined XTENSA_OPCODE_TO_ENUM 
MAPOPCODE(ANDI,       OPC_NULL,           RISCV::ANDI      )
MAPOPCODE(LB,         OPC_NULL,           RISCV::LB        )
MAPOPCODE(SEXT_B,     OPC_NULL,           RISCV::SEXT_B    )
MAPOPCODE(SEXT_H,     OPC_NULL,           RISCV::SEXT_H    )
MAPOPCODE(ZEXT_H,     OPC_NULL,           RISCV::ZEXT_H    )
MAPOPCODE(ANDN,       OPC_NULL,           RISCV::ANDN      )
MAPOPCODE(ORI,        OPC_NULL,           RISCV::ORI       )
MAPOPCODE(ORN,        OPC_NULL,           RISCV::ORN       )
MAPOPCODE(XNOR,       OPC_NULL,           RISCV::XNOR      )
MAPOPCODE(CLZ,        OPC_NULL,           RISCV::CLZ       )
MAPOPCODE(CTZ,        OPC_NULL,           RISCV::CTZ       )
MAPOPCODE(CPOP,       OPC_NULL,           RISCV::CPOP      )
MAPOPCODE(ORC_B,      OPC_NULL,           RISCV::ORC_B     )
MAPOPCODE(REV8,       OPC_NULL,           RISCV::REV8      )
MAPOPCODE(ROL,        OPC_NULL,           RISCV::ROL       )
MAPOPCODE(ROR,        OPC_NULL,           RISCV::ROR       )
MAPOPCODE(RORI,       OPC_NULL,           RISCV::RORI      )
#if defined RISCV_OPCODE_TO_ENUM 
// Duplicate
#endif
#endif // RISCV only

#undef MAPOPCODE
#undef TENSILICA_OPCODE
#undef XTENSA_ENUM_TO_OPCODE
#undef XTENSA_OPCODE_TO_ENUM 
#undef RISCV_ENUM_TO_OPCODE
#undef RISCV_OPCODE_TO_ENUM 
