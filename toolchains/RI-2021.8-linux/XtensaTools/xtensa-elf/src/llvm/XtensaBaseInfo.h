//===-- XtensaBaseInfo.h - Top level definitions for Xtensa----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains small standalone helper functions and enum definitions for
// the Xtensa target useful for the compiler back-end and the MC libraries.
// As such, it deliberately does not include references to LLVM core
// code gen types, passes, etc..
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_XTENSA_XTENSABASEINFO_H
#define LLVM_LIB_TARGET_XTENSA_XTENSABASEINFO_H

namespace llvm {

/// XtensaII - This namespace holds all of the target specific flags
/// that instruction info tracks.
///
namespace XtensaII {
/// Target Operand Flag enum.
enum TOF {
  //===------------------------------------------------------------------===//
  // Xtensa-Specific MachineOperand flags.

  MO_NO_FLAG = 0,

  /// MO_LO16 - On a symbol operand, this represents a relocation
  /// containing lower 16 bit of the address.  Used only via const16
  /// instruction.
  MO_LO16 = 0x1,

  /// MO_HI16 - On a symbol operand, this represents a relocation
  /// containing higher 16 bit of the address.  Used only via
  /// const16 instruction.
  MO_HI16 = 0x2
};
} // end namespace XtensaII

} // end namespace llvm;

#endif
