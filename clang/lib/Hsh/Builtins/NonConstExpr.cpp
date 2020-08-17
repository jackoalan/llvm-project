//===--- NonConstExpr.cpp - utility for NonConstExpr traversal ------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "NonConstExpr.h"

namespace clang::hshgen {

bool CheckConstexprTemplateSpecialization(
    ASTContext &Context, ClassTemplateSpecializationDecl *Spec,
    SmallVectorImpl<NonConstExpr> *NonConstExprs) {
  bool Ret = true;
  unsigned Position = 0;
  for (const auto &Arg : Spec->getTemplateArgs().asArray()) {
    switch (Arg.getKind()) {
    case TemplateArgument::Type:
      Ret &= CheckConstexprTemplateSpecialization(Context, Arg.getAsType(),
                                                  NonConstExprs, Position);
      break;
    case TemplateArgument::Expression: {
      Optional<llvm::APSInt> Value =
          Arg.getAsExpr()->getIntegerConstantExpr(Context);
      if (!Value) {
        Ret = false;
        if (NonConstExprs)
          NonConstExprs->emplace_back(Arg.getAsExpr(), Position);
      } else if (NonConstExprs) {
        NonConstExprs->emplace_back(*Value, Arg.getAsExpr()->getType());
      }
      break;
    }
    case TemplateArgument::Integral:
      if (NonConstExprs)
        NonConstExprs->emplace_back(Arg.getAsIntegral(), Arg.getIntegralType());
      break;
    default:
      break;
    }
    ++Position;
  }
  return Ret;
}

bool CheckConstexprTemplateSpecialization(
    ASTContext &Context, QualType Tp,
    SmallVectorImpl<NonConstExpr> *NonConstExprs, unsigned Position) {
  if (auto *ExpDecl = Tp->getAsCXXRecordDecl()) {
    if (auto *Spec = dyn_cast<ClassTemplateSpecializationDecl>(ExpDecl)) {
      if (NonConstExprs)
        NonConstExprs->emplace_back(Spec, Position);
      bool Ret =
          CheckConstexprTemplateSpecialization(Context, Spec, NonConstExprs);
      if (NonConstExprs)
        NonConstExprs->emplace_back(NonConstExpr::Pop{});
      return Ret;
    }
  }
  return true;
}

} // namespace clang::hshgen
