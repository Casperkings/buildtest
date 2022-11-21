//===-- RISCVInrinsicsInfo.td - Target Description for RISCV ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file describes the RISCV intrinsics in TableGen format.
//
//===----------------------------------------------------------------------===//


let TargetPrefix = "Tensilica", isTarget = 1 in {

def int_Tensilica_unknown  : Intrinsic<[]>;

}

include "TensilicaCustomIntrinsics.td"

