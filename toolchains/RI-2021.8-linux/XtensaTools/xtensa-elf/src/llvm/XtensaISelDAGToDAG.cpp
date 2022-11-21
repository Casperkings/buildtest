//===-- XtensaISelDAGToDAG.cpp - A dag to dag inst selector for Xtensa ----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines an instruction selector for the Xtensa target.
//
//===----------------------------------------------------------------------===//

#include "Tensilica/TensilicaIntrinsicISelLower.h"
#include "Tensilica/TensilicaISelDAGToDAG.h"
#include "Xtensa.h"
#include "XtensaTargetMachine.h"
#include "XtensaBaseInfo.h"
#include "XtensaMachineFunctionInfo.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/CodeGen/TargetLowering.h"
#include "llvm/Support/KnownBits.h"
using namespace llvm;

/// XtensaDAGToDAGISel - Xtensa specific code to select Xtensa machine
/// instructions for SelectionDAG operations.
///
namespace {

class XtensaDAGToDAGISel : public Tensilica::SDAGISel {
  const XtensaSubtarget *Subtarget;

public:
  XtensaDAGToDAGISel(XtensaTargetMachine &TM, CodeGenOpt::Level OptLevel)
      : Tensilica::SDAGISel(TM, OptLevel), Subtarget(nullptr) {}

  bool runOnMachineFunction(MachineFunction &MF) override {
    // Reset the subtarget each time through.
    Subtarget = &MF.getSubtarget<XtensaSubtarget>();
    SelectionDAGISel::runOnMachineFunction(MF);
    return true;
  }

  SDNode *SelectADD(SDNode *N);
  SDNode *SelectSMIN_SMAX(SDNode *N);
  SDNode *GenerateSynthMul(SDNode *N,
                           const SmallVectorImpl<XtensaInstrInfo::MulAlg> &BestSeq);
  SDNode *SelectMUL(SDNode *N);
  SDNode *SelectAND(SDNode *N);
  SDNode *SelectOR(SDNode *N);
  SDNode *SelectSHL_SRL_SRA(SDNode *N);
  SDNode *SelectROTL_ROTR(SDNode *N);
  SDNode *SelectSETCC(SDNode *N);
  SDNode *SelectSELECT_CC(SDNode *N);
  SDNode *SelectConstant(SDNode *N);
  SDNode *SelectGlobalAddress(SDNode *N);
  SDNode *SelectExternalSymbol(SDNode *N);
  SDNode *SelectPCRelativeWrapper(SDNode *N);
  SDNode *SelectBlockAddress(SDNode *N);
  bool SelectINTRINSIC_W_CHAIN(SDNode *N);
  bool SelectINTRINSIC_WO_CHAIN(SDNode *N);
  SDNode *SelectJumpTable(SDNode *N);

  void Select(SDNode *N) override;

  /// getI32Imm - Return a target constant with the specified value, of type
  /// i32.
  inline SDValue getI32Imm(unsigned Imm, const SDLoc &dl) {
    return CurDAG->getTargetConstant(Imm, dl, MVT::i32);
  }

  // Return true if Imm can be loaded with MOVI_N.
  inline bool immMOVI_N(int32_t Imm) {
    return Subtarget->hasDensity() && -32 <= Imm && Imm <= 95;
  }

  // Return true if Imm can be loaded with MOVI.
  inline bool immMOVI(int32_t Imm) {
    return isInt<12>(Imm);
  }

  // Return true if Imm can be loaded with a single instruction.
  inline bool immSingleMovi(int32_t Imm) {
    return immMOVI_N(Imm) || immMOVI(Imm);
  }

  // Return the best opcode for loading Imm.
  inline unsigned getMOVIOpcode(int32_t Imm) {
    // We do not generate MOVI_N here to simplify the scheduler.
    if (immMOVI(Imm))
      return Xtensa::MOVI;

    return Xtensa::NOP;
  }

  inline SDNode *movImmediate(const SDLoc &dl, int32_t Imm) {
    unsigned opcode = getMOVIOpcode(Imm);
    assert(opcode != Xtensa::NOP && "Invalid Imm");
    return CurDAG->getMachineNode(opcode, dl, MVT::i32,
                                  getI32Imm(Imm, dl));
  }

  // Return true if Imm can be added with ADDI.N.
  inline bool immADDI_N(int32_t Imm) {
    return (Subtarget->hasDensity() &&
            (Imm == -1 || (1 <= Imm && Imm <= 15)));
  }

  // Return true if Imm can be added with ADDI.
  inline bool immADDI(int32_t Imm) {
    return isInt<8>(Imm);
  }

  // Return true if Imm can be added with ADDMI.
  inline bool immADDMI(int32_t Imm) {
    return isShiftedInt<8,8>(Imm);
  }

  // Return true if Imm can be added with a single instruction.
  inline bool immSingleAddi(int32_t Imm) {
    return immADDI_N(Imm) || immADDI(Imm) || immADDMI(Imm);
  }

  // Return the best opcode for adding Imm.
  inline unsigned getADDIOpcode(int32_t Imm) {
    // We do not generate ADDI_N here to simplify the scheduler.
    if (immADDI(Imm))
      return Xtensa::ADDI;
    if (immADDMI(Imm))
      return Xtensa::ADDMI;

    return Xtensa::NOP;
  }

  inline SDNode *addImmediate(SDLoc &dl, SDValue N, int32_t Imm) {
    unsigned opcode = getADDIOpcode(Imm);
    assert(opcode != Xtensa::NOP && "Invalid Imm");
    return CurDAG->getMachineNode(opcode, dl, MVT::i32,
                                  N, getI32Imm(Imm, dl));
  }

  inline SDNode *shiftRightLogicalImmediate(SDLoc &dl, SDValue N, uint32_t Imm) {
    if (Imm <= 15)
      return CurDAG->getMachineNode(Xtensa::SRLI, dl, MVT::i32,
                                    N, getI32Imm(Imm, dl));
    return CurDAG->getMachineNode(Xtensa::EXTUI, dl, MVT::i32, N,
                                  getI32Imm(Imm, dl), getI32Imm(32 - Imm, dl));
  }

  // Return true if Imm is appropriate as the mask in EXTUI.
  inline bool immEXTUIMask(int32_t Imm) {
    return isMask_32(Imm) && isUInt<16>(Imm);
  }

  // Return the cost of loading an immediate in terms of the
  // instruction size.  We penalize L32R a little bit.
  inline unsigned immCost(int32_t Imm) {
    if (immMOVI_N(Imm))
      // We can use MOVI_N.
      return 2;
    if (immMOVI(Imm))
      // We can use MOVI.
      return 3;

    // We must use L32R.
    return 4;
  }

  // Return the cost of ANDing with Imm.
  inline unsigned addCost(int32_t Imm) {
    if (immADDI_N(Imm))
      // We can use ADDI_N.
      return 2;
    if (immADDI(Imm))
      // We can use ADDI.
      return 3;
    if (immADDMI(Imm))
      // We can use ADDMI.
      return 4;

    // We must use L32R.
    return 5;
  }

  // Return the cost of ANDing with Imm.
  inline unsigned andCost(int32_t Imm) {
    if (immEXTUIMask(Imm))
      // Use EXTUI.
      return 3;
    // Load a constant and then AND.
    return 3 + immCost(Imm);
  }

  void EmitFunctionEntryCode() override;

  // Complex Pattern Selectors.
  bool SelectAddrCommon(SDValue N, SDValue &Base, SDValue &Offset,
                        std::function<bool(int32_t)> func);
  bool SelectAddrFIA(SDValue Addr, SDValue &OSP, SDValue &Base, SDValue &Offset);

  bool SelectADDRspii(SDValue Addr, SDValue &Base, SDValue &Offset) {
    return SelectAddrCommon(Addr, Base, Offset,
                            [](int32_t Imm) { return false; });
  }

  bool SelectAddr8(SDValue N, SDValue &Base, SDValue &Offset) {
    return SelectAddrCommon(N, Base, Offset, [](int32_t Imm) {
        return isUInt<8>(Imm);
      });
  }

  bool SelectAddr16(SDValue N, SDValue &Base, SDValue &Offset) {
    return SelectAddrCommon(N, Base, Offset, [](int32_t Imm) {
        return isShiftedUInt<8,1>(Imm);
      });
  }

  bool SelectAddr32(SDValue N, SDValue &Base, SDValue &Offset) {
    return SelectAddrCommon(N, Base, Offset, [](int32_t Imm) {
        return isShiftedUInt<8,2>(Imm);
      });
  }

  bool SelectInlineAsmMemoryOperand(const SDValue &Op, unsigned ConstraintID,
                                    std::vector<SDValue> &OutOps) override;

  void
  ReplaceUsesOfIntrnOutputs(const Tensilica::IntrinsicISelInfo *IntrISelInfo,
                            SDNode *Old, SDNode *New);

  StringRef getPassName() const override {
    return "Xtensa DAG->DAG Pattern Instruction Selection";
  }

// Include the pieces autogenerated from the target description.
#include "XtensaGenDAGISel.inc"

// Include the autogenerated complex pattern definitions
#define GET_ISEL_LOWER_IMM_PRED_COMPLEX_PATTERN
#include "TensilicaGenIntrinsicISelLower.inc"
};
} // end anonymous namespace

/// createXtensaISelDag - This pass converts a legalized DAG into a
/// Xtensa-specific DAG, ready for instruction scheduling.
///
FunctionPass *llvm::createXtensaISelDag(XtensaTargetMachine &TM,
                                        CodeGenOpt::Level OptLevel) {
  return new XtensaDAGToDAGISel(TM, OptLevel);
}

void XtensaDAGToDAGISel::EmitFunctionEntryCode() {
  XtensaFunctionInfo *XFI = MF->getInfo<XtensaFunctionInfo>();

  // If we don't have any reference to incoming arguments, don't
  // bother copying the original stack pointer to a vreg.
  if (!XFI->getIncomingArgOnStackReferenced())
    return;

  // Emit code to copy the original stack pointer in A9 to a vreg.
  MachineBasicBlock *EntryBB = &MF->front();
  const XtensaInstrInfo &TII = *Subtarget->getInstrInfo();
  MachineRegisterInfo &RegInfo = MF->getRegInfo();
  const TargetRegisterClass *ARRegClass =
      TII.getRegisterInfo().getPointerRegClass();
  unsigned AR = RegInfo.createVirtualRegister(ARRegClass);
  BuildMI(EntryBB, DebugLoc(), TII.get(Xtensa::COPY), AR)
    .addReg(Xtensa::a9);
  XFI->setOSPVReg(AR);
  EntryBB->addLiveIn(Xtensa::a9);
}

bool XtensaDAGToDAGISel::SelectAddrCommon(SDValue N,
                                          SDValue &Base,
                                          SDValue &Offset,
                                          std::function<bool(int32_t)> func) {
  SDValue BaseAddr = N;
  int32_t OffsetVal = 0;

  // Recognize Base + Offset.
  if (CurDAG->isBaseWithConstantOffset(N)) {
    BaseAddr = N->getOperand(0);
    OffsetVal = cast<ConstantSDNode>(N->getOperand(1))->getSExtValue();
  }

  // Accept FrameIndex for local objects.  We do not accept FrameIndex
  // for incoming arguments on the stack here.
  FrameIndexSDNode *FIN = nullptr;
  MachineFrameInfo &MFI = MF->getFrameInfo();
  SDLoc dl(N);
  if ((FIN = dyn_cast<FrameIndexSDNode>(BaseAddr)) &&
      !MFI.isFixedObjectIndex(FIN->getIndex())) {
    Base = CurDAG->getTargetFrameIndex(FIN->getIndex(), MVT::i32);
    Offset = getI32Imm(OffsetVal, dl);
    return true;
  }

  // Verify that the offset is valid if a FrameIndex is not involved.
  if (func(OffsetVal)) {
    Base = BaseAddr;
    Offset = getI32Imm(OffsetVal, dl);
    return true;
  }

  // Accept the address as is if zero is a valid offset.
  if (func(0)) {
    Base = N;
    Offset = getI32Imm(0, dl);
    return true;
  }

  return false;
}

bool XtensaDAGToDAGISel::SelectAddrFIA(SDValue Addr,
                                       SDValue &OSP,
                                       SDValue &Base,
                                       SDValue &Offset) {
  SDValue BaseAddr = Addr;
  int32_t OffsetVal = 0;

  // Recognize Base + Offset.
  if (CurDAG->isBaseWithConstantOffset(Addr)) {
    BaseAddr = Addr.getOperand(0);
    OffsetVal = cast<ConstantSDNode>(Addr.getOperand(1))->getSExtValue();
  }

  // Accept FrameIndex for incoming arguments on the stack.  We do not
  // accept FrameIndex for local objects here.
  FrameIndexSDNode *FIN = nullptr;
  MachineFrameInfo &MFI = MF->getFrameInfo();
  if ((FIN = dyn_cast<FrameIndexSDNode>(BaseAddr)) &&
      MFI.isFixedObjectIndex(FIN->getIndex())) {
    auto &XFI = *MF->getInfo<XtensaFunctionInfo>();
    SDLoc dl(Addr);
    OSP = CurDAG->getRegister(XFI.getOSPVReg(), MVT::i32);
    Base = CurDAG->getTargetFrameIndex(FIN->getIndex(), MVT::i32);
    Offset = getI32Imm(OffsetVal, dl);
    return true;
  }

  return false;
}

bool XtensaDAGToDAGISel::SelectInlineAsmMemoryOperand(
    const SDValue &Op, unsigned ConstraintID, std::vector<SDValue> &OutOps) {
  SDValue Reg;
  switch (ConstraintID) {
  default:
    return true;
  case InlineAsm::Constraint_m: // Memory.
    OutOps.push_back(Op);
    return false;
  }
  OutOps.push_back(Reg);
  OutOps.push_back(Op.getOperand(0));
  return false;
}

// If an instruction generates the intrinsics output, replace the uses
// of the original intrinsic output operand with the output of the instruction
void XtensaDAGToDAGISel::ReplaceUsesOfIntrnOutputs(
    const Tensilica::IntrinsicISelInfo *IntrISelInfo, SDNode *Old,
    SDNode *New) {
  assert(IntrISelInfo->NumOutputs <= XTENSA_INTRN_ISEL_MAX_OUTPUTS);
  for (unsigned i = 0; i < IntrISelInfo->NumOutputs; ++i) {
    const Tensilica::IntrnISelOperandInfo &Opnd = IntrISelInfo->OutOpndInfos[i];
    if (Opnd.OpndType == Tensilica::XTENSA_INTRN_ISEL_OPND)
      ReplaceUses(SDValue(Old, Opnd.Index.IntrnOpnd), SDValue(New, i));
  }
}

bool XtensaDAGToDAGISel::SelectINTRINSIC_W_CHAIN(SDNode *N) {
  const XtensaTargetLowering *TL = Subtarget->getTargetLowering();

  unsigned IntNo = cast<ConstantSDNode>(N->getOperand(1))->getZExtValue();
  const Tensilica::IntrinsicISelInfo *IntrISelInfo =
      TL->getIntrinsicISelInfo(IntNo);
  if (!IntrISelInfo)
    return false;

  // Check if the immediate constraints are satisfied
  if (!TL->checkIntrinsicImmConstraints(N, IntrISelInfo))
    return false;

  // Vector to store the generated instructions. An index into the vector
  // is used to access the results of any temporaries used by a later
  // instruction
  std::vector<SDNode *> Nodes;
  SDNode *Node = nullptr;
  
  Node = TL->getIntrinsicInst(Nodes, N, N->getOperand(0), CurDAG, IntrISelInfo,
                              getOutLinkType(IntrISelInfo, true));
  Nodes.push_back(Node);
  ReplaceUsesOfIntrnOutputs(IntrISelInfo, N, Node);

  // Generate all the instructions in the proto sequence
  while (!IntrISelInfo->LastInst) {
    // The instructions in the sequence are 'chained'
    SDValue Chain = SDValue(Nodes.back(), IntrISelInfo->NumOutputs);
    ++IntrISelInfo; 
    Node = TL->getIntrinsicInst(Nodes, N, Chain, CurDAG, IntrISelInfo,
                                getOutLinkType(IntrISelInfo, true));
    Nodes.push_back(Node);
    ReplaceUsesOfIntrnOutputs(IntrISelInfo, N, Node);
  } 
  SDNode *LastNode = Nodes.back();
  // Replace the chain
  assert(SDValue(LastNode, LastNode->getNumValues()-1).getValueType() ==
         MVT::Other);
  ReplaceUses(SDValue(N, N->getNumValues() - 1),
              SDValue(LastNode, LastNode->getNumValues() - 1));
  // Return null since all uses have been replaced
  CurDAG->RemoveDeadNode(N);
  return true;
}

bool XtensaDAGToDAGISel::SelectINTRINSIC_WO_CHAIN(SDNode *N) {
  const XtensaTargetLowering *TL = Subtarget->getTargetLowering();

  unsigned IntNo = cast<ConstantSDNode>(N->getOperand(0))->getZExtValue();
  const Tensilica::IntrinsicISelInfo *IntrISelInfo =
      TL->getIntrinsicISelInfo(IntNo);
  // Couldn't find a match; return the input node
  if (!IntrISelInfo)
    return false;

  // Check if the immediate constraints are satisfied. Return the input
  // node.
  if (!TL->checkIntrinsicImmConstraints(N, IntrISelInfo))
    return false;

  // Array to store the generated instructions. An index into the array
  // is used to access the results of any temporaries used by a later
  // instruction
  std::vector<SDNode *> Nodes;
  SDNode *Node = nullptr;

  Node = TL->getIntrinsicInst(Nodes, N, SDValue(), CurDAG,
                             IntrISelInfo, getOutLinkType(IntrISelInfo, false));
  Nodes.push_back(Node);
  ReplaceUsesOfIntrnOutputs(IntrISelInfo, N, Node);

  // Generate all the instructions in the proto sequence
  while (!IntrISelInfo->LastInst) {
    SDValue LastValue;
    getLastValue(Nodes.back(), LastValue);
    ++IntrISelInfo;
    Node = TL->getIntrinsicInst(Nodes, N, LastValue, CurDAG, IntrISelInfo,
                                getOutLinkType(IntrISelInfo, false));
    Nodes.push_back(Node);
    ReplaceUsesOfIntrnOutputs(IntrISelInfo, N, Node);
  }
  // Return null since all uses have been replaced
  CurDAG->RemoveDeadNode(N);
  return true;
}

SDNode *XtensaDAGToDAGISel::SelectADD(SDNode *N) {
  assert(N->getOpcode() == ISD::ADD && "Wrong opcode");
  SDLoc dl(N);

  SDValue N0 = N->getOperand(0);
  SDValue N1 = N->getOperand(1);

  // Transform (x << C1) + C2 to (x + C3) << C1 if doing so allows us
  // to add C3 in a cheaper way.  We might replace L32R, ADDMI, ADDI,
  // and ADDI.N with ADDMI, ADDI, and ADDI.N.
  if (N0->hasOneUse() && N0->getOpcode() == ISD::SHL) {
    ConstantSDNode *ShiftCntN = dyn_cast<ConstantSDNode>(N0->getOperand(1));
    ConstantSDNode *AddendN = dyn_cast<ConstantSDNode>(N1);

    if (ShiftCntN != nullptr && AddendN != nullptr) {
      uint32_t ShiftCnt = ShiftCntN->getZExtValue();
      int32_t Addend = AddendN->getSExtValue();

      // Check that Addend can be added before shifting.
      if ((Addend & ((1 << ShiftCnt) - 1)) == 0) {
        int32_t NewAddend = Addend >> ShiftCnt;
        unsigned OldCost = addCost(Addend);
        unsigned NewCost = addCost(NewAddend);
        if (NewCost < OldCost) {
          SDNode *Add = addImmediate(dl, N0->getOperand(0), NewAddend);
          return CurDAG->SelectNodeTo(N, Xtensa::SLLI, MVT::i32,
                                      SDValue(Add, 0),
                                      getI32Imm(ShiftCnt, dl));
        }
      }
    }
  }

  return nullptr;
}

SDNode *XtensaDAGToDAGISel::SelectSMIN_SMAX(SDNode *N) {
  assert((N->getOpcode() == ISD::SMIN || N->getOpcode() == ISD::SMAX) &&
         "Wrong opcode");
  SDLoc dl(N);

  // We must have CLAMPS for the transformation here.
  if (!Subtarget->hasClamps())
    return nullptr;

  // Recognize (smin (smax x, C1), C2) and (smax (smin x, C1), C2)
  // as CLAMPS.
  bool outer_smin_p = N->getOpcode() == ISD::SMIN;
  SDValue N0 = N->getOperand(0);
  SDValue N1 = N->getOperand(1);
  if (ConstantSDNode *N1C = dyn_cast<ConstantSDNode>(N1)) {
    if (N0->getOpcode() == (outer_smin_p ? ISD::SMAX : ISD::SMIN)) {
      SDValue N00 = N0->getOperand(0);
      SDValue N01 = N0->getOperand(1);
      if (ConstantSDNode *N01C = dyn_cast<ConstantSDNode>(N01)) {
        int32_t Hi, Lo;
        if (outer_smin_p) {
          Hi = N1C->getSExtValue();
          Lo = N01C->getSExtValue();
        } else {
          Hi = N01C->getSExtValue();
          Lo = N1C->getSExtValue();
        }
        if (isMask_32(Hi) && -Lo - 1 == Hi) {
          unsigned ct1 = countTrailingOnes((uint32_t) Hi);
          if (7 <= ct1 && ct1 <= 22)
            return CurDAG->SelectNodeTo(N, Xtensa::CLAMPS, MVT::i32,
                                        N00, getI32Imm(ct1, dl));
        }
      }
    }
  }

  return nullptr;
}

SDNode *XtensaDAGToDAGISel::GenerateSynthMul(
    SDNode *N, const SmallVectorImpl<XtensaInstrInfo::MulAlg> &BestSeq) {
  SDLoc dl(N);
  SDValue N0 = N->getOperand(0);
  SDValue T = N0;
  SDNode *TN;

  for (auto i = BestSeq.rbegin(), e = BestSeq.rend(); i != e; ++i) {
    unsigned opcode = i->Opcode;
    unsigned del0 = i->Arg0 == XtensaInstrInfo::DELEGATE;
    unsigned del1 = i->Arg1 == XtensaInstrInfo::DELEGATE;

    switch (opcode) {
    case Xtensa::ADD:
    case Xtensa::ADDX2:
    case Xtensa::ADDX4:
    case Xtensa::ADDX8:
    case Xtensa::SUB:
    case Xtensa::SUBX2:
    case Xtensa::SUBX4:
    case Xtensa::SUBX8:
      TN = CurDAG->getMachineNode(opcode, dl, MVT::i32,
                                  del0 ? T : N0, del1 ? T : N0);
      T = SDValue(TN, 0);
      break;
    case Xtensa::SLLI:
      TN = CurDAG->getMachineNode(opcode, dl, MVT::i32,
                                  del0 ? T : N0,
                                  getI32Imm(i->Arg1, dl));
      T = SDValue(TN, 0);
      break;
    case Xtensa::NEG:
      TN = CurDAG->getMachineNode(opcode, dl, MVT::i32,
                                  del0 ? T : N0);
      T = SDValue(TN, 0);
      break;
    default:
      llvm_unreachable("unexpected opcode");
    }
  }
  return CurDAG->SelectNodeTo(N, Xtensa::COPY, MVT::i32, T);
}

SDNode *XtensaDAGToDAGISel::SelectMUL(SDNode *N) {
  assert(N->getOpcode() == ISD::MUL && "Wrong opcode");
  SDLoc dl(N);

  SDValue N0 = N->getOperand(0);
  SDValue N1 = N->getOperand(1);

  // Keep the following code in sync with
  // XtensaTargetLowering::LowerMul.  See the function for more
  // details.

  // Use ADDX[248] and SUBX8 for multiplications by 3, 5, 7, and 9
  // if those instructions are available.  We are going to let the
  // DAG matcher match these multiplications.
  if (isa<ConstantSDNode>(N1)) {
    uint64_t Val = cast<ConstantSDNode>(N1)->getZExtValue();
    if (Val == 3 || Val == 5 || Val == 7 || Val == 9)
      return nullptr;
  }

  SmallVector<XtensaInstrInfo::MulAlg, 8> BestSeq;
  bool SynthMulSeq = false;

  // Construct synthehtic multiplication.  Make a note if the sequence
  // involves one or two instructions.
  if (isa<ConstantSDNode>(N1)) {
    int64_t Val = cast<ConstantSDNode>(N1)->getSExtValue();
    const XtensaInstrInfo &XII = *Subtarget->getInstrInfo();

    SynthMulSeq = XII.synthMul(Val, BestSeq);
    if (SynthMulSeq && BestSeq.size() <= 2)
      return GenerateSynthMul(N, BestSeq);
  }

  // Attempt to generate MUL16U or MUL16S.
  if (Subtarget->hasMul16()) {
    // Use MUL16S if both Op0 and Op1 appear to be sign-extended from
    // int16_t.
    if (CurDAG->ComputeNumSignBits(N0) >= 17 &&
        CurDAG->ComputeNumSignBits(N1) >= 17)
      return CurDAG->SelectNodeTo(N, Xtensa::MUL16S, MVT::i32, N0, N1);

    // Use MUL16U if both Op0 and OP1 appear to be zero-extended
    // from uint16_t.
    APInt Mask = APInt::getHighBitsSet(32, 16);
    if ((CurDAG->computeKnownBits(N0).Zero & Mask) == Mask &&
        (CurDAG->computeKnownBits(N1).Zero & Mask) == Mask)
      return CurDAG->SelectNodeTo(N, Xtensa::MUL16U, MVT::i32, N0, N1);
  }

  if (!Subtarget->hasMul32() && SynthMulSeq)
    return GenerateSynthMul(N, BestSeq);

  // If MULL is not available, construct a sequence with MUL16U and
  // UMUL_AA_LH instructions, subject to the instruction
  // availability.
  if (!Subtarget->hasMul32() && Subtarget->hasMul16() &&
      Subtarget->hasMac16()) {
    // We are going to build:
    //
    // (ADD (SLLI (ADD (RSR_ACCLO (UMUL_AA_LH AR:$a, AR:$b)),
    //                 (RSR_ACCLO (UMUL_AA_LH AR:$b, AR:$a))),
    //            16),
    //      (MUL16U AR:$a, AR:$b))>;
    //
    // Note that the link between RSR_ACCLO and UMUL_AA_LH is via ACCLO.

    SDValue ab = SDValue(CurDAG->getMachineNode(Xtensa::UMUL_AA_LH,
                                                dl, MVT::Glue, N0, N1), 0);
    SDValue ab_acclo = SDValue(CurDAG->getMachineNode(Xtensa::RSR_ACCLO,
                                                      dl, MVT::i32, ab), 0);
    SDValue ba = SDValue(CurDAG->getMachineNode(Xtensa::UMUL_AA_LH,
                                                dl, MVT::Glue, N1, N0), 0);
    SDValue ba_acclo = SDValue(CurDAG->getMachineNode(Xtensa::RSR_ACCLO,
                                                      dl, MVT::i32, ba), 0);
    SDValue high = SDValue(CurDAG->getMachineNode(Xtensa::ADD,
                                                  dl, MVT::i32,
                                                  ab_acclo, ba_acclo), 0);
    SDValue high_shift = SDValue(
                                 CurDAG->getMachineNode(Xtensa::SLLI,
                                                        dl, MVT::i32,
                                                        high,
                                                        getI32Imm(16, dl)), 0);
    SDValue mul16u = SDValue(CurDAG->getMachineNode(Xtensa::MUL16U,
                                                    dl, MVT::i32, N0, N1), 0);
    return CurDAG->SelectNodeTo(N, Xtensa::ADD, MVT::i32, high_shift, mul16u);
  }

  // If we get here, we must have MUL.
  assert (Subtarget->hasMul32() && "We do not have Mul32.");

  return nullptr;
}

SDNode *XtensaDAGToDAGISel::SelectSHL_SRL_SRA(SDNode *N) {
  unsigned opcode = N->getOpcode();
  assert((opcode == ISD::SHL || opcode == ISD::SRL || opcode == ISD::SRA) &&
         "Wrong opcode");
  SDLoc dl(N);
  SDValue N0 = N->getOperand(0);
  SDValue N1 = N->getOperand(1);

  // We are going to let the DAG matcher match those shifts by
  // constant amounts.
  if (isa<ConstantSDNode>(N1))
    return nullptr;

  // Build a {SSL,SSR}-{SLL,SRA,SRL} sequence with Glue connecting
  // them.
  unsigned op1, op2;
  switch (opcode) {
  case ISD::SHL: op1 = Xtensa::SSL; op2 = Xtensa::SLL; break;
  case ISD::SRA: op1 = Xtensa::SSR; op2 = Xtensa::SRA; break;
  case ISD::SRL: op1 = Xtensa::SSR; op2 = Xtensa::SRL; break;
  default: llvm_unreachable("not implemented");
  }

  SDValue glue = SDValue(CurDAG->getMachineNode(op1, dl, MVT::Glue, N1), 0);
  return CurDAG->SelectNodeTo(N, op2, MVT::i32, N0, glue);
}

SDNode *XtensaDAGToDAGISel::SelectROTL_ROTR(SDNode *N) {
  assert((N->getOpcode() == ISD::ROTL || N->getOpcode() == ISD::ROTR) &&
         "Wrong opcode");
  SDLoc dl(N);

  SDValue N0 = N->getOperand(0);
  SDValue N1 = N->getOperand(1);
  unsigned rot_opcode = N->getOpcode();
  SDNode *Sar;
  if (isa<ConstantSDNode>(N1)) {
    uint32_t RotateAmount = dyn_cast<ConstantSDNode>(N1)->getZExtValue();
    if (rot_opcode == ISD::ROTL)
      RotateAmount = 32 - RotateAmount;
    Sar = CurDAG->getMachineNode(Xtensa::SSAI, dl, MVT::Glue,
                                 getI32Imm(RotateAmount, dl));
  } else {
    unsigned opcode = rot_opcode == ISD::ROTL ? Xtensa::SSL : Xtensa::SSR;
    Sar = CurDAG->getMachineNode(opcode, dl, MVT::Glue, N1);
  }
  return CurDAG->SelectNodeTo(N, Xtensa::SRC, MVT::i32, N0, N0,
                              SDValue(Sar, 0));
}

SDNode *XtensaDAGToDAGISel::SelectAND(SDNode *N) {
  assert(N->getOpcode() == ISD::AND && "Wrong opcode");
  SDLoc dl(N);

  SDValue N0 = N->getOperand(0);
  SDValue N1 = N->getOperand(1);

  switch (N0->getOpcode()) {
  case ISD::SHL:
    if (N0->hasOneUse()) {
      SDValue N00 = N0->getOperand(0);
      SDValue N01 = N0->getOperand(1);
      ConstantSDNode *ShiftCntN = dyn_cast<ConstantSDNode>(N01);
      ConstantSDNode *MaskN = dyn_cast<ConstantSDNode>(N1);

      if (ShiftCntN != nullptr && MaskN != nullptr) {
        uint32_t ShiftCnt = ShiftCntN->getZExtValue();
        uint32_t Mask = MaskN->getZExtValue();

        // Transform (x << C1) & C2 to (x >>u C3) << C4.  We require
        // that C2 be a mask pegged to the MSB.
        if (isMask_32(~Mask)) {
          unsigned MaskWidth = countLeadingOnes(Mask);
          if (ShiftCnt + MaskWidth < 32) {
            // The bit fields in the number of bits look like so:
            // Before: ShiftCnt, MaskWidth intereting, (32 - ShiftCnt - MaskWidth)
            // After:  MaskWidth interesting, (32 - MaskWidth)
            unsigned R = 32 - ShiftCnt - MaskWidth;
            unsigned L = 32 - MaskWidth;
            SDNode *srli = shiftRightLogicalImmediate(dl, N00, R);
            return CurDAG->SelectNodeTo(N, Xtensa::SLLI, MVT::i32,
                                        SDValue(srli, 0),
                                        getI32Imm(L, dl));
          }
        }

        // Transform (x << C1) & C2 to (x & C3) << C1 if doing so
        // allows us to AND with C3 in a cheaper way.  We might
        // replace L32R, MOVI, and MOVI_N with MOVI, MOVI_N, and
        // EXTUI.
        uint64_t NewMask = Mask >> ShiftCnt;
        unsigned OldCost = andCost((int32_t) Mask);
        unsigned NewCost = andCost((int32_t) NewMask);
        if (NewCost < OldCost) {
          SDNode *And;
          if (immEXTUIMask(NewMask))
            And = CurDAG->getMachineNode(Xtensa::EXTUI, dl, MVT::i32,
                                         N00,
                                         getI32Imm(0, dl),
                                         getI32Imm(countTrailingOnes(NewMask),
                                                   dl));
          else {
            SDNode *Imm = movImmediate(dl, NewMask);
            And = CurDAG->getMachineNode(Xtensa::AND, dl, MVT::i32,
                                         N00, SDValue(Imm, 0));
          }
          return CurDAG->SelectNodeTo(N, Xtensa::SLLI, MVT::i32,
                                      SDValue(And, 0),
                                      getI32Imm(ShiftCnt, dl));
        }
      }
    }
    break;
  case ISD::SRL:
    if (N0->hasOneUse()) {
      SDValue N00 = N0->getOperand(0);
      SDValue N01 = N0->getOperand(1);
      ConstantSDNode *ShiftCntN = dyn_cast<ConstantSDNode>(N01);
      ConstantSDNode *MaskN = dyn_cast<ConstantSDNode>(N1);

      if (ShiftCntN != nullptr && MaskN != nullptr) {
        uint32_t ShiftCnt = ShiftCntN->getZExtValue();
        uint32_t Mask = MaskN->getZExtValue();

        if (isMask_32(Mask)) {
          unsigned MaskWidth = countTrailingOnes(Mask);

          // Recognize (x >>u C1) & C2 as EXTUI.
          if (MaskWidth <= 16 && (MaskWidth + ShiftCnt <= 32))
            return CurDAG->SelectNodeTo(N, Xtensa::EXTUI, MVT::i32,
                                        N00,
                                        getI32Imm(ShiftCnt, dl),
                                        getI32Imm(MaskWidth, dl));

          // Transform (x >>u C1) & C2 to (x << C3) >>u C4.
          else if (MaskWidth >= 17 && ShiftCnt + MaskWidth < 32) {
            // The bit fields in the number of bits look like so:
            // Before: (32 - MaskWidth - ShiftCnt), MaskWidth, ShiftCnt
            // After:  (32 - MaskWidth), MaskWidth
            unsigned L = 32 - MaskWidth - ShiftCnt;
            unsigned R = 32 - MaskWidth;
            SDNode *Slli = CurDAG->getMachineNode(Xtensa::SLLI,
                                                  dl, MVT::i32,
                                                  N00, getI32Imm(L, dl));
            return CurDAG->SelectNodeTo(N, Xtensa::SRLI, MVT::i32,
                                        SDValue(Slli, 0),
                                        getI32Imm(R, dl));
          }
        } else if (isShiftedMask_32(Mask)) {
          unsigned MaskShift = countTrailingZeros(Mask);
          unsigned MaskWidth = countTrailingOnes((Mask - 1) | Mask) - MaskShift;

          // Transform (x >>u C1) & C2 to either:
          //   (EXTUI x, C3, C4) << C5
          // or
          //   (x >>u C6) << C5
          // where C2 is a shifted mask.
          if (MaskWidth <= 16 || ShiftCnt + MaskWidth + MaskShift == 32) {
            unsigned R = MaskShift + ShiftCnt;
            unsigned L = MaskShift;
            SDNode *Srli;
            if (MaskWidth <= 16)
              Srli = CurDAG->getMachineNode(Xtensa::EXTUI, dl, MVT::i32,
                                            N00,
                                            getI32Imm(R, dl),
                                            getI32Imm(MaskWidth, dl));
            else
              Srli = shiftRightLogicalImmediate(dl, N00, R);

            return CurDAG->SelectNodeTo(N, Xtensa::SLLI, MVT::i32,
                                        SDValue(Srli, 0),
                                        getI32Imm(L, dl));
          }
        }
      }
    }
    break;
  case ISD::SRA: {
    SDValue N01 = N0->getOperand(1);
    ConstantSDNode *ShiftCntN = dyn_cast<ConstantSDNode>(N01);
    ConstantSDNode *MaskN = dyn_cast<ConstantSDNode>(N1);

    if (ShiftCntN != nullptr && MaskN != nullptr &&
        ShiftCntN->getZExtValue() == 31) {
      uint32_t Mask = MaskN->getZExtValue();

      // Transform (x >>s 31) & C1 to (x >>s 31) >>u C2 if C1 is a
      // mask of the form 1^N-1 greater than 0xffff.  This
      // transformation can trigger on expressions like
      // x < 0 ? 0xfffff : 0.
      if (isMask_32(Mask) && Mask > 0xffff) {
        unsigned R = countLeadingZeros(Mask);
        return CurDAG->SelectNodeTo(N, Xtensa::SRLI, MVT::i32,
                                    N0, getI32Imm(R, dl));
      }

      // Transform (x >>s 31) & C1 to (x >>s 31) << C2 if C1 is a
      // ~mask of the form 1^N-1.  This transformation can trigger
      // on expressions like x < 0 ? 0xffff0000 : 0.
      if (isMask_32(~Mask)) {
        unsigned L = countTrailingZeros(Mask);
        return CurDAG->SelectNodeTo(N, Xtensa::SLLI, MVT::i32,
                                    N0, getI32Imm(L, dl));
      }
    }
    break;
  }
  default:
    break;
  }

  return nullptr;
}

SDNode *XtensaDAGToDAGISel::SelectOR(SDNode *N) {
  assert(N->getOpcode() == ISD::OR && "Wrong opcode");
  SDLoc dl(N);

  // Return if we do not have DEPBITS.
  if (!Subtarget->hasDepbits())
    return nullptr;

  // Handle commutativity of OR.
  for (unsigned i = 0; i <= 1; i++) {
    SDValue Src = N->getOperand(i);
    SDValue Dst = N->getOperand(1-i);

    // We must have AND in each side with exactly one use.
    if (!Src.hasOneUse() || !Dst.hasOneUse())
      continue;

    if (Src.getOpcode() == ISD::AND && Dst.getOpcode() == ISD::AND) {
      // We try to insert Src into Dst.
      SDValue SrcReg = Src.getOperand(0);
      SDValue SrcMask = Src.getOperand(1);
      SDValue DstReg = Dst.getOperand(0);
      SDValue DstMask = Dst.getOperand(1);

      // Each AND must have a constant as its Dst.
      if (!isa<ConstantSDNode>(SrcMask) || !isa<ConstantSDNode>(DstMask))
        continue;

      // The two masks must add up to 0xffffffff without overlapping
      // bits.
      uint64_t SrcMaskVal = cast<ConstantSDNode>(SrcMask)->getZExtValue();
      uint64_t DstMaskVal = cast<ConstantSDNode>(DstMask)->getZExtValue();

      if ((SrcMaskVal ^ DstMaskVal) != 0xffffffff)
        continue;

      if (isMask_32(SrcMaskVal)) {
        // We have a DEPBIT that does not involve a shift.
        unsigned Width = countTrailingOnes(SrcMaskVal);

        if (Width <= 16) {
          SDValue Args[] = {
            DstReg,
            SrcReg,
            getI32Imm(0, dl),
            getI32Imm(Width, dl)
          };
          return CurDAG->SelectNodeTo(N, Xtensa::DEPBITS, MVT::i32, Args);
        }
      } else if (isShiftedMask_32(SrcMaskVal)) {
        // We have a DEPBITS with a nonzero shift count.
        unsigned Shift = countTrailingZeros(SrcMaskVal);
        unsigned Width = countTrailingOnes(SrcMaskVal >> Shift);

        // Check that the shift count matches the one in SHL.
        if (Width <= 16 && SrcReg.getOpcode() == ISD::SHL) {
          SDValue SrcReg0 = SrcReg.getOperand(0);
          SDValue SrcReg1 = SrcReg.getOperand(1);

          if (isa<ConstantSDNode>(SrcReg1) &&
              cast<ConstantSDNode>(SrcReg1)->getZExtValue() == Shift) {
            SDValue Args[] = {
              DstReg,
              SrcReg0,
              getI32Imm(Shift, dl),
              getI32Imm(Width, dl)
            };
            return CurDAG->SelectNodeTo(N, Xtensa::DEPBITS, MVT::i32, Args);
          }
        }
      }
    } else if (Src.getOpcode() == ISD::SHL && Dst.getOpcode() == ISD::AND) {
      // Recognize DEPBITS updating the most significant bits.
      SDValue SrcReg = Src.getOperand(0);
      SDValue SrcShift = Src.getOperand(1);
      SDValue DstReg = Dst.getOperand(0);
      SDValue DstMask = Dst.getOperand(1);

      if (!isa<ConstantSDNode>(SrcShift) || !isa<ConstantSDNode>(DstMask))
        continue;

      uint64_t SrcShiftVal = cast<ConstantSDNode>(SrcShift)->getZExtValue();
      uint64_t DstMaskVal = cast<ConstantSDNode>(DstMask)->getZExtValue();

      if (!isMask_32(DstMaskVal) ||
          countTrailingOnes(DstMaskVal) != SrcShiftVal ||
          SrcShiftVal < 16)
        continue;

      SDValue Args[] = {
        DstReg,
        SrcReg,
        getI32Imm(SrcShiftVal, dl),
        getI32Imm(32 - SrcShiftVal, dl)
      };
      return CurDAG->SelectNodeTo(N, Xtensa::DEPBITS, MVT::i32, Args);
    }
  }

  return nullptr;
}

SDNode *XtensaDAGToDAGISel::SelectSETCC(SDNode *N) {
  assert(N->getOpcode() == ISD::SETCC && "Wrong opcode");
  SDLoc dl(N);

  SDValue N0 = N->getOperand(0);
  SDValue N1 = N->getOperand(1);
  SDValue N2 = N->getOperand(2);
  ISD::CondCode CC = cast<CondCodeSDNode>(N2)->get();
  if ((CC == ISD::SETEQ || CC == ISD::SETNE) &&
      Subtarget->hasSalt() &&
      isa<ConstantSDNode>(N1)) {
    int64_t Val = cast<ConstantSDNode>(N1)->getSExtValue();
    if (Val == 0) {
      SDNode *movi = movImmediate(dl, CC == ISD::SETEQ);
      return CC == ISD::SETEQ
                 ?
                 // ay = ax == 0;
                 // ->
                 // movi  at,1
                 // saltu ay,ax,at
                 CurDAG->SelectNodeTo(N, Xtensa::SALTU, MVT::i32, N0,
                                      SDValue(movi, 0))
                 :
                 // ay = ax != 0;
                 // ->
                 // movi  at,0
                 // saltu ay,at,ax
                 CurDAG->SelectNodeTo(N, Xtensa::SALTU, MVT::i32,
                                      SDValue(movi, 0), N0);
    } else if (Val == -1) {
      SDNode *addi = addImmediate(dl, N0, 1);
      return CC == ISD::SETEQ
                 ?
                 // ay = ax == -1
                 // ->
                 // addi.n at,ax,1
                 // saltu  ay,at,ax
                 CurDAG->SelectNodeTo(N, Xtensa::SALTU, MVT::i32,
                                      SDValue(addi, 0), N0)
                 :
                 // ay = ax != -1
                 // ->
                 // addi.n at,ax,1
                 // saltu  ay,ax,at
                 CurDAG->SelectNodeTo(N, Xtensa::SALTU, MVT::i32, N0,
                                      SDValue(addi, 0));
    } else if (immSingleAddi(-Val)) {
      SDNode *addi = addImmediate(dl, N0, -Val);
      SDNode *movi = movImmediate(dl, CC == ISD::SETEQ);
      return CC == ISD::SETEQ
                 ?
                 // ay = ax == C
                 // ->
                 // addi.n/addi/addmi at,ax,-C
                 // movi.n            au,1
                 // saltu             ay,at,au
                 CurDAG->SelectNodeTo(N, Xtensa::SALTU, MVT::i32,
                                      SDValue(addi, 0), SDValue(movi, 0))
                 :
                 // ay = ax != C
                 // ->
                 // addi.n/addi/addmi at,ax,-C
                 // movi.n            au,0
                 // saltu             ay,au,at
                 CurDAG->SelectNodeTo(N, Xtensa::SALTU, MVT::i32,
                                      SDValue(movi, 0), SDValue(addi, 0));
    } else if (immSingleAddi(-(Val + 1))) {
      SDNode *addi = addImmediate(dl, N0, -(Val + 1));
      SDNode *inc = addImmediate(dl, SDValue(addi, 0), 1);
      return CC == ISD::SETEQ
                 ?
                 // ay = ax == C
                 // ->
                 // addi.n/addi/addmi at,ax,-(C+1)
                 // addi.n            au,at,1
                 // saltu             ay,au,at
                 CurDAG->SelectNodeTo(N, Xtensa::SALTU, MVT::i32,
                                      SDValue(inc, 0), SDValue(addi, 0))
                 :
                 // ay = ax != C
                 // ->
                 // addi.n/addi/addmi at,ax,-(C+1)
                 // addi.n            au,at,1
                 // saltu             ay,at,au
                 CurDAG->SelectNodeTo(N, Xtensa::SALTU, MVT::i32,
                                      SDValue(addi, 0), SDValue(inc, 0));
    }
  }

  return nullptr;
}

SDNode *XtensaDAGToDAGISel::SelectSELECT_CC(SDNode *N) {
  assert(N->getOpcode() == ISD::SELECT_CC && "Wrong opcode");
  SDLoc dl(N);

  EVT VT = N->getValueType(0);
  if (!VT.isInteger())
    return nullptr;

  SDValue CondLHS = N->getOperand(0);
  SDValue CondRHS = N->getOperand(1);
  SDValue TrueValue = N->getOperand(2);
  SDValue FalseValue = N->getOperand(3);

  // Verify that everything is integer.
  assert((CondLHS.getValueType() == MVT::i32 &&
          CondRHS.getValueType() == MVT::i32 &&
          TrueValue.getValueType() == MVT::i32 &&
          FalseValue.getValueType() == MVT::i32) &&
         "Types must be all MVT::i32.");

  SDValue tem;
  ISD::CondCode CC = cast<CondCodeSDNode>(N->getOperand(4))->get();

  // Canonicalize to EQ, LT, or ULT.
  switch (CC) {
  case ISD::SETGT:
  case ISD::SETUGT:
    tem = CondLHS, CondLHS = CondRHS, CondRHS = tem;
    CC = CC == ISD::SETGT ? ISD::SETLT : ISD::SETULT;
    break;
  case ISD::SETGE:
  case ISD::SETUGE:
    tem = TrueValue, TrueValue = FalseValue, FalseValue = tem;
    CC = CC == ISD::SETGE ? ISD::SETLT : ISD::SETULT;
    break;
  case ISD::SETLE:
  case ISD::SETULE:
    tem = CondLHS, CondLHS = CondRHS, CondRHS = tem;
    tem = TrueValue, TrueValue = FalseValue, FalseValue = tem;
    CC = CC == ISD::SETLE ? ISD::SETLT : ISD::SETULT;
    break;
  case ISD::SETNE:
    tem = TrueValue, TrueValue = FalseValue, FalseValue = tem;
    CC = ISD::SETEQ;
    break;
  default:
    break;
  }

  ConstantSDNode *CondLHSC = dyn_cast<ConstantSDNode>(CondLHS);
  ConstantSDNode *CondRHSC = dyn_cast<ConstantSDNode>(CondRHS);
  ConstantSDNode *TrueValueC = dyn_cast<ConstantSDNode>(TrueValue);
  ConstantSDNode *FalseValueC = dyn_cast<ConstantSDNode>(FalseValue);

  int32_t L = CondLHSC ? CondLHSC->getSExtValue() : 0;
  int32_t R = CondRHSC ? CondRHSC->getSExtValue() : 0;
  int32_t T = TrueValueC ? TrueValueC->getSExtValue() : 0;
  int32_t F = FalseValueC ? FalseValueC->getSExtValue() : 0;

  switch (CC) {
  case ISD::SETEQ: {
    SDValue NewCondLHS;
    bool ZeroOrOneP = false;
    if (CondRHSC && R == 0) {
      NewCondLHS = CondLHS;
      // If we have (X & C) == 0, consider using EXTUI or SLLI to
      // extract the bits we are interested in.
      if (CondLHS->hasOneUse() && CondLHS.getOpcode() == ISD::AND) {
        SDValue CondLHS0 = CondLHS.getOperand(0);
        SDValue CondLHS1 = CondLHS.getOperand(1);
        if (ConstantSDNode *MaskC = dyn_cast<ConstantSDNode>(CondLHS1)) {
          uint32_t Mask = MaskC->getZExtValue();
          if (isShiftedMask_32(Mask)) {
            unsigned Shift = countTrailingZeros(Mask);
            unsigned Width = countTrailingOnes(Mask >> Shift);
            if (Width <= 16) {
              // Use EXTUI to extract bits we are interested in.
              SDNode *extui = CurDAG->getMachineNode(Xtensa::EXTUI,
                                                     dl, MVT::i32,
                                                     CondLHS0,
                                                     getI32Imm(Shift, dl),
                                                     getI32Imm(Width, dl));
              NewCondLHS = SDValue(extui, 0);
              ZeroOrOneP = isPowerOf2_32(Mask);
            } else if (Shift == 0) {
              // Use SLLI to get rid of the bits we don't care.
              unsigned L = 32 - Width;
              SDNode *slli = CurDAG->getMachineNode(Xtensa::SLLI,
                                                    dl, MVT::i32,
                                                    CondLHS0,
                                                    getI32Imm(L, dl));
              NewCondLHS = SDValue(slli, 0);
            }
            // Similarly, we could try SRLI here, but we don't.
            // LLVM transforms X & 0xffff0000 to X >u 65535.
          }
        }
      }
    } else {
      // Subtract CondRHS from CondLHS.
      SDNode *sub;
      if (CondRHSC && immSingleAddi(-R))
        sub = addImmediate(dl, CondLHS, -R);
      else
        sub = CurDAG->getMachineNode(Xtensa::SUB, dl,
                                     MVT::i32, CondLHS, CondRHS);
      NewCondLHS = SDValue(sub, 0);
    }

    if (ZeroOrOneP && TrueValueC && T == 0 &&
        FalseValueC && (F == 3 || F == 5 || F == 7 || F == 9)) {
      unsigned addx_opcode = (F == 3 ? Xtensa::ADDX2 :
                              F == 5 ? Xtensa::ADDX4 :
                              F == 7 ? Xtensa::SUBX8 :
                              Xtensa::ADDX8);
      return CurDAG->SelectNodeTo(N, addx_opcode, MVT::i32,
                                  NewCondLHS, NewCondLHS);
    }

    if (TrueValueC) {
      if (T == 0)
        // If T == 0, use NewCondLHS as 0 to avoid loading 0.
        TrueValue = NewCondLHS;
      else if (CondRHSC && T == R)
        // If T == R, use CondLHS as R to avoid loading R.
        TrueValue = CondLHS;
    }

    // Recognize CondLHS == CondRHS ? TrueValue : FalseValue.
    // Two instructions, with T and F already in regs.
    // addi.n/addi/addmi/nop
    // movnez
    return CurDAG->SelectNodeTo(N, Xtensa::MOVNEZ, MVT::i32,
                                TrueValue,  // when CondLHS == R
                                FalseValue, // when CondLHS != R
                                NewCondLHS);
  }
  case ISD::SETLT:
    // The target-independent optimizer already optimizes:
    //   CondLHS < 0 ? TrueValue : 0
    // to
    //   (CondLHS >>s 31) & TrueValue
    // Recognize CondLHS < 0 ? TrueValue : FalseValue.
    if (CondRHSC && R == 0)
      // One instruction, with F and T already in regs.
      // movltz
      return CurDAG->SelectNodeTo(N, Xtensa::MOVLTZ, MVT::i32,
                                  FalseValue, // when CondLHS < 0
                                  TrueValue,  // when CondLHS >= 0
                                  CondLHS);

    // Recognize the following as clamps
    //
    //   32766 < CondRHS ? 32767 : smax(CondLHS, -32768);
    if (Subtarget->hasClamps() &&
        CondLHSC &&
        TrueValueC &&
        isMask_32(T) &&
        (T == L || T - 1 == L)) {
      ConstantSDNode *FV1C = nullptr;
      const SDValue *FalseValue1 = nullptr;
      unsigned Bits = 32;
      if (FalseValue.getOpcode() == ISD::SMAX &&
          FalseValue.getOperand(0) == CondRHS) {
        FV1C = dyn_cast<ConstantSDNode>(FalseValue.getOperand(1));
      } else if (FalseValue.getOpcode() == ISD::SIGN_EXTEND_INREG) {
        EVT Evt = cast<VTSDNode>(FalseValue.getOperand(1))->getVT();
        Bits = Evt.getScalarType().getSizeInBits();
        FalseValue1 = &FalseValue.getOperand(0);
        if (FalseValue1->getOpcode() == ISD::SMAX &&
            FalseValue1->getOperand(0) == CondRHS) {
          FV1C = dyn_cast<ConstantSDNode>(FalseValue1->getOperand(1));
        }
      }
      if (FV1C && FV1C->getSExtValue() == -T - 1) {
        unsigned ct1 = countTrailingOnes((uint32_t) T);
        if (7 <= ct1 && ct1 <= (Bits > 22 ? 22 : Bits - 1)) {
          if (FalseValue1) {
            if (!FalseValue.hasOneUse() || !FalseValue1->hasOneUse())
              return nullptr;

            ReplaceUses(N, FalseValue.getNode());
            ReplaceUses(FalseValue1->getNode(), N);
          }
          return CurDAG->SelectNodeTo(N, Xtensa::CLAMPS, MVT::i32,
                                      CondRHS, getI32Imm(ct1, dl));
        }
      }
    }

    // Recognize the following as clamps
    //
    //   CondLHS < -32768 ? -32768 : smin(CondLHS, 32767)
    if (Subtarget->hasClamps() &&
        CondRHSC &&
        TrueValueC &&
        isPowerOf2_32(-T) &&
        (T == R || T + 1 == R)) {
      ConstantSDNode *FV1C = nullptr;
      const SDValue *FalseValue1 = nullptr;
      unsigned Bits = 32;
      if (FalseValue.getOpcode() == ISD::SMIN &&
          FalseValue.getOperand(0) == CondLHS) {
        FV1C = dyn_cast<ConstantSDNode>(FalseValue.getOperand(1));
      } else if (FalseValue.getOpcode() == ISD::SIGN_EXTEND_INREG) {
        EVT Evt = cast<VTSDNode>(FalseValue.getOperand(1))->getVT();
        Bits = Evt.getScalarType().getSizeInBits();
        FalseValue1 = &FalseValue.getOperand(0);
        if (FalseValue1->getOpcode() == ISD::SMIN &&
            FalseValue1->getOperand(0) == CondLHS) {
          FV1C = dyn_cast<ConstantSDNode>(FalseValue1->getOperand(1));
        }
      }
      if (FV1C && FV1C->getSExtValue() == -T - 1) {
        unsigned ct1 = countTrailingOnes((uint32_t) FV1C->getSExtValue());
        if (7 <= ct1 && ct1 <= (Bits > 22 ? 22 : Bits - 1)) {
          if (FalseValue1) {
            if (!FalseValue.hasOneUse() || !FalseValue1->hasOneUse())
              return nullptr;

            ReplaceUses(N, FalseValue.getNode());
            ReplaceUses(FalseValue1->getNode(), N);
          }
          return CurDAG->SelectNodeTo(N, Xtensa::CLAMPS, MVT::i32,
                                      CondLHS, getI32Imm(ct1, dl));
        }
      }
    }

    // Recognize -1 < CondRHS ? TrueValue : FalseValue.
    if (CondLHSC && L == -1)
      // One instruction, with F and T already in regs.
      // movltz
      return CurDAG->SelectNodeTo(N, Xtensa::MOVLTZ, MVT::i32,
                                  TrueValue,  // when CondRHS >= 0
                                  FalseValue, // when CondRHS < 0
                                  CondRHS);

    // Fall through.
  case ISD::SETULT: {
    // Recognize CondLHS < CondRHS ? op(CondLHS, X) : op(CondRHS, X),
    // where op can be any binary operator.  We handle commutative ops
    // as well.
    if (TrueValue.getOpcode() == FalseValue.getOpcode() &&
        (TrueValue.getOpcode() == ISD::UMIN ||
         TrueValue.getOpcode() == ISD::UMAX ||
         TrueValue.getOpcode() == ISD::SMIN ||
         TrueValue.getOpcode() == ISD::SMAX)) {
      unsigned innerOpcode = CC == ISD::SETULT ? Xtensa::MINU : Xtensa::MIN;
      unsigned outerOpcode = Xtensa::NOP;

      switch (TrueValue.getOpcode()) {
      case ISD::SMAX: outerOpcode = Xtensa::MAX; break;
      case ISD::SMIN: outerOpcode = Xtensa::MIN; break;
      case ISD::UMAX: outerOpcode = Xtensa::MAXU; break;
      case ISD::UMIN: outerOpcode = Xtensa::MINU; break;
      default:
        llvm_unreachable("unexpected opcode");
      }

      for (unsigned i = 0; i < 2; i++) {
        for (unsigned j = 0; j < 2; j++) {
          if (CondLHS == TrueValue.getOperand(i) &&
              CondRHS == FalseValue.getOperand(j) &&
              TrueValue.getOperand(1 - i) == TrueValue.getOperand(1 - j)) {
            SDNode *inner = CurDAG->getMachineNode(innerOpcode, dl, MVT::i32,
                                                  CondLHS, CondRHS);
            return CurDAG->SelectNodeTo(N, outerOpcode, MVT::i32,
                                        TrueValue.getOperand(1 - i),
                                        SDValue(inner, 0));
          }
        }
      }
    }

    unsigned salt_opcode = (CC == ISD::SETLT
                            ? Xtensa::SALT : Xtensa::SALTU);

    if (CC == ISD::SETULT && CondLHSC && isPowerOf2_32(L + 1)) {
      unsigned ShiftCount = Log2_32(L + 1);
      SDNode *srli = shiftRightLogicalImmediate(dl, CondRHS, ShiftCount);

      if (FalseValueC && F == 0)
        // If F == 0, use the result of SRLI/EXTUI as 0 to avoid
        // loading 0.
        FalseValue = SDValue(srli, 0);

      // Transform
      //   ((1 << C) - 1) < CondRHS ? T : F
      // to
      //   t = CondLHS >>u C
      //   t == 0 ? F : T
      //
      // Two instructions, with T and F already in a reg.
      // srli/extui
      // movnez
      return CurDAG->SelectNodeTo(N, Xtensa::MOVNEZ, MVT::i32,
                                  FalseValue,
                                  TrueValue,
                                  SDValue(srli,0));
    }

    if (CC == ISD::SETULT && CondRHSC && isPowerOf2_32(R)) {
      unsigned ShiftCount = Log2_32(R);
      SDNode *srli = shiftRightLogicalImmediate(dl, CondLHS, ShiftCount);

      if (TrueValueC && T == 0)
        // If T == 0, use the result of SRLI/EXTUI as 0 to avoid
        // loading 0.
        TrueValue = SDValue(srli, 0);

      // Transform
      //   CondLHS < (1 << C) ? T : F
      // to
      //   t = CondLHS >>u C
      //   t == 0 ? T : F
      //
      // Two instructions, with T and F already in a reg.
      // srli/extui
      // movnez
      return CurDAG->SelectNodeTo(N, Xtensa::MOVNEZ, MVT::i32,
                                  TrueValue,
                                  FalseValue,
                                  SDValue(srli,0));
    }

    if (Subtarget->hasSalt() && FalseValueC && F == 0) {
      // Transform
      //   CondLHS < CondRHS ? 3 : 0
      // to
      //   (CondLHS < CondRHS) * 3
      // where the multiplication is done with ADDX[248] or SUBX8.
      if (TrueValueC && (T == 3 || T == 5 || T == 7 || T == 9)) {
        unsigned addx_opcode = (T == 3 ? Xtensa::ADDX2 :
                                T == 5 ? Xtensa::ADDX4 :
                                T == 7 ? Xtensa::SUBX8 :
                                Xtensa::ADDX8);
        SDNode *salt = CurDAG->getMachineNode(salt_opcode, dl,
                                              MVT::i32, CondLHS, CondRHS);
        return CurDAG->SelectNodeTo(N, addx_opcode, MVT::i32,
                                    SDValue(salt, 0),
                                    SDValue(salt, 0));
      }

      // Transform
      //   CondLHS < CondRHS ? -1 : 0
      // to
      //   -(CondLHS < CondRHS)
      if (TrueValueC && T == -1) {
        // Two instructions
        // salt/saltu
        // neg
        SDNode *salt = CurDAG->getMachineNode(salt_opcode, dl,
                                              MVT::i32, CondLHS, CondRHS);
        return CurDAG->SelectNodeTo(N, Xtensa::NEG, MVT::i32,
                                    SDValue(salt, 0));
      }
      // We do not have to handle:
      //
      //   CondLHS < CondRHS ? (1 << C) : 0
      //
      // here because the target-independent optimizer transforms
      // it to:
      //
      //   (CondLHS < CondRHS) << C

      // We do not have to handle:
      //
      //   CondLHS < 0 ? (1 << C) : 0
      //
      // here because the target-independent optimizer transforms it to:
      //
      //   (CondLHS >>u (31 - C)) & (1 << C)
      //
      // and a Pat in XtensaInstrInfo.td further transforms it to:
      //
      //   (CondLHS >>u 31) << C

      // Recognize CondLHS < CondRHS ? TrueValue : 0.
      {
        SDNode *salt = CurDAG->getMachineNode(salt_opcode, dl,
                                              MVT::i32, CondLHS, CondRHS);
        return CurDAG->SelectNodeTo(N, Xtensa::MOVNEZ, MVT::i32,
                                    SDValue(salt, 0),
                                    TrueValue,
                                    SDValue(salt, 0));
      }
    }

    // Transform
    //   CondLHS < CondRHS ? N+1 : N
    // to
    //   (CondLHS < CondRHS) + N
    // if we can perform the addition with ADDI.N, ADDI, or ADDMI.
    if (Subtarget->hasSalt() && FalseValueC && TrueValueC &&
        F != 0 && (unsigned) F + 1 == (unsigned) T &&
        immSingleAddi(F)) {
      // Two instructions
      // salt/saltu
      // addi.n/addi/addmi
      unsigned add_opcode = getADDIOpcode(F);
      SDNode *salt = CurDAG->getMachineNode(salt_opcode, dl,
                                            MVT::i32, CondLHS, CondRHS);
      return CurDAG->SelectNodeTo(N, add_opcode, MVT::i32,
                                  SDValue(salt, 0),
                                  getI32Imm(F, dl));
    }

    // Transform
    //   CondLHS < CondRHS ? N-1 : N
    // to
    //   N - (CondLHS < CondRHS)
    if (Subtarget->hasSalt() && FalseValueC && TrueValueC &&
        F != 0 && (unsigned) T + 1 == (unsigned) F &&
        immSingleMovi(F)) {
      // Three instructions, with the tree height being 2
      // movi.n/movi
      // salt/saltu
      // sub
      SDNode *movi = movImmediate(dl, F);
      SDNode *salt = CurDAG->getMachineNode(salt_opcode, dl,
                                            MVT::i32, CondLHS, CondRHS);
      return CurDAG->SelectNodeTo(N, Xtensa::SUB, MVT::i32,
                                  SDValue(movi, 0),
                                  SDValue(salt, 0));
    }

    // Transform
    //   CondLHS < CondRHS ? N+{2,4,8} : N
    // to
    //   N + (CondLHS < CondRHS) * {2,4,8}
    if (Subtarget->hasSalt() &&
        FalseValueC && TrueValueC && F != 0 && immSingleMovi(F)) {
      unsigned addx_opcode = Xtensa::NOP; // Something other than a MOVI.

      // Pick an opcode for the immediate.
      if ((unsigned) F + 2 == (unsigned) T)
        addx_opcode = Xtensa::ADDX2;
      else if ((unsigned) F + 4 == (unsigned) T)
        addx_opcode = Xtensa::ADDX4;
      else if ((unsigned) F + 8 == (unsigned) T)
        addx_opcode = Xtensa::ADDX8;

      if (addx_opcode != Xtensa::NOP) {
        // Three instructions, with the tree height being 2
        // movi.n/movi
        // salt/saltu
        // addx2/4/8
        SDNode *movi = movImmediate(dl, F);
        SDNode *salt = CurDAG->getMachineNode(salt_opcode, dl,
                                              MVT::i32, CondLHS, CondRHS);
        return CurDAG->SelectNodeTo(N, addx_opcode, MVT::i32,
                                    SDValue(salt, 0), SDValue(movi, 0));
      }
    }

    // Recognize CondLHS < CondRHS ? TrueValue : FalseValue.
    if (Subtarget->hasSalt()) {
      // Two instructions, assuming T and F are in regs
      // salt/saltu
      // movnez
      SDNode *salt = CurDAG->getMachineNode(salt_opcode, dl,
                                            MVT::i32, CondLHS, CondRHS);
      return CurDAG->SelectNodeTo(N, Xtensa::MOVNEZ, MVT::i32,
                                  FalseValue,
                                  TrueValue,
                                  SDValue(salt, 0));
    }
    break;
  }
  default:
    break;
  }

  return nullptr;
}

SDNode *XtensaDAGToDAGISel::SelectConstant(SDNode *N) {
  assert(N->getOpcode() == ISD::Constant && "Wrong opcode");
  SDLoc dl(N);

  int64_t Val = cast<ConstantSDNode>(N)->getSExtValue();
  if (immMOVI(Val))
    return nullptr;

  // Emit pseudo instruction LOAD_CONST.
  // Depends on configuration we will convert it into two CONST16 or L32.
  return CurDAG->SelectNodeTo(N, Xtensa::LOAD_CONST, MVT::i32,
                                getI32Imm(Val, dl));
}

SDNode *XtensaDAGToDAGISel::SelectPCRelativeWrapper(SDNode *N) {
  assert(N->getOpcode() == XtensaISD::PCRelativeWrapper && "Wrong opcode");

  const ConstantPoolSDNode *CP = cast<ConstantPoolSDNode>(N->getOperand(0));
  SDLoc DL(CP);
  EVT PtrVT = CP->getValueType(0);

  if (Subtarget->hasConst16()) {
    // Create a pair of CONST16 low/hi nodes
    SDValue TGALo, TGAHi;
    if (CP->isMachineConstantPoolEntry()) {
      TGALo = CurDAG->getTargetConstantPool(CP->getMachineCPVal(), PtrVT,
                                            CP->getAlignment(), CP->getOffset(),
                                            XtensaII::MO_LO16);
      TGAHi = CurDAG->getTargetConstantPool(CP->getMachineCPVal(), PtrVT,
                                            CP->getAlignment(), CP->getOffset(),
                                            XtensaII::MO_HI16);
    } else {
      TGALo = CurDAG->getTargetConstantPool(CP->getConstVal(), PtrVT,
                                            CP->getAlignment(), CP->getOffset(),
                                            XtensaII::MO_LO16);
      TGAHi = CurDAG->getTargetConstantPool(CP->getConstVal(), PtrVT,
                                            CP->getAlignment(), CP->getOffset(),
                                            XtensaII::MO_HI16);
    }
    SDNode *ImplicitDef = CurDAG->getMachineNode(TargetOpcode::IMPLICIT_DEF,
                                                 DL, MVT::i32);
    SDNode *Const16 = CurDAG->getMachineNode(Xtensa::CONST16, DL, MVT::i32,
                                             SDValue(ImplicitDef, 0),
                                             TGAHi);
    return CurDAG->SelectNodeTo(N, Xtensa::CONST16, MVT::i32,
                                SDValue(Const16, 0), TGALo);
  } else {
    // Create the pseudo movi from the targetconstpool 
    SDValue CPAddr;
    if (CP->isMachineConstantPoolEntry()) {
      CPAddr = CurDAG->getTargetConstantPool(CP->getMachineCPVal(), PtrVT,
                                             CP->getAlignment(), 
                                             CP->getOffset());
    } else {
      CPAddr = CurDAG->getTargetConstantPool(CP->getConstVal(), PtrVT,
                                             CP->getAlignment(), 
                                             CP->getOffset());
                                            
    }
    SDNode *Movi = CurDAG->SelectNodeTo(N, Xtensa::MOVI_ADDR, MVT::i32, 
                                        MVT::Other, CPAddr, 
                                        CurDAG->getEntryNode());
    MachineMemOperand *MemOp =
        MF->getMachineMemOperand(MachinePointerInfo::getConstantPool(*MF),
                                 MachineMemOperand::MOLoad, 4, 4);
    CurDAG->setNodeMemRefs(cast<MachineSDNode>(Movi), {MemOp});
    return Movi;
  }
}

SDNode *XtensaDAGToDAGISel::SelectGlobalAddress(SDNode *N) {
  assert(N->getOpcode() == ISD::GlobalAddress && "Wrong opcode");

  SDLoc dl(N);
  const GlobalAddressSDNode *GN = cast<GlobalAddressSDNode>(N);
  const GlobalValue *GV = GN->getGlobal();
  int64_t Offset = GN->getOffset();

  // Emit pseudo instruction LOAD_CONST.
  // Depends on configuration we will convert it into two CONST16 or L32.
  return CurDAG->SelectNodeTo(N, Xtensa::LOAD_CONST, MVT::i32,
		  CurDAG->getTargetGlobalAddress(GV, dl, MVT::i32, Offset));

}

SDNode *XtensaDAGToDAGISel::SelectExternalSymbol(SDNode *N) {
  assert(N->getOpcode() == ISD::ExternalSymbol && "Wrong opcode");

  const ExternalSymbolSDNode *GE = cast<ExternalSymbolSDNode>(N);

  // Emit pseudo instruction LOAD_CONST.
  // Depends on configuration we will convert it into two CONST16 or L32.
  return CurDAG->SelectNodeTo(N, Xtensa::LOAD_CONST, MVT::i32,
		  CurDAG->getTargetExternalSymbol(GE->getSymbol(), MVT::i32));
}


SDNode *XtensaDAGToDAGISel::SelectBlockAddress(SDNode *N) {
  assert(N->getOpcode() == ISD::BlockAddress && "Wrong opcode");
  SDLoc dl(N);

  // We generate an L32R here for ISD::GlobalAddress instead of
  // wrapping ISD::GlobalAddress with pcrelwrapper in
  // XtensaTargetLowering::LowerOperation.  We do so because wrapping
  // ISD::GlobalAddress with pcrelwrapper would cause a problem with:
  //
  // (select cond (load (pcrelwrapper ...) (load (pcrelwrapper))))
  //
  // Specifically, DAGCombiner::SimplifySelectOps would factor out the
  // load ouside the select and generate:
  //
  // (load (select cond (pcrelwrapper ...) (pcrelwrapper)))
  //
  // We would not be able to select instructions for this DAG.
  const BlockAddress *BA = cast<BlockAddressSDNode>(N)->getBlockAddress();
  return CurDAG->SelectNodeTo(N, Xtensa::LOAD_CONST, MVT::i32,
                              CurDAG->getTargetBlockAddress(BA, MVT::i32));
}

SDNode *XtensaDAGToDAGISel::SelectJumpTable(SDNode *N) {
  assert(N->getOpcode() == ISD::JumpTable && "Wrong opcode");
  SDLoc dl(N);

  JumpTableSDNode *JT = cast<JumpTableSDNode>(N);
  SDValue JTAddr =
      CurDAG->getTargetJumpTable(JT->getIndex(), MVT::i32);
  return CurDAG->SelectNodeTo(N, Xtensa::LOAD_CONST, MVT::i32, JTAddr);
}


void XtensaDAGToDAGISel::Select(SDNode *N) {
  SDLoc dl(N);
  SDNode *NewN = nullptr;

  if (N->isMachineOpcode()) {
    N->setNodeId(-1);
    return;   // Already selected.
  }

  switch (N->getOpcode()) {
  default:
    break;
  case XtensaISD::PCRelativeWrapper:
    NewN = SelectPCRelativeWrapper(N);
    break;
  case ISD::ADD:
    NewN = SelectADD(N);
    break;
  case ISD::SMIN:
  case ISD::SMAX:
    NewN = SelectSMIN_SMAX(N);
    break;
  case ISD::MUL:
    NewN = SelectMUL(N);
    break;
  case ISD::AND:
    NewN = SelectAND(N);
    break;
  case ISD::OR:
    NewN = SelectOR(N);
    break;
  case ISD::SHL:
  case ISD::SRL:
  case ISD::SRA:
    NewN = SelectSHL_SRL_SRA(N);
    break;
  case ISD::ROTL:
  case ISD::ROTR:
    NewN = SelectROTL_ROTR(N);
    break;
  case ISD::SETCC:
    NewN = SelectSETCC(N);
    break;
  case ISD::SELECT_CC:
    NewN = SelectSELECT_CC(N);
    break;
  case ISD::Constant:
    NewN = SelectConstant(N);
    break;
  case ISD::GlobalAddress:
    NewN = SelectGlobalAddress(N);
    break;
  case ISD::ExternalSymbol:
    NewN = SelectExternalSymbol(N);
    break;
  case ISD::BlockAddress:
    NewN = SelectBlockAddress(N);
    break;
  // Note, we return early here since the SelectINTRINSIC could return a
  // a nullptr to denote that ReplaceAllUses is already done
  case ISD::INTRINSIC_VOID:
  case ISD::INTRINSIC_W_CHAIN: {
    if (SelectINTRINSIC_W_CHAIN(N))
      // We've taken care of everything.
      return;

    break;
  }
  case ISD::INTRINSIC_WO_CHAIN: {
    if (SelectINTRINSIC_WO_CHAIN(N))
      // We've taken care of everything.
      return;

    break;
  }
  case ISD::JumpTable:
    NewN = SelectJumpTable(N);
    break;
  }

  // If the helpers above return non-null, we are done.
  if (NewN)
    return;

  SelectCode(N);
}
