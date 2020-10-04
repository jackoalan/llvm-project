//===--- Partitioner.h - hshgen statement stage partitioning --------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Partitioner.h"
#include "Builder.h"
#include "Dumper.h"
#include "Reporter.h"

#include "llvm/Support/SaveAndRestore.h"

#include "clang/AST/StmtVisitor.h"
#include "clang/Analysis/AnalysisDeclContext.h"
#include "clang/Analysis/CFG.h"

using namespace llvm;

namespace clang::hshgen {
namespace {

using DeclDepInfo = Partitioner::DeclDepInfo;
using DeclMapType = Partitioner::DeclMapType;

struct DependencyPass : ConstStmtVisitor<DependencyPass, HshStage> {
  class Partitioner &Partitioner;
  class Builder &Builder;
  explicit DependencyPass(class Partitioner &Partitioner,
                          class Builder &Builder)
      : Partitioner(Partitioner), Builder(Builder) {}

  /*
   * Statement stage dependencies include condition variables that decide
   * if their containing block is reached. The BlockScopeStack follows the
   * block scope of supported flow control statements. Parallel branches
   * (i.e. if-else, switch) are kept separate so they have fresh declaration
   * context from the point of flow control entry.
   */
  struct BlockScopeStack {
    struct StackEntry {
      const CFGBlock *OrigSucc;
      HshStage Stage;
      SmallVector<DeclMapType, 8> ParallelDeclMaps{DeclMapType{}};
      explicit StackEntry(const CFGBlock *OrigSucc, HshStage Stage)
          : OrigSucc(OrigSucc), Stage(Stage) {}
      void popMerge(const StackEntry &Other) {
        /*
         * Only declaration dependencies are merged since they represent
         * forward mutations. The stack entry stage follows the context of
         * condition expressions, therefore not merged.
         */
        auto &NewDeclMap = ParallelDeclMaps.back();
        for (const auto &OldDeclMap : Other.ParallelDeclMaps) {
          for (const auto &[OldDecl, OldMapEntry] : OldDeclMap) {
            auto &NewMapEntry = NewDeclMap[OldDecl];
            NewMapEntry.Stage = std::max(NewMapEntry.Stage, OldMapEntry.Stage);
          }
        }
      }
    };
    SmallVector<StackEntry, 8> Stack;

    void push(const CFGBlock *OrigSucc, HshStage Stage) {
      Stack.emplace_back(OrigSucc, Stage);
    }

    void pop(const CFGBlock *ToSucc) {
      while (Stack.back().OrigSucc == ToSucc) {
        assert(Stack.size() >= 2 && "stack underflow");
        auto It = Stack.rbegin();
        auto &OldTop = *It++;
        auto &NewTop = *It;
        NewTop.popMerge(OldTop);
        Stack.pop_back();
      }
    }

    size_t size() const { return Stack.size(); }

    void newParallelBranch() { Stack.back().ParallelDeclMaps.emplace_back(); }

    DeclDepInfo &getDeclDepInfo(const Decl *D) {
      for (auto I = Stack.rbegin(), E = Stack.rend(); I != E; ++I) {
        auto &DeclMap = I->ParallelDeclMaps.back();
        auto Search = DeclMap.find(D);
        if (Search != DeclMap.end())
          return Search->second;
      }
      return Stack.back().ParallelDeclMaps.back()[D];
    }

    HshStage getContextStage() const { return Stack.back().Stage; }
  };
  BlockScopeStack ScopeStack;

  HshStage VisitDeclStmt(const DeclStmt *DS) {
    HshStage MaxStage = ScopeStack.getContextStage();
    for (const auto *D : DS->decls()) {
      if (auto *VD = dyn_cast<VarDecl>(D)) {
        if (const Expr *Init = VD->getInit()) {
          Partitioner.StmtMap[Init].Dependents.insert(DS);
          auto &DepInfo = ScopeStack.getDeclDepInfo(D);
          DepInfo.Stage = Visit(Init);
          DepInfo.MutatorStmts.insert(DS);
          MaxStage = std::max(MaxStage, DepInfo.Stage);
        }
      }
    }
    Partitioner.StmtMap[DS].setPrimaryStage(MaxStage);
    return MaxStage;
  }

  Expr *AssignMutator = nullptr;
  HshStage VisitDeclRefExpr(const DeclRefExpr *DRE) {
    auto &DepInfo = ScopeStack.getDeclDepInfo(DRE->getDecl());
    if (AssignMutator) {
      DepInfo.MutatorStmts.insert(AssignMutator);
      DepInfo.Stage = std::max(
          DepInfo.Stage, Partitioner.StmtMap[AssignMutator].getMaxStage());
    }
    for (auto *MS : DepInfo.MutatorStmts)
      Partitioner.StmtMap[MS].Dependents.insert(DRE);
    if (auto *VD = dyn_cast<VarDecl>(DRE->getDecl())) {
      DepInfo.Stage =
          std::max(DepInfo.Stage, Partitioner.Builtins.determineVarStage(VD));
    }
    Partitioner.StmtMap[DRE].setPrimaryStage(DepInfo.Stage);
    return DepInfo.Stage;
  }

  HshStage VisitMemberExpr(const MemberExpr *ME) {
    if (auto *FD = dyn_cast<FieldDecl>(ME->getMemberDecl())) {
      auto HPF = Partitioner.Builtins.identifyBuiltinPipelineField(FD);
      if (HPF != HPF_None) {
        Builder.setReferencedPipelineField(HPF);
        auto Stage = Partitioner.Builtins.stageOfBuiltinPipelineField(HPF);
        if (Stage != HshNoStage) {
          if (AssignMutator)
            Builder.setStageUsed(Stage);
          return Stage;
        }
      }
    }
    return VisitStmt(ME);
  }

  HshStage VisitBinaryOperator(const BinaryOperator *BO) {
    if (BO->isAssignmentOp()) {
      HshStage MaxStage = ScopeStack.getContextStage();
      {
        auto *RHS = BO->getRHS();
        Partitioner.StmtMap[RHS].Dependents.insert(BO);
        MaxStage = std::max(MaxStage, Visit(RHS));
      }
      Partitioner.StmtMap[BO].setPrimaryStage(MaxStage);
      {
        SaveAndRestore<Expr *> SavedAssignMutator(AssignMutator, (Expr *)BO);
        auto *LHS = BO->getLHS();
        Partitioner.StmtMap[LHS].Dependents.insert(BO);
        MaxStage = std::max(MaxStage, Visit(LHS));
      }
      Partitioner.StmtMap[BO].setPrimaryStage(MaxStage);
      return MaxStage;
    }
    return VisitStmt(BO);
  }

  HshStage VisitCXXOperatorCallExpr(const CXXOperatorCallExpr *OC) {
    if (OC->isAssignmentOp()) {
      HshStage MaxStage = ScopeStack.getContextStage();
      {
        auto *RHS = OC->getArg(1);
        Partitioner.StmtMap[RHS].Dependents.insert(OC);
        MaxStage = std::max(MaxStage, Visit(RHS));
      }
      Partitioner.StmtMap[OC].setPrimaryStage(MaxStage);
      {
        SaveAndRestore<Expr *> SavedAssignMutator(AssignMutator, (Expr *)OC);
        auto *LHS = OC->getArg(0);
        Partitioner.StmtMap[LHS].Dependents.insert(OC);
        MaxStage = std::max(MaxStage, Visit(LHS));
      }
      Partitioner.StmtMap[OC].setPrimaryStage(MaxStage);
      return MaxStage;
    }
    return VisitStmt(OC);
  }

  HshStage VisitStmt(const Stmt *S) {
    HshStage MaxStage = ScopeStack.getContextStage();
    for (auto *Child : S->children()) {
      Partitioner.StmtMap[Child].Dependents.insert(S);
      MaxStage = std::max(MaxStage, Visit(Child));
    }
    Partitioner.StmtMap[S].setPrimaryStage(MaxStage);
    return MaxStage;
  }

  void run(AnalysisDeclContext &AD) {
    auto *CFG = AD.getCFG();
    size_t EstStmtCount = 0;
    for (auto *B : *CFG)
      EstStmtCount += B->size();
    Partitioner.StmtMap.reserve(EstStmtCount);
    Partitioner.OrderedStmts.reserve(EstStmtCount);
    ScopeStack.push(nullptr, HshNoStage);

    struct SuccStack {
      using PopReturn = PointerIntPair<const CFGBlock *, 2>;
      enum : unsigned { ePoppedExit = 1, ePoppedDo = 2 };
      struct Entry {
        CFGBlock::succ_const_range Succs;
        PopReturn Exit;
        Entry(CFGBlock::succ_const_range Succs, const CFGBlock *Exit,
              bool ExitRoot)
            : Succs(Succs), Exit(Exit, ExitRoot ? ePoppedExit : 0) {}
      };
      SmallVector<Entry, 8> Stack;
      SmallVector<const CFGBlock *, 8> PreStack;
      BitVector ClosedSet;
      explicit SuccStack(class CFG *CFG) : ClosedSet(CFG->getNumBlockIDs()) {
        push(CFG->getEntry().succs(), &CFG->getExit(), true);
      }
      void push(CFGBlock::succ_const_range Succs, const CFGBlock *Exit,
                bool ExitRoot) {
        Stack.emplace_back(Succs, Exit, ExitRoot);
      }
      PopReturn _pop() {
        assert(!Stack.empty() && "popping empty stack");
        auto &StackTop = Stack.back();
        while (!StackTop.Succs.empty()) {
          const auto *Ret = StackTop.Succs.begin()->getReachableBlock();
          StackTop.Succs =
              make_range(StackTop.Succs.begin() + 1, StackTop.Succs.end());
          if (Ret)
            return PopReturn{Ret, 0};
        }
        auto Ret = StackTop.Exit;
        Stack.pop_back();
        return Ret;
      }
      PopReturn pop() {
        while (true) {
          auto Ret = _pop();
          if (Stack.empty())
            return Ret;
          const auto *Block = Ret.getPointer();
          bool PoppedExit = Ret.getInt() & ePoppedExit;
          const auto *Exit = Stack.back().Exit.getPointer();
          if (!PoppedExit && Block->getBlockID() <= Exit->getBlockID())
            continue;
          if (ClosedSet.test(Block->getBlockID()))
            continue;
          ClosedSet.set(Block->getBlockID());
          if (!PreStack.empty() &&
              PreStack.back()->getBlockID() == Block->getBlockID()) {
            PreStack.pop_back();
            Ret.setInt(Ret.getInt() | ePoppedDo);
          }
          if (const auto *DoLoopTarget = Block->getDoLoopTarget()) {
            PreStack.emplace_back(DoLoopTarget);
          }
          const auto *OrigSucc = Block->getTerminator().getOrigSucc();
          push(Block->succs(), OrigSucc ? OrigSucc : Exit, OrigSucc);
          return Ret;
        }
      }
    } SuccStack{CFG};

    bool NeedsParallelBranch = false;
    while (true) {
      auto Ret = SuccStack.pop();
      const auto *Block = Ret.getPointer();
      bool PoppedExit = Ret.getInt() & SuccStack::ePoppedExit;
      bool PoppedDo = Ret.getInt() & SuccStack::ePoppedDo;
      bool AtExit = Block->getBlockID() == CFG->getExit().getBlockID();

      /*
       * The AtExit check is required for the case where a function ends
       * with a DoStmt. The CFG inserts an empty loopback statement that
       * would trigger a second pop and underflow the scope stack.
       */
      if (PoppedExit && !(AtExit && ScopeStack.size() == 1)) {
        dumper() << ScopeStack.size();
        ScopeStack.pop(Block);
        NeedsParallelBranch = false;
        dumper() << " Popped Succ\n";
      }

      /*
       * Exit occurs here to ensure the scope stack is in the correct
       * exit state.
       */
      if (AtExit) {
        assert(ScopeStack.size() == 1 && "unbalanced scope stack");
        break;
      }

      /*
       * DoStmt body scopes are handled in a somewhat reversed fashion.
       * They are not opened by a condition terminator, but hsh still
       * wants to treat their contents as dependents of the while condition.
       */
      if (const auto *DoLoopTarget = Block->getDoLoopTarget()) {
        ScopeStack.push(DoLoopTarget,
                        Visit(DoLoopTarget->getTerminatorCondition()));
        NeedsParallelBranch = false;
        dumper() << ScopeStack.size() << " Pushed Do Succ\n";
      }

      /*
       * If no push-pop operation occurs between blocks, this is a parallel
       * block (i.e. else parallel with if)
       */
      if (NeedsParallelBranch) {
        ScopeStack.newParallelBranch();
        dumper() << "New parallel branch\n";
      }
      NeedsParallelBranch = true;

      /*
       * Scope stack is in the correct state for processing the block at
       * this point.
       */
      dumper() << "visit B" << Block->getBlockID() << '\n';
      for (const auto &Elem : *Block) {
        if (auto Stmt = Elem.getAs<CFGStmt>()) {
          dumper() << Stmt->getStmt() << " ";
          dumper() << Visit(Stmt->getStmt()) << "\n";
          Partitioner.OrderedStmts.push_back(Stmt->getStmt());
        }
      }

      if (PoppedDo) {
        dumper() << ScopeStack.size();
        ScopeStack.pop(Block);
        NeedsParallelBranch = false;
        dumper() << " Popped Do Succ\n";
      }
      if (const auto *OrigSucc = Block->getTerminator().getOrigSucc()) {
        ScopeStack.push(OrigSucc, Visit(Block->getTerminatorCondition()));
        NeedsParallelBranch = false;
        dumper() << ScopeStack.size() << " Pushed Succ\n";
      }
    }

    Partitioner.FinalDeclMap =
        std::move(ScopeStack.Stack[0].ParallelDeclMaps[0]);
  }
};

struct LiftPass : ConstStmtVisitor<LiftPass, bool> {
  class Partitioner &Partitioner;
  explicit LiftPass(class Partitioner &Partitioner)
      : Partitioner(Partitioner) {}

  static bool VisitStmt(const Stmt *) { return false; }

  static bool VisitDeclRefExpr(const DeclRefExpr *) { return true; }
  static bool VisitIntegerLiteral(const IntegerLiteral *) { return true; }
  static bool VisitFloatingLiteral(const FloatingLiteral *) { return true; }
  static bool VisitCXXBoolLiteralExpr(const CXXBoolLiteralExpr *) {
    return true;
  }

  bool VisitCXXConstructExpr(const CXXConstructExpr *CE) {
    for (auto *Arg : CE->arguments())
      if (!CanLift(Arg))
        return false;
    return true;
  }

  bool VisitDeclStmt(const DeclStmt *DS) {
    for (auto *Decl : DS->decls()) {
      if (auto *VD = dyn_cast<VarDecl>(Decl)) {
        auto Search = Partitioner.FinalDeclMap.find(Decl);
        if (Search != Partitioner.FinalDeclMap.end()) {
          for (auto *S : Search->second.MutatorStmts)
            if (S != DS && !CanLift(S))
              return false;
        }
        if (auto *Init = VD->getInit()) {
          if (!CanLift(Init))
            return false;
        }
      }
    }
    return true;
  }

  bool VisitCallExpr(const CallExpr *C) {
    if (auto *DeclRef =
            dyn_cast<DeclRefExpr>(C->getCallee()->IgnoreParenImpCasts())) {
      if (auto *FD = dyn_cast<FunctionDecl>(DeclRef->getDecl())) {
        auto HBF = Partitioner.Builtins.identifyBuiltinFunction(FD);
        if (HBF != HBF_None && !HshBuiltins::isInterpolationDistributed(HBF))
          return true;
      }
    }
    return false;
  }

  bool CanLift(const Stmt *S) {
    if (auto *E = dyn_cast<Expr>(S))
      S = E->IgnoreParenImpCasts();
    return Visit(S);
  }

  void LiftToDependents(const Stmt *S) {
    dumper() << "Can lift " << S;
    if (!CanLift(S)) {
      dumper() << " No\n";
      return;
    }
    dumper() << " Yes\n";
    auto Search = Partitioner.StmtMap.find(S);
    if (Search != Partitioner.StmtMap.end()) {
      StageBits DepStageBits;
      for (const auto *Dep : Search->second.Dependents) {
        dumper() << "  Checking dep " << Dep;
        auto DepSearch = Partitioner.StmtMap.find(Dep);
        if (DepSearch != Partitioner.StmtMap.end()) {
          DepStageBits |= DepSearch->second.StageBits;
          dumper() << " " << DepSearch->second.StageBits;
        }
        dumper() << "\n";
      }
      if (Search->second.StageBits != DepStageBits) {
        dumper() << "  Lifted From " << Search->second.StageBits << " To "
                 << DepStageBits << "\n";
        Search->second.StageBits = DepStageBits;

        if (auto *DS = dyn_cast<DeclStmt>(S))
          Partitioner.UpdateDeclRefExprStages(DS, DepStageBits);
      }
    }
  }

  unsigned PassthroughDependents() {
    unsigned LiftCount = 0;
    for (auto &Stmt : Partitioner.StmtMap) {
      auto &StmtDeps = Stmt.second.Dependents;
      if (StmtDeps.empty())
        continue;
      DenseSet<const class Stmt *> NewStmtDeps;
      NewStmtDeps.reserve(StmtDeps.size());
      for (auto *Dep : StmtDeps) {
        if (isa<CastExpr>(Dep) || isa<DeclStmt>(Dep) || isa<DeclRefExpr>(Dep) ||
            isa<CXXConstructExpr>(Dep)) {
          auto Search = Partitioner.StmtMap.find(Dep);
          if (Search != Partitioner.StmtMap.end()) {
            auto &DREDeps = Search->second.Dependents;
            dumper() << "passing " << Dep->getStmtClassName()
                     << " dependent to " << Stmt.first->getStmtClassName()
                     << " " << Stmt.first << "\n";
            for (const auto *Deps : DREDeps)
              dumper() << "  " << Deps << "\n";
            NewStmtDeps.insert(DREDeps.begin(), DREDeps.end());
            ++LiftCount;
            continue;
          }
        }
        NewStmtDeps.insert(Dep);
      }
      StmtDeps = std::move(NewStmtDeps);
    }
    return LiftCount;
  }

  void run(AnalysisDeclContext &AD) {
    while (PassthroughDependents()) {
    }

    for (auto I = Partitioner.OrderedStmts.rbegin(),
              E = Partitioner.OrderedStmts.rend();
         I != E; ++I) {
      LiftToDependents(*I);
    }
  }
};

struct BlockDependencyPass : ConstStmtVisitor<BlockDependencyPass, unsigned> {
  class Partitioner &Partitioner;
  explicit BlockDependencyPass(class Partitioner &Partitioner)
      : Partitioner(Partitioner) {}

  unsigned VisitStmt(const Stmt *S) {
    if (auto *E = dyn_cast<Expr>(S))
      S = E->IgnoreParenImpCasts();
    auto Search = Partitioner.StmtMap.find(S);
    if (Search != Partitioner.StmtMap.end())
      return Search->second.StageBits;
    return 0;
  }

  unsigned VisitCompoundStmt(const CompoundStmt *CS) {
    StageBits NewStageBits;
    for (auto *Child : CS->body())
      NewStageBits |= Visit(Child);
    auto &DepInfo = Partitioner.StmtMap[CS];
    DepInfo.StageBits |= NewStageBits;
    return DepInfo.StageBits;
  }

  unsigned VisitIfStmt(const IfStmt *S) {
    StageBits NewStageBits;
    if (auto *Then = S->getThen())
      NewStageBits |= Visit(Then);
    if (auto *Else = S->getElse())
      NewStageBits |= Visit(Else);
    auto &DepInfo = Partitioner.StmtMap[S];
    DepInfo.StageBits |= NewStageBits;
    return DepInfo.StageBits;
  }

  template <typename T> unsigned VisitBody(const T *S) {
    StageBits NewStageBits;
    if (auto *Body = S->getBody())
      NewStageBits |= Visit(Body);
    auto &DepInfo = Partitioner.StmtMap[S];
    DepInfo.StageBits |= NewStageBits;
    return DepInfo.StageBits;
  }
  unsigned VisitForStmt(const ForStmt *S) { return VisitBody(S); }
  unsigned VisitWhileStmt(const WhileStmt *S) { return VisitBody(S); }
  unsigned VisitDoStmt(const DoStmt *S) { return VisitBody(S); }
  unsigned VisitSwitchStmt(const SwitchStmt *S) { return VisitBody(S); }

  template <typename T> unsigned VisitSubStmt(const T *S) {
    StageBits NewStageBits;
    if (auto *Sub = S->getSubStmt())
      NewStageBits |= Visit(Sub);
    auto &DepInfo = Partitioner.StmtMap[S];
    DepInfo.StageBits |= NewStageBits;
    return DepInfo.StageBits;
  }
  unsigned VisitCaseStmt(const CaseStmt *S) { return VisitSubStmt(S); }
  unsigned VisitDefaultStmt(const DefaultStmt *S) { return VisitSubStmt(S); }

  void run(AnalysisDeclContext &AD) { Visit(AD.getBody()); }
};

struct DeclUsagePass : StmtVisitor<DeclUsagePass, void, HshStage> {
  class Partitioner &Partitioner;
  class Builder &Builder;
  explicit DeclUsagePass(class Partitioner &Partitioner, class Builder &Builder)
      : Partitioner(Partitioner), Builder(Builder) {}

  void InterStageReferenceExpr(Expr *E, HshStage ToStage) {
    if (E->isIntegerConstantExpr(Partitioner.Context))
      return;
    if (auto *DRE = dyn_cast<DeclRefExpr>(E))
      if (isa<EnumConstantDecl>(DRE->getDecl()))
        return;
    auto Search = Partitioner.StmtMap.find(E);
    if (Search != Partitioner.StmtMap.end()) {
      auto &DepInfo = Search->second;
      if (DepInfo.StageBits) {
        Visit(E, DepInfo.getMaxStage());
        return;
      }
    }
    Visit(E, ToStage);
  }

  void DoVisit(Stmt *S, HshStage Stage, bool ScopeBody = false) {
    dumper() << "Used Visiting for " << Stage << " " << S << "("
             << S->getStmtClassName() << ")\n";
    if (auto *E = dyn_cast<Expr>(S))
      S = E->IgnoreParenImpCasts();
    if (isa<DeclStmt>(S) || isa<CaseStmt>(S) || isa<DefaultStmt>(S)) {
      /* DeclStmts and switch components passthrough unconditionally */
      Visit(S, Stage);
      return;
    } else if (isa<IntegerLiteral>(S) || isa<FloatingLiteral>(S) ||
               isa<CXXBoolLiteralExpr>(S) || isa<BreakStmt>(S) ||
               isa<ContinueStmt>(S) || isa<CXXThisExpr>(S) ||
               isa<CXXDefaultArgExpr>(S)) {
      /* Literals, flow control leaves, and this can go right where they
       * are used
       */
      return;
    } else if (ScopeBody) {
      /* "root" statement if immediate child of a scope body */
    } else if (auto *E = dyn_cast<Expr>(S)) {
      /* Trace expression tree and establish inter-stage references */
      InterStageReferenceExpr(E, Stage);
      return;
    }
    /* "Root" statements of bodies are conditionally emitted based on stage
     */
    auto Search = Partitioner.StmtMap.find(S);
    if (Search != Partitioner.StmtMap.end() && Search->second.hasStage(Stage))
      Visit(S, Stage);
    /* Prune this statement */
  }

  template <typename T> void DoVisitExprRange(T Range, HshStage Stage) {
    for (Expr *E : Range)
      DoVisit(E, Stage);
  }

  /* Begin ignores */
  void VisitValueStmt(ValueStmt *ValueStmt, HshStage Stage) {
    DoVisit(ValueStmt->getExprStmt(), Stage);
  }

  void VisitConstantExpr(ConstantExpr *CE, HshStage Stage) {
    DoVisit(CE->getSubExpr(), Stage);
  }

  void VisitMaterializeTemporaryExpr(MaterializeTemporaryExpr *MTE,
                                     HshStage Stage) {
    DoVisit(MTE->getSubExpr(), Stage);
  }

  void VisitSubstNonTypeTemplateParmExpr(SubstNonTypeTemplateParmExpr *NTTP,
                                         HshStage Stage) {
    DoVisit(NTTP->getReplacement(), Stage);
  }
  /* End ignores */

  static void VisitStmt(Stmt *S, HshStage) {}

  void VisitDeclStmt(DeclStmt *DeclStmt, HshStage Stage) {
    for (Decl *D : DeclStmt->decls())
      if (auto *VD = dyn_cast<VarDecl>(D))
        if (Expr *Init = VD->getInit())
          DoVisit(Init, Stage);
  }

  void VisitUnaryOperator(UnaryOperator *UnOp, HshStage Stage) {
    DoVisit(UnOp->getSubExpr(), Stage);
  }

  void VisitCallExpr(CallExpr *CallExpr, HshStage Stage) {
    if (auto *DeclRef = dyn_cast<DeclRefExpr>(
            CallExpr->getCallee()->IgnoreParenImpCasts())) {
      if (auto *FD = dyn_cast<FunctionDecl>(DeclRef->getDecl())) {
        HshBuiltinFunction Func =
            Partitioner.Builtins.identifyBuiltinFunction(FD);
        bool CompatibleFD = FD->isConstexpr() &&
                            Partitioner.Builtins.checkHshFunctionCompatibility(
                                Partitioner.Context, FD);
        if (CompatibleFD || Func != HBF_None)
          DoVisitExprRange(CallExpr->arguments(), Stage);
      }
    }
  }

  void VisitCXXMemberCallExpr(CXXMemberCallExpr *CallExpr, HshStage Stage) {
    CXXMethodDecl *MD = CallExpr->getMethodDecl();
    Expr *ObjArg = CallExpr->getImplicitObjectArgument()->IgnoreParenImpCasts();
    HshBuiltinCXXMethod Method = Partitioner.Builtins.identifyBuiltinMethod(MD);
    if (HshBuiltins::isSwizzleMethod(Method)) {
      DoVisit(ObjArg, Stage);
    }
    switch (Method) {
    case HBM_sample2d:
    case HBM_sample_bias2d:
    case HBM_read2d:
    case HBM_sample2da:
    case HBM_sample_bias2da:
    case HBM_read2da:
    case HBM_render_sample2d:
    case HBM_render_read2d:
      DoVisit(CallExpr->getArg(0), Stage);
      if (Method == HBM_sample_bias2d || Method == HBM_sample_bias2da ||
          Method == HBM_read2d || Method == HBM_read2da ||
          Method == HBM_render_read2d)
        DoVisit(CallExpr->getArg(1), Stage);
      if (Method == HBM_read2da)
        DoVisit(CallExpr->getArg(2), Stage);
      break;
    default:
      break;
    }
  }

  void VisitCastExpr(CastExpr *CastExpr, HshStage Stage) {
    if (Partitioner.Builtins.identifyBuiltinType(CastExpr->getType()) ==
        HBT_None)
      return;
    DoVisit(CastExpr->getSubExpr(), Stage);
  }

  void VisitCXXConstructExpr(CXXConstructExpr *ConstructExpr, HshStage Stage) {
    if (Partitioner.Builtins.identifyBuiltinType(ConstructExpr->getType()) ==
        HBT_None)
      return;
    DoVisitExprRange(ConstructExpr->arguments(), Stage);
  }

  void VisitCXXOperatorCallExpr(CXXOperatorCallExpr *CallExpr, HshStage Stage) {
    DoVisitExprRange(CallExpr->arguments(), Stage);
  }

  void VisitBinaryOperator(BinaryOperator *BinOp, HshStage Stage) {
    DoVisit(BinOp->getLHS(), Stage);
    DoVisit(BinOp->getRHS(), Stage);
  }

  bool InMemberExpr = false;

  void VisitDeclRefExpr(DeclRefExpr *DeclRef, HshStage Stage) {
    if (auto *PVD = dyn_cast<ParmVarDecl>(DeclRef->getDecl())) {
    } else if (auto *VD = dyn_cast<VarDecl>(DeclRef->getDecl())) {
      dumper() << VD << " Used in " << Stage << "\n";
      Partitioner.UsedDecls[Stage].insert(VD);
    }
  }

  void VisitInitListExpr(InitListExpr *InitList, HshStage Stage) {
    DoVisitExprRange(InitList->inits(), Stage);
  }

  void VisitMemberExpr(MemberExpr *MemberExpr, HshStage Stage) {
    if (!InMemberExpr && !Partitioner.Builtins.checkHshFieldTypeCompatibility(
                             Partitioner.Context, MemberExpr->getMemberDecl()))
      return;
    SaveAndRestore<bool> SavedInMemberExpr(InMemberExpr, true);
    DoVisit(MemberExpr->getBase(), Stage);
  }

  void VisitDoStmt(DoStmt *S, HshStage Stage) {
    if (auto *Cond = S->getCond())
      DoVisit(Cond, Stage);
    if (auto *Body = S->getBody())
      DoVisit(Body, Stage, true);
  }

  void VisitForStmt(ForStmt *S, HshStage Stage) {
    if (auto *Init = S->getInit())
      DoVisit(Init, Stage);
    if (auto *Cond = S->getCond())
      DoVisit(Cond, Stage);
    if (auto *Inc = S->getInc())
      DoVisit(Inc, Stage);
    if (auto *Body = S->getBody())
      DoVisit(Body, Stage, true);
  }

  void VisitIfStmt(IfStmt *S, HshStage Stage) {
    if (auto *Init = S->getInit())
      DoVisit(Init, Stage);
    if (auto *Cond = S->getCond())
      DoVisit(Cond, Stage);
    if (auto *Then = S->getThen())
      DoVisit(Then, Stage, true);
    if (auto *Else = S->getElse())
      DoVisit(Else, Stage, true);
  }

  void VisitConditionalOperator(ConditionalOperator *S, HshStage Stage) {
    if (auto *Cond = S->getCond())
      DoVisit(Cond, Stage);
    if (auto *True = S->getTrueExpr())
      DoVisit(True, Stage);
    if (auto *False = S->getFalseExpr())
      DoVisit(False, Stage);
  }

  void VisitCaseStmt(CaseStmt *S, HshStage Stage) {
    if (Stmt *St = S->getSubStmt())
      DoVisit(St, Stage, true);
  }

  void VisitDefaultStmt(DefaultStmt *S, HshStage Stage) {
    if (Stmt *St = S->getSubStmt())
      DoVisit(St, Stage, true);
  }

  void VisitSwitchStmt(SwitchStmt *S, HshStage Stage) {
    if (auto *Cond = S->getCond())
      DoVisit(Cond, Stage);
    if (auto *Body = S->getBody())
      DoVisit(Body, Stage);
  }

  void VisitWhileStmt(WhileStmt *S, HshStage Stage) {
    if (auto *Cond = S->getCond())
      DoVisit(Cond, Stage);
    if (auto *Body = S->getBody())
      DoVisit(Body, Stage, true);
  }

  void VisitCompoundStmt(CompoundStmt *S, HshStage Stage) {
    for (auto *CS : S->body())
      DoVisit(CS, Stage, true);
  }

  void run(AnalysisDeclContext &AD) {
    for (int i = HshVertexStage; i < HshMaxStage; ++i) {
      auto Stage = HshStage(i);
      if (!Builder.isStageUsed(Stage))
        continue;
      if (auto *Body = dyn_cast<CompoundStmt>(AD.getBody())) {
        for (auto *Stmt : Body->body())
          DoVisit(Stmt, Stage, true);
      } else {
        DoVisit(AD.getBody(), Stage, true);
      }
    }
  }
};

struct BuildPass : StmtVisitor<BuildPass, Stmt *, HshStage> {
  class Partitioner &Partitioner;
  class Builder &Builder;
  explicit BuildPass(class Partitioner &Partitioner, class Builder &Builder)
      : Partitioner(Partitioner), Builder(Builder) {}

  bool hasErrorOccurred() const {
    return Partitioner.Context.getDiagnostics().hasErrorOccurred();
  }

  Expr *InterStageReferenceExpr(Expr *E, HshStage ToStage) {
    if (Optional<llvm::APSInt> ConstVal =
            E->getIntegerConstantExpr(Partitioner.Context)) {
      if (E->isKnownToHaveBooleanValue()) {
        return new (Partitioner.Context)
            CXXBoolLiteralExpr(!!ConstVal, Partitioner.Context.BoolTy, {});
      } else {
        return IntegerLiteral::Create(Partitioner.Context,
                                      ConstVal->extOrTrunc(32),
                                      Partitioner.Context.IntTy, {});
      }
    }
    if (auto *DRE = dyn_cast<DeclRefExpr>(E)) {
      if (isa<EnumConstantDecl>(DRE->getDecl()))
        return E;
    }
    auto Search = Partitioner.StmtMap.find(E);
    if (Search != Partitioner.StmtMap.end()) {
      auto &DepInfo = Search->second;
      if (!DepInfo.StageBits)
        return cast_or_null<Expr>(Visit(E, ToStage));
      E = cast_or_null<Expr>(Visit(E, DepInfo.getMaxStage()));
      if (!E)
        return nullptr;
      return Builder.createInterStageReferenceExpr(
          E, DepInfo.getLeqStage(ToStage), ToStage);
    }
    return cast_or_null<Expr>(Visit(E, ToStage));
  }

  Stmt *DoVisit(Stmt *S, HshStage Stage, bool ScopeBody = false) {
    dumper() << "Visiting for " << Stage << " " << S << "\n";
    /* For building purposes, ParenExpr passes through unconditionally */
    if (auto *P = dyn_cast<ParenExpr>(S))
      return Visit(P, Stage);
    if (auto *E = dyn_cast<Expr>(S))
      S = E->IgnoreParenImpCasts();
    if (isa<DeclStmt>(S) || isa<CaseStmt>(S) || isa<DefaultStmt>(S)) {
      /* DeclStmts and switch components passthrough unconditionally */
      return Visit(S, Stage);
    } else if (isa<IntegerLiteral>(S) || isa<FloatingLiteral>(S) ||
               isa<CXXBoolLiteralExpr>(S) || isa<BreakStmt>(S) ||
               isa<ContinueStmt>(S) || isa<CXXThisExpr>(S) ||
               isa<CXXDefaultArgExpr>(S)) {
      /* Literals, flow control leaves, and this can go right where they are
       * used
       */
      return S;
    } else if (ScopeBody) {
      /* "root" statement if immediate child of a scope body */
    } else if (auto *E = dyn_cast<Expr>(S)) {
      /* Trace expression tree and establish inter-stage references */
      return InterStageReferenceExpr(E, Stage);
    }
    /* "Root" statements of bodies are conditionally emitted based on stage
     */
    auto Search = Partitioner.StmtMap.find(S);
    if (Search != Partitioner.StmtMap.end() && Search->second.hasStage(Stage))
      return Visit(S, Stage);
    /* Prune this statement */
    return nullptr;
  }

  using ExprRangeRet = SmallVector<Expr *, 4>;
  template <typename T>
  Optional<ExprRangeRet> DoVisitExprRange(T Range, HshStage Stage) {
    ExprRangeRet Res;
    for (Expr *E : Range) {
      Stmt *ExprStmt = DoVisit(E, Stage);
      if (!ExprStmt)
        return {};
      Res.push_back(cast<Expr>(ExprStmt));
    }
    return {Res};
  }

  /* Begin ignores */
  Stmt *VisitValueStmt(ValueStmt *ValueStmt, HshStage Stage) {
    return DoVisit(ValueStmt->getExprStmt(), Stage);
  }

  Stmt *VisitConstantExpr(ConstantExpr *CE, HshStage Stage) {
    return DoVisit(CE->getSubExpr(), Stage);
  }

  Stmt *VisitMaterializeTemporaryExpr(MaterializeTemporaryExpr *MTE,
                                      HshStage Stage) {
    return DoVisit(MTE->getSubExpr(), Stage);
  }

  Stmt *VisitSubstNonTypeTemplateParmExpr(SubstNonTypeTemplateParmExpr *NTTP,
                                          HshStage Stage) {
    return DoVisit(NTTP->getReplacement(), Stage);
  }
  /* End ignores */

  Stmt *VisitStmt(Stmt *S, HshStage) {
    Reporter(Partitioner.Context).UnsupportedStmt(S);
    return nullptr;
  }

  Stmt *VisitExpr(Expr *E, HshStage) {
    Reporter(Partitioner.Context).UnsupportedStmt(E);
    return nullptr;
  }

  Stmt *VisitParenExpr(ParenExpr *P, HshStage Stage) {
    Stmt *ExprStmt = DoVisit(P->getSubExpr(), Stage);
    if (!ExprStmt)
      return {};
    return new (Partitioner.Context) ParenExpr({}, {}, cast<Expr>(ExprStmt));
  }

  Stmt *VisitUnaryOperator(UnaryOperator *UnOp, HshStage Stage) {
    Stmt *ExprStmt = DoVisit(UnOp->getSubExpr(), Stage);
    if (!ExprStmt)
      return {};
    return UnaryOperator::Create(Partitioner.Context, cast<Expr>(ExprStmt),
                                 UnOp->getOpcode(), UnOp->getType(), VK_XValue,
                                 OK_Ordinary, {}, false, {});
  }

  Stmt *VisitDeclStmt(DeclStmt *DS, HshStage Stage) {
    SmallVector<Decl *, 4> NewDecls;
    for (auto *Decl : DS->decls()) {
      if (auto *VD = dyn_cast<VarDecl>(Decl)) {
        if (Partitioner.UsedDecls[Stage].find(VD) !=
            Partitioner.UsedDecls[Stage].end()) {
          auto *NewVD =
              VarDecl::Create(Partitioner.Context, VD->getDeclContext(), {}, {},
                              VD->getIdentifier(), VD->getType(),
                              VD->getTypeSourceInfo(), VD->getStorageClass());
          if (Expr *Init = VD->getInit()) {
            auto *InitStmt = DoVisit(Init, Stage);
            if (!InitStmt)
              return nullptr;
            NewVD->setInit(cast<Expr>(InitStmt));
          }
          NewDecls.push_back(NewVD);
        }
      } else {
        Reporter(Partitioner.Context).UnsupportedTypeReference(DS);
        return nullptr;
      }
    }
    if (!NewDecls.empty()) {
      return new (Partitioner.Context)
          DeclStmt(DeclGroupRef::Create(Partitioner.Context, NewDecls.data(),
                                        NewDecls.size()),
                   {}, {});
    }
    return nullptr;
  }

  static Stmt *VisitNullStmt(NullStmt *NS, HshStage) { return NS; }

  Stmt *VisitCallExpr(CallExpr *CallExpr, HshStage Stage) {
    if (auto *DeclRef = dyn_cast<DeclRefExpr>(
            CallExpr->getCallee()->IgnoreParenImpCasts())) {
      if (auto *FD = dyn_cast<FunctionDecl>(DeclRef->getDecl())) {
        HshBuiltinFunction Func =
            Partitioner.Builtins.identifyBuiltinFunction(FD);
        bool CompatibleFD = FD->isConstexpr() &&
                            Partitioner.Builtins.checkHshFunctionCompatibility(
                                Partitioner.Context, FD);
        if (CompatibleFD || Func != HBF_None) {
          auto Arguments = DoVisitExprRange(CallExpr->arguments(), Stage);
          if (!Arguments)
            return nullptr;
          if (Func == HBF_None) {
            Partitioner.Builtins.identifyBuiltinFunction(FD);
            Builder.registerFunctionDecl(FD, Stage);
          }
          return CallExpr::Create(Partitioner.Context, CallExpr->getCallee(),
                                  *Arguments, CallExpr->getType(), VK_XValue,
                                  {}, {});
        }
      }
    }
    Reporter(Partitioner.Context).UnsupportedFunctionCall(CallExpr);
    return nullptr;
  }

  Stmt *VisitCXXMemberCallExpr(CXXMemberCallExpr *CallExpr, HshStage Stage) {
    CXXMethodDecl *MD = CallExpr->getMethodDecl();
    Expr *ObjArg = CallExpr->getImplicitObjectArgument()->IgnoreParenImpCasts();
    HshBuiltinCXXMethod Method = Partitioner.Builtins.identifyBuiltinMethod(MD);
    if (HshBuiltins::isSwizzleMethod(Method)) {
      auto *BaseStmt = DoVisit(ObjArg, Stage);
      auto *ParenBase =
          new (Partitioner.Context) ParenExpr({}, {}, cast<Expr>(BaseStmt));
      return MemberExpr::CreateImplicit(Partitioner.Context, ParenBase, false,
                                        MD, MD->getReturnType(), VK_XValue,
                                        OK_Ordinary);
    }
    switch (Method) {
    case HBM_sample2d:
    case HBM_render_sample2d:
    case HBM_sample_bias2d:
    case HBM_sample2da:
    case HBM_sample_bias2da: {
      ParmVarDecl *PVD = nullptr;
      if (auto *TexRef = dyn_cast<DeclRefExpr>(ObjArg))
        PVD = dyn_cast<ParmVarDecl>(TexRef->getDecl());
      if (PVD)
        Builder.registerParmVarRef(PVD, Stage);
      else
        Reporter(Partitioner.Context).BadTextureReference(CallExpr);
      auto *UVStmt = DoVisit(CallExpr->getArg(0), Stage);
      if (!UVStmt)
        return nullptr;
      if (Method == HBM_sample_bias2d || Method == HBM_sample_bias2da) {
        auto *BiasStmt = DoVisit(CallExpr->getArg(1), Stage);
        if (!BiasStmt)
          return nullptr;
        std::array<Expr *, 3> NewArgs{cast<Expr>(UVStmt), cast<Expr>(BiasStmt),
                                      CallExpr->getArg(2)};
        auto *NMCE = CXXMemberCallExpr::Create(
            Partitioner.Context, CallExpr->getCallee(), NewArgs,
            CallExpr->getType(), VK_XValue, {}, {});
        Builder.registerSampleCall(Method, NMCE, Stage);
        return NMCE;
      } else {
        std::array<Expr *, 2> NewArgs{cast<Expr>(UVStmt), CallExpr->getArg(1)};
        auto *NMCE = CXXMemberCallExpr::Create(
            Partitioner.Context, CallExpr->getCallee(), NewArgs,
            CallExpr->getType(), VK_XValue, {}, {});
        Builder.registerSampleCall(Method, NMCE, Stage);
        return NMCE;
      }
    }
    case HBM_read2d:
    case HBM_render_read2d: {
      ParmVarDecl *PVD = nullptr;
      if (auto *TexRef = dyn_cast<DeclRefExpr>(ObjArg))
        PVD = dyn_cast<ParmVarDecl>(TexRef->getDecl());
      if (PVD)
        Builder.registerParmVarRef(PVD, Stage);
      else
        Reporter(Partitioner.Context).BadTextureReference(CallExpr);
      auto *CoordStmt = DoVisit(CallExpr->getArg(0), Stage);
      if (!CoordStmt)
        return nullptr;
      auto *LODStmt = DoVisit(CallExpr->getArg(1), Stage);
      if (!LODStmt)
        return nullptr;
      std::array<Expr *, 2> NewArgs{cast<Expr>(CoordStmt), cast<Expr>(LODStmt)};
      return CXXMemberCallExpr::Create(
          Partitioner.Context, CallExpr->getCallee(), NewArgs,
          CallExpr->getType(), VK_XValue, {}, {});
    }
    case HBM_read2da: {
      ParmVarDecl *PVD = nullptr;
      if (auto *TexRef = dyn_cast<DeclRefExpr>(ObjArg))
        PVD = dyn_cast<ParmVarDecl>(TexRef->getDecl());
      if (PVD)
        Builder.registerParmVarRef(PVD, Stage);
      else
        Reporter(Partitioner.Context).BadTextureReference(CallExpr);
      auto *CoordStmt = DoVisit(CallExpr->getArg(0), Stage);
      if (!CoordStmt)
        return nullptr;
      auto *ArrayStmt = DoVisit(CallExpr->getArg(1), Stage);
      if (!ArrayStmt)
        return nullptr;
      auto *LODStmt = DoVisit(CallExpr->getArg(2), Stage);
      if (!LODStmt)
        return nullptr;
      std::array<Expr *, 3> NewArgs{cast<Expr>(CoordStmt),
                                    cast<Expr>(ArrayStmt), cast<Expr>(LODStmt)};
      return CXXMemberCallExpr::Create(
          Partitioner.Context, CallExpr->getCallee(), NewArgs,
          CallExpr->getType(), VK_XValue, {}, {});
    }
    default:
      Reporter(Partitioner.Context).UnsupportedFunctionCall(CallExpr);
      break;
    }
    return nullptr;
  }

  Stmt *VisitCastExpr(CastExpr *CastExpr, HshStage Stage) {
    if (Partitioner.Builtins.identifyBuiltinType(CastExpr->getType()) ==
        HBT_None) {
      Reporter(Partitioner.Context).UnsupportedTypeCast(CastExpr);
      return nullptr;
    }
    return DoVisit(CastExpr->getSubExpr(), Stage);
  }

  Stmt *VisitCXXConstructExpr(CXXConstructExpr *ConstructExpr, HshStage Stage) {
    if (Partitioner.Builtins.identifyBuiltinType(ConstructExpr->getType()) ==
        HBT_None) {
      Reporter(Partitioner.Context).UnsupportedTypeConstruct(ConstructExpr);
      return nullptr;
    }

    auto Arguments = DoVisitExprRange(ConstructExpr->arguments(), Stage);
    if (!Arguments)
      return nullptr;
    return CXXTemporaryObjectExpr::Create(
        Partitioner.Context, ConstructExpr->getConstructor(),
        ConstructExpr->getType(),
        Partitioner.Context.getTrivialTypeSourceInfo(ConstructExpr->getType()),
        *Arguments, {}, ConstructExpr->hadMultipleCandidates(),
        ConstructExpr->isListInitialization(),
        ConstructExpr->isStdInitListInitialization(),
        ConstructExpr->requiresZeroInitialization());
  }

  Stmt *VisitCXXOperatorCallExpr(CXXOperatorCallExpr *CallExpr,
                                 HshStage Stage) {
    auto Arguments = DoVisitExprRange(CallExpr->arguments(), Stage);
    if (!Arguments)
      return nullptr;
    if (CallExpr->isAssignmentOp()) {
      Expr *LHS = (*Arguments)[0];
      if (LHS->getType().isConstQualified())
        Reporter(Partitioner.Context).ConstAssignment(CallExpr);
    }
    return CXXOperatorCallExpr::Create(
        Partitioner.Context, CallExpr->getOperator(), CallExpr->getCallee(),
        *Arguments, CallExpr->getType(), VK_XValue, {}, {});
  }

  Stmt *VisitBinaryOperator(BinaryOperator *BinOp, HshStage Stage) {
    auto *LStmt = DoVisit(BinOp->getLHS(), Stage);
    if (!LStmt)
      return nullptr;
    auto *RStmt = DoVisit(BinOp->getRHS(), Stage);
    if (!RStmt)
      return nullptr;
    if (BinOp->isAssignmentOp()) {
      if (cast<Expr>(LStmt)->getType().isConstQualified())
        Reporter(Partitioner.Context).ConstAssignment(BinOp);
    }
    return BinaryOperator::Create(
        Partitioner.Context, cast<Expr>(LStmt), cast<Expr>(RStmt),
        BinOp->getOpcode(), BinOp->getType(), VK_XValue, OK_Ordinary, {}, {});
  }

  bool InMemberExpr = false;

  Stmt *VisitDeclRefExpr(DeclRefExpr *DeclRef, HshStage Stage) {
    if (auto *PVD = dyn_cast<ParmVarDecl>(DeclRef->getDecl())) {
      Builder.registerParmVarRef(PVD, Stage);
      return DeclRef;
    } else if (auto *VD = dyn_cast<VarDecl>(DeclRef->getDecl())) {
      if (!InMemberExpr && !Partitioner.Builtins.checkHshFieldTypeCompatibility(
                               Partitioner.Context, VD)) {
        Reporter(Partitioner.Context).UnsupportedTypeReference(DeclRef);
        return nullptr;
      }
      auto Search = Partitioner.StmtMap.find(DeclRef);
      if (Search != Partitioner.StmtMap.end()) {
        auto &DepInfo = Search->second;
        if (DepInfo.StageBits)
          return Builder.createInterStageReferenceExpr(
              DeclRef, DepInfo.getLeqStage(Stage), Stage);
      }
      return DeclRef;
    } else if (isa<EnumConstantDecl>(DeclRef->getDecl())) {
      return DeclRef;
    } else {
      Reporter(Partitioner.Context).UnsupportedTypeReference(DeclRef);
      return nullptr;
    }
  }

  Stmt *VisitInitListExpr(InitListExpr *InitList, HshStage Stage) {
    auto Exprs = DoVisitExprRange(InitList->inits(), Stage);
    if (!Exprs)
      return nullptr;
    return new (Partitioner.Context)
        InitListExpr(Partitioner.Context, {}, *Exprs, {});
  }

  Stmt *VisitMemberExpr(MemberExpr *MemberExpr, HshStage Stage) {
    if (!InMemberExpr &&
        !Partitioner.Builtins.checkHshFieldTypeCompatibility(
            Partitioner.Context, MemberExpr->getMemberDecl())) {
      Reporter(Partitioner.Context).UnsupportedTypeReference(MemberExpr);
      return nullptr;
    }
    SaveAndRestore<bool> SavedInMemberExpr(InMemberExpr, true);
    auto *BaseStmt = DoVisit(MemberExpr->getBase(), Stage);
    return MemberExpr::CreateImplicit(Partitioner.Context, cast<Expr>(BaseStmt),
                                      false, MemberExpr->getMemberDecl(),
                                      MemberExpr->getType(), VK_XValue,
                                      OK_Ordinary);
  }

  Stmt *StmtOrNull(Stmt *S) {
    if (S)
      return S;
    return new (Partitioner.Context) NullStmt({}, false);
  };

  Stmt *VisitDoStmt(DoStmt *S, HshStage Stage) {
    dumper() << "Do ";
    Expr *NewCond = nullptr;
    if (auto *Cond = S->getCond()) {
      NewCond = cast_or_null<Expr>(DoVisit(Cond, Stage));
      dumper() << Cond;
    }
    dumper() << ":\n";
    Stmt *NewBody = nullptr;
    if (auto *Body = S->getBody()) {
      NewBody = StmtOrNull(DoVisit(Body, Stage, true));
      dumper() << Body;
    }
    if (hasErrorOccurred())
      return nullptr;
    return new (Partitioner.Context) DoStmt(NewBody, NewCond, {}, {}, {});
  }

  Stmt *VisitForStmt(ForStmt *S, HshStage Stage) {
    dumper() << "For ";
    Stmt *NewInit = nullptr;
    if (auto *Init = S->getInit()) {
      NewInit = DoVisit(Init, Stage);
      dumper() << Init;
    }
    dumper() << "; ";
    Expr *NewCond = nullptr;
    if (auto *Cond = S->getCond()) {
      NewCond = cast_or_null<Expr>(DoVisit(Cond, Stage));
      dumper() << Cond;
    }
    dumper() << "; ";
    Expr *NewInc = nullptr;
    if (auto *Inc = S->getInc()) {
      NewInc = cast_or_null<Expr>(DoVisit(Inc, Stage));
      dumper() << Inc;
    }
    dumper() << ":\n";
    Stmt *NewBody = nullptr;
    if (auto *Body = S->getBody()) {
      NewBody = StmtOrNull(DoVisit(Body, Stage, true));
      dumper() << Body;
    }
    if (hasErrorOccurred())
      return nullptr;
    return new (Partitioner.Context)
        ForStmt(Partitioner.Context, NewInit, NewCond,
                S->getConditionVariable(), NewInc, NewBody, {}, {}, {});
  }

  Stmt *VisitIfStmt(IfStmt *S, HshStage Stage) {
    dumper() << "If ";
    Stmt *NewInit = nullptr;
    if (auto *Init = S->getInit()) {
      NewInit = DoVisit(Init, Stage);
    }
    Expr *NewCond = nullptr;
    if (auto *Cond = S->getCond()) {
      NewCond = cast_or_null<Expr>(DoVisit(Cond, Stage));
      dumper() << Cond;
    }
    dumper() << ":\n";
    Stmt *NewThen = nullptr;
    if (auto *Then = S->getThen()) {
      NewThen = StmtOrNull(DoVisit(Then, Stage, true));
      dumper() << Then;
    }
    Stmt *NewElse = nullptr;
    if (auto *Else = S->getElse()) {
      NewElse = DoVisit(Else, Stage, true);
      dumper() << "Else:\n" << Else;
    }
    if (hasErrorOccurred())
      return nullptr;
    return IfStmt::Create(Partitioner.Context, {}, S->isConstexpr(), NewInit,
                          S->getConditionVariable(), NewCond, {}, {}, NewThen,
                          {}, NewElse);
  }

  Stmt *VisitConditionalOperator(ConditionalOperator *S, HshStage Stage) {
    if (auto *Cond = S->getCond()) {
      if (Optional<llvm::APSInt> ConstVal =
              Cond->getIntegerConstantExpr(Partitioner.Context)) {
        dumper() << "Constant Cond ";
        dumper() << S->getCond();
        if (!!ConstVal) {
          if (auto *True = S->getTrueExpr()) {
            dumper() << "True:\n" << True;
            return cast_or_null<Expr>(DoVisit(True, Stage));
          }
        } else {
          if (auto *False = S->getFalseExpr()) {
            dumper() << "Else:\n" << False;
            return cast_or_null<Expr>(DoVisit(False, Stage));
          }
        }
        return nullptr;
      }
    }

    dumper() << "Cond ";
    Expr *NewCond = nullptr;
    if (auto *Cond = S->getCond()) {
      NewCond = cast_or_null<Expr>(DoVisit(Cond, Stage));
      dumper() << Cond;
    }
    dumper() << ":\n";
    Expr *NewTrue = nullptr;
    if (auto *True = S->getTrueExpr()) {
      NewTrue = cast_or_null<Expr>(DoVisit(True, Stage));
      dumper() << "True:\n" << True;
    }
    Expr *NewFalse = nullptr;
    if (auto *False = S->getFalseExpr()) {
      NewFalse = cast_or_null<Expr>(DoVisit(False, Stage));
      dumper() << "Else:\n" << False;
    }
    if (hasErrorOccurred())
      return nullptr;
    return new (Partitioner.Context)
        ConditionalOperator(NewCond, {}, NewTrue, {}, NewFalse, S->getType(),
                            VK_XValue, OK_Ordinary);
  }

  Stmt *VisitCaseStmt(CaseStmt *S, HshStage Stage) {
    dumper() << "case " << S->getLHS() << ":\n";
    Stmt *NewSubStmt = nullptr;
    if (Stmt *St = S->getSubStmt())
      NewSubStmt = DoVisit(St, Stage, true);
    if (hasErrorOccurred())
      return nullptr;
    auto *NewCase =
        CaseStmt::Create(Partitioner.Context, S->getLHS(), nullptr, {}, {}, {});
    NewCase->setSubStmt(NewSubStmt);
    dumper() << "\n";
    return NewCase;
  }

  Stmt *VisitDefaultStmt(DefaultStmt *S, HshStage Stage) {
    dumper() << "default:\n";
    Stmt *NewSubStmt = nullptr;
    if (Stmt *St = S->getSubStmt())
      NewSubStmt = DoVisit(St, Stage, true);
    if (hasErrorOccurred())
      return nullptr;
    auto *NewDefault =
        new (Partitioner.Context) DefaultStmt({}, {}, NewSubStmt);
    dumper() << "\n";
    return NewDefault;
  }

  static Stmt *VisitBreakStmt(BreakStmt *S, HshStage Stage) {
    dumper() << S;
    return S;
  }

  static Stmt *VisitContinueStmt(ContinueStmt *S, HshStage Stage) {
    dumper() << S;
    return S;
  }

  Stmt *VisitSwitchStmt(SwitchStmt *S, HshStage Stage) {
    dumper() << "Switch ";
    if (Stmt *Init = S->getInit()) {
      auto &Diags = Partitioner.Context.getDiagnostics();
      Diags.Report(
          Init->getBeginLoc(),
          Diags.getCustomDiagID(DiagnosticsEngine::Error,
                                "C++17 switch init statements not supported"))
          << Init->getSourceRange();
    }
    Expr *NewCond = nullptr;
    if (auto *Cond = S->getCond()) {
      NewCond = cast_or_null<Expr>(DoVisit(Cond, Stage));
      dumper() << Cond;
    }
    dumper() << ":\n";
    Stmt *NewBody = nullptr;
    if (auto *Body = S->getBody())
      NewBody = DoVisit(Body, Stage);
    if (hasErrorOccurred())
      return nullptr;
    auto *NewSwitch =
        SwitchStmt::Create(Partitioner.Context, nullptr,
                           S->getConditionVariable(), NewCond, {}, {});
    NewSwitch->setBody(NewBody);
    return NewSwitch;
  }

  Stmt *VisitWhileStmt(WhileStmt *S, HshStage Stage) {
    dumper() << "While ";
    Expr *NewCond = nullptr;
    if (auto *Cond = S->getCond()) {
      NewCond = cast_or_null<Expr>(DoVisit(Cond, Stage));
      dumper() << Cond;
    }
    dumper() << ":\n";
    Stmt *NewBody = nullptr;
    if (auto *Body = S->getBody())
      NewBody = DoVisit(Body, Stage, true);
    if (hasErrorOccurred())
      return nullptr;
    return WhileStmt::Create(Partitioner.Context, S->getConditionVariable(),
                             NewCond, NewBody, {}, {}, {});
  }

  Stmt *VisitCompoundStmt(CompoundStmt *S, HshStage Stage) {
    dumper() << "{\n";
    SmallVector<Stmt *, 16> Stmts;
    Stmts.reserve(S->size());
    for (auto *CS : S->body())
      if (auto *NewStmt = DoVisit(CS, Stage, true))
        Stmts.push_back(NewStmt);
    dumper() << "}\n";
    if (hasErrorOccurred())
      return nullptr;
    return CompoundStmt::Create(Partitioner.Context, Stmts, {}, {});
  }

  void run(AnalysisDeclContext &AD) {
    for (int i = HshVertexStage; i < HshMaxStage; ++i) {
      auto Stage = HshStage(i);
      if (!Builder.isStageUsed(Stage))
        continue;
      dumper() << "Statements for " << Stage << ":\n";
      if (auto *Body = dyn_cast<CompoundStmt>(AD.getBody())) {
        dumper() << "{\n";
        for (auto *Stmt : Body->body()) {
          if (auto *NewStmt = DoVisit(Stmt, Stage, true)) {
            dumper() << NewStmt << "\n";
            Builder.addStageStmt(NewStmt, Stage);
          }
          if (hasErrorOccurred())
            return;
        }
        dumper() << "}";
      } else {
        if (auto *NewStmt = DoVisit(AD.getBody(), Stage, true)) {
          dumper() << NewStmt << "\n";
          Builder.addStageStmt(NewStmt, Stage);
        }
        if (hasErrorOccurred())
          return;
      }
      dumper() << "\n";
    }
  }
};

} // namespace

void Partitioner::run(AnalysisDeclContext &AD, Builder &Builder) {
  DependencyPass(*this, Builder).run(AD);
  Builder.updateUseStages();
  LiftPass(*this).run(AD);
  BlockDependencyPass(*this).run(AD);
  for (auto &P : StmtMap)
    dumper() << "Stmt " << P.first << " " << P.second.StageBits << "\n";
  DeclUsagePass(*this, Builder).run(AD);
  BuildPass(*this, Builder).run(AD);
}

} // namespace clang::hshgen
