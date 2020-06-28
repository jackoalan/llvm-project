//===--- Partitioner.h - hshgen statement stage partitioning --------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma once

#include "Builtins/Builtins.h"

namespace clang {
class AnalysisDeclContext;
}

namespace clang::hshgen {
class Builder;

class Partitioner {
public:
  /*
   * Per-statement stage dependencies are centrally tracked here.
   * The first DependencyPass populates the stage bits of each statement in
   * post-order graph traversal. Statements that depend on the output of each
   * statement are also collected here for later lifting and pruning.
   */
  struct StmtDepInfo {
    struct StageBits StageBits;
    llvm::DenseSet<const Stmt *> Dependents;
    void setPrimaryStage(HshStage Stage) {
      if (Stage == HshNoStage)
        StageBits = 0;
      else
        StageBits = 1 << Stage;
    }
    HshStage getMaxStage() const {
      for (int i = HshMaxStage - 1; i >= HshVertexStage; --i) {
        if ((1 << i) & StageBits)
          return HshStage(i);
      }
      return HshVertexStage;
    }
    HshStage getMinStage() const {
      for (int i = HshVertexStage; i < HshMaxStage; ++i) {
        if ((1 << i) & StageBits)
          return HshStage(i);
      }
      return HshVertexStage;
    }
    HshStage getLeqStage(HshStage RefStage) const {
      for (int i = HshMaxStage - 1; i >= HshVertexStage; --i) {
        if (HshStage(i) > RefStage)
          continue;
        if ((1 << i) & StageBits)
          return HshStage(i);
      }
      return HshVertexStage;
    }
    bool hasStage(HshStage Stage) const { return (1 << Stage) & StageBits; }
  };
  using StmtMapType = llvm::DenseMap<const Stmt *, StmtDepInfo>;
  StmtMapType StmtMap;
  std::vector<const Stmt *> OrderedStmts;
  std::array<llvm::DenseSet<const VarDecl *>, HshMaxStage> UsedDecls;

  void UpdateDeclRefExprStages(const DeclStmt *DS, unsigned StageBits) {
    for (auto *Decl : DS->decls()) {
      if (auto *VD = dyn_cast<VarDecl>(Decl)) {
        for (auto &P : StmtMap) {
          if (auto *DRE = dyn_cast<DeclRefExpr>(P.first)) {
            if (DRE->getDecl() == VD)
              P.second.StageBits = StageBits;
          }
        }
      }
    }
  }

  /*
   * Assignments into declarations will mutate their stage dependency.
   * This is information is stored according to the parallel block scope
   * of nested flow control statements.
   */
  struct DeclDepInfo {
    HshStage Stage = HshNoStage;
    /* Mutator statements of decl so far, starting with original DeclStmt */
    llvm::DenseSet<const Stmt *> MutatorStmts;
  };
  using DeclMapType = llvm::DenseMap<const Decl *, DeclDepInfo>;
  DeclMapType FinalDeclMap;

  ASTContext &Context;
  HshBuiltins &Builtins;

  Partitioner(ASTContext &Context, HshBuiltins &Builtins)
      : Context(Context), Builtins(Builtins) {}

  void run(AnalysisDeclContext &AD, Builder &Builder);
};

} // namespace clang::hshgen