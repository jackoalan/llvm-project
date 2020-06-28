//===--- Reporter.h - Diagnostic reporter for hshgen ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma once

#include "clang/AST/ExprCXX.h"

namespace clang::hshgen {

class Reporter {
  const ASTContext &Context;

  template <typename T,
            std::enable_if_t<!std::is_base_of_v<Attr, std::remove_pointer_t<T>>,
                             int> = 0>
  static std::pair<SourceLocation, SourceRange> GetReportLocation(const T *S) {
    return {S->getBeginLoc(), S->getSourceRange()};
  }
  template <typename T,
            std::enable_if_t<std::is_base_of_v<Attr, std::remove_pointer_t<T>>,
                             int> = 0>
  static std::pair<SourceLocation, SourceRange> GetReportLocation(const T *S) {
    return {S->getLocation(), S->getRange()};
  }

public:
  Reporter(const ASTContext &Context) : Context(Context) {}

  template <typename T, unsigned N>
  DiagnosticBuilder
  Custom(const T *S, const char (&FormatString)[N],
         DiagnosticsEngine::Level level = DiagnosticsEngine::Error) {
    auto [Loc, Range] = GetReportLocation(S);
    DiagnosticsEngine &Diags = Context.getDiagnostics();
    return Diags.Report(Loc, Diags.getCustomDiagID(level, FormatString))
           << CharSourceRange(Range, false);
  }

  void UnsupportedStmt(const Stmt *S) {
    auto Diag = Custom(
        S, "statements of type %0 are not supported in hsh generator lambdas");
    Diag.AddString(S->getStmtClassName());
  }

  void UnsupportedFunctionCall(const Stmt *S) {
    Custom(S, "function calls are limited to hsh intrinsics and static "
              "constexpr functions");
  }

  void UnsupportedTypeReference(const Stmt *S) {
    Custom(S, "references to values are limited to hsh types");
  }

  void UnsupportedTypeConstruct(const Stmt *S) {
    Custom(S, "constructors are limited to hsh types");
  }

  void UnsupportedTypeCast(const Stmt *S) {
    Custom(S, "type casts are limited to hsh types");
  }

  void BadTextureReference(const Stmt *S) {
    Custom(S, "texture samples must be performed on lambda parameters");
  }

  void NonConstexprSampler(const Expr *E) {
    Custom(E, "sampler arguments must be constexpr");
  }

  void BadIntegerType(const Decl *D) {
    Custom(D, "integers must be 32-bits in length");
  }

  void BadRecordType(const Decl *D) {
    Custom(D, "hsh record fields must be a builtin hsh vector or matrix, "
              "float, double, or 32-bit integer");
  }

  void ConstAssignment(const Expr *AssignExpr) {
    Custom(AssignExpr, "cannot assign data to previous stages");
  }

  void OverloadedFunctionUsage(const FunctionDecl *Overloaded,
                               const FunctionDecl *Prev) {
    Custom(Overloaded,
           "overloaded functions may not be used in the same pipeline");
    Custom(Prev, "previously used function is here", DiagnosticsEngine::Note);
  }

  void UndefinedFunctionUsage(const FunctionDecl *FD) {
    Custom(FD, "constexpr functions must be fully defined");
  }
};

} // namespace clang::hshgen