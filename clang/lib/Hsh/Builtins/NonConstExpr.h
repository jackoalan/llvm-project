//===--- NonConstExpr.h - utility for NonConstExpr traversal --------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma once

#include "clang/AST/ExprCXX.h"

#include <stack>

namespace clang::hshgen {

class NonConstExpr {
public:
  enum Kind { NonTypeParm, TypePush, TypePop, Integer };

private:
  enum Kind Kind;
  llvm::PointerUnion<Expr *, ClassTemplateSpecializationDecl *,
                     const clang::Type *>
      ExprOrType;
  llvm::APSInt Int;
  unsigned Position;

public:
  explicit NonConstExpr(Expr *E, unsigned Position)
      : Kind(NonTypeParm), ExprOrType(E), Position(Position) {}
  explicit NonConstExpr(ClassTemplateSpecializationDecl *D, unsigned Position)
      : Kind(TypePush), ExprOrType(D), Position(Position) {}
  struct Pop {};
  explicit NonConstExpr(Pop)
      : Kind(TypePop), ExprOrType(nullptr), Position(0) {}
  explicit NonConstExpr(const llvm::APSInt &Int, QualType Tp)
      : Kind(Integer), ExprOrType(Tp.getTypePtr()), Int(Int) {}

  enum Kind getKind() const { return Kind; }
  Expr *getExpr() const {
    assert(Kind == NonTypeParm);
    return ExprOrType.get<Expr *>();
  }
  unsigned getPosition() const {
    assert(Kind == NonTypeParm || Kind == TypePush);
    return Position;
  }
  ClassTemplateSpecializationDecl *getType() const {
    assert(Kind == TypePush);
    return ExprOrType.get<ClassTemplateSpecializationDecl *>();
  }
  const llvm::APSInt &getInt() const {
    assert(Kind == Integer);
    return Int;
  }
  QualType getIntType() const {
    assert(Kind == Integer);
    return QualType{ExprOrType.get<const clang::Type *>(), 0};
  }
};

bool CheckConstexprTemplateSpecialization(
    ASTContext &Context, ClassTemplateSpecializationDecl *Spec,
    SmallVectorImpl<NonConstExpr> *NonConstExprs = nullptr);

bool CheckConstexprTemplateSpecialization(
    ASTContext &Context, QualType Tp,
    SmallVectorImpl<NonConstExpr> *NonConstExprs, unsigned Position = 0);

template <typename Func>
inline void TraverseNonConstExprs(ArrayRef<NonConstExpr> NCEs, Func F) {
  std::stack<ClassTemplateSpecializationDecl *,
             SmallVector<ClassTemplateSpecializationDecl *, 4>>
      TypeStack;
  for (const auto &Expr : NCEs) {
    switch (Expr.getKind()) {
    case NonConstExpr::NonTypeParm:
      F(cast<NonTypeTemplateParmDecl>(TypeStack.top()
                                          ->getSpecializedTemplateOrPartial()
                                          .get<ClassTemplateDecl *>()
                                          ->getTemplateParameters()
                                          ->getParam(Expr.getPosition())));
      break;
    case NonConstExpr::TypePush:
      TypeStack.push(Expr.getType());
      break;
    case NonConstExpr::TypePop:
      TypeStack.pop();
      break;
    case NonConstExpr::Integer:
      break;
    }
  }
}

template <typename Func, typename PushFunc, typename PopFunc,
          typename IntegerFunc>
inline void TraverseNonConstExprs(ArrayRef<NonConstExpr> NCEs, Func F,
                                  PushFunc Push, PopFunc Pop,
                                  IntegerFunc Integer) {
  std::stack<ClassTemplateSpecializationDecl *,
             SmallVector<ClassTemplateSpecializationDecl *, 4>>
      TypeStack;
  for (const auto &Expr : NCEs) {
    switch (Expr.getKind()) {
    case NonConstExpr::NonTypeParm:
      F(cast<NonTypeTemplateParmDecl>(TypeStack.top()
                                          ->getSpecializedTemplateOrPartial()
                                          .get<ClassTemplateDecl *>()
                                          ->getTemplateParameters()
                                          ->getParam(Expr.getPosition())));
      break;
    case NonConstExpr::TypePush:
      Push(Expr.getType());
      TypeStack.push(Expr.getType());
      break;
    case NonConstExpr::TypePop:
      Pop();
      TypeStack.pop();
      break;
    case NonConstExpr::Integer:
      Integer(Expr.getInt(), Expr.getIntType());
      break;
    }
  }
}

template <typename Func>
inline void TraverseNonConstExprs(ArrayRef<NonConstExpr> NCEs,
                                  ClassTemplateSpecializationDecl *Spec,
                                  Func F) {
  std::stack<ClassTemplateSpecializationDecl *,
             SmallVector<ClassTemplateSpecializationDecl *, 4>>
      TypeStack, SpecStack;
  for (const auto &Expr : NCEs) {
    switch (Expr.getKind()) {
    case NonConstExpr::NonTypeParm:
      F(cast<NonTypeTemplateParmDecl>(TypeStack.top()
                                          ->getSpecializedTemplateOrPartial()
                                          .get<ClassTemplateDecl *>()
                                          ->getTemplateParameters()
                                          ->getParam(Expr.getPosition())),
        SpecStack.top()->getTemplateArgs().get(Expr.getPosition()));
      break;
    case NonConstExpr::TypePush:
      TypeStack.push(Expr.getType());
      if (SpecStack.empty())
        SpecStack.push(Spec);
      else
        SpecStack.push(
            cast<ClassTemplateSpecializationDecl>(SpecStack.top()
                                                      ->getTemplateArgs()
                                                      .get(Expr.getPosition())
                                                      .getAsType()
                                                      ->getAsCXXRecordDecl()));
      break;
    case NonConstExpr::TypePop:
      TypeStack.pop();
      SpecStack.pop();
      break;
    case NonConstExpr::Integer:
      break;
    }
  }
}

} // namespace clang::hshgen
