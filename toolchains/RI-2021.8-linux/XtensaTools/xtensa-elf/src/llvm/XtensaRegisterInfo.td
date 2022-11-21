//===-- XtensaRegisterInfo.td - Xtensa Register defs ---------*- tablegen--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
//  Declarations that describe the Xtensa register file
//===----------------------------------------------------------------------===//

class XtensaReg<string n> : Register<n> {
  let Namespace = "Xtensa";
}

class TensilicaReg<string n> : Register<n> {
  let Namespace = "Xtensa";
}

//===----------------------------------------------------------------------===//
// Register definitions for regfile AR
//===----------------------------------------------------------------------===//

def LR : XtensaReg<"a0">, DwarfRegNum<[ 0 ]>;
def SP : XtensaReg<"a1">, DwarfRegNum<[ 1 ]>;
def a2 : XtensaReg<"a2">, DwarfRegNum<[ 2 ]>;
def a3 : XtensaReg<"a3">, DwarfRegNum<[ 3 ]>;
def a4 : XtensaReg<"a4">, DwarfRegNum<[ 4 ]>;
def a5 : XtensaReg<"a5">, DwarfRegNum<[ 5 ]>;
def a6 : XtensaReg<"a6">, DwarfRegNum<[ 6 ]>;
def a7 : XtensaReg<"a7">, DwarfRegNum<[ 7 ]>;
def a8 : XtensaReg<"a8">, DwarfRegNum<[ 8 ]>;
def a9 : XtensaReg<"a9">, DwarfRegNum<[ 9 ]>;
def a10 : XtensaReg<"a10">, DwarfRegNum<[ 10 ]>;
def a11 : XtensaReg<"a11">, DwarfRegNum<[ 11 ]>;
def a12 : XtensaReg<"a12">, DwarfRegNum<[ 12 ]>;
def a13 : XtensaReg<"a13">, DwarfRegNum<[ 13 ]>;
def a14 : XtensaReg<"a14">, DwarfRegNum<[ 14 ]>;
def a15 : XtensaReg<"a15">, DwarfRegNum<[ 15 ]>;

//===----------------------------------------------------------------------===//
// Register definitions for regfile MR
//===----------------------------------------------------------------------===//

def m_0 : XtensaReg<"m0">;
def m_1 : XtensaReg<"m1">;
def m_2 : XtensaReg<"m2">;
def m_3 : XtensaReg<"m3">;

//===----------------------------------------------------------------------===//
// State definitions
//===----------------------------------------------------------------------===//

def XTS_CORE_ASID1 : XtensaReg<"ASID1">;
def XTS_CORE_ASID2 : XtensaReg<"ASID2">;
def XTS_CORE_ASID3 : XtensaReg<"ASID3">;
def XTS_CORE_LCOUNT : XtensaReg<"LCOUNT">;
def XTS_CORE_PC : XtensaReg<"PC">;
def XTS_CORE_ICOUNT : XtensaReg<"ICOUNT">;
def XTS_CORE_DDR : XtensaReg<"DDR">;
def XTS_CORE_INTERRUPT : XtensaReg<"INTERRUPT">;
def XTS_CORE_CCOUNT : XtensaReg<"CCOUNT">;
def XTS_CORE_XTSYNC : XtensaReg<"XTSYNC">;
def XTS_CORE_VECBASE : XtensaReg<"VECBASE">;
def XTS_CORE_VECBASELOCK : XtensaReg<"VECBASELOCK">;
def XTS_CORE_EPC1 : XtensaReg<"EPC1">;
def XTS_CORE_EPC2 : XtensaReg<"EPC2">;
def XTS_CORE_EPC3 : XtensaReg<"EPC3">;
def XTS_CORE_EPC4 : XtensaReg<"EPC4">;
def XTS_CORE_EPC5 : XtensaReg<"EPC5">;
def XTS_CORE_EPC6 : XtensaReg<"EPC6">;
def XTS_CORE_EPC7 : XtensaReg<"EPC7">;
def XTS_CORE_EXCSAVE1 : XtensaReg<"EXCSAVE1">;
def XTS_CORE_EXCSAVE2 : XtensaReg<"EXCSAVE2">;
def XTS_CORE_EXCSAVE3 : XtensaReg<"EXCSAVE3">;
def XTS_CORE_EXCSAVE4 : XtensaReg<"EXCSAVE4">;
def XTS_CORE_EXCSAVE5 : XtensaReg<"EXCSAVE5">;
def XTS_CORE_EXCSAVE6 : XtensaReg<"EXCSAVE6">;
def XTS_CORE_EXCSAVE7 : XtensaReg<"EXCSAVE7">;
def XTS_CORE_EPS2 : XtensaReg<"EPS2">;
def XTS_CORE_EPS3 : XtensaReg<"EPS3">;
def XTS_CORE_EPS4 : XtensaReg<"EPS4">;
def XTS_CORE_EPS5 : XtensaReg<"EPS5">;
def XTS_CORE_EPS6 : XtensaReg<"EPS6">;
def XTS_CORE_EPS7 : XtensaReg<"EPS7">;
def XTS_CORE_EXCCAUSE : XtensaReg<"EXCCAUSE">;
def XTS_CORE_PSINTLEVEL : XtensaReg<"PSINTLEVEL">;
def XTS_CORE_PSUM : XtensaReg<"PSUM">;
def XTS_CORE_PSWOE : XtensaReg<"PSWOE">;
def XTS_CORE_PSRING : XtensaReg<"PSRING">;
def XTS_CORE_PSEXCM : XtensaReg<"PSEXCM">;
def XTS_CORE_DEPC : XtensaReg<"DEPC">;
def XTS_CORE_EXCVADDR : XtensaReg<"EXCVADDR">;
def XTS_CORE_WINDOWBASE : XtensaReg<"WindowBase">;
def XTS_CORE_WINDOWSTART : XtensaReg<"WindowStart">;
def XTS_CORE_PSCALLINC : XtensaReg<"PSCALLINC">;
def XTS_CORE_PSOWB : XtensaReg<"PSOWB">;
def XTS_CORE_MECR : XtensaReg<"MECR">;
def XTS_CORE_MEPC : XtensaReg<"MEPC">;
def XTS_CORE_MEPS : XtensaReg<"MEPS">;
def XTS_CORE_MESAVE : XtensaReg<"MESAVE">;
def XTS_CORE_MEVADDR : XtensaReg<"MEVADDR">;
def XTS_CORE_MESRERRENAB : XtensaReg<"MESRErrEnab">;
def XTS_CORE_MESRERRTEST : XtensaReg<"MESRErrTest">;
def XTS_CORE_MESRMEME : XtensaReg<"MESRMemE">;
def XTS_CORE_MESRDME : XtensaReg<"MESRDME">;
def XTS_CORE_MESRMEMTYPE : XtensaReg<"MESRMemType">;
def XTS_CORE_MESRACCTYPE : XtensaReg<"MESRAccType">;
def XTS_CORE_MESRERRTYPE : XtensaReg<"MESRErrType">;
def XTS_CORE_MESRWAY : XtensaReg<"MESRWay">;
def XTS_CORE_MESRDATAEXC : XtensaReg<"MESRDataExc">;
def XTS_CORE_MESRINSTEXC : XtensaReg<"MESRInstExc">;
def XTS_CORE_MESRRCE : XtensaReg<"MESRRCE">;
def XTS_CORE_MESRDLCE : XtensaReg<"MESRDLCE">;
def XTS_CORE_MESRILCE : XtensaReg<"MESRILCE">;
def XTS_CORE_LBEG : XtensaReg<"LBEG">;
def XTS_CORE_LEND : XtensaReg<"LEND">;
def XTS_CORE_LITBADDR : XtensaReg<"LITBADDR">;
def XTS_CORE_LITBEN : XtensaReg<"LITBEN">;
def XTS_CORE_MEMCTL : XtensaReg<"MEMCTL">;
def XTS_CORE_SAR : XtensaReg<"SAR">;
def XTS_CORE_MISC0 : XtensaReg<"MISC0">;
def XTS_CORE_MISC1 : XtensaReg<"MISC1">;
def XTS_CORE_MISC2 : XtensaReg<"MISC2">;
def XTS_CORE_MISC3 : XtensaReg<"MISC3">;
def XTS_CORE_ACC : XtensaReg<"ACC">;
def XTS_CORE_THREADPTR : XtensaReg<"THREADPTR">;
def XTS_CORE_MPUNUMENTRIES : XtensaReg<"MPUNUMENTRIES">;
def XTS_CORE_INOCDMODE : XtensaReg<"InOCDMode">;
def XTS_CORE_INTENABLE : XtensaReg<"INTENABLE">;
def XTS_CORE_DBREAKA0 : XtensaReg<"DBREAKA0">;
def XTS_CORE_DBREAKC0 : XtensaReg<"DBREAKC0">;
def XTS_CORE_DBREAKC_SG0 : XtensaReg<"DBREAKC_SG0">;
def XTS_CORE_DBREAKA1 : XtensaReg<"DBREAKA1">;
def XTS_CORE_DBREAKC1 : XtensaReg<"DBREAKC1">;
def XTS_CORE_DBREAKC_SG1 : XtensaReg<"DBREAKC_SG1">;
def XTS_CORE_IBREAKA0 : XtensaReg<"IBREAKA0">;
def XTS_CORE_IBREAKA1 : XtensaReg<"IBREAKA1">;
def XTS_CORE_IBREAKENABLE : XtensaReg<"IBREAKENABLE">;
def XTS_CORE_ICOUNTLEVEL : XtensaReg<"ICOUNTLEVEL">;
def XTS_CORE_DEBUGCAUSE : XtensaReg<"DEBUGCAUSE">;
def XTS_CORE_DBNUM : XtensaReg<"DBNUM">;
def XTS_CORE_CCOMPARE0 : XtensaReg<"CCOMPARE0">;
def XTS_CORE_CCOMPARE1 : XtensaReg<"CCOMPARE1">;
def XTS_CORE_CCOMPARE2 : XtensaReg<"CCOMPARE2">;
def XTS_CORE_CACHEADRDIS : XtensaReg<"CACHEADRDIS">;
def XTS_CORE_MPUENB : XtensaReg<"MPUENB">;
def XTS_CORE_CPENABLE : XtensaReg<"CPENABLE">;
def XTS_CORE_ATOMCTL : XtensaReg<"ATOMCTL">;
def XTS_CORE_ERI_RAW_INTERLOCK : XtensaReg<"ERI_RAW_INTERLOCK">;
def XTS_CORE_ERACCESS : XtensaReg<"ERACCESS">;
def XTS_CORE_SCOMPARE1 : XtensaReg<"SCOMPARE1">;
def XTS_CORE_GSERR : XtensaReg<"GSERR">;
def XTS_CORE_GSMES : XtensaReg<"GSMES">;
def XTS_CORE_GSMEA : XtensaReg<"GSMEA">;
def XTS_CORE_INSTPGSZID4 : XtensaReg<"INSTPGSZID4">;
def XTS_CORE_INSTPGSZID5 : XtensaReg<"INSTPGSZID5">;
def XTS_CORE_INSTPGSZID6 : XtensaReg<"INSTPGSZID6">;
def XTS_CORE_DATAPGSZID4 : XtensaReg<"DATAPGSZID4">;
def XTS_CORE_DATAPGSZID5 : XtensaReg<"DATAPGSZID5">;
def XTS_CORE_DATAPGSZID6 : XtensaReg<"DATAPGSZID6">;
def XTS_CORE_PTBASE : XtensaReg<"PTBASE">;
def XTS_CORE_PSNSM : XtensaReg<"PSNSM">;
def XTS_CORE_PSPNSM : XtensaReg<"PSPNSM">;
def XTS_CORE_MPULOCK : XtensaReg<"MPULOCK">;
def XTS_CORE_OPMODEECCFENCE : XtensaReg<"OPMODEECCFENCE">;
def XTS_CORE_CSRPCTL : XtensaReg<"CSRPCTL">;
def XTS_CORE_CSRPCTLENABLE : XtensaReg<"CSRPCTLENABLE">;
def XTS_CORE_CSRPCTLTEST : XtensaReg<"CSRPCTLTEST">;
def XTS_CORE_CSRPCTLHALT : XtensaReg<"CSRPCTLHALT">;
def XTS_CORE_CSRPCTLSYNCRAR : XtensaReg<"CSRPCTLSYNCRAR">;
def XTS_CORE_CSRPCTLSYNCRAW : XtensaReg<"CSRPCTLSYNCRAW">;
def XTS_CORE_CSRPCTLSYNCWAW : XtensaReg<"CSRPCTLSYNCWAW">;
def XTS_CORE_VADDR0 : XtensaReg<"VADDR0">;
def XTS_CORE_VADDR1 : XtensaReg<"VADDR1">;
def XTS_CORE_VADDRSTATUS : XtensaReg<"VADDRSTATUS">;

// Core stats from NX pipeline
def XTS_CORE_EPC : XtensaReg<"EPC">;
def XTS_CORE_IEEXTERN : XtensaReg<"IEEXTERN">;
def XTS_CORE_IEVEC : XtensaReg<"IEVEC">;
def XTS_CORE_PSDIEXC : XtensaReg<"PSDIEXC">;
def XTS_CORE_PSDI : XtensaReg<"PSDI">;
def XTS_CORE_PSSTACK : XtensaReg<"PSSTACK">;
def XTS_CORE_MS_DISPST : XtensaReg<"MS_DISPST">;
def XTS_CORE_MS_DE : XtensaReg<"MS_DE">;
def XTS_CORE_EXCINF : XtensaReg<"EXCINF">;
def XTS_CORE_MESR : XtensaReg<"MESR">;
def XTS_CORE_ISB : XtensaReg<"ISB">;
def XTS_CORE_ISL : XtensaReg<"ISL">;
def XTS_CORE_KSL : XtensaReg<"KSL">;
def XTS_CORE_ACTIVEPRIORITY : XtensaReg<"ACTIVEPRIORITY">;
def XTS_CORE_ACTIVEINTERRUPT : XtensaReg<"ACTIVEINTERRUPT">;
def XTS_CORE_ACTIVEFAIRNESS : XtensaReg<"ACTIVEFAIRNESS">;
def XTS_CORE_SECONDPRIORITY : XtensaReg<"SECONDPRIORITY">;
def XTS_CORE_SECONDINTERRUPT : XtensaReg<"SECONDINTERRUPT">;
def XTS_CORE_SECONDFAIRNESS : XtensaReg<"SECONDFAIRNESS">;
def XTS_CORE_CURRENTPRIORITY : XtensaReg<"CURRENTDPRIORITY">;
def XTS_CORE_CURRENTDINTERRUPT : XtensaReg<"CURRENTDINTERRUPT">;
def XTS_CORE_CURRENTDFAIRNESS : XtensaReg<"CURRENTDFAIRNESS">;
def XTS_CORE_IBREAKC0 : XtensaReg<"IBREAKC0">;
def XTS_CORE_IBREAKC1 : XtensaReg<"IBREAKC1">;
def XTS_CORE_PSSS : XtensaReg<"PSSS">;
def XTS_CORE_MESRCLR : XtensaReg<"MESRCLR">;
def XTS_CORE_PSENTRYNR : XtensaReg<"PSEENTRYNR">;
def XTS_CORE_WB_S : XtensaReg<"WB_S">;
def XTS_CORE_WB_P : XtensaReg<"WB_P">;
def XTS_CORE_WB_C : XtensaReg<"WB_C">;
def XTS_CORE_WB_N : XtensaReg<"WB_N">;
def XTS_CORE_WB_T : XtensaReg<"WB_T">;
def XTS_CORE_WB_M : XtensaReg<"WB_M">;
def XTS_CORE_REV8AR : XtensaReg<"REV8AR">;
def XTS_CORE_APB0CFG : XtensaReg<"APB0CFG">;
// End of core states from NX

// Register classes.
//

def MR : RegisterClass<"Xtensa", [ i32 ], 32, (sequence "m_%u", 0, 3)> {
  let isAllocatable = 0;
}

def CORE_ASID1_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_ASID1)> {
  let isAllocatable = 0;
}
def CORE_ASID2_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_ASID2)> {
  let isAllocatable = 0;
}
def CORE_ASID3_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_ASID3)> {
  let isAllocatable = 0;
}
def CORE_LCOUNT_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_LCOUNT)> {
  let isAllocatable = 0;
}
def CORE_PC_RegClass : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_PC)> {
  let isAllocatable = 0;
}
def CORE_ICOUNT_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_ICOUNT)> {
  let isAllocatable = 0;
}
def CORE_DDR_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_DDR)> {
  let isAllocatable = 0;
}
def CORE_INTERRUPT_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 10, (add XTS_CORE_INTERRUPT)> {
  let isAllocatable = 0;
}
def CORE_CCOUNT_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_CCOUNT)> {
  let isAllocatable = 0;
}
def CORE_XTSYNC_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 1, (add XTS_CORE_XTSYNC)> {
  let isAllocatable = 0;
}
def CORE_VECBASE_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 22, (add XTS_CORE_VECBASE)> {
  let isAllocatable = 0;
}
def CORE_VECBASELOCK_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 22, (add XTS_CORE_VECBASELOCK)> {
  let isAllocatable = 0;
}
def CORE_EPC1_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_EPC1)> {
  let isAllocatable = 0;
}
def CORE_EPC2_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_EPC2)> {
  let isAllocatable = 0;
}
def CORE_EPC3_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_EPC3)> {
  let isAllocatable = 0;
}
def CORE_EPC4_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_EPC4)> {
  let isAllocatable = 0;
}
def CORE_EPC5_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_EPC5)> {
  let isAllocatable = 0;
}
def CORE_EPC6_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_EPC6)> {
  let isAllocatable = 0;
}
def CORE_EPC7_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_EPC7)> {
  let isAllocatable = 0;
}
def CORE_EXCSAVE1_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_EXCSAVE1)> {
  let isAllocatable = 0;
}
def CORE_EXCSAVE2_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_EXCSAVE2)> {
  let isAllocatable = 0;
}
def CORE_EXCSAVE3_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_EXCSAVE3)> {
  let isAllocatable = 0;
}
def CORE_EXCSAVE4_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_EXCSAVE4)> {
  let isAllocatable = 0;
}
def CORE_EXCSAVE5_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_EXCSAVE5)> {
  let isAllocatable = 0;
}
def CORE_EXCSAVE6_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_EXCSAVE6)> {
  let isAllocatable = 0;
}
def CORE_EXCSAVE7_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_EXCSAVE7)> {
  let isAllocatable = 0;
}
def CORE_EPS2_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 13, (add XTS_CORE_EPS2)> {
  let isAllocatable = 0;
}
def CORE_EPS3_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 13, (add XTS_CORE_EPS3)> {
  let isAllocatable = 0;
}
def CORE_EPS4_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 15, (add XTS_CORE_EPS4)> {
  let isAllocatable = 0;
}
def CORE_EPS5_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 8, (add XTS_CORE_EPS5)> {
  let isAllocatable = 0;
}
def CORE_EPS6_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 8, (add XTS_CORE_EPS6)> {
  let isAllocatable = 0;
}
def CORE_EPS7_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 8, (add XTS_CORE_EPS7)> {
  let isAllocatable = 0;
}
def CORE_EXCCAUSE_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 6, (add XTS_CORE_EXCCAUSE)> {
  let isAllocatable = 0;
}
def CORE_PSINTLEVEL_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 4, (add XTS_CORE_PSINTLEVEL)> {
  let isAllocatable = 0;
}
def CORE_PSUM_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 1, (add XTS_CORE_PSUM)> {
  let isAllocatable = 0;
}
def CORE_PSWOE_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 1, (add XTS_CORE_PSWOE)> {
  let isAllocatable = 0;
}
def CORE_PSRING_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 2, (add XTS_CORE_PSRING)> {
  let isAllocatable = 0;
}
def CORE_PSEXCM_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 1, (add XTS_CORE_PSEXCM)> {
  let isAllocatable = 0;
}
def CORE_DEPC_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_DEPC)> {
  let isAllocatable = 0;
}
def CORE_EXCVADDR_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_EXCVADDR)> {
  let isAllocatable = 0;
}
def CORE_WINDOWBASE_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 4, (add XTS_CORE_WINDOWBASE)> {
  let isAllocatable = 0;
}
def CORE_WINDOWSTART_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 16, (add XTS_CORE_WINDOWSTART)> {
  let isAllocatable = 0;
}
def CORE_PSCALLINC_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 2, (add XTS_CORE_PSCALLINC)> {
  let isAllocatable = 0;
}
def CORE_PSOWB_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 4, (add XTS_CORE_PSOWB)> {
  let isAllocatable = 0;
}
def CORE_MECR_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 7, (add XTS_CORE_MECR)> {
  let isAllocatable = 0;
}
def CORE_MEPC_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_MEPC)> {
  let isAllocatable = 0;
}
def CORE_MEPS_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 15, (add XTS_CORE_MEPS)> {
  let isAllocatable = 0;
}
def CORE_MESAVE_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_MESAVE)> {
  let isAllocatable = 0;
}
def CORE_MEVADDR_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_MEVADDR)> {
  let isAllocatable = 0;
}
def CORE_MESRERRENAB_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 1, (add XTS_CORE_MESRERRENAB)> {
  let isAllocatable = 0;
}
def CORE_MESRERRTEST_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 1, (add XTS_CORE_MESRERRTEST)> {
  let isAllocatable = 0;
}
def CORE_MESRMEME_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 1, (add XTS_CORE_MESRMEME)> {
  let isAllocatable = 0;
}
def CORE_MESRDME_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 1, (add XTS_CORE_MESRDME)> {
  let isAllocatable = 0;
}
def CORE_MESRMEMTYPE_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 4, (add XTS_CORE_MESRMEMTYPE)> {
  let isAllocatable = 0;
}
def CORE_MESRACCTYPE_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 2, (add XTS_CORE_MESRACCTYPE)> {
  let isAllocatable = 0;
}
def CORE_MESRERRTYPE_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 2, (add XTS_CORE_MESRERRTYPE)> {
  let isAllocatable = 0;
}
def CORE_MESRWAY_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 2, (add XTS_CORE_MESRWAY)> {
  let isAllocatable = 0;
}
def CORE_MESRDATAEXC_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 1, (add XTS_CORE_MESRDATAEXC)> {
  let isAllocatable = 0;
}
def CORE_MESRINSTEXC_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 1, (add XTS_CORE_MESRINSTEXC)> {
  let isAllocatable = 0;
}
def CORE_MESRRCE_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 1, (add XTS_CORE_MESRRCE)> {
  let isAllocatable = 0;
}
def CORE_MESRDLCE_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 1, (add XTS_CORE_MESRDLCE)> {
  let isAllocatable = 0;
}
def CORE_MESRILCE_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 1, (add XTS_CORE_MESRILCE)> {
  let isAllocatable = 0;
}
def CORE_LBEG_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_LBEG)> {
  let isAllocatable = 0;
}
def CORE_LEND_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_LEND)> {
  let isAllocatable = 0;
}
def CORE_LITBADDR_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_LITBADDR)> {
  let isAllocatable = 0;
}
def CORE_LITBEN_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_LITBEN)> {
  let isAllocatable = 0;
}
def CORE_MEMCTL_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 24, (add XTS_CORE_MEMCTL)> {
  let isAllocatable = 0;
}
def CORE_SAR_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 6, (add XTS_CORE_SAR)> {
  let isAllocatable = 0;
}
def CORE_MISC0_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_MISC0)> {
  let isAllocatable = 0;
}
def CORE_MISC1_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_MISC1)> {
  let isAllocatable = 0;
}
def CORE_MISC2_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_MISC2)> {
  let isAllocatable = 0;
}
def CORE_MISC3_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_MISC3)> {
  let isAllocatable = 0;
}
def CORE_ACC_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 40, (add XTS_CORE_ACC)> {
  let isAllocatable = 0;
}
def CORE_THREADPTR_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_THREADPTR)> {
  let isAllocatable = 0;
}
def CORE_MPUNUMENTRIES_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 6, (add XTS_CORE_MPUNUMENTRIES)> {
  let isAllocatable = 0;
}
def CORE_INOCDMODE_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 1, (add XTS_CORE_INOCDMODE)> {
  let isAllocatable = 0;
}
def CORE_INTENABLE_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 25, (add XTS_CORE_INTENABLE)> {
  let isAllocatable = 0;
}
def CORE_DBREAKA0_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_DBREAKA0)> {
  let isAllocatable = 0;
}
def CORE_DBREAKC0_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 8, (add XTS_CORE_DBREAKC0)> {
  let isAllocatable = 0;
}
def CORE_DBREAKC_SG0_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 2, (add XTS_CORE_DBREAKC_SG0)> {
  let isAllocatable = 0;
}
def CORE_DBREAKA1_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_DBREAKA1)> {
  let isAllocatable = 0;
}
def CORE_DBREAKC1_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 8, (add XTS_CORE_DBREAKC1)> {
  let isAllocatable = 0;
}
def CORE_DBREAKC_SG1_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 2, (add XTS_CORE_DBREAKC_SG1)> {
  let isAllocatable = 0;
}
def CORE_IBREAKA0_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_IBREAKA0)> {
  let isAllocatable = 0;
}
def CORE_IBREAKA1_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_IBREAKA1)> {
  let isAllocatable = 0;
}
def CORE_IBREAKENABLE_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 2, (add XTS_CORE_IBREAKENABLE)> {
  let isAllocatable = 0;
}
def CORE_ICOUNTLEVEL_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 4, (add XTS_CORE_ICOUNTLEVEL)> {
  let isAllocatable = 0;
}
def CORE_DEBUGCAUSE_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 6, (add XTS_CORE_DEBUGCAUSE)> {
  let isAllocatable = 0;
}
def CORE_DBNUM_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 4, (add XTS_CORE_DBNUM)> {
  let isAllocatable = 0;
}
def CORE_CCOMPARE0_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_CCOMPARE0)> {
  let isAllocatable = 0;
}
def CORE_CCOMPARE1_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_CCOMPARE1)> {
  let isAllocatable = 0;
}
def CORE_CCOMPARE2_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_CCOMPARE2)> {
  let isAllocatable = 0;
}
def CORE_CACHEADRDIS_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 8, (add XTS_CORE_CACHEADRDIS)> {
  let isAllocatable = 0;
}
def CORE_MPUENB_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 16, (add XTS_CORE_MPUENB)> {
  let isAllocatable = 0;
}
def CORE_CPENABLE_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 3, (add XTS_CORE_CPENABLE)> {
  let isAllocatable = 0;
}
def CORE_ATOMCTL_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 9, (add XTS_CORE_ATOMCTL)> {
  let isAllocatable = 0;
}
def CORE_ERI_RAW_INTERLOCK_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 1, (add XTS_CORE_ERI_RAW_INTERLOCK)> {
  let isAllocatable = 0;
}
def CORE_ERACCESS_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 16, (add XTS_CORE_ERACCESS)> {
  let isAllocatable = 0;
}
def CORE_SCOMPARE1_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_SCOMPARE1)> {
  let isAllocatable = 0;
}
def CORE_GSERR_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_GSERR)> {
  let isAllocatable = 0;
}
def CORE_GSMES_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_GSMES)> {
  let isAllocatable = 0;
}
def CORE_GSMEA_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_GSMEA)> {
  let isAllocatable = 0;
}
def CORE_INSTPGSZID4_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_INSTPGSZID4)> {
  let isAllocatable = 0;
}
def CORE_INSTPGSZID5_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_INSTPGSZID5)> {
  let isAllocatable = 0;
}
def CORE_INSTPGSZID6_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_INSTPGSZID6)> {
  let isAllocatable = 0;
}
def CORE_DATAPGSZID4_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_DATAPGSZID4)> {
  let isAllocatable = 0;
}
def CORE_DATAPGSZID5_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_DATAPGSZID5)> {
  let isAllocatable = 0;
}
def CORE_DATAPGSZID6_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_DATAPGSZID6)> {
  let isAllocatable = 0;
}
def CORE_PTBASE_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_PTBASE)> {
  let isAllocatable = 0;
}
def CORE_PSNSM_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_PSNSM)> {
  let isAllocatable = 0;
}
def CORE_PSPNSM_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_PSPNSM)> {
  let isAllocatable = 0;
}
def CORE_MPULOCK_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_MPULOCK)> {
  let isAllocatable = 0;
}
def CORE_OPMODEECCFENCE_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_OPMODEECCFENCE)> {
  let isAllocatable = 0;
}
def CORE_CSRPCTL_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_CSRPCTL)> {
  let isAllocatable = 0;
}
def CORE_CSRPCTLHALT_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_CSRPCTLHALT)> {
  let isAllocatable = 0;
}
def CORE_CSRPCTLTEST_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_CSRPCTLTEST)> {
  let isAllocatable = 0;
}
def CORE_CSRPCTLENABLE_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_CSRPCTLENABLE)> {
  let isAllocatable = 0;
}
def CORE_CSRPCTLSYNCRAR_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_CSRPCTLSYNCRAR)> {
  let isAllocatable = 0;
}
def CORE_CSRPCTLSYNCRAW_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_CSRPCTLSYNCRAW)> {
  let isAllocatable = 0;
}
def CORE_CSRPCTLSYNCWAW_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_CSRPCTLSYNCWAW)> {
  let isAllocatable = 0;
}
def CORE_VADDRSTATUS_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_VADDRSTATUS)> {
  let isAllocatable = 0;
}
def CORE_VADDR0_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_VADDR0)> {
  let isAllocatable = 0;
}
def CORE_VADDR1_RegClass
    : RegisterClass<"Xtensa", [ i32 ], 32, (add XTS_CORE_VADDR1)> {
  let isAllocatable = 0;
}

def NX_CORE_STATES
    : RegisterClass<"Xtensa", [ i32 ], 32,
                    (add XTS_CORE_EPC, XTS_CORE_IEEXTERN, XTS_CORE_IEVEC,
                     XTS_CORE_PSDIEXC, XTS_CORE_PSDI, XTS_CORE_PSSTACK,
                     XTS_CORE_MS_DISPST, XTS_CORE_MS_DE, XTS_CORE_EXCINF,
                     XTS_CORE_MESR, XTS_CORE_ISB, XTS_CORE_ISL, XTS_CORE_KSL,
                     XTS_CORE_ACTIVEPRIORITY, XTS_CORE_ACTIVEINTERRUPT,
                     XTS_CORE_ACTIVEFAIRNESS, XTS_CORE_SECONDPRIORITY,
                     XTS_CORE_SECONDINTERRUPT, XTS_CORE_SECONDFAIRNESS,
                     XTS_CORE_CURRENTPRIORITY, XTS_CORE_CURRENTDINTERRUPT,
                     XTS_CORE_CURRENTDFAIRNESS, XTS_CORE_IBREAKC0,
                     XTS_CORE_IBREAKC1, XTS_CORE_PSSS, XTS_CORE_MESRCLR,
                     XTS_CORE_PSENTRYNR, XTS_CORE_WB_S, XTS_CORE_WB_P,
                     XTS_CORE_WB_C, XTS_CORE_WB_N, XTS_CORE_WB_T, XTS_CORE_WB_M,
                     XTS_CORE_REV8AR, XTS_CORE_APB0CFG)> {
  let isAllocatable = 0;
}
