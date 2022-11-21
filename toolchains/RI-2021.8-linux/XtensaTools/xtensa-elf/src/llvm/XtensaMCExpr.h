//===-- XtensaMCExpr.h - Xtensa specific MC expression classes ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_XTENSA_XTENSAMCEXPR_H
#define LLVM_LIB_TARGET_XTENSA_XTENSAMCEXPR_H

#include "llvm/MC/MCAsmLayout.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCValue.h"

namespace llvm {

class XtensaMCExpr : public MCTargetExpr {
public:
  enum VariantKind {
    VK_Xtensa_None,
    VK_Xtensa_LO,
    VK_Xtensa_HI,
    VK_Xtensa_Comment
  };

private:
  const VariantKind Kind;
  const MCExpr *Expr;
  const StringRef Comment;

  explicit XtensaMCExpr(VariantKind Kind, const MCExpr *Expr)
      : Kind(Kind), Expr(Expr), Comment() {}

  explicit XtensaMCExpr(VariantKind Kind, const MCExpr *Expr, StringRef S)
      : Kind(Kind), Expr(Expr), Comment(S) {}

public:
  static const XtensaMCExpr *create(VariantKind Kind, const MCExpr *Expr,
                                    MCContext &Ctx);

  static const XtensaMCExpr *create(VariantKind Kind, const MCExpr *Expr,
                                    StringRef S, MCContext &Ctx);

  static const XtensaMCExpr *createLo(const MCExpr *Expr, MCContext &Ctx) {
    return create(VK_Xtensa_LO, Expr, Ctx);
  }

  static const XtensaMCExpr *createHi(const MCExpr *Expr, MCContext &Ctx) {
    return create(VK_Xtensa_HI, Expr, Ctx);
  }

  /// getKind - Get the kind of this expression.
  VariantKind getKind() const { return Kind; }

  /// getSubExpr - Get the child of this expression.
  const MCExpr *getSubExpr() const { return Expr; }

  /// getString - Get Comment for kind VK_Xtensa_Comment
  const StringRef getComment() const {
    if (getKind() == VK_Xtensa_Comment)
      return Comment;
    return StringRef();
  }

  void printImpl(raw_ostream &OS, const MCAsmInfo *MAI) const override;
  bool evaluateAsRelocatableImpl(MCValue &Res, const MCAsmLayout *Layout,
                                 const MCFixup *Fixup) const override {
    return false;
  }
  void visitUsedExpr(MCStreamer &Streamer) const override;
  MCFragment *findAssociatedFragment() const override {
    return getSubExpr()->findAssociatedFragment();
  }

  // There are no TLS XtensaMCExprs at the moment.
  void fixELFSymbolsInTLSFixups(MCAssembler &Asm) const override {}

  static bool classof(const MCExpr *E) {
    return E->getKind() == MCExpr::Target;
  }
};
} // end namespace llvm

#endif
