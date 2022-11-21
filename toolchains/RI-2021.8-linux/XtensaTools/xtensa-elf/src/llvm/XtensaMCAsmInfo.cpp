//===-- XtensaMCAsmInfo.cpp - Xtensa asm properties------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/XtensaMCAsmInfo.h"
using namespace llvm;

void XtensaMCAsmInfo::anchor() {}

XtensaMCAsmInfo::XtensaMCAsmInfo(const Triple &TT) {
  bool BigEndian = false;
#define GET_ENDIAN
#include "TensilicaGenFeatures.inc"
#undef GET_ENDIAN
  IsLittleEndian = !BigEndian;
  SupportsDebugInformation = true;
  Data16bitsDirective = "\t.short\t";
  Data32bitsDirective = "\t.long\t";
  Data64bitsDirective = nullptr;
  ZeroDirective = "\t.space\t";
  CommentString = "#";

  AscizDirective = "\t.string\t";

  UsesELFSectionDirectiveForBSS = true;

  // Debug
  ExceptionsType = ExceptionHandling::DwarfCFI;
  DwarfRegNumForCFI = true;
}
